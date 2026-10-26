/*
  ============================================================================
   ESP32 DevKit C - Generador de Portadora Continua (CW / Constant TX Mode)
   Módulos: 2x TRW24HP (nRF24L01+ HP)  -  Arduino Framework (setup/loop)
  ============================================================================

   Pines según el ESQUEMA REAL del circuito:

   Módulo 1 (U1)                    Módulo 2 (U2)
   ENABLE -> GPIO4  (ENABLE1)       ENABLE -> GPIO32 (ENABLE2)
   CE     -> GPIO23                 CE     -> GPIO25
   CSN    -> GPIO22                 CSN    -> GPIO26
   SCK    -> GPIO21                 SCK    -> GPIO14
   MOSI   -> GPIO19                 MOSI   -> GPIO12
   MISO   -> GPIO18                 MISO   -> GPIO13
   IRQ    -> sin conectar           IRQ    -> sin conectar

   IMPORTANTE:
   - El módulo tiene un pin ENABLE aparte del pin CE. ENABLE NO es CE.
     Este código maneja ENABLE por su cuenta (habilitarModulos()) y le
     pasa a la librería RF24 los pines CE y CSN reales.
   - Verificá en el datasheet del TRW24HP si ENABLE es activo en ALTO o en
     BAJO y ajustá ENABLE_ACTIVE_LEVEL más abajo.
   - Los pines SPI no son los "por defecto" del ESP32, pero el ESP32 permite
     asignar el SPI a casi cualquier GPIO, así que funciona igual.
   - GPIO12 (MOSI del módulo 2) es un pin de arranque del ESP32 (MTDI).

   SEGURO DE RF:
   - HABILITAR_SALIDA_RF = 0 -> NO se emite portadora. Sirve para probar
     SPI, ENABLE y el barrido con los módulos conectados y SIN antena.
   - HABILITAR_SALIDA_RF = 1 -> emite portadora real. Usar SOLO con antena
     o carga de 50 ohm conectada (si no, se puede dañar el amplificador).

   Librería necesaria: "RF24" de TMRh20 (nrf24/RF24 en PlatformIO).
   Core Arduino-ESP32 v2.x (API de timer clásica).
  ============================================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// ============================================================
// 0) SEGURO DE RF (EDITABLE)
// ============================================================
#define HABILITAR_SALIDA_RF 1 // 0 = sin emitir (seguro) / 1 = emite portadora real

// ============================================================
// 1) CONFIGURACIÓN DE PINES (EDITABLE) - según esquema real
// ============================================================

// --- Módulo 1 (bus VSPI, pines remapeados) ---
#define EN1_PIN 4 // ENABLE1
#define CE1_PIN 23
#define CSN1_PIN 22
#define SCK1_PIN 21
#define MOSI1_PIN 19
#define MISO1_PIN 18

// --- Módulo 2 (bus HSPI, pines remapeados) ---
#define EN2_PIN 32 // ENABLE2
#define CE2_PIN 25
#define CSN2_PIN 26
#define SCK2_PIN 14
#define MOSI2_PIN 12
#define MISO2_PIN 13

// --- LED indicador de "portadora activa" ---
#define LED_ACTIVO_PIN 27

// --- Pin ENABLE de los módulos ---
#define ENABLE_ACTIVE_LEVEL HIGH // nivel que HABILITA el módulo (confirmar en datasheet)
#define ENABLE_STARTUP_MS 50     // espera tras habilitar, antes de hablar por SPI

// ============================================================
// 2) CONFIGURACIÓN DE RF (EDITABLE)
// ============================================================
#define PA_LEVEL RF24_PA_MAX
#define DATA_RATE RF24_1MBPS // RF24_250KBPS / RF24_1MBPS / RF24_2MBPS

// ============================================================
// 3) MODO DE OPERACIÓN (EDITABLE)
// ============================================================
#define MODE_FIXED_CHANNEL 0
#define MODE_SWEEP 1

#define OPERATION_MODE MODE_SWEEP
#define FIXED_CHANNEL 40 // solo en MODE_FIXED_CHANNEL (0-125)

// ============================================================
// 4) CONFIGURACIÓN DEL BARRIDO / SWEEP (EDITABLE)
// ============================================================
#define SWEEP_CH_START 0
#define SWEEP_CH_END 125
#define SWEEP_CH_STEP 1

// Tiempo entre cambios de canal, en microsegundos (más alto = más lento)
#define SWEEP_INTERVAL_US 9000UL

// ============================================================
// OBJETOS SPI Y RF24
// ============================================================
SPIClass vspi(VSPI); // módulo 1
SPIClass hspi(HSPI); // módulo 2

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
void habilitarModulos(bool estado);
void configurarModuloCW(RF24 &radio, SPIClass &spiBus);
void iniciarPortadora(RF24 &radio, uint8_t channel);
void actualizarCanalAmbosModulos(uint8_t channel);
void indicarActivo(bool estado);

// ============================================================
// ISR DEL TIMER (solo levanta una bandera, nada de SPI acá)
// ============================================================
void IRAM_ATTR onSweepTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    channelChangeFlag = true;
    portEXIT_CRITICAL_ISR(&timerMux);
}

// ============================================================
// LED indicador de portadora activa (D27)
// ============================================================
void indicarActivo(bool estado)
{
    digitalWrite(LED_ACTIVO_PIN, estado ? HIGH : LOW);
}

// ============================================================
// Habilita / deshabilita ambos módulos por su pin ENABLE
// ============================================================
void habilitarModulos(bool estado)
{
    const uint8_t nivelOff = (ENABLE_ACTIVE_LEVEL == HIGH) ? LOW : HIGH;
    digitalWrite(EN1_PIN, estado ? ENABLE_ACTIVE_LEVEL : nivelOff);
    digitalWrite(EN2_PIN, estado ? ENABLE_ACTIVE_LEVEL : nivelOff);
}

// ============================================================
// Configura un módulo sobre SU bus SPI (no emite nada todavía)
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
// Inicia la portadora continua en un canal (respeta el seguro de RF)
// ============================================================
void iniciarPortadora(RF24 &radio, uint8_t channel)
{
    radio.setChannel(channel);
#if HABILITAR_SALIDA_RF
    radio.stopConstCarrier();
    radio.startConstCarrier(PA_LEVEL, channel);
    indicarActivo(true);
#endif
}

// ============================================================
// Cambia el canal de ambos módulos (usado en el sweep)
// ============================================================
void actualizarCanalAmbosModulos(uint8_t channel)
{
#if HABILITAR_SALIDA_RF
    radio1.stopConstCarrier();
    radio2.stopConstCarrier();
#endif

    radio1.setChannel(channel);
    radio2.setChannel(channel);

#if HABILITAR_SALIDA_RF
    radio1.startConstCarrier(PA_LEVEL, channel);
    radio2.startConstCarrier(PA_LEVEL, channel);
    indicarActivo(true);
#endif
}

// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println("=== Constant TX (CW) - TRW24HP x2 ===");
#if HABILITAR_SALIDA_RF
    Serial.println("ATENCION: salida RF HABILITADA (usar solo con antena/carga)");
#else
    Serial.println("Salida RF DESHABILITADA (modo seguro, solo prueba de SPI/ENABLE)");
#endif

    pinMode(LED_ACTIVO_PIN, OUTPUT);
    indicarActivo(false);

    // ENABLE de los módulos (pin distinto de CE): primero apagados, luego activos
    pinMode(EN1_PIN, OUTPUT);
    pinMode(EN2_PIN, OUTPUT);
    habilitarModulos(false);
    delay(10);
    habilitarModulos(true);
    delay(ENABLE_STARTUP_MS);

    // Cada bus SPI con sus pines reales
    vspi.begin(SCK1_PIN, MISO1_PIN, MOSI1_PIN, CSN1_PIN);
    hspi.begin(SCK2_PIN, MISO2_PIN, MOSI2_PIN, CSN2_PIN);

    configurarModuloCW(radio1, vspi);
    configurarModuloCW(radio2, hspi);

    Serial.print("Modulo 1 (VSPI) -> ");
    Serial.println(radio1.isChipConnected() ? "OK conectado" : "NO responde");
    Serial.print("Modulo 2 (HSPI) -> ");
    Serial.println(radio2.isChipConnected() ? "OK conectado" : "NO responde");

#if OPERATION_MODE == MODE_FIXED_CHANNEL
    Serial.print("Modo: CANAL FIJO -> canal ");
    Serial.println(FIXED_CHANNEL);
    iniciarPortadora(radio1, FIXED_CHANNEL);
    iniciarPortadora(radio2, FIXED_CHANNEL);

#elif OPERATION_MODE == MODE_SWEEP
    Serial.println("Modo: BARRIDO SECUENCIAL (SWEEP)");
    currentChannel = SWEEP_CH_START;
    actualizarCanalAmbosModulos(currentChannel);

    sweepTimer = timerBegin(0, 80, true); // 1 tick = 1 us
    timerAttachInterrupt(sweepTimer, &onSweepTimer, true);
    timerAlarmWrite(sweepTimer, SWEEP_INTERVAL_US, true);
    timerAlarmEnable(sweepTimer);
#endif

    Serial.println("Setup terminado.");
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

        // Lectura de vuelta del canal: si el SPI funciona, ambos valores
        // coinciden con el canal pedido. Si dan 0 o 255, no hay comunicación.
        Serial.print("Canal pedido: ");
        Serial.print(currentChannel);
        Serial.print(" | leido mod1: ");
        Serial.print(radio1.getChannel());
        Serial.print(" | leido mod2: ");
        Serial.println(radio2.getChannel());
    }
#endif
}