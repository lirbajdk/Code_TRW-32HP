#ifndef __TIMER_H
#define __TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>

/*
devuelve el tiempo en milisegundos(32bit) desde que arranco el timer0
*/
uint32_t get_time(void);
void set_time(uint32_t time);


uint8_t get_valor(void);
void set_valor(uint8_t new_valor);

#ifdef __cplusplus
}
#endif


#endif