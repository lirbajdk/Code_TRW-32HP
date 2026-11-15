#ifndef V2ANALIZADOR_H
#define V2ANALIZADOR_H

#include <Arduino.h>

// Escáner de WiFi 2.4GHz + recomendación de canal libre entre 1/6/11,
// mostrando todo por el LCD (LiquidCrystal_I2C, ya inicializado afuera).
// Son funciones sueltas, no una clase: el LCD se inicializa en main.cpp
// y acá solo se usa el objeto global "lcd" declarado en ese archivo.

void v2analizador_begin(); // Muestra el mensaje de arranque y prepara el WiFi
void v2analizador_loop();  // Un ciclo completo: escanea, muestra redes, recomienda canal

uint32_t get_canal(void);
void set_canal(int16_t new_valor);

#endif