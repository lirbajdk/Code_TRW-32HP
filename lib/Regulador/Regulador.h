#ifndef REGULADOR_H
#define REGULADOR_H

// #ifdef __cplusplus
// extern "C" {
// #endif

/***************************************************************************************************
 *  INCLUDES
 ***************************************************************************************************/
#include <Arduino.h>

/***************************************************************************************************
 *  DEFINES Y CONSTANTES
 ***************************************************************************************************/
// ------------------------------------------
// PINOUT 1ra Antena (HSPI)
// ------------------------------------------
#define HSPI_CE 23
#define HSPI_CSN 22
#define HSPI_SCK 21
#define HSPI_MOSI 19
#define HSPI_MISO 18
#define HSPI_ENABLE 4

// ------------------------------------------
// PINOUT 2da Antena (VSPI)
// ------------------------------------------
#define VSPI_CE 25
#define VSPI_CSN 26
#define VSPI_SCK 14
#define VSPI_MOSI 12
#define VSPI_MISO 13
#define VSPI_ENABLE 32

// ------------------------------------------
// Potencia de los modulos
// ------------------------------------------
#define POTENCIA_H RF24_PA_MAX
#define POTENCIA_V RF24_PA_MAX

// PARAMETROS DE ELECCIÓN
/**
 * (0) represents:
 * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
 * :-------:|:-----------------------------:|:----------------------------:
 *  -18 dBm | -6 dBm | -12 dBm
 */
//! RF24_PA_MIN = 0,
/**
 * (1) represents:
 * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
 * :-------:|:-----------------------------:|:----------------------------:
 *  -12 dBm | 0 dBm | -4 dBm
 */
//! RF24_PA_LOW,
/**
 * (2) represents:
 * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
 * :-------:|:-----------------------------:|:----------------------------:
 *  -6 dBm | 3 dBm | 1 dBm
 */
//! RF24_PA_HIGH,
/**
 * (3) represents:
 * nRF24L01 | Si24R1 with<br>lnaEnabled = 1 | Si24R1 with<br>lnaEnabled = 0
 * :-------:|:-----------------------------:|:----------------------------:
 *  0 dBm | 7 dBm | 4 dBm
 */
//! RF24_PA_MAX,
/**
 * (4) This should not be used and remains for backward compatibility.
 */
//! RF24_PA_ERROR

/***************************************************************************************************
 *  VARIABLES GLOBALES
 ***************************************************************************************************/

/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
 ***************************************************************************************************/
void regulador_begin(void); // Mensaje de arranque + init del WiFi en modo escaneo
void regulador_loop(void);  // Un ciclo completo: escanea, muestra redes, recomienda canal

void init_HP(void); // inicialización del modulo H
void init_SP(void); // inicialización del módulo S
void salto(void);   // modo de regulación por salto
void barrido(void); // modo de regulación por barrido
// #ifdef __cplusplus
// }
// #endif
#endif