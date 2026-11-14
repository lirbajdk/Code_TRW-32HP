
//-----------------------------------------------//
//---------------    ANALIZADOR   ---------------//
//-----------------------------------------------//

#include <SPI.h>
#include <RF24.h>
#include <Arduino.h>           // Librería base de Arduino
#include <WiFi.h>              // Módulo WiFi del ESP32
#include <Wire.h>              // Comunicación I2C
#include <LiquidCrystal_I2C.h> // Control del LCD por I2C

// --- Configuración Módulo 1 ---
#define MOD1_CE 23
#define MOD1_CSN 22
#define MOD1_SCK 21
#define MOD1_MOSI 19
#define MOD1_MISO 18
#define MOD1_ENABLE 4

// --- Configuración Módulo 2 ---
#define MOD2_CE 25
#define MOD2_CSN 26
#define MOD2_SCK 14
#define MOD2_MOSI 12
#define MOD2_MISO 13
#define MOD2_ENABLE 32

#define LCD_ADDR 0x27 // Dirección I2C del LCD (si no anda, probá 0x3F)
#define LCD_COLS 16   // Columnas del LCD
#define LCD_ROWS 2    // Filas del LCD

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

void setup()
{
    Wire.begin(2, 15);                  // Inicia I2C con SDA = GPIO21 y SCL = GPIO22
    lcd.init();                         // Inicializa el LCD
    lcd.backlight();                    // Enciende la luz de fondo
    mostrar("Escaner WiFi", "2.4 GHz"); // Mensaje de arranque

    WiFi.persistent(false);      // No guarda la config WiFi en flash
    WiFi.mode(WIFI_OFF);         // Apaga el WiFi para reiniciar el módulo
    delay(500);                  // Pausa para que se apague bien
    WiFi.mode(WIFI_STA);         // Modo estación (necesario para escanear)
    WiFi.disconnect(true, true); // Se desconecta y borra credenciales
    delay(1000);                 // Deja que la radio se estabilice
}

void loop()
{
    mostrar("Escaneando...", ""); // Avisa que está escaneando

    // false = bloqueante, true = incluye ocultas, false = activo, 500 ms por canal
    int cantidad = WiFi.scanNetworks(false, true, false, 500);

    if (cantidad == WIFI_SCAN_FAILED)
    { // -2: el escaneo falló
        mostrar("ERROR", "Escaneo fallo");
        delay(3000);
    }
    else if (cantidad == 0)
    { // 0: sin redes
        mostrar("Sin redes", "encontradas");
        delay(3000);
    }
    else
    {                         // Hay redes
        int conteo[14] = {0}; // Redes por canal (índices 1 a 13)

        mostrar("Redes halladas:", String(cantidad)); // Muestra el total
        delay(2000);                                  // Lo deja 2 segundos

        for (int i = 0; i < cantidad; i++)
        {                                // Recorre cada red
            int canal = WiFi.channel(i); // Canal de la red i
            int rssi = WiFi.RSSI(i);     // Intensidad de señal
            if (canal >= 1 && canal <= 13)
                conteo[canal]++; // Suma al conteo del canal

            String ssid = WiFi.SSID(i); // Nombre de la red
            if (ssid.length() == 0)
                ssid = "(oculta)"; // Si es oculta, avisa

            char linea2[17];                                               // Buffer para la segunda línea
            snprintf(linea2, sizeof(linea2), "Ch%2d %4ddBm", canal, rssi); // Ej: "Ch10  -95dBm"

            char linea1[17];                                                             // Buffer para la primera línea
            snprintf(linea1, sizeof(linea1), "%d/%d %s", i + 1, cantidad, ssid.c_str()); // Ej: "1/3 CLAROWIFI"

            mostrar(String(linea1), String(linea2)); // Muestra la red en el LCD
            delay(2500);                             // La deja 2,5 segundos
        }

        // ---------- Recomendación entre 1, 6 y 11 ----------
        int candidatos[3] = {1, 6, 11}; // Canales que no se solapan
        int carga[3];                   // Carga estimada de cada uno
        int mejorCanal = 1;             // Canal recomendado
        int menorCarga = 9999;          // Menor carga encontrada

        for (int n = 0; n < 3; n++)
        { // Evalúa cada candidato
            int c = candidatos[n];
            carga[n] = 0;
            for (int j = c - 4; j <= c + 4; j++)
            { // Suma redes en canales vecinos (se solapan)
                if (j >= 1 && j <= 13)
                    carga[n] += conteo[j];
            }
            if (carga[n] < menorCarga)
            { // Si es la menor hasta ahora...
                menorCarga = carga[n];
                mejorCanal = c; // ...la guarda como recomendada
            }
        }

        char l1[17];
        snprintf(l1, sizeof(l1), "Carga 1/6/11:"); // Título de la pantalla de resumen
        char l2[17];
        snprintf(l2, sizeof(l2), "%d / %d / %d", carga[0], carga[1], carga[2]); // Ej: "3 / 1 / 0"
        mostrar(String(l1), String(l2));
        delay(3000);

        char l3[17];
        snprintf(l3, sizeof(l3), "Usa el canal %d", mejorCanal); // Recomendación final
        mostrar("Mejor canal:", String(l3));
        delay(3000);
    }

    WiFi.scanDelete(); // Libera la memoria del escaneo
}

//-----------------------------------------------//
//----------------    ACTUADOR   ----------------//
//-----------------------------------------------//

SPIClass hspi(HSPI);
SPIClass vspi(VSPI);

RF24 radio1(MOD1_CE, MOD1_CSN);
RF24 radio2(MOD2_CE, MOD2_CSN);

// Paquete de datos
byte datosBartolo[32];

// Variables
uint8_t canal1 = 0;  // Inicia en 2
uint8_t canal2 = 41; // Inicia en4

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Serial.println("\n--- Iniciando Enlace ---");

    pinMode(MOD1_ENABLE, OUTPUT);
    pinMode(MOD2_ENABLE, OUTPUT);
    digitalWrite(MOD1_ENABLE, HIGH);
    digitalWrite(MOD2_ENABLE, HIGH);
    delay(150);

    hspi.begin(MOD1_SCK, MOD1_MISO, MOD1_MOSI, MOD1_CSN);
    vspi.begin(MOD2_SCK, MOD2_MISO, MOD2_MOSI, MOD2_CSN);

    if (!radio1.begin(&hspi) || !radio2.begin(&vspi))
    {
        Serial.println("Error crítico inicializando radios.");
        while (1)
            ;
    }

    // Configuración de máxima potencia y velocidad
    radio1.setPALevel(RF24_PA_MAX);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setAutoAck(false);
    radio1.stopListening();

    radio2.setPALevel(RF24_PA_MAX);
    radio2.setDataRate(RF24_2MBPS);
    radio2.setAutoAck(false);
    radio2.stopListening();

    // Rellenar buffer con datos aleatorios
    for (int i = 0; i < 32; i++)
    {
        datosBartolo[i] = random(0, 255);
    }
}

void loop()
{
    // Aplicar canales actuales
    radio1.setChannel(canal1);
    radio2.setChannel(canal2);

    // Ráfaga rápida en la frecuencia seleccionada
    // unsigned long inicio = millis();
    // while (millis() - inicio < 50)
    // {
    radio1.write(&datosBartolo, sizeof(datosBartolo), true);
    radio2.write(&datosBartolo, sizeof(datosBartolo), true);
    // }

    // Incrementar secuencialmente y resetear al llegar al canal 83 (2.4835 GHz)
    canal1++;
    if (canal1 > 83)
    {
        canal1 = 0; // Vuelve a 2400 MHz
    }

    canal2++;
    if (canal2 > 83)
    {
        canal2 = 0; // Vuelve a 2400 MHz
    }
}

/*
#include <Arduino.h> // Librería base de Arduino (Serial, delay, etc.)
#include <WiFi.h>    // Librería del módulo WiFi del ESP32

void setup()
{
  Serial.begin(115200); // Inicia el puerto serie a 115200 baudios (para el monitor)
  delay(1000);          // Espera 1 seg para que el monitor serie se conecte

  WiFi.mode(WIFI_STA); // Modo estación (cliente): necesario para poder escanear
  WiFi.disconnect();   // Se desconecta de cualquier red previa para empezar limpio
  delay(100);          // Pequeña pausa para que el módulo se estabilice

  Serial.println("Escaner WiFi 2.4 GHz iniciado"); // Mensaje de arranque
}

void loop()
{
  Serial.println("\nEscaneando redes..."); // Aviso de que empieza el escaneo

  int cantidad = WiFi.scanNetworks(); // Escanea y devuelve la cantidad de redes encontradas
                                      // (si falla devuelve un número negativo)

  if (cantidad <= 0)
  {                                            // Si no se encontró nada (o hubo error)...
    Serial.println("No se encontraron redes"); // ...lo informa
  }
  else
  {                                                          // Si se encontraron redes...
    Serial.printf("Se encontraron %d redes:\n\n", cantidad); // ...muestra cuántas

    // Cabecera de la "tabla" que se imprime por el monitor
    Serial.println("Nro | Canal | Freq (MHz) | RSSI (dBm) | Seguridad | SSID");
    Serial.println("-----------------------------------------------------------");

    for (int i = 0; i < cantidad; i++)
    { // Recorre cada red encontrada

      int canal = WiFi.channel(i);       // Canal WiFi (1 a 13) al que pertenece esta red
      int freq = 2412 + (canal - 1) * 5; // Frecuencia central: el canal 1 es 2412 MHz y
                                         // cada canal siguiente suma 5 MHz

      // Imprime una fila con todos los datos de la red i
      Serial.printf("%2d  |  %2d   |   %4d     |    %4d    | %-9s | %s\n",
                    i + 1,                                                                // Número de red (empezando en 1)
                    canal,                                                                // Canal
                    freq,                                                                 // Frecuencia en MHz
                    WiFi.RSSI(i),                                                         // Intensidad de señal (más cerca de 0 = más fuerte)
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Abierta" : "Protegida", // ¿Tiene clave?
                    WiFi.SSID(i).c_str());                                                // Nombre de la red (SSID)
    }
  }

  WiFi.scanDelete(); // Libera la memoria usada por los resultados del escaneo
  delay(5000);       // Espera 5 segundos y vuelve a escanear
}*/
