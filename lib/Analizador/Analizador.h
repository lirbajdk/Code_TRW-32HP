#ifndef ANALIZADOR_H
#define ANALIZADOR_H

#ifdef __cplusplus
extern "C" {
#endif

// Escanea WiFi 2.4GHz con la API nativa de ESP-IDF (en C puro, sin
// WiFi.h de Arduino que es C++) y recomienda el mejor canal libre
// entre 1/6/11, mostrando todo por el LCD ya inicializado en main().

/*********************************************************************************************************
 *** INCLUDES GLOBALES
*********************************************************************************************************/
    #include "LCD_I2C_Registros.h"
    #include "esp32-hal.h" // delay()

    #include <stdio.h>
    
    // API nativa de ESP-IDF para WiFi: es la capa en C puro que, en un
    // sketch Arduino normal, WiFi.h termina llamando por detrás.
        #include "esp_wifi.h"
        #include "esp_event.h"
        #include "esp_netif.h"
        #include "nvs_flash.h"

/***************************************************************************************************
 *  DEFINES Y CONSTANTES
***************************************************************************************************/
    #define MAX_REDES 20 // Tope de redes a traer de cada escaneo (ajustable)

/***************************************************************************************************
 *  VARIABLES GLOBALES
***************************************************************************************************/

/***************************************************************************************************
 *  PROTOTIPOS DE FUNCIONES
***************************************************************************************************/

    void analizador_begin(void); // Mensaje de arranque + init del WiFi en modo escaneo
    void analizador_loop(void);  // Un ciclo completo: escanea, muestra redes, recomienda canal

    static void mostrar(const char *linea1, const char *linea2);
    static void recomendarCanal(const int conteoPorCanal[14]);
    static void escanearYMostrar(void);


#ifdef __cplusplus
}
#endif

#endif