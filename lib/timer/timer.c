#include "timer.h"


uint32_t tiempo=0;

uint32_t get_time(void){
    return tiempo;
}

void set_time(uint32_t time){
    tiempo = time;
}

ISR(TIMER0_COMPA_vect)
{
    tiempo++;
}
/**************************************/
uint8_t valor= 0; 

uint8_t get_valor(void){
    return valor;
}

void set_valor(uint8_t new_valor){
    valor = new_valor;
}

