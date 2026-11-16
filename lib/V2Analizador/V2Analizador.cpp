#include "V2Analizador.h"
#include "timer.h"
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;
uint8_t canalx = 0;

// ---------- Tiempos (ms) ----------
#define T_ARRANQUE_OFF 500  // WiFi apagado antes de reiniciar
#define T_ARRANQUE_STA 1000 // radio estabilizándose
#define T_TOTAL_REDES 700   // pantalla "Redes halladas"
#define T_POR_RED 1500      // cada red en pantalla
#define T_RESULTADO 700     // pantallas de carga / mejor canal
#define T_ERROR 30

// ---------- Estados ----------
enum Estado
{
    ST_ARRANQUE_OFF,
    ST_ARRANQUE_STA,
    ST_INICIAR_SCAN,
    ST_ESPERAR_SCAN,
    ST_MOSTRAR_TOTAL,
    ST_MOSTRAR_RED,
    ST_MOSTRAR_CARGA,
    ST_MOSTRAR_MEJOR,
    ST_PAUSA_FINAL
};

static Estado estado = ST_ARRANQUE_OFF;
static uint32_t proximo = 0; // vencimiento de la espera actual

// Datos que se comparten entre estados
static int cantidad = 0;
static int idx = 0;
static int conteo[14];
static int16_t carga[3];
static int16_t mejorCanal = 1;

// ---------- Helpers de tiempo ----------
static inline void esperar(uint32_t ms) { proximo = get_time() + ms; }
static inline bool vencio(void) { return (int32_t)(get_time() - proximo) >= 0; }

// ---------- LCD ----------
static void mostrar(String linea1, String linea2)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(linea1.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(linea2.substring(0, 16));
}

// ---------- Canal ----------
void set_canal(int16_t new_canalx) { canalx = new_canalx; }
uint32_t get_canal(void) { return canalx; }

// (sin delay; no estaba en el flujo, la dejo por si la usas)
static void mostrarCanalMasUsado(const int conteoPorCanal[14])
{
    int canalMasUsado = 1;
    int mayorConteo = conteoPorCanal[1];
    for (int c = 2; c <= 13; c++)
        if (conteoPorCanal[c] > mayorConteo)
        {
            mayorConteo = conteoPorCanal[c];
            canalMasUsado = c;
        }

    char l2[17];
    snprintf(l2, sizeof(l2), "Canal %d (%d red)", canalMasUsado, mayorConteo);
    mostrar("Mas usado:", String(l2));
    set_canal(canalMasUsado);
}

// Solo calcula; ya no muestra nada
static void calcularCanal(const int conteoPorCanal[14])
{
    int16_t candidatos[3] = {1, 6, 11};
    int16_t menorCarga = 9999;
    mejorCanal = 1;

    for (int n = 0; n < 3; n++)
    {
        int c = candidatos[n];
        carga[n] = 0;
        for (int j = c - 4; j <= c + 4; j++)
            if (j >= 1 && j <= 13)
                carga[n] += conteoPorCanal[j];
        if (carga[n] < menorCarga)
        {
            menorCarga = carga[n];
            mejorCanal = c;
        }
    }
}

// ---------- API pública ----------
void v2analizador_begin()
{
    mostrar("Escaner WiFi", "2.4 GHz");
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    esperar(T_ARRANQUE_OFF);
    estado = ST_ARRANQUE_OFF;
}

// Llamar en cada pasada del loop(): NO bloquea
void v2analizador_loop()
{
    switch (estado)
    {
    case ST_ARRANQUE_OFF:
        if (!vencio())
            return;
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true, true);
        esperar(T_ARRANQUE_STA);
        estado = ST_ARRANQUE_STA;
        break;

    case ST_ARRANQUE_STA:
        if (!vencio())
            return;
        estado = ST_INICIAR_SCAN;
        break;

    case ST_INICIAR_SCAN:
        mostrar("Escaneando...", "");
        // true = ASÍNCRONO: vuelve enseguida, el escaneo sigue en segundo plano
        WiFi.scanNetworks(true, true, false, 500);
        estado = ST_ESPERAR_SCAN;
        break;

    case ST_ESPERAR_SCAN:
    {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING)
            return; // -1: todavía escaneando

        if (n == WIFI_SCAN_FAILED)
        { // -2: falló
            mostrar("ERROR", "Escaneo fallo");
            esperar(T_ERROR);
            estado = ST_PAUSA_FINAL;
            return;
        }
        if (n == 0)
        {
            mostrar("Sin redes", "encontradas");
            esperar(T_ERROR);
            estado = ST_PAUSA_FINAL;
            return;
        }

        cantidad = n;
        idx = 0;
        memset(conteo, 0, sizeof(conteo));

        // Cuenta todas las redes de una vez (es rápido, no hace falta esperar)
        for (int i = 0; i < cantidad; i++)
        {
            int canal = WiFi.channel(i);
            if (canal >= 1 && canal <= 13)
                conteo[canal]++;
        }
        calcularCanal(conteo);

        mostrar("Redes halladas:", String(cantidad));
        esperar(T_TOTAL_REDES);
        estado = ST_MOSTRAR_TOTAL;
        break;
    }

    case ST_MOSTRAR_TOTAL:
        if (!vencio())
            return;
        estado = ST_MOSTRAR_RED;
        break;

    case ST_MOSTRAR_RED:
    {
        if (!vencio())
            return;

        if (idx >= cantidad)
        { // ya mostró todas
            char l2[17];
            snprintf(l2, sizeof(l2), "%d / %d / %d", carga[0], carga[1], carga[2]);
            mostrar("Carga 1/6/11:", String(l2));
            esperar(T_RESULTADO);
            estado = ST_MOSTRAR_CARGA;
            return;
        }

        int canal = WiFi.channel(idx);
        int rssi = WiFi.RSSI(idx);
        String ssid = WiFi.SSID(idx);
        if (ssid.length() == 0)
            ssid = "(oculta)";

        char linea1[17], linea2[17];
        snprintf(linea2, sizeof(linea2), "Ch%2d %4ddBm", canal, rssi);
        snprintf(linea1, sizeof(linea1), "%d/%d %s", idx + 1, cantidad, ssid.c_str());
        mostrar(String(linea1), String(linea2));

        idx++;
        esperar(T_POR_RED);
        break;
    }

    case ST_MOSTRAR_CARGA:
    {
        if (!vencio())
            return;
        char l3[17];
        snprintf(l3, sizeof(l3), "Usa el canal %d", mejorCanal);
        mostrar("Mejor canal:", String(l3));
        esperar(T_RESULTADO);
        estado = ST_MOSTRAR_MEJOR;
        break;
    }

    case ST_MOSTRAR_MEJOR:
        if (!vencio())
            return;
        estado = ST_PAUSA_FINAL;
        break;

    case ST_PAUSA_FINAL:
        if (!vencio())
            return;
        WiFi.scanDelete();        // libera memoria
        estado = ST_INICIAR_SCAN; // nuevo ciclo
        break;
    }
}