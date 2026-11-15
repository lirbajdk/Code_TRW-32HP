#include <Arduino.h>
#include "Regulador.h"
#include "V2Analizador.h"
#include <SPI.h>
#include <RF24.h>

SPIClass hspi(HSPI);
SPIClass vspi(VSPI);

RF24 radio1(MOD1_CE, MOD1_CSN);
RF24 radio2(MOD2_CE, MOD2_CSN);

// Paquete de datos
byte datosBartolo[32];

// Variables
uint8_t canal1 = 0;  // Inicia en 2
uint8_t canal2 = 41; // Inicia en4

void regulador_begin(void)
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Serial.println("\n--- Iniciando Enlace ---");

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
        datosBartolo[i] = random(0, 255);
    }
}

void regulador_loop(void)
{
    // Aplicar canales actuales
    radio1.setChannel(canal1);
    radio2.setChannel(canal2);

    // Ráfaga rápida en la frecuencia seleccionada
    // unsigned long inicio = millis();
    // while (millis() - inicio < 50)
    // {
    radio1.write(&datosBartolo, sizeof(datosBartolo), true);
    radio2.write(&datosBartolo, sizeof(datosBartolo), true);
    // }

    // 4. Margen de tiempo para que el aire vacíe el buffer físico de transmisión
    // (A 2 Mbps, 32 bytes tardan ~130 µs en salir de la antena)
    delayMicroseconds(150);
    // Incrementar secuencialmente y resetear al llegar al canal 83 (2.4835 GHz)
    canal1 = get_canal();

    canal2 = get_canal();
}