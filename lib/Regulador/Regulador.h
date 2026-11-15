#ifndef REGULADOR_H
#define REGULADOR_H

// #ifdef __cplusplus
// extern "C" {
// #endif

/***************************************************************************************************
 *  INCLUDES
 ***************************************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

/***************************************************************************************************
 *  DEFINES Y CONSTANTES
 ***************************************************************************************************/
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

/***************************************************************************************************
 *  VARIABLES GLOBALES
 ***************************************************************************************************/

/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
 ***************************************************************************************************/
void regulador_begin(void); // Mensaje de arranque + init del WiFi en modo escaneo
void regulador_loop(void);  // Un ciclo completo: escanea, muestra redes, recomienda canal

// #ifdef __cplusplus
// }
// #endif
#endif