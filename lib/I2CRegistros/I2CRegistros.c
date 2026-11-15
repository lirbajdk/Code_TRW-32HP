#include "I2CRegistros.h"

// Estado del módulo: qué pines son SDA y SCL. Al ser un solo bus,
// alcanza con variables estáticas de archivo en vez de una struct.
static uint8_t _sda;
static uint8_t _scl;

static inline void pinSalida(uint8_t pin)
{
    // Habilita el pin como salida escribiendo un 1 en su bit del registro ENABLE
    GPIO.enable_w1ts = (1UL << pin);
}

static inline void pinEntrada(uint8_t pin)
{
    // Pone el bit del registro ENABLE en 0: el pin pasa a ser entrada.
    // Así simulamos "open-drain": lo soltamos y el pull-up externo del
    // módulo I2C/LCD es el que lo lleva a 1.
    GPIO.enable_w1tc = (1UL << pin);
}

static inline void escribirPin(uint8_t pin, bool nivel)
{
    
    if (nivel)
        GPIO.out_w1ts = (1UL << pin); // Escribe un 1 en el registro OUT (pin a alto)
    else
        GPIO.out_w1tc = (1UL << pin); // Escribe un 1 en el registro "clear" (pin a bajo)
}

static inline bool leerPin(uint8_t pin)
{
    return (GPIO.in >> pin) & 0x1; // Lee el registro IN completo y se queda con el bit del pin
}

static inline void delayCuartoBit(void)
{
    delayMicroseconds(I2C_DELAY_US); // Pausa fija entre cada transición de SDA/SCL
}

static void iniciarCondicion(void)
{
    // SDA baja estando SCL en alto = condición de START
    pinEntrada(_sda); // SDA en 1
    pinEntrada(_scl); // SCL en 1
    delayCuartoBit();
    pinSalida(_sda); // SDA a 0 mientras SCL sigue en 1 -> START
    delayCuartoBit();
    pinSalida(_scl); // Baja SCL, listo para transmitir
    delayCuartoBit();
}

static void detenerCondicion(void)
{
    // SDA sube estando SCL en alto = condición de STOP
    pinSalida(_sda); // Asegura SDA en 0 antes de levantar SCL
    delayCuartoBit();
    pinEntrada(_scl); // Sube SCL
    delayCuartoBit();
    pinEntrada(_sda); // Con SCL en alto, sube SDA -> STOP
    delayCuartoBit();
}

static bool escribirBitABit(uint8_t byteDato)
{
    for (int i = 7; i >= 0; i--) // Del bit más significativo al menos significativo
    {
        bool bit = (byteDato >> i) & 0x1;
        if (bit)
            pinEntrada(_sda); // Bit 1: suelta SDA
        else
            pinSalida(_sda); // Bit 0: fuerza SDA a bajo
        delayCuartoBit();
        pinEntrada(_scl); // Flanco de subida: el esclavo lee el bit acá
        delayCuartoBit();
        pinSalida(_scl); // Flanco de bajada, prepara el siguiente bit
        delayCuartoBit();
    }

    // Noveno pulso: acá el esclavo contesta el ACK
    pinEntrada(_sda); // Soltamos SDA para que la maneje el esclavo
    pinEntrada(_scl);
    delayCuartoBit();
    bool ack = !leerPin(_sda); // ACK = línea en 0
    pinSalida(_scl);
    pinSalida(_sda);
    delayCuartoBit();

    return ack;
}

void i2c_begin(uint8_t pinSDA, uint8_t pinSCL)
{
    _sda = pinSDA;
    _scl = pinSCL;

    // Si el pin es de los 32-39 (dominio RTC), hay que desengancharlo
    // del RTC antes de usarlo como GPIO digital común
    if (rtc_gpio_is_valid_gpio(_sda)) rtc_gpio_deinit(_sda);
    if (rtc_gpio_is_valid_gpio(_scl)) rtc_gpio_deinit(_scl);

    // Selecciona la función GPIO en el IO MUX para estos dos pines
    // (esto es lo que pinMode() hace por vos y que acá hay que hacer a mano)
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[_sda], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[_scl], PIN_FUNC_GPIO);
    // Deja preparado el bit de salida en 0 (para cuando se use) y
    // ambas líneas sueltas (nivel alto por el pull-up externo).
    escribirPin(_sda, false);
    escribirPin(_scl, false);
    pinEntrada(_sda);
    pinEntrada(_scl);
}

bool i2c_escribirByte(uint8_t direccion7bits, uint8_t dato)
{
    iniciarCondicion();
    bool ack1 = escribirBitABit((direccion7bits << 1) | 0x0); // Dirección + R/W=0 (escritura)
    bool ack2 = escribirBitABit(dato);                          // El byte de datos en sí
    detenerCondicion();
    return ack1 && ack2;
}
