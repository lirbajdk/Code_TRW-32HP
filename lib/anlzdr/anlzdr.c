#include "anlzdr.h"
#include "timer.h"

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); // Crea el objeto del LCD

// Función auxiliar: borra la pantalla y escribe dos líneas
void mostrar(String linea1, String linea2)
{
  lcd.clear();                              // Borra todo el LCD
  lcd.setCursor(0, 0);                      // Cursor en columna 0, fila 0
  lcd.print(linea1.substring(0, LCD_COLS)); // Escribe la línea 1 (recortada al ancho del LCD)
  lcd.setCursor(0, 1);                      // Cursor en columna 0, fila 1
  lcd.print(linea2.substring(0, LCD_COLS)); // Escribe la línea 2 (recortada)
}


// void config_timer0()
// {
//     TCCR0A = 0b00000010; // modo CTC
//     TIMSK0 = 0b00000010; // habilitar interrupción de comparación A
//     OCR0A = 249;         //
//     TCCR0B = 0b00000011; // prescaler 64
// }

//void config_IO()
//{
//    DDRD |= 0b11110000; // Configurar LEDs como salida //este y el de abajo son de los displays
//    DDRB |= 0b00001111; // Configurar LEDs como salida //este y el de arriba son de los displays
//    // set_bit(DDRB, PB1); // individual para el puntico    // este tmb es del display pero es el pin del puntito
//
//    DDRD |= 0b00001100; // PARA EL ANODO/CATODO (dependiendo de lo tenido)
//    DDRB |= 0b00110000; // PARA EL ANODO/CATODO (dependiendo de lo tenido)
//}
//////////////////////////////////////////////////////////


