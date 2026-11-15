#ifndef LCD_I2C_REGISTROS_H
#define LCD_I2C_REGISTROS_H

#ifdef __cplusplus
extern "C" {
#endif
// Maneja un LCD 16x2 HD44780 a través de un expansor PCF8574, usando
// el bus I2CRegistros (bit-banging por registros). Módulo en C puro:
// un solo LCD, estado guardado en variables estáticas del .c (no en
// una struct).
//
// Mapeo de bits del PCF8574 (estándar en estos backpacks):
//   P0=RS  P1=RW  P2=EN  P3=Backlight  P4..P7 = D4..D7

/*********************************************************************************************************
 * INCLUDES GLOBALES
*********************************************************************************************************/
    #include <stdint.h>

    #include "esp32-hal.h" // delay() / delayMicroseconds()
    #include <stdbool.h>
    #include <stddef.h>

/***************************************************************************************************
 *  DEFINES Y CONSTANTES
***************************************************************************************************/
    #define BIT_RS        0x01 // Register Select: 0 = comando, 1 = dato
    #define BIT_RW        0x02 // Read/Write: siempre en 0, solo escribimos
    #define BIT_EN        0x04 // Enable: el flanco que hace que el LCD "tome" el byte
    #define BIT_BACKLIGHT 0x08 // Luz de fondo, la maneja el propio PCF8574

/***************************************************************************************************
 *  VARIABLES GLOBALES
***************************************************************************************************/

/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
***************************************************************************************************/
    void lcd_init(uint8_t direccionI2C, uint8_t columnas, uint8_t filas); // Secuencia de arranque del HD44780
    void lcd_backlight(void);                                              // Prende la luz de fondo
    void lcd_clear(void);                                                   // Borra toda la pantalla
    void lcd_setCursor(uint8_t columna, uint8_t fila);                     // Ubica el cursor antes de escribir
    void lcd_print(const char *texto);                                      // Escribe texto en la posición actual

    static void pulsoEnable(uint8_t datoConBits);
    static void enviarNibble(uint8_t nibble, bool esDato);
    static void enviarByte(uint8_t valor, bool esDato);
    static void comando(uint8_t valor);
    static void dato(uint8_t valor);

#ifdef __cplusplus
}
#endif
#endif