/**
 * Leccion 01 - Blink: el "hola mundo" de la ESP32-CAM.
 *
 * Hace parpadear el LED blanco del flash (al lado de la camara) y escribe en el
 * monitor serie. Sirve para comprobar que la cadena completa funciona: cable,
 * driver USB, carga del programa y monitor.
 */

#include <Arduino.h>
#include <panel.h>

// --- Parte 1: constantes y parametros ----------------------------------------
// El LED blanco grande (el "flash" de la camara) esta en el GPIO 4.
// Logica normal: HIGH = encendido, LOW = apagado.
const int LED_FLASH = 4;

// Parametros: valores que se pueden cambiar desde el panel SIN recompilar.
// Encendido corto y apagado largo: el flash es MUY brillante y se calienta.
const long& ENCENDIDO_MS = panel::parametro("ENCENDIDO_MS", 200, 20, 3000, "Tiempo encendido (ms)");
const long& APAGADO_MS = panel::parametro("APAGADO_MS", 1000, 20, 5000, "Tiempo apagado (ms)");

// --- Parte 2: setup() corre UNA vez al encender o resetear -------------------
void setup() {
    Serial.begin(921600);  // debe coincidir con monitor_speed de platformio.ini
    delay(1000);           // tiempo para que abras el monitor y no pierdas el saludo
    panel::iniciar("01_blink");
    panel::parte(2);

    Serial.println();
    Serial.println("================================");
    Serial.println("  ESP32-CAM - Leccion 01: Blink");
    Serial.println("================================");
    Serial.printf("Chip: %s rev %d, %d nucleos, %d MHz\n",
                  ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getChipCores(), ESP.getCpuFreqMHz());
    Serial.printf("PSRAM: %s (%u bytes)\n",
                  psramFound() ? "SI" : "NO", (unsigned)ESP.getPsramSize());

    pinMode(LED_FLASH, OUTPUT);
}

// loop() se repite para siempre. Lo dividimos en dos partes para seguirlo paso a paso.
void loop() {
    // --- Parte 3: encender -------------------------------------------------------
    panel::parte(3);
    digitalWrite(LED_FLASH, HIGH);
    Serial.println("LED encendido");
    delay(ENCENDIDO_MS);

    // --- Parte 4: apagar ---------------------------------------------------------
    panel::parte(4);
    digitalWrite(LED_FLASH, LOW);
    Serial.println("LED apagado");
    delay(APAGADO_MS);
}
