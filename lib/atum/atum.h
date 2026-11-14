#ifndef ATUM_H
#define ATUM_H

#ifdef __cplusplus
extern "C"
{
#endif
    /*********************************************************************************************************
     *** INCLUDES GLOBALES
     *********************************************************************************************************/

#include <Arduino.h>
#include "anlzdr.h"
#include <SPI.h>
#include <RF24.h>

    /*********************************************************************************************************
        DEFINES
    **********************************************************************************************************/
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

    /*********************************************************************************************************
        PROTOTIPOS DE FUNCIONES GLOBALES
    **********************************************************************************************************/
    SPIClass hspi(HSPI);
    SPIClass vspi(VSPI);

    RF24 radio1(MOD1_CE, MOD1_CSN);
    RF24 radio2(MOD2_CE, MOD2_CSN);

    void init_Analizador();
    /*********************************************************************************************************
        PROTOTIPOS DE ESTRUCTURAS/ENUM
    **********************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
