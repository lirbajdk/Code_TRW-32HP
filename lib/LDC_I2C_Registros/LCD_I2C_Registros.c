#include "LCD_I2C_Registros.h"
#include "I2CRegistros.h"

// Estado del módulo (un solo LCD por archivo)
static uint8_t _direccion;
static uint8_t _columnas;
static uint8_t _filas;
static uint8_t _estadoBacklight = BIT_BACKLIGHT; // Arranca "marcado" como prendido

static void pulsoEnable(uint8_t datoConBits)
{
    i2c_escribirByte(_direccion, datoConBits | BIT_EN); // Sube EN con los datos ya puestos
    delayMicroseconds(1);                                 // Mínimo de EN en alto que pide el HD44780
    i2c_escribirByte(_direccion, datoConBits & ~BIT_EN); // Baja EN: en ese flanco el LCD lee el byte
    delayMicroseconds(50);                                 // Tiempo de procesamiento interno del LCD
}

static void enviarNibble(uint8_t nibble, bool esDato)
{
    uint8_t salida = (nibble & 0xF0) | _estadoBacklight; // Nibble en los bits altos + backlight
    if (esDato)
        salida |= BIT_RS; // Si es un dato (no comando), prende RS
    i2c_escribirByte(_direccion, salida); // Deja el nibble "armado" en las líneas
    pulsoEnable(salida);                  // Pulso de EN para que el LCD lo capture
}

static void enviarByte(uint8_t valor, bool esDato)
{
    enviarNibble(valor & 0xF0, esDato);        // Nibble alto (bits 7-4)
    enviarNibble((valor << 4) & 0xF0, esDato); // Nibble bajo (bits 3-0), corrido a la izquierda
}

static void comando(uint8_t valor) { enviarByte(valor, false); } // RS en 0
static void dato(uint8_t valor) { enviarByte(valor, true); }      // RS en 1

void lcd_init(uint8_t direccionI2C, uint8_t columnas, uint8_t filas)
{
    _direccion = direccionI2C;
    _columnas = columnas;
    _filas = filas;

    delay(50); // Espera a que el LCD arranque con la alimentación estable

    // Secuencia estándar del HD44780 para forzar el modo 4 bits
    enviarNibble(0x30, false);
    delay(5);
    enviarNibble(0x30, false);
    delayMicroseconds(150);
    enviarNibble(0x30, false);
    enviarNibble(0x20, false); // Pasa definitivamente a modo 4 bits

    comando(0x28); // Function set: 4 bits, 2 líneas, fuente 5x8
    comando(0x0C); // Display ON, cursor OFF, blink OFF
    comando(0x06); // Entry mode: el cursor avanza solo
    lcd_clear();
}

void lcd_backlight(void)
{
    _estadoBacklight = BIT_BACKLIGHT;
    i2c_escribirByte(_direccion, _estadoBacklight); // Lo aplica ya mismo en el PCF8574
}

void lcd_clear(void)
{
    comando(0x01); // "Clear display" del HD44780
    delay(2);      // Este comando tarda más que los otros
}

void lcd_setCursor(uint8_t columna, uint8_t fila)
{
    static const uint8_t offsetsFila[] = {0x00, 0x40}; // Direcciones base de cada fila
    if (fila >= _filas)
        fila = _filas - 1;
    comando(0x80 | (columna + offsetsFila[fila])); // "Set DDRAM address" + posición
}

void lcd_print(const char *texto)
{
    for (size_t i = 0; texto[i] != '\0'; i++) // Recorre el texto caracter por caracter
        dato((uint8_t)texto[i]);               // Lo manda como dato (RS en 1)
}