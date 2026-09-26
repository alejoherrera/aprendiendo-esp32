/**
 * Implementacion de la libreria del panel (ver panel.h y protocolo_serie.v5.json).
 */

#include "panel.h"
#include <Preferences.h>
#include "mbedtls/base64.h"

namespace panel {
namespace {

struct Parametro {
    const char* nombre;
    long valor;
    long porDefecto;
    long minimo;
    long maximo;
    const char* descripcion;
};

// Capacidad fija A PROPOSITO: parametro() devuelve referencias a estos elementos, y un
// arreglo que crece (std::vector) las invalidaria al reubicarse en memoria.
const int MAX_PARAMETROS = 12;
Parametro parametros[MAX_PARAMETROS];
int cantidad = 0;

Preferences memoria;
String leccionId = "?";
String bufferEntrada;
String lineaParaLeccion;
bool hayLineaParaLeccion = false;
int parteActual = 0;
bool pausado = false;
bool pasoPedido = false;
String ultimaRed;  // ultimo aviso RED: se repite en INFO por si el panel se conecto despues

Parametro* buscar(const String& nombre) {
    for (int i = 0; i < cantidad; i++) {
        if (nombre.equalsIgnoreCase(parametros[i].nombre)) return &parametros[i];
    }
    return nullptr;
}

void comandoSet(const String& argumentos) {
    int espacio = argumentos.indexOf(' ');
    String nombre = argumentos.substring(0, espacio);
    String texto = espacio < 0 ? "" : argumentos.substring(espacio + 1);
    texto.trim();
    Parametro* p = buscar(nombre);
    if (!p) { Serial.printf("ERR param_desconocido %s\n", nombre.c_str()); return; }
    if (texto.isEmpty() || (!isDigit(texto[0]) && texto[0] != '-')) {
        Serial.printf("ERR valor_invalido %s\n", texto.c_str());
        return;
    }
    p->valor = constrain(texto.toInt(), p->minimo, p->maximo);
    memoria.putLong(p->nombre, p->valor);
    Serial.printf("OK SET %s %ld\n", p->nombre, p->valor);
}

void comandoList() {
    for (int i = 0; i < cantidad; i++) {
        const Parametro& p = parametros[i];
        Serial.printf("PARAM %s %ld %ld %ld %s\n", p.nombre, p.valor, p.minimo, p.maximo, p.descripcion);
    }
    Serial.println("END LIST");
}

// WIFI <red>TAB<clave>: guarda la red en su propio espacio de NVS ("wifi"), compartido por
// todas las lecciones, y reinicia para usarla. La clave nunca se vuelve a imprimir.
void comandoWifi(const String& argumentos) {
    int tab = argumentos.indexOf('\t');
    String red = tab < 0 ? argumentos : argumentos.substring(0, tab);
    String clave = tab < 0 ? "" : argumentos.substring(tab + 1);
    if (red.isEmpty() || red.length() > 32 || clave.length() > 63) {
        Serial.println("ERR wifi_invalido");
        return;
    }
    Preferences wifi;
    wifi.begin("wifi", false);
    wifi.putString("red", red);
    wifi.putString("clave", clave);
    wifi.end();
    Serial.printf("OK WIFI %s\n", red.c_str());
    Serial.flush();
    delay(300);
    ESP.restart();
}

// SECRETO <nombre>TAB<valor>: guarda un dato privado (ej. la API key de Gemini) en su propio
// espacio de NVS. Valor vacio = borrar. Nunca se reimprime; no reinicia la placa.
void comandoSecreto(const String& argumentos) {
    int tab = argumentos.indexOf('\t');
    String nombre = tab < 0 ? argumentos : argumentos.substring(0, tab);
    String valor = tab < 0 ? "" : argumentos.substring(tab + 1);
    nombre.trim();
    valor.trim();  // una API key nunca lleva espacios; se limpian los del copiar y pegar
    if (nombre.isEmpty() || nombre.length() > 15 || valor.length() > 180) {
        Serial.println("ERR secreto_invalido");
        return;
    }
    Preferences secretos;
    secretos.begin("secretos", false);
    if (valor.isEmpty()) secretos.remove(nombre.c_str());
    else secretos.putString(nombre.c_str(), valor);
    secretos.end();
    Serial.printf("OK SECRETO %s\n", nombre.c_str());
}

// Devuelve false si la linea no es un comando del protocolo (es para la leccion).
bool procesarComando(const String& linea) {
    if (linea == "LIST") { comandoList(); return true; }
    if (linea.startsWith("SET ")) { comandoSet(linea.substring(4)); return true; }
    if (linea == "RESET") {
        memoria.clear();
        for (int i = 0; i < cantidad; i++) parametros[i].valor = parametros[i].porDefecto;
        Serial.println("OK RESET");
        return true;
    }
    if (linea == "PAUSA") { pausado = true; Serial.println("OK PAUSA"); return true; }
    if (linea == "SIGUE") { pausado = false; Serial.println("OK SIGUE"); return true; }
    if (linea == "PASO") {
        if (!pausado) { Serial.println("ERR no_pausado"); return true; }
        pasoPedido = true;
        Serial.println("OK PASO");
        return true;
    }
    if (linea == "INFO") {
        Serial.printf("INFO leccion=%s pausado=%d parte=%d\n", leccionId.c_str(), pausado ? 1 : 0, parteActual);
        if (!ultimaRed.isEmpty()) Serial.println(ultimaRed);
        return true;
    }
    return false;
}

// Lee lo que haya llegado por serie sin bloquear, linea por linea.
void atender() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c != '\n') {
            if (bufferEntrada.length() < 200) bufferEntrada += c;
            continue;
        }
        String linea = bufferEntrada;
        bufferEntrada = "";
        // WIFI va ANTES del trim(): una clave puede terminar en espacio, y si la clave esta
        // vacia el trim() se comeria el TAB separador.
        if (linea.startsWith("WIFI ")) { comandoWifi(linea.substring(5)); continue; }
        if (linea.startsWith("SECRETO ")) { comandoSecreto(linea.substring(8)); continue; }
        linea.trim();
        if (linea.isEmpty() || procesarComando(linea)) continue;
        lineaParaLeccion = linea;
        hayLineaParaLeccion = true;
    }
}

}  // namespace

const long& parametro(const char* nombre, long porDefecto, long minimo, long maximo,
                      const char* descripcion) {
    // Corre antes de setup() (inicializacion global): todavia no hay Serial ni NVS.
    // Solo se registra; iniciar() carga despues los valores guardados.
    if (cantidad >= MAX_PARAMETROS) {
        static long sobrante = porDefecto;  // no deberia pasar en el curso
        return sobrante;
    }
    parametros[cantidad] = {nombre, porDefecto, porDefecto, minimo, maximo, descripcion};
    return parametros[cantidad++].valor;
}

void iniciar(const char* leccion) {
    leccionId = leccion;
    // Espacio de NVS por leccion ("p01", "p02"...): los ajustes de una no pisan a otra.
    String espacio = "p" + leccionId.substring(0, 2);
    memoria.begin(espacio.c_str(), false);
    for (int i = 0; i < cantidad; i++) {
        Parametro& p = parametros[i];
        p.valor = constrain(memoria.getLong(p.nombre, p.porDefecto), p.minimo, p.maximo);
    }
    // El chip escribe sus mensajes de arranque a 115200: a 921600 llegan como basura SIN
    // salto de linea. El "\n" inicial separa el HOLA de esa basura (medido 2026-09-25).
    Serial.printf("\nHOLA leccion=%s\n", leccionId.c_str());
}

void parte(int n) {
    atender();
    if (n != parteActual) {
        parteActual = n;
        Serial.printf("[PARTE %d]\n", n);
    }
    if (!pausado) return;
    Serial.printf("[PAUSA %d]\n", n);
    while (pausado && !pasoPedido) {
        atender();
        delay(10);
    }
    pasoPedido = false;
}

void enviarJpeg(const uint8_t* datos, size_t largo) {
    // Se codifica de a 57 bytes (= 76 caracteres base64 por linea): asi no hace falta
    // reservar memoria para toda la foto codificada, que pesa un 33 % mas que el JPEG.
    const size_t BYTES_POR_LINEA = 57;
    unsigned char linea[80];
    Serial.printf("FOTO INICIO %u\n", (unsigned)largo);
    for (size_t i = 0; i < largo; i += BYTES_POR_LINEA) {
        size_t n = min(BYTES_POR_LINEA, largo - i);
        size_t escritos = 0;
        mbedtls_base64_encode(linea, sizeof(linea), &escritos, datos + i, n);
        Serial.print("F:");
        Serial.write(linea, escritos);
        Serial.print('\n');
    }
    Serial.println("FOTO FIN");
}

String wifiRed() {
    Preferences wifi;
    wifi.begin("wifi", false);  // lectura-escritura: si el espacio no existe lo crea, sin error NOT_FOUND
    String red = wifi.getString("red", "");
    wifi.end();
    return red;
}

String wifiClave() {
    Preferences wifi;
    wifi.begin("wifi", false);  // lectura-escritura: si el espacio no existe lo crea, sin error NOT_FOUND
    String clave = wifi.getString("clave", "");
    wifi.end();
    return clave;
}

bool hayWifi() {
    return !wifiRed().isEmpty();
}

void anunciarRed(const String& ip, int rssi) {
    ultimaRed = "RED ip=" + ip + " rssi=" + String(rssi);
    Serial.println(ultimaRed);
}

void anunciarSinRed() {
    Serial.println("RED sin_wifi");
}

void anunciarNoConecto() {
    Serial.printf("RED no_conecto red=%s\n", wifiRed().c_str());
}

String secreto(const char* nombre) {
    Preferences secretos;
    secretos.begin("secretos", false);  // idem: evita el error NOT_FOUND en el monitor
    String valor = secretos.getString(nombre, "");
    secretos.end();
    return valor;
}

void anunciarRespuestaIA(const String& texto) {
    Serial.println("IA INICIO");
    int desde = 0;
    while (desde <= (int)texto.length()) {
        int salto = texto.indexOf('\n', desde);
        if (salto < 0) salto = texto.length();
        Serial.print("IA:");
        Serial.println(texto.substring(desde, salto));
        desde = salto + 1;
    }
    Serial.println("IA FIN");
}

void anunciarErrorIA(const String& mensaje) {
    Serial.printf("IA ERROR %s\n", mensaje.c_str());
}

bool leerLinea(String& linea) {
    atender();
    if (!hayLineaParaLeccion) return false;
    linea = lineaParaLeccion;
    hayLineaParaLeccion = false;
    return true;
}

}  // namespace panel
