#include <Arduino.h>
// #include "I2CRegistros.h"
// #include "LCD_I2C_Registros.h"
// #include "Analizador.h"
#include "V2Analizador.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include "Regulador.h"

// Pines I2C del LCD (según tu código original: Wire.begin(2, 15))
#define PIN_SDA 33 // SDA = GPIO2
#define PIN_SCL 27 // SCL = GPIO15

#define LCD_ADDR 0x27 // Dirección I2C del LCD (si no anda, probá 0x3F)
#define LCD_COLS 16   // Columnas del LCD
#define LCD_ROWS 2    // Filas del LCD

// Objeto global del LCD: lo usa tanto este archivo como Analizador.cpp
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// setup() y loop() están declaradas como extern "C" en el core de
// Arduino-ESP32, justamente para poder definirlas desde un .c puro.
void setup(void)
{
    Wire.begin(PIN_SDA, PIN_SCL); // Inicia I2C con los pines elegidos
    lcd.init();                   // Inicializa el LCD
    lcd.backlight();              // Enciende la luz de fondo
                                  // Prende la luz de fondo
    v2analizador_begin();         // Mensaje de arranque + init del WiFi
    regulador_begin();
}

void loop(void)
{
    v2analizador_loop(); // Escanea, muestra redes y recomienda canal
    regulador_loop();
}
