/**
 * Leccion 05 - Ojos de un agente.
 *
 * La ESP32-CAM saca una foto y se la manda a Gemini (IA de Google) junto con una pregunta.
 * Gemini "mira" la foto y responde. Asi un dispositivo IoT se convierte en los ojos de un
 * agente de IA. Cada estudiante usa su propia API key, cargada desde el panel.
 * Se pregunta desde el panel (por USB) o desde la pagina de la placa (celular, sin compu).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "camera_pins.h"
#include "gemini.h"
#include <panel.h>

// --- Parte 1: el agente: modelo, pregunta y parametros ------------------------
// La conexion segura con Google usa mucha "pila" (la memoria donde cada funcion guarda sus
// variables). Los 8 KB que Arduino le da a loop() no alcanzan: la placa se reiniciaba con
// "Stack canary watchpoint triggered". Se le dan 16 KB.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// Modelo de Gemini: rapido, economico y con nivel gratuito (verificado 2026-09-25).
// Si Google lo retira, se cambia aca por otro de ai.google.dev/gemini-api/docs/models.
const char* MODELO = "gemini-3.5-flash-lite";
const char* PREGUNTA_POR_DEFECTO = "Describí lo que ves en la imagen.";

const long& CALIDAD_JPEG = panel::parametro("CALIDAD_JPEG", 12, 4, 63, "Calidad JPEG: menor = mejor y mas pesada");
const long& GIRAR_180 = panel::parametro("GIRAR_180", 1, 0, 1, "1 = girar la foto 180 grados");

WebServer servidor(80);
String ultimaPregunta = "";
String ultimaRespuesta = "Todavia no le preguntaste nada al agente.";
uint8_t* ultimaFoto = nullptr;           // copia de la ultima foto que "vio" el agente (en PSRAM)
size_t largoUltimaFoto = 0;

// --- Parte 2: camara (igual que la leccion 04) --------------------------------
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
    config.frame_size = FRAMESIZE_SVGA;  // 800x600: suficiente para que la IA entienda la escena
    config.jpeg_quality = CALIDAD_JPEG;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    return esp_camera_init(&config) == ESP_OK;
}

// --- Parte 3: conectarse al WiFi (igual que la leccion 04) --------------------
bool conectarWiFi() {
    while (!panel::hayWifi()) {
        panel::anunciarSinRed();
        Serial.println("Falta la red WiFi: cargala en el panel (tarjeta WiFi).");
        for (int i = 0; i < 50; i++) { panel::parte(3); delay(100); }
    }
    String red = panel::wifiRed();
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(red.c_str(), panel::wifiClave().c_str());
    Serial.printf("Conectando a '%s'", red.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > 20000) return false;
        Serial.print(".");
        delay(500);
    }
    Serial.printf("\n[OK] Conectado. Senal: %d dBm\n", WiFi.RSSI());
    return true;
}

// --- Parte 4: mirar y preguntar -------------------------------------------------
// Saca una foto, se la muestra al panel y se la manda a Gemini con la pregunta.
bool mirarYPreguntar(String pregunta, String& respuesta) {
    panel::parte(4);
    if (pregunta.isEmpty()) pregunta = PREGUNTA_POR_DEFECTO;
    sensor_t* s = esp_camera_sensor_get();
    s->set_quality(s, CALIDAD_JPEG);
    s->set_vflip(s, GIRAR_180);
    s->set_hmirror(s, GIRAR_180);

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { respuesta = "No se pudo sacar la foto."; return false; }

    // Se guarda una copia para mostrarla en la pagina web ("lo que vio el agente").
    if (!ultimaFoto || largoUltimaFoto < fb->len) {
        free(ultimaFoto);
        ultimaFoto = (uint8_t*)ps_malloc(fb->len);
    }
    if (ultimaFoto) { memcpy(ultimaFoto, fb->buf, fb->len); largoUltimaFoto = fb->len; }

    panel::enviarJpeg(fb->buf, fb->len);   // el panel ve lo mismo que va a ver el agente
    Serial.printf("Preguntando a %s: \"%s\" (foto de %u bytes)...\n", MODELO, pregunta.c_str(), (unsigned)fb->len);
    unsigned long t0 = millis();
    bool ok = preguntarAGemini(fb->buf, fb->len, pregunta, MODELO, panel::secreto("GEMINI_KEY"), respuesta);
    esp_camera_fb_return(fb);
    Serial.printf("Gemini tardo %.1f s\n", (millis() - t0) / 1000.0);

    ultimaPregunta = pregunta;
    ultimaRespuesta = respuesta;
    if (ok) panel::anunciarRespuestaIA(respuesta);
    else panel::anunciarErrorIA(respuesta);
    return ok;
}

// --- Parte 5: la pagina web: preguntarle al agente desde el celular -------------
const char PAGINA_HTML[] = R"html(<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ojos de un agente</title>
<style>
 body{font-family:sans-serif;margin:16px;max-width:820px}
 img{width:100%;border-radius:8px;background:#ddd}
 input,button{font-size:1.1em;padding:8px}
 input{width:100%;box-sizing:border-box;margin-bottom:8px}
 #respuesta{white-space:pre-wrap;background:#f2f4f7;padding:12px;border-radius:8px}
</style></head><body>
<h1>Ojos de un agente</h1>
<input id="pregunta" placeholder="Pregunta (vacio = describir lo que ve)">
<button id="boton" onclick="preguntar()">Mirar y responder</button>
<p id="respuesta">Escribi una pregunta y toca el boton.</p>
<img id="foto" alt="Lo que vio el agente">
<script>
 async function preguntar(){
   const b = document.getElementById('boton');
   b.disabled = true; document.getElementById('respuesta').textContent = 'Mirando y pensando...';
   const q = encodeURIComponent(document.getElementById('pregunta').value);
   const r = await fetch('/preguntar?q=' + q).then(x => x.text()).catch(() => 'Sin respuesta de la placa.');
   document.getElementById('respuesta').textContent = r;
   document.getElementById('foto').src = '/ultima?t=' + Date.now();
   b.disabled = false;
 }
</script></body></html>)html";

void paginaInicio() {
    panel::parte(5);
    servidor.send(200, "text/html", PAGINA_HTML);
}

void paginaPreguntar() {
    panel::parte(5);
    String respuesta;
    mirarYPreguntar(servidor.arg("q"), respuesta);
    servidor.send(200, "text/plain; charset=utf-8", respuesta);
}

void paginaUltimaFoto() {
    panel::parte(5);
    if (!ultimaFoto) { servidor.send(404, "text/plain", "Todavia no hay foto"); return; }
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.send_P(200, "image/jpeg", (const char*)ultimaFoto, largoUltimaFoto);
}

// --- Parte 6: setup y loop -----------------------------------------------------
void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // parche brownout (ver leccion 03)
    Serial.begin(921600);
    delay(1000);
    panel::iniciar("05_ojos_de_un_agente");
    panel::parte(6);
    Serial.println("\nLeccion 05: ojos de un agente");
    if (panel::secreto("GEMINI_KEY").isEmpty()) {
        Serial.println("Falta tu API key de Gemini: cargala en el panel (tarjeta Agente).");
    }

    if (!iniciarCamara()) {
        Serial.println("[ERROR] Camara no inicio (ver leccion 03).");
        while (true) delay(1000);
    }
    while (!conectarWiFi()) {
        panel::anunciarNoConecto();
        Serial.println("\nRevisa la red y la clave en el panel. Reintento en 10 s.");
        for (int i = 0; i < 100; i++) { panel::parte(6); delay(100); }
    }

    servidor.on("/", paginaInicio);
    servidor.on("/preguntar", paginaPreguntar);
    servidor.on("/ultima", paginaUltimaFoto);
    servidor.begin();
    String ip = WiFi.localIP().toString();
    panel::anunciarRed(ip, WiFi.RSSI());
    Serial.printf("Pagina del agente: http://%s/\n", ip.c_str());
}

void loop() {
    panel::parte(6);
    servidor.handleClient();
    // Desde el panel (o el monitor): "? tu pregunta" -> mirar y responder.
    String linea, respuesta;
    if (panel::leerLinea(linea) && linea.startsWith("?")) {
        String pregunta = linea.substring(1);
        pregunta.trim();
        mirarYPreguntar(pregunta, respuesta);
    }
}
