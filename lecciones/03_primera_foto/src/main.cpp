/**
 * Leccion 03 - Primera foto.
 *
 * Inicia la camara, toma una foto cada vez que escribis "f" en el monitor serie
 * y muestra que hay adentro del frame buffer: tamano, dimensiones y la firma JPEG.
 * Todavia no vemos la imagen: eso llega en la leccion 04 con el navegador.
 */

#include <Arduino.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "camera_pins.h"
#include <panel.h>

// --- Parte 1: configurar e iniciar la camara ---------------------------------
// Parametro ajustable desde el panel: se aplica en la PROXIMA foto, sin reiniciar.
const long& CALIDAD_JPEG = panel::parametro("CALIDAD_JPEG", 10, 4, 63, "Calidad JPEG: menor = mejor y mas pesada");
// El sensor va montado "de cabeza" en la placa: sin girar, la foto sale invertida.
const long& GIRAR_180 = panel::parametro("GIRAR_180", 1, 0, 1, "1 = girar la foto 180 grados, 0 = tal como sale del sensor");

bool iniciarCamara() {
    camera_config_t config = {};

    // 1a. Pines (ver camera_pins.h)
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_pclk = CAM_PIN_PCLK;

    // 1b. Reloj del sensor: canal/temporizador 0 de LEDC (por eso el flash usa el 4).
    config.xclk_freq_hz = 20000000;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    // 1c. Formato: el sensor comprime a JPEG por hardware. Sin eso, una foto 800x600
    // en crudo (RGB565) pesaria 960 KB; en JPEG pesa ~20-60 KB.
    config.pixel_format = PIXFORMAT_JPEG;

    // 1d. Memoria: la foto se guarda en un "frame buffer". Si hay PSRAM (4 MB extra),
    // cabe una foto grande; si no, solo la RAM interna (~320 KB) y hay que achicar.
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;   // 1600x1200
        config.jpeg_quality = CALIDAD_JPEG;   // 0-63: MENOR numero = MEJOR calidad
        config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
        config.frame_size = FRAMESIZE_SVGA;   // 800x600
        config.jpeg_quality = 12;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // 1e. Arrancar el driver
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[ERROR] esp_camera_init fallo: 0x%x\n", err);
        return false;
    }

    // 1f. Quien es el sensor? La AI-Thinker trae OV2640; algunas traen OV3660.
    sensor_t* s = esp_camera_sensor_get();
    Serial.printf("[OK] Camara lista. Sensor PID=0x%x (%s)\n", s->id.PID,
                  s->id.PID == OV2640_PID ? "OV2640" :
                  s->id.PID == OV3660_PID ? "OV3660" : "otro");
    return true;
}

// --- Parte 2: tomar una foto e inspeccionarla --------------------------------
// mostrar = true: ademas de inspeccionarla, la manda al panel para verla.
void tomarFoto(bool mostrar) {
    panel::parte(2);
    // Aplica lo pedido desde el panel: calidad y giro.
    sensor_t* s = esp_camera_sensor_get();
    s->set_quality(s, CALIDAD_JPEG);
    // Girar 180 grados = dar vuelta de arriba a abajo (vflip) Y de izquierda a derecha (hmirror).
    s->set_vflip(s, GIRAR_180);
    s->set_hmirror(s, GIRAR_180);

    // El driver siempre tiene una foto YA sacada, esperando: la saco cuando devolvimos la
    // anterior, y puede tener minutos. Se descarta para que la que sigue sea de AHORA.
    esp_camera_fb_return(esp_camera_fb_get());

    unsigned long t0 = millis();
    camera_fb_t* fb = esp_camera_fb_get();   // pide un cuadro nuevo al driver
    if (!fb) {
        Serial.println("[ERROR] No se pudo capturar");
        return;
    }

    Serial.printf("Foto: %ux%u px, %u bytes, %lu ms\n",
                  fb->width, fb->height, (unsigned)fb->len, millis() - t0);

    // Todo archivo JPEG empieza con FF D8 y termina con FF D9. Si no, esta corrupto.
    bool inicioOk = fb->len > 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;
    bool finOk = fb->len > 2 && fb->buf[fb->len - 2] == 0xFF && fb->buf[fb->len - 1] == 0xD9;
    Serial.printf("Firma JPEG: inicio %s, fin %s\n", inicioOk ? "OK" : "MAL", finOk ? "OK" : "MAL");

    if (mostrar) panel::enviarJpeg(fb->buf, fb->len);   // la foto viaja por el USB al panel

    // IMPORTANTE: devolver el buffer. Si no, el driver se queda sin donde guardar
    // la siguiente foto y esp_camera_fb_get() empieza a devolver NULL.
    esp_camera_fb_return(fb);
}

// --- Parte 3: setup ------------------------------------------------------------
void setup() {
    // Truco comun en ESP32-CAM: al arrancar la camara el consumo pega un salto y, con
    // un cable USB o fuente floja, el voltaje cae y el chip se reinicia ("Brownout
    // detector was triggered"). Esto apaga ese detector. Es un PARCHE: lo correcto es
    // alimentar con 5 V estables y un buen cable.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(921600);
    delay(1000);
    panel::iniciar("03_primera_foto");
    panel::parte(3);
    Serial.println("\nLeccion 03: primera foto");
    Serial.printf("PSRAM: %s\n", psramFound() ? "SI" : "NO");

    if (!iniciarCamara()) {
        Serial.println("Revisa el cable plano de la camara y reinicia.");
        while (true) delay(1000);
    }
    Serial.println("Escribi 'f' para inspeccionar una foto, o 'v' para verla en el panel.");
}

// --- Parte 4: loop -------------------------------------------------------------
void loop() {
    panel::parte(4);
    String linea;
    if (panel::leerLinea(linea)) {
        if (linea.equalsIgnoreCase("f")) tomarFoto(false);
        else if (linea.equalsIgnoreCase("v")) tomarFoto(true);
    }
}
