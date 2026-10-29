#include <SPI.h>
#include <RF24.h>

// --- Configuración Módulo 1 ---
#define MOD1_CE 23
#define MOD1_CSN 22
#define MOD1_SCK 21
#define MOD1_MOSI 19
#define MOD1_MISO 18
#define MOD1_ENABLE 4

// --- Configuración Módulo 2 ---
#define MOD2_CE 25
#define MOD2_CSN 26
#define MOD2_SCK 14
#define MOD2_MOSI 12
#define MOD2_MISO 13
#define MOD2_ENABLE 32

SPIClass hspi(HSPI);
SPIClass vspi(VSPI);

RF24 radio1(MOD1_CE, MOD1_CSN);
RF24 radio2(MOD2_CE, MOD2_CSN);

// Paquete de datos basura de 32 bytes
byte datosBasura[32];

// Variables para el barrido acotado (Canal 0 al 83)
uint8_t canal1 = 0;  // Inicia en 2400 MHz (Canal 0)
uint8_t canal2 = 41; // Inicia a mitad del rango (Canal 41 = 2441 MHz)

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Serial.println("\n--- Iniciando Barrido Restringido (2.400 - 2.4835 GHz) ---");

    pinMode(MOD1_ENABLE, OUTPUT);
    pinMode(MOD2_ENABLE, OUTPUT);
    digitalWrite(MOD1_ENABLE, HIGH);
    digitalWrite(MOD2_ENABLE, HIGH);
    delay(150);

    hspi.begin(MOD1_SCK, MOD1_MISO, MOD1_MOSI, MOD1_CSN);
    vspi.begin(MOD2_SCK, MOD2_MISO, MOD2_MOSI, MOD2_CSN);

    if (!radio1.begin(&hspi) || !radio2.begin(&vspi))
    {
        Serial.println("Error crítico inicializando radios.");
        while (1)
            ;
    }

    // Configuración de máxima potencia y velocidad
    radio1.setPALevel(RF24_PA_MAX);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setAutoAck(false);
    radio1.stopListening();

    radio2.setPALevel(RF24_PA_MAX);
    radio2.setDataRate(RF24_2MBPS);
    radio2.setAutoAck(false);
    radio2.stopListening();

    // Rellenar buffer con datos aleatorios
    for (int i = 0; i < 32; i++)
    {
        datosBasura[i] = random(0, 255);
    }
}

void loop()
{
    // Aplicar canales actuales
    radio1.setChannel(canal1);
    radio2.setChannel(canal2);

    // Ráfaga rápida en la frecuencia seleccionada
    // unsigned long inicio = millis();
    // while (millis() - inicio < 50)
    // {
    radio1.write(&datosBasura, sizeof(datosBasura), true);
    radio2.write(&datosBasura, sizeof(datosBasura), true);
    // }

    // Incrementar secuencialmente y resetear al llegar al canal 83 (2.4835 GHz)
    canal1++;
    if (canal1 > 83)
    {
        canal1 = 0; // Vuelve a 2400 MHz
    }

    canal2++;
    if (canal2 > 83)
    {
        canal2 = 0; // Vuelve a 2400 MHz
    }
}