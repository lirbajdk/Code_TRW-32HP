#include "timer.h"

static hw_timer_t *timer = nullptr;

static volatile uint32_t tiempo = 0; // volatile: lo modifica la ISR
static volatile uint8_t valor = 0;

void IRAM_ATTR onTimer()
{ // reemplaza a ISR(TIMER0_COMPA_vect)
    tiempo++;
}

void timer_init(void)
{
    timer = timerBegin(0, 80, true);             // timer 0, divisor 80 -> 1 tick = 1 µs
    timerAttachInterrupt(timer, &onTimer, true); // true = por flanco
    timerAlarmWrite(timer, 1000, true);          // cada 1 ms, auto-reload
    timerAlarmEnable(timer);                     // en 2.x hay que habilitarla
}

uint32_t get_time(void) { return tiempo; }
void set_time(uint32_t time) { tiempo = time; }
uint8_t get_valor(void) { return valor; }
void set_valor(uint8_t v) { valor = v; }