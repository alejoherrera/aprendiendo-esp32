/**
 * Pines de la camara en la placa AI-Thinker ESP32-CAM.
 *
 * Son fijos: estan soldados en la placa. Otras placas (M5Stack, ESP-EYE, Wrover-Kit)
 * usan pines distintos y este archivo es lo unico que habria que cambiar.
 */
#pragma once

#define CAM_PIN_PWDN   32   // apaga/enciende el sensor (power down)
#define CAM_PIN_RESET  -1   // sin pin de reset: se resetea por software
#define CAM_PIN_XCLK    0   // reloj que el ESP32 le da al sensor (tambien es IO0 de arranque)
#define CAM_PIN_SIOD   26   // SCCB datos  (bus tipo I2C para configurar el sensor)
#define CAM_PIN_SIOC   27   // SCCB reloj
#define CAM_PIN_D7     35   // D0..D7: bus paralelo de 8 bits por donde llega la imagen
#define CAM_PIN_D6     34
#define CAM_PIN_D5     39
#define CAM_PIN_D4     36
#define CAM_PIN_D3     21
#define CAM_PIN_D2     19
#define CAM_PIN_D1     18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC  25   // pulso de "empieza un cuadro nuevo"
#define CAM_PIN_HREF   23   // pulso de "empieza una fila nueva"
#define CAM_PIN_PCLK   22   // reloj de pixel: un pulso por byte recibido
