#pragma once

/***************************************************************************************************
 *  INCLUDES
 ***************************************************************************************************/
#include <Arduino.h>
#include "timer.h"

/***************************************************************************************************
 *  DEFINES Y CONSTANTES
 ***************************************************************************************************/
#define PIN_BOT1 5
#define BOT1 digitalRead(PIN_BOT1)

#define DEBOUNCING 40 // tiempo de rebote del pulsador
#define TRES_SEGUNDOS 3000
#define ONE_SEGUNDO 1000
#define APAGADO 0
#define PRENDIDO 1
//-----DEBOUNCING-----//

/////////////////////////////////////////
////EL TIEMPO PARA YA SABËS Q
////////////////////////////////////////

#define APRETADO 0
#define SUELTO 1
#define TRES_ESTADO 3

//-----DEBOUNCING-----//   MEFFFFF :D BOTÖN 1
typedef enum
{
    SUELTO_BOT,
    SUELTO_APRETADO_BOT,
    APRETADO_BOT,
    APRETADO_SUELTO_BOT,
    TRES_BOT
} estado_bot_t;
/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
 ***************************************************************************************************/
void debouncing_init(void);
int8_t estado_botn(void);
