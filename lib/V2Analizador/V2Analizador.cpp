#include "V2Analizador.h"
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

// El objeto del LCD vive en main.cpp; acá solo lo "pedimos prestado"
extern LiquidCrystal_I2C lcd;
uint8_t canalx = 0;

static void mostrar(String linea1, String linea2)
{
    lcd.clear();                        // Borra todo el LCD
    lcd.setCursor(0, 0);                // Cursor en columna 0, fila 0
    lcd.print(linea1.substring(0, 16)); // Escribe la línea 1 (recortada al ancho del LCD)
    lcd.setCursor(0, 1);                // Cursor en columna 0, fila 1
    lcd.print(linea2.substring(0, 16)); // Escribe la línea 2 (recortada)
}

static void mostrarCanalMasUsado(const int conteoPorCanal[14])
{
    int canalMasUsado = 1; // Canal con más redes (arranca en 1 por defecto)
    int mayorConteo = conteoPorCanal[1];

    for (int c = 2; c <= 13; c++) // Busca el canal individual con más redes encima
    {
        if (conteoPorCanal[c] > mayorConteo)
        {
            mayorConteo = conteoPorCanal[c];
            canalMasUsado = c;
        }
    }

    char l2[17];
    snprintf(l2, sizeof(l2), "Canal %d (%d red)", canalMasUsado, mayorConteo);
    mostrar("Mas usado:", String(l2));
    delay(5);
    set_canal(canalMasUsado);
}

static void recomendarCanal(const int conteoPorCanal[14])
{
    int16_t candidatos[3] = {1, 6, 11}; // Canales que no se solapan entre sí
    int16_t carga[3];                   // Carga estimada de cada uno
    int16_t mejorCanal = 1;             // Canal recomendado (por ahora, el primero)
    int16_t menorCarga = 9999;          // Menor carga vista hasta el momento

    for (int n = 0; n < 3; n++) // Evalúa cada candidato
    {
        int c = candidatos[n];
        carga[n] = 0;
        for (int j = c - 4; j <= c + 4; j++) // Suma redes en canales vecinos (se solapan)
            if (j >= 1 && j <= 13)
                carga[n] += conteoPorCanal[j];
        if (carga[n] < menorCarga) // Si es la menor hasta ahora...
        {
            menorCarga = carga[n];
            mejorCanal = c; // ...la guarda como recomendada
        }
    }

    char l2[17];
    snprintf(l2, sizeof(l2), "%d / %d / %d", carga[0], carga[1], carga[2]); // Ej: "3 / 1 / 0"
    mostrar("Carga 1/6/11:", String(l2));
    delay(5);

    char l3[17];
    snprintf(l3, sizeof(l3), "Usa el canal %d", mejorCanal); // Recomendación final
    mostrar("Mejor canal:", String(l3));
    delay(5);
}

void set_canal(int16_t new_canalx)
{
    canalx = new_canalx;
}

uint32_t get_canal(void)
{
    return canalx;
}

static void escanearYMostrar()
{
    mostrar("Escaneando...", ""); // Avisa que está escaneando

    // false = bloqueante, true = incluye ocultas, false = activo, 500 ms por canal
    int cantidad = WiFi.scanNetworks(false, true, false, 500);

    if (cantidad == WIFI_SCAN_FAILED) // -2: el escaneo falló
    {
        mostrar("ERROR", "Escaneo fallo");
        delay(30);
        return;
    }
    if (cantidad == 0) // 0: sin redes
    {
        mostrar("Sin redes", "encontradas");
        delay(30);
        return;
    }

    int conteo[14] = {0}; // Redes por canal (índices 1 a 13)

    mostrar("Redes halladas:", String(cantidad)); // Muestra el total
    delay(500);                                   // Lo deja 2 segundos

    for (int i = 0; i < cantidad; i++) // Recorre cada red encontrada
    {
        int canal = WiFi.channel(i); // Canal de la red i
        int rssi = WiFi.RSSI(i);     // Intensidad de señal
        if (canal >= 1 && canal <= 13)
            conteo[canal]++; // Suma al conteo del canal

        String ssid = WiFi.SSID(i); // Nombre de la red
        if (ssid.length() == 0)
            ssid = "(oculta)"; // Si no tiene nombre, avisa que es oculta

        char linea2[17];
        snprintf(linea2, sizeof(linea2), "Ch%2d %4ddBm", canal, rssi); // Ej: "Ch10  -95dBm"

        char linea1[17];
        snprintf(linea1, sizeof(linea1), "%d/%d %s", i + 1, cantidad, ssid.c_str()); // Ej: "1/3 CLAROWIFI"

        mostrar(String(linea1), String(linea2)); // Muestra la red en el LCD
        delay(25);                               // La deja 2,5 segundos
    }

    recomendarCanal(conteo); // Con todo ya contado, calcula y muestra el mejor canal
}

void v2analizador_begin()
{
    mostrar("Escaner WiFi", "2.4 GHz"); // Mensaje de arranque

    WiFi.persistent(false);      // No guarda la config WiFi en flash
    WiFi.mode(WIFI_OFF);         // Apaga el WiFi para reiniciar el módulo
    delay(500);                  // Pausa para que se apague bien
    WiFi.mode(WIFI_STA);         // Modo estación (necesario para escanear)
    WiFi.disconnect(true, true); // Se desconecta y borra credenciales
    delay(1000);                 // Deja que la radio se estabilice
}

void v2analizador_loop()
{
    escanearYMostrar(); // Hace un ciclo completo de escaneo + recomendación
    WiFi.scanDelete();  // Libera la memoria del escaneo
}