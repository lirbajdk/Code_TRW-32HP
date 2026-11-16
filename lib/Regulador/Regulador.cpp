#include <Arduino.h>
#include <SPI.h>
#include "RF24.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "Regulador.h"
#include "V2Analizador.h"

// haceer a los modulos SPI independentes
SPIClass *hp = nullptr; // HSPI
SPIClass *sp = nullptr; // VSPI

// los modulos de nRF24L01 (CE, CSN, SPI_SPEED)
RF24 radioHSPI(HSPI_CE, HSPI_CSN, 16000000);
RF24 radioVSPI(VSPI_CE, VSPI_CSN, 16000000);

bool hasHSPI = false;
bool hasVSPI = false;

byte ch_hspi = 45;
byte ch_vspi = 45;

unsigned int flag_hspi = 0;
unsigned int flag_vspi = 0;

void init_HP()
{
    hp = new SPIClass(HSPI);
    hp->begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, HSPI_CSN);
    if (radioHSPI.begin(hp))
    {
        delay(200);
        Serial.println("[HSPI] Regulador (Antena 1) iniciado con exito");
        radioHSPI.setAutoAck(false);
        radioHSPI.stopListening();
        radioHSPI.setRetries(0, 0);
        radioHSPI.setPayloadSize(5);
        radioHSPI.setAddressWidth(3);
        radioHSPI.setPALevel(POTENCIA_H, true); // potencia xD
        radioHSPI.setDataRate(RF24_2MBPS);      // rangoq abardca el barrido tipo en este caso es 2Mbps, pero puede ser 1 y 250kbps
        radioHSPI.setCRCLength(RF24_CRC_DISABLED);
        radioHSPI.printPrettyDetails();
        radioHSPI.startConstCarrier(POTENCIA_H, ch_hspi);
        hasHSPI = true;
    }
    else
    {
        Serial.println("[HSPI] Falla al iniciar Antena 1 (HSPI)");
    }
}

void init_SP()
{
    sp = new SPIClass(VSPI);
    sp->begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_CSN);
    if (radioVSPI.begin(sp))
    {
        delay(200);
        Serial.println("[VSPI] Regulador (Antena 2) iniciado con exito");
        radioVSPI.setAutoAck(false);
        radioVSPI.stopListening();
        radioVSPI.setRetries(0, 0);
        radioVSPI.setPayloadSize(5);
        radioVSPI.setAddressWidth(3);
        radioVSPI.setPALevel(POTENCIA_V, true);    // potencia xD
        radioVSPI.setDataRate(RF24_2MBPS);         // rangoq abardca el barrido tipo en este caso es 2Mbps, pero puede ser 1 y 250kbps
        radioVSPI.setCRCLength(RF24_CRC_DISABLED); //
        radioVSPI.printPrettyDetails();
        radioVSPI.startConstCarrier(POTENCIA_V, ch_vspi);
        hasVSPI = true;
    }
    else
    {
        Serial.println("[VSPI] Falla al iniciar Antena 2 (VSPI)");
    }
}

void regulador_begin(void)
{
    pinMode(HSPI_ENABLE, OUTPUT);
    pinMode(VSPI_ENABLE, OUTPUT);
    digitalWrite(HSPI_ENABLE, HIGH);
    digitalWrite(VSPI_ENABLE, HIGH);
    esp_bt_controller_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_disconnect();

    Serial.begin(9600);
    delay(500);
    Serial.println("\n--- ESP32 Regulador De Celulares ---");

    init_HP();
    init_SP();

    if (!hasHSPI && !hasVSPI)
    {
        Serial.println("AVISO: El modulo TRW-24HP falló al iniciar, revisar conexiones y capacitores de 10uF.");
    }
}

void salto()
{
    /// Salto de canal (hopping alternado)
    if (flag_hspi == 0)
    {
        ch_hspi += 3;
    }
    else
    {
        ch_hspi -= 3;
    }

    if ((ch_hspi > 79) && (flag_hspi == 0))
    {
        flag_hspi = 1;
    }
    else if ((ch_hspi < 2) && (flag_hspi == 1))
    {
        flag_hspi = 0;
    }

    if (flag_vspi == 0)
    {
        ch_vspi += 4;
    }
    else
    {
        ch_vspi -= 4;
    }

    if ((ch_vspi > 79) && (flag_vspi == 0))
    {
        flag_vspi = 1;
    }
    else if ((ch_vspi < 2) && (flag_vspi == 1))
    {
        flag_vspi = 0;
    }

    if (hasHSPI)
        radioHSPI.setChannel(ch_hspi);
    if (hasVSPI)
        radioVSPI.setChannel(ch_vspi);
}

void barrido()
{
    /// bariido rapido (sweep)
    for (int c = 0; c < 79; c++)
    {
        if (hasHSPI)
            radioHSPI.setChannel(c);
        if (hasVSPI)
            radioVSPI.setChannel(79 - c); // Varredura invertida para maior dispersao de sinal
    }
}

void regulador_loop(void)
{
    if (random(0, 2))
    {
        salto();
    }
    else
    {
        barrido();
    }
}