/**
 * Libreria del panel de aprendizaje (lado placa).
 *
 * Permite que el panel web vea que parte del codigo se ejecuta, detenga la placa
 * paso a paso y cambie parametros sin recompilar. Habla el protocolo
 * docs/contracts/protocolo_serie.v5.json. Con un monitor serie comun tambien se usa:
 * basta escribir LIST, SET, PAUSA, PASO o SIGUE.
 */
#pragma once

#include <Arduino.h>

namespace panel {

/**
 * Declara un parametro ajustable. Se usa a nivel global, fuera de setup():
 *   const long& PAUSA_MS = panel::parametro("PAUSA_MS", 1000, 50, 5000, "Pausa (ms)");
 * Devuelve una referencia: cuando el panel manda SET, la variable cambia sola.
 * Nombre de hasta 15 caracteres (limite de la memoria NVS).
 */
const long& parametro(const char* nombre, long porDefecto, long minimo, long maximo,
                      const char* descripcion);

/** Anuncia la leccion y carga los parametros guardados. Llamar despues de Serial.begin(). */
void iniciar(const char* leccion);

/** Marca que empieza la parte n del codigo. Si el panel pidio pausa, se detiene aca. */
void parte(int n);

/** Devuelve true si llego una linea que NO es un comando del panel (para la leccion). */
bool leerLinea(String& linea);

/**
 * Manda una foto JPEG al panel por el cable USB, codificada en base64, para que la muestre.
 * A 921600 baudios tarda ~0.15 s cada 10 KB de foto. Protocolo: FOTO INICIO / F: / FOTO FIN.
 */
void enviarJpeg(const uint8_t* datos, size_t largo);

/** true si ya se cargo una red WiFi desde el panel (comando WIFI). */
bool hayWifi();

/** Nombre y clave de la red guardada en la placa (vacios si no hay). */
String wifiRed();
String wifiClave();

/** Avisa al panel la direccion de la placa en la red, para que muestre su pagina. */
void anunciarRed(const String& ip, int rssi);

/** Avisa al panel que falta cargar la red, o que no se pudo conectar. */
void anunciarSinRed();
void anunciarNoConecto();

/**
 * Un dato secreto cargado desde el panel (comando SECRETO), por ejemplo "GEMINI_KEY".
 * Devuelve "" si no se cargo. Nunca se imprime.
 */
String secreto(const char* nombre);

/** Manda al panel la respuesta del agente (varias lineas) o un error para el estudiante. */
void anunciarRespuestaIA(const String& texto);
void anunciarErrorIA(const String& mensaje);

}  // namespace panel
