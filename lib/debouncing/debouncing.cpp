#include "debouncing.h"

void debouncing_init(void)
{
    pinMode(PIN_BOT1, INPUT_PULLUP);
}

int8_t estado_botn(void)
{
    static estado_bot_t bot_state = SUELTO_BOT;
    static uint32_t t0 = 0;
    static int8_t estado_final = SUELTO;

    switch (bot_state)
    {
    case SUELTO_BOT:
        if (BOT1 == 0)
        {
            bot_state = SUELTO_APRETADO_BOT;
            t0 = get_time();
        }
        break;

    case SUELTO_APRETADO_BOT:
        if (BOT1 != 0)
        { // fue ruido, vuelve
            bot_state = SUELTO_BOT;
        }
        else if ((uint32_t)(get_time() - t0) >= DEBOUNCING)
        {
            bot_state = APRETADO_BOT;
            estado_final = APRETADO;
        }
        break;

    case APRETADO_BOT:
        if (BOT1 != 0)
        {
            bot_state = APRETADO_SUELTO_BOT;
            t0 = get_time();
        }
        else if ((uint32_t)(get_time() - t0) >= TRES_SEGUNDOS)
        {
            bot_state = TRES_BOT;
        }
        break;

    case APRETADO_SUELTO_BOT:
        if (BOT1 == 0)
        { // fue ruido, sigue apretado
            bot_state = APRETADO_BOT;
        }
        else if ((uint32_t)(get_time() - t0) >= DEBOUNCING)
        {
            bot_state = SUELTO_BOT;
            estado_final = SUELTO;
        }
        break;

    case TRES_BOT:
        estado_final = TRES_ESTADO;
        if (BOT1 != 0)
        {
            bot_state = APRETADO_SUELTO_BOT;
            t0 = get_time(); // antes no se reiniciaba acá
        }
        break;
    }
    return estado_final;
}