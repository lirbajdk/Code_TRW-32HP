#ifndef I2C_REGISTROS_H
#define I2C_REGISTROS_H

#ifdef __cplusplus
extern "C"
{
#endif
    // Driver I2C "bit-banging" en C puro, por acceso directo a los
    // registros GPIO del ESP32 (sin Wire.h, sin clases). Pensado para
    // hablar con un expansor PCF8574 (el "backpack" I2C de los LCD 16x2).
    //
    // Es un módulo de UN solo bus: los pines SDA/SCL se guardan en
    // variables estáticas de I2CRegistros.c, no en una struct. Si algún
    // día necesitás dos buses por registros a la vez, ahí sí convendría
    // una struct de "instancia" (o duplicar el archivo con otro nombre).

/*********************************************************************************************************
 *** INCLUDES GLOBALES
*********************************************************************************************************/
    #include <stdint.h>
    #include <stdbool.h>

    #include "soc/gpio_struct.h" // Struct GPIO mapeada a los registros reales (es C-compatible)
    #include "esp32-hal.h"       // delay() / delayMicroseconds(), expuestas como extern "C" por el core

    #include "soc/io_mux_reg.h"
    #include "driver/gpio.h" // trae la tabla GPIO_PIN_MUX_REG[]
    #include "driver/rtc_io.h"


/***************************************************************************************************
 *  DEFINES Y CONSTANTES
***************************************************************************************************/
    #define I2C_DELAY_US 5 // Medio período aproximado del bit-banging (ajustable)

/***************************************************************************************************
 *  VARIABLES GLOBALES
***************************************************************************************************/

/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
***************************************************************************************************/

    void i2c_begin(uint8_t pinSDA, uint8_t pinSCL);              // Deja el bus en reposo (ambas líneas "sueltas")
    bool i2c_escribirByte(uint8_t direccion7bits, uint8_t dato); // Manda un byte a un esclavo I2C, devuelve si hubo ACK

    static inline void pinSalida(uint8_t pin);
    static inline void pinEntrada(uint8_t pin);
    static inline void escribirPin(uint8_t pin, bool nivel);
    static inline bool leerPin(uint8_t pin);
    static inline void delayCuartoBit(void);
    static void iniciarCondicion(void);
    static void detenerCondicion(void);
    static bool escribirBitABit(uint8_t byteDato);

#ifdef __cplusplus
}
#endif

#endif