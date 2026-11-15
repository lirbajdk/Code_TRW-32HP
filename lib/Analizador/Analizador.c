#include "Analizador.h"

static void mostrar(const char *linea1, const char *linea2)
{
    char l1[17];
    char l2[17];
    snprintf(l1, sizeof(l1), "%s", linea1); // Recorta a 16 caracteres (ancho del LCD)
    snprintf(l2, sizeof(l2), "%s", linea2);

    lcd_clear();        // Borra todo el LCD
    lcd_setCursor(0, 0); // Cursor en columna 0, fila 0
    lcd_print(l1);       // Escribe la línea 1
    lcd_setCursor(0, 1); // Cursor en columna 0, fila 1
    lcd_print(l2);       // Escribe la línea 2
}

static void recomendarCanal(const int conteoPorCanal[14])
{
    int candidatos[3] = {1, 6, 11}; // Canales que no se solapan entre sí
    int carga[3];                   // Carga estimada de cada uno
    int mejorCanal = 1;             // Canal recomendado (por ahora, el primero)
    int menorCarga = 9999;          // Menor carga vista hasta el momento

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
    mostrar("Carga 1/6/11:", l2);
    delay(3000);

    char l3[17];
    snprintf(l3, sizeof(l3), "Usa el canal %d", mejorCanal); // Recomendación final
    mostrar("Mejor canal:", l3);
    delay(3000);
}

static void escanearYMostrar(void)
{
    mostrar("Escaneando...", ""); // Avisa que está escaneando

    wifi_scan_config_t config = {0};
    config.show_hidden = true; // Incluye redes ocultas, igual que el original

    esp_err_t err = esp_wifi_scan_start(&config, true); // true = bloqueante
    if (err != ESP_OK)
    {
        mostrar("ERROR", "Escaneo fallo");
        delay(3000);
        return;
    }

    uint16_t cantidad = 0;
    esp_wifi_scan_get_ap_num(&cantidad);

    if (cantidad == 0) // Sin redes
    {
        mostrar("Sin redes", "encontradas");
        delay(3000);
        return;
    }

    uint16_t aTraer = (cantidad > MAX_REDES) ? MAX_REDES : cantidad;
    static wifi_ap_record_t registros[MAX_REDES]; // Buffer estático: evita pedir memoria dinámica
    esp_wifi_scan_get_ap_records(&aTraer, registros);

    int conteo[14] = {0}; // Redes por canal (índices 1 a 13)

    char total[8];
    snprintf(total, sizeof(total), "%d", aTraer);
    mostrar("Redes halladas:", total); // Muestra el total
    delay(2000);

    for (int i = 0; i < aTraer; i++) // Recorre cada red encontrada
    {
        int canal = registros[i].primary; // Canal de la red i
        int rssi = registros[i].rssi;      // Intensidad de señal
        if (canal >= 1 && canal <= 13)
            conteo[canal]++;

        const char *ssid = (const char *)registros[i].ssid; // Nombre de la red
        if (ssid[0] == '\0')
            ssid = "(oculta)"; // Si no tiene nombre, avisa que es oculta

        char linea2[17];
        snprintf(linea2, sizeof(linea2), "Ch%2d %4ddBm", canal, rssi); // Ej: "Ch10  -95dBm"

        char linea1[17];
        snprintf(linea1, sizeof(linea1), "%d/%d %s", i + 1, aTraer, ssid); // Ej: "1/3 CLAROWIFI"

        mostrar(linea1, linea2);
        delay(2500);
    }

    recomendarCanal(conteo);
}

void analizador_begin(void)
{
    mostrar("Escaner WiFi", "2.4 GHz");

    // Inicialización mínima de ESP-IDF para poder escanear: esto es
    // lo que WiFi.h de Arduino hacía por detrás de forma automática.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA); // Modo estación, necesario para escanear
    esp_wifi_start();
}

void analizador_loop(void)
{
    escanearYMostrar(); // Hace un ciclo completo de escaneo + recomendación
    // No hace falta un "scanDelete" explícito: esp_wifi_scan_start ya
    // reemplaza los resultados del escaneo anterior en el siguiente llamado.
}