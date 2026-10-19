/*
  ============================================================================
   ESP32 DevKit C - Generador de Portadora Continua (CW / Constant TX Mode)
   Módulos: 2x nRF24L01+ HP (TRW24HP)  -  Arduino Framework (setup/loop)
  ============================================================================

   Qué hace este código:
   - Pone ambos módulos en modo de PORTADORA CONTINUA (CONT_WAVE del chip
     nRF24L01+). No se transmiten paquetes, solo una señal de RF pura, ideal
     para verla en un analizador de espectro.
   - Permite elegir entre CANAL FIJO o BARRIDO SECUENCIAL de canal (sweep).
   - El barrido se controla con un TIMER por hardware (ISR): el ISR solo
     levanta una bandera (flag); el cambio real de canal (que usa SPI) se
     hace en loop(), como corresponde en ESP32 (no se debe usar SPI dentro
     de una ISR).
   - Un LED en D27 indica, con una función aparte, cuándo la portadora
     está realmente activa.
   - TODOS los pines y velocidades son editables en la sección de macros.

   Librería necesaria (Arduino Library Manager / PlatformIO):
     "RF24" de TMRh20  ->  https://github.com/nRF24/RF24
     (Debe incluir startConstCarrier() / stopConstCarrier(); son parte de la
      librería desde hace varios años, si tu versión es muy vieja actualizala)

   Notas de hardware (bus dual):
   - Cada módulo usa su PROPIO bus SPI de hardware: módulo 1 -> HSPI,
     módulo 2 -> VSPI. No comparten SCK/MOSI/MISO entre sí, así que no hay
     arbitraje de bus si en algún momento operan al mismo tiempo.
   - CE1 = GPIO4 (D4) y CE2 = GPIO32 (D32), tal como pediste. El resto de los
     pines de cada bus (CSN, SCK, MOSI, MISO) quedaron en los pines
     estándar de HSPI/VSPI del ESP32 DevKit C, pero son macros editables
     igual que todo lo demás.
   - VCC de ambos módulos va a una fuente externa de 5V adaptada al
     TRW24HP (no al 3.3V del ESP32); GND de esa fuente debe estar unido al
     GND del ESP32. Las líneas de control (CE/CSN/SCK/MOSI/MISO) sí salen
     del ESP32 en lógica 3.3V.
   - No se usa WiFi ni Bluetooth internos del ESP32, solo SPI + GPIO.

   Nota sobre el core de Arduino-ESP32:
   - Este código usa la API de timer "clásica" (timerBegin con prescaler,
     timerAttachInterrupt con función y edge). Corresponde al core
     Arduino-ESP32 v2.x. Si tenés instalado el core v3.x, la API de timer
     cambió (timerBegin ya no lleva prescaler ni timerAlarmWrite se usa
     igual); en ese caso avisame y te paso la versión adaptada.
  ============================================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// ============================================================
// 1) CONFIGURACIÓN DE PINES (EDITABLE)
//    Bus dual: cada módulo tiene su propio SPI de hardware.
// ============================================================

// --- Módulo 1 -> bus HSPI ---
#define CE1_PIN 4   // Enable 1  (pedido: D4)
#define CSN1_PIN 15 // Chip Select 1
#define SCK1_PIN 14
#define MOSI1_PIN 13
#define MISO1_PIN 12

// --- Módulo 2 -> bus VSPI ---
#define CE2_PIN 32  // Enable 2  (pedido: D32)
#define CSN2_PIN 21 // Chip Select 2
#define SCK2_PIN 18
#define MOSI2_PIN 23
#define MISO2_PIN 19

// --- LED indicador de "portadora activa" ---
#define LED_ACTIVO_PIN 27 // D27 (pedido)

// ============================================================
// 2) CONFIGURACIÓN DE RF (EDITABLE)
// ============================================================
#define PA_LEVEL RF24_PA_MAX // Potencia interna del chip nRF24
                             // (no es la potencia del PA externo del
                             //  TRW24HP, eso lo maneja la fuente externa)
#define DATA_RATE RF24_1MBPS // RF24_250KBPS / RF24_1MBPS / RF24_2MBPS

// ============================================================
// 3) MODO DE OPERACIÓN (EDITABLE)
// ============================================================
#define MODE_FIXED_CHANNEL 0
#define MODE_SWEEP 1

#define OPERATION_MODE MODE_SWEEP // <-- cambiar a MODE_FIXED_CHANNEL
                                  //     para transmitir en un solo canal

#define FIXED_CHANNEL 40 // Solo se usa si OPERATION_MODE = MODE_FIXED_CHANNEL
                         // (rango válido nRF24: 0 a 125)

// ============================================================
// 4) CONFIGURACIÓN DEL BARRIDO / SWEEP (EDITABLE)
// ============================================================
#define SWEEP_CH_START 0 // Canal inicial (0-125)
#define SWEEP_CH_END 125 // Canal final   (0-125)
#define SWEEP_CH_STEP 1  // Incremento entre canales

// Velocidad del barrido: tiempo entre cada cambio de canal, en microsegundos.
// Subí este valor para verlo "lento" y analizar la salida con más detalle.
//   500000UL   = 0.5 s por canal (rápido-medio)
//   2000000UL  = 2 s por canal   (lento, ideal para observar en el analizador)
#define SWEEP_INTERVAL_US 100000UL // <-- EDITAR ACÁ LA VELOCIDAD

// ============================================================
// OBJETOS SPI (uno por bus de hardware) Y OBJETOS RF24
// ============================================================
SPIClass hspi(HSPI);
SPIClass vspi(VSPI);

RF24 radio1(CE1_PIN, CSN1_PIN);
RF24 radio2(CE2_PIN, CSN2_PIN);

// ============================================================
// VARIABLES DE TIMER / ISR
// ============================================================
hw_timer_t *sweepTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool channelChangeFlag = false;
volatile uint8_t currentChannel = SWEEP_CH_START;

// ============================================================
// PROTOTIPOS
// ============================================================
void IRAM_ATTR onSweepTimer();
void configurarModuloCW(RF24 &radio, SPIClass &spiBus);
void iniciarPortadora(RF24 &radio, uint8_t channel);
void actualizarCanalAmbosModulos(uint8_t channel);
void indicarActivo(bool estado);

// ============================================================
// ISR DEL TIMER
// Solo marca una bandera. NUNCA se hacen llamadas SPI/RF24 acá.
// ============================================================
void IRAM_ATTR onSweepTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    channelChangeFlag = true;
    portEXIT_CRITICAL_ISR(&timerMux);
}

// ============================================================
// Prende/apaga el LED indicador de "portadora activa" (D27).
// Función aparte, no toca nada del RF, solo controla el LED,
// para poder llamarla desde cualquier parte del código.
// ============================================================
void indicarActivo(bool estado)
{
    digitalWrite(LED_ACTIVO_PIN, estado ? HIGH : LOW);
}

// ============================================================
// Configura un módulo para modo de portadora continua (CW)
// (cada uno arranca sobre SU bus SPI de hardware)
// ============================================================
void configurarModuloCW(RF24 &radio, SPIClass &spiBus)
{
    radio.begin(&spiBus);
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setPALevel(PA_LEVEL);
    radio.setDataRate(DATA_RATE);
    radio.setRetries(0, 0);
}

// ============================================================
// Inicia la portadora continua (CONT_WAVE) en un canal específico
// ============================================================
void iniciarPortadora(RF24 &radio, uint8_t channel)
{
    radio.stopConstCarrier(); // aseguramos estado limpio antes de arrancar
    radio.setChannel(channel);
    radio.startConstCarrier(PA_LEVEL, channel);
    indicarActivo(true); // LED D27: portadora activa
}

// ============================================================
// Cambia el canal de ambos módulos al mismo tiempo (usado en el sweep)
// ============================================================
void actualizarCanalAmbosModulos(uint8_t channel)
{
    radio1.stopConstCarrier();
    radio2.stopConstCarrier();

    radio1.setChannel(channel);
    radio2.setChannel(channel);

    radio1.startConstCarrier(PA_LEVEL, channel);
    radio2.startConstCarrier(PA_LEVEL, channel);

    indicarActivo(true); // LED D27: portadora activa (ya en el nuevo canal)
}

// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println("=== Iniciando modo Constant TX (CW) - TRW24HP x2 ===");

    // LED indicador: apagado hasta que la portadora realmente arranque
    pinMode(LED_ACTIVO_PIN, OUTPUT);
    indicarActivo(false);

    // Inicializa cada bus SPI con sus propios pines (bus dual, sin compartir)
    hspi.begin(SCK1_PIN, MISO1_PIN, MOSI1_PIN, CSN1_PIN);
    vspi.begin(SCK2_PIN, MISO2_PIN, MOSI2_PIN, CSN2_PIN);

    configurarModuloCW(radio1, hspi);
    configurarModuloCW(radio2, vspi);

    // --- Diagnóstico: ¿el ESP32 detecta hardware real en cada bus? ---
    // Sin los módulos conectados esto va a imprimir "NO conectado" y es
    // ESPERADO: sirve para confirmar que el resto del programa (timer,
    // ISR, barrido) corre igual aunque no haya radio físico todavía.
    Serial.print("Modulo 1 (HSPI) -> ");
    Serial.println(radio1.isChipConnected() ? "OK conectado" : "NO conectado (esperado si aun no lo enchufaste)");

    Serial.print("Modulo 2 (VSPI) -> ");
    Serial.println(radio2.isChipConnected() ? "OK conectado" : "NO conectado (esperado si aun no lo enchufaste)");

#if OPERATION_MODE == MODE_FIXED_CHANNEL
    Serial.print("Modo: CANAL FIJO -> canal ");
    Serial.println(FIXED_CHANNEL);

    iniciarPortadora(radio1, FIXED_CHANNEL);
    iniciarPortadora(radio2, FIXED_CHANNEL);

#elif OPERATION_MODE == MODE_SWEEP
    Serial.println("Modo: BARRIDO SECUENCIAL DE CANAL (SWEEP)");

    currentChannel = SWEEP_CH_START;
    actualizarCanalAmbosModulos(currentChannel);

    // --- Configuración del timer hardware para el barrido ---
    // timerBegin(numero_timer, prescaler, cuenta_ascendente)
    // Con prescaler 80 sobre un clock de 80MHz, cada tick del timer = 1 us
    sweepTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(sweepTimer, &onSweepTimer, true);
    timerAlarmWrite(sweepTimer, SWEEP_INTERVAL_US, true); // recarga automática
    timerAlarmEnable(sweepTimer);
#endif

    Serial.println("Portadora activa.");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop()
{
#if OPERATION_MODE == MODE_SWEEP
    if (channelChangeFlag)
    {
        portENTER_CRITICAL(&timerMux);
        channelChangeFlag = false;
        portEXIT_CRITICAL(&timerMux);

        currentChannel += SWEEP_CH_STEP;
        if (currentChannel > SWEEP_CH_END)
        {
            currentChannel = SWEEP_CH_START;
        }

        actualizarCanalAmbosModulos(currentChannel);

        Serial.print("Canal actual: ");
        Serial.println(currentChannel);
    }
#endif

    // En modo canal fijo no hace falta hacer nada más en el loop:
    // la portadora queda transmitiendo continuamente.
}