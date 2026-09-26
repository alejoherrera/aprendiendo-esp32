/**
 * Leccion 04 - Servidor web: ver la foto en el navegador.
 *
 * La ESP32-CAM se conecta a tu WiFi y levanta una pagina web. Desde el celular o la
 * computadora (en la MISMA red) abris la IP que avisa la placa y ves la foto,
 * con un control para el flash. La red se carga desde el panel: ninguna clave va en el codigo.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "camera_pins.h"
#include <panel.h>

// --- Parte 1: constantes, parametros y el servidor ---------------------------
const int LED_FLASH = 4;
const int CANAL_FLASH = 4;              // canal LEDC distinto al 0 de la camara (leccion 02)
const unsigned long WIFI_TIMEOUT_MS = 20000;

// Ajustables desde el panel; se aplican en la foto siguiente.
const long& CALIDAD_JPEG = panel::parametro("CALIDAD_JPEG", 12, 4, 63, "Calidad JPEG: menor = mejor y mas pesada");
const long& GIRAR_180 = panel::parametro("GIRAR_180", 1, 0, 1, "1 = girar la foto 180 grados");

WebServer servidor(80);                 // puerto 80 = el de HTTP por defecto

// --- Parte 2: camara (igual que la leccion 03, con dos cambios) --------------
bool iniciarCamara() {
    camera_config_t config = {};
    config.pin_pwdn = CAM_PIN_PWDN;     config.pin_reset = CAM_PIN_RESET;
    config.pin_xclk = CAM_PIN_XCLK;     config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_d7 = CAM_PIN_D7; config.pin_d6 = CAM_PIN_D6;
    config.pin_d5 = CAM_PIN_D5; config.pin_d4 = CAM_PIN_D4;
    config.pin_d3 = CAM_PIN_D3; config.pin_d2 = CAM_PIN_D2;
    config.pin_d1 = CAM_PIN_D1; config.pin_d0 = CAM_PIN_D0;
    config.pin_vsync = CAM_PIN_VSYNC;   config.pin_href = CAM_PIN_HREF;
    config.pin_pclk = CAM_PIN_PCLK;
    config.xclk_freq_hz = 20000000;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pixel_format = PIXFORMAT_JPEG;

    // Cambio 1: SVGA en vez de UXGA. Por WiFi una foto chica viaja mucho mas rapido.
    // Cambio 2: DOS buffers + GRAB_LATEST. Con uno solo, la foto que te entrega el driver
    // se saco cuando devolviste la anterior (puede tener minutos). Con dos, el driver
    // sigue capturando en el de reserva y te da siempre la mas reciente.
    if (psramFound()) {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = CALIDAD_JPEG;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 15;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count = 1;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    }
    return esp_camera_init(&config) == ESP_OK;
}

// --- Parte 3: conectarse al WiFi ---------------------------------------------
bool conectarWiFi() {
    // La red y la clave las cargaste en el panel: viajaron por el cable USB y quedaron
    // guardadas en la memoria de la placa. Asi ninguna clave queda escrita en el codigo.
    while (!panel::hayWifi()) {
        panel::anunciarSinRed();
        Serial.println("Falta la red WiFi: cargala en el panel (tarjeta WiFi).");
        for (int i = 0; i < 50; i++) { panel::parte(3); delay(100); }  // atiende al panel 5 s
    }
    String red = panel::wifiRed();
    WiFi.mode(WIFI_STA);                // "estacion": cliente de un router existente
    WiFi.setSleep(false);               // menos latencia en el servidor web
    WiFi.begin(red.c_str(), panel::wifiClave().c_str());
    Serial.printf("Conectando a '%s'", red.c_str());

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > WIFI_TIMEOUT_MS) {
            Serial.println("\n[ERROR] No conecto. La ESP32 solo usa WiFi de 2.4 GHz.");
            return false;
        }
        Serial.print(".");
        delay(500);
    }
    Serial.printf("\n[OK] Conectado. Senal: %d dBm\n", WiFi.RSSI());
    return true;
}

// --- Parte 4: las paginas del servidor ---------------------------------------
// R"html( ... )html" es un "raw string" de C++: permite pegar HTML sin escapar comillas.
// El delimitador "html" hace falta porque el HTML contiene )" (en onclick="nuevaFoto()").
const char PAGINA_HTML[] = R"html(<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-CAM</title>
<style>
 body{font-family:sans-serif;margin:16px;max-width:820px}
 img{width:100%;border-radius:8px;background:#ddd}
 button{font-size:1.1em;padding:8px 16px}
</style></head><body>
<h1>ESP32-CAM</h1>
<img id="foto" src="/foto" alt="foto de la camara">
<p><button onclick="nuevaFoto()">Nueva foto</button>
   <button onclick="video()">Video en vivo</button></p>
<p>Flash: <input type="range" min="0" max="50" value="0" onchange="flash(this.value)"></p>
<script>
 // "?t=" con la hora evita que el navegador muestre la foto vieja guardada en cache.
 function nuevaFoto(){ document.getElementById('foto').src = '/foto?t=' + Date.now(); }
 function video(){ document.getElementById('foto').src = '/video?t=' + Date.now(); }
 // Con video en curso la placa no atiende otros pedidos: primero se corta el video.
 function flash(b){ document.getElementById('foto').src = '';
   fetch('/flash?brillo=' + b).then(nuevaFoto); }
</script></body></html>)html";

void paginaInicio() {
    panel::parte(4);
    servidor.send(200, "text/html", PAGINA_HTML);
}

void paginaFoto() {
    panel::parte(4);
    sensor_t* s = esp_camera_sensor_get();
    s->set_quality(s, CALIDAD_JPEG);
    s->set_vflip(s, GIRAR_180);
    s->set_hmirror(s, GIRAR_180);

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        servidor.send(500, "text/plain", "No se pudo capturar");
        return;
    }
    // Se manda en dos pasos: primero las cabeceras con el largo exacto, despues los bytes
    // directo desde el frame buffer (sin copiarlos a un String, que duplicaria la memoria).
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.setContentLength(fb->len);
    servidor.send(200, "image/jpeg", "");
    servidor.sendContent((const char*)fb->buf, fb->len);
    Serial.printf("Foto enviada: %u bytes\n", (unsigned)fb->len);
    esp_camera_fb_return(fb);           // siempre devolver el buffer (leccion 03)
}

// Video en vivo (MJPEG): una sola respuesta que nunca termina, con un JPEG tras otro separados
// por la marca "--cuadro". El navegador reemplaza la imagen con cada cuadro nuevo.
// Mientras alguien mira, loop() no corre: la placa atiende a UN espectador a la vez.
void paginaVideo() {
    panel::parte(4);
    WiFiClient cliente = servidor.client();
    cliente.print("HTTP/1.1 200 OK\r\n"
                  "Content-Type: multipart/x-mixed-replace; boundary=cuadro\r\n"
                  "Cache-Control: no-store\r\n\r\n");
    int cuadros = 0;
    unsigned long t0 = millis();
    while (cliente.connected()) {
        panel::parte(4);                // si pausas desde el panel, el video se congela aca
        sensor_t* s = esp_camera_sensor_get();
        s->set_quality(s, CALIDAD_JPEG);
        s->set_vflip(s, GIRAR_180);
        s->set_hmirror(s, GIRAR_180);
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) break;
        cliente.printf("--cuadro\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                       (unsigned)fb->len);
        size_t enviados = cliente.write(fb->buf, fb->len);
        cliente.print("\r\n");
        esp_camera_fb_return(fb);
        if (enviados != fb->len) break;  // el navegador se fue a mitad de un cuadro
        cuadros++;
    }
    float segundos = (millis() - t0) / 1000.0;
    Serial.printf("Video terminado: %d cuadros en %.1f s (%.1f cuadros/s)\n",
                  cuadros, segundos, segundos > 0 ? cuadros / segundos : 0);
}

void paginaFlash() {
    panel::parte(4);
    // Porcentaje percibido -> PWM de 12 bits con correccion gamma (leccion 02).
    int porcentaje = constrain(servidor.arg("brillo").toInt(), 0, 50);
    ledcWrite(CANAL_FLASH, round(4095 * pow(porcentaje / 100.0, 2.2)));
    servidor.send(200, "text/plain", String(porcentaje));
}

// --- Parte 5: setup y loop -----------------------------------------------------
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // parche brownout (ver leccion 03)
    Serial.begin(921600);
    delay(1000);
    panel::iniciar("04_servidor_web");
    panel::parte(5);
    Serial.println("\nLeccion 04: servidor web");

    ledcSetup(CANAL_FLASH, 5000, 12);
    ledcAttachPin(LED_FLASH, CANAL_FLASH);
    ledcWrite(CANAL_FLASH, 0);

    if (!iniciarCamara()) {
        Serial.println("[ERROR] Camara no inicio (ver leccion 03).");
        while (true) delay(1000);
    }
    while (!conectarWiFi()) {
        panel::anunciarNoConecto();
        Serial.println("Revisa la red y la clave en el panel. Reintento en 10 s.");
        for (int i = 0; i < 100; i++) { panel::parte(5); delay(100); }
    }

    servidor.on("/", paginaInicio);     // "ruta" -> funcion que la atiende
    servidor.on("/foto", paginaFoto);
    servidor.on("/flash", paginaFlash);
    servidor.on("/video", paginaVideo);
    servidor.begin();

    String ip = WiFi.localIP().toString();
    panel::anunciarRed(ip, WiFi.RSSI());
    Serial.printf("Abri en el navegador: http://%s/\n", ip.c_str());
}

void loop() {
    panel::parte(5);
    servidor.handleClient();            // atiende UNA peticion si hay; no usar delay() largo aca
}
