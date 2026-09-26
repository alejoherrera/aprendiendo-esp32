/**
 * Implementacion de preguntarAGemini() (ver gemini.h).
 *
 * Formato verificado en ai.google.dev el 2026-09-25: POST
 * /v1beta/models/<modelo>:generateContent, clave en la cabecera x-goog-api-key, foto en
 * contents[0].parts[].inline_data, texto de respuesta en candidates[0].content.parts[].text.
 */

#include "gemini.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

namespace {

const char* SERVIDOR = "generativelanguage.googleapis.com";
const unsigned long ESPERA_MAXIMA_MS = 40000;  // cuanto esperar a que Gemini responda

// Certificados RAIZ de Google Trust Services (GTS Root R1 a R4, vencen en 2036). Con ellos la
// placa comprueba que del otro lado esta Google y no un impostor. Van LOS CUATRO porque Google
// alterna cadenas: medido 2026-09-25, 15 de 20 conexiones terminaban en R1 (RSA) y 5 en R4
// (curva eliptica); con solo R1 la placa fallaba 1 de cada 4 veces con "X509 - Certificate
// verification failed". Descargados de pki.goog y verificados contra las dos cadenas reales.
const char* RAICES_GOOGLE = R"pem(-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlrsWNBCUaqxElqjANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjIwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjIwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQDO3v2m++zsFDQ8BwZabFn3GTXd98GdVarTzTukk3LvCvpt
nfbwhYBboUhSnznFt+4orO/LdmgUud+tAWyZH8QiHZ/+cnfgLFuv5AS/T3KgGjSY
6Dlo7JUle3ah5mm5hRm9iYz+re026nO8/4Piy33B0s5Ks40FnotJk9/BW9BuXvAu
MC6C/Pq8tBcKSOWIm8Wba96wyrQD8Nr0kLhlZPdcTK3ofmZemde4wj7I0BOdre7k
RXuJVfeKH2JShBKzwkCX44ofR5GmdFrS+LFjKBC4swm4VndAoiaYecb+3yXuPuWg
f9RhD1FLPD+M2uFwdNjCaKH5wQzpoeJ/u1U8dgbuak7MkogwTZq9TwtImoS1mKPV
+3PBV2HdKFZ1E66HjucMUQkQdYhMvI35ezzUIkgfKtzra7tEscszcTJGr61K8Yzo
dDqs5xoic4DSMPclQsciOzsSrZYuxsN2B6ogtzVJV+mSSeh2FnIxZyuWfoqjx5RW
Ir9qS34BIbIjMt/kmkRtWVtd9QCgHJvGeJeNkP+byKq0rxFROV7Z+2et1VsRnTKa
G73VululycslaVNVJ1zgyjbLiGH7HrfQy+4W+9OmTN6SpdTi3/UGVN4unUu0kzCq
gc7dGtxRcw1PcOnlthYhGXmy5okLdWTK1au8CcEYof/UVKGFPP0UJAOyh9OktwID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQUu//KjiOfT5nK2+JopqUVJxce2Q4wDQYJKoZIhvcNAQEMBQADggIBAB/Kzt3H
vqGf2SdMC9wXmBFqiN495nFWcrKeGk6c1SuYJF2ba3uwM4IJvd8lRuqYnrYb/oM8
0mJhwQTtzuDFycgTE1XnqGOtjHsB/ncw4c5omwX4Eu55MaBBRTUoCnGkJE+M3DyC
B19m3H0Q/gxhswWV7uGugQ+o+MePTagjAiZrHYNSVc61LwDKgEDg4XSsYPWHgJ2u
NmSRXbBoGOqKYcl3qJfEycel/FVL8/B/uWU9J2jQzGv6U53hkRrJXRqWbTKH7QMg
yALOWr7Z6v2yTcQvG99fevX4i8buMTolUVVnjWQye+mew4K6Ki3pHrTgSAai/Gev
HyICc/sgCq+dVEuhzf9gR7A/Xe8bVr2XIZYtCtFenTgCR2y59PYjJbigapordwj6
xLEokCZYCDzifqrXPW+6MYgKBesntaFJ7qBFVHvmJ2WZICGoo7z7GJa7Um8M7YNR
TOlZ4iBgxcJlkoKM8xAfDoqXvneCbT+PHV28SSe9zE8P4c52hgQjxcCMElv924Sg
JPFI/2R80L5cFtHvma3AH/vLrrw4IgYmZNralw4/KBVEqE8AyvCazM90arQ+POuV
7LXTWtiBmelDGDfrs7vRWGJB82bSj6p4lVQgw1oudCvV0b4YacCs1aTPObpRhANl
6WLAYv7YTVWW4tAR+kg0Eeye7QUd5MjWHYbL
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPluILrIPglJ209ZjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjMwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjMwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AAQfTzOHMymKoYTey8chWEGJ6ladK0uFxh1MJ7x/JlFyb+Kf1qPKzEUURout736G
jOyxfi//qXGdGIRFBEFVbivqJn+7kAHjSxm65FSWRQmx1WyRRK2EE46ajA2ADDL2
4CejQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBTB8Sa6oC2uhYHP0/EqEr24Cmf9vDAKBggqhkjOPQQDAwNpADBmAjEA9uEglRR7
VKOQFhG/hMjqb2sXnh5GmCCbn9MN2azTL818+FsuVbu/3ZL3pAzcMeGiAjEA/Jdm
ZuVDFhOD3cffL74UOO0BzrEXGhF16b0DjyZ+hOXJYKaV11RZt+cRLInUue4X
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)pem";

// Instruccion fija para el agente: idioma y largo de las respuestas.
// (No pasa por el monitor serie, asi que puede llevar tildes.)
const char* INSTRUCCION = "Sos los ojos de un agente: una cámara ESP32-CAM. Respondé en español, "
                          "claro y en pocas oraciones, solo sobre lo que se ve en la imagen. "
                          "Si algo no se distingue, decilo.";

// Texto JSON seguro: agrega comillas y escapa lo que haga falta (comillas, saltos de linea...).
String comoTextoJson(const String& texto) {
    JsonDocument doc;
    doc.set(texto);
    String salida;
    serializeJson(doc, salida);
    return salida;
}

// Manda la foto en base64 de a 1536 bytes (= 2048 caracteres): nunca hay mas de 2 KB en memoria.
bool enviarBase64(WiFiClientSecure& red, const uint8_t* datos, size_t largo) {
    const size_t BLOQUE = 1536;
    // "static": el buffer vive fuera de la pila (que es chica y la conexion segura ya usa mucha).
    static unsigned char codificado[2050];
    for (size_t i = 0; i < largo; i += BLOQUE) {
        size_t n = min(BLOQUE, largo - i);
        size_t escritos = 0;
        mbedtls_base64_encode(codificado, sizeof(codificado), &escritos, datos + i, n);
        if (red.write(codificado, escritos) != escritos) return false;
    }
    return true;
}

// Traduce los errores de la API a algo que un estudiante pueda resolver.
String explicarError(int codigo, const String& mensajeApi, const char* modelo) {
    if (codigo == 400 && mensajeApi.indexOf("API key") >= 0)
        return "La API key no es valida: revisala en la tarjeta Agente del panel.";
    if (codigo == 403) return "La API key no tiene permiso para usar Gemini (" + mensajeApi + ").";
    if (codigo == 404) return String("El modelo ") + modelo + " no existe o ya no esta disponible.";
    if (codigo == 429) return "Se agoto el limite gratuito por ahora: espera un minuto y proba de nuevo.";
    if (codigo >= 500) return "Gemini tuvo un problema de su lado. Proba de nuevo en unos segundos.";
    return "Error " + String(codigo) + ": " + mensajeApi;
}

enum class Resultado { RESPONDIO, ERROR_API, ERROR_RED };

// Un intento completo: conectar, mandar el pedido y leer la respuesta.
// ERROR_RED = se corto algo en el camino (vale la pena reintentar); ERROR_API = Gemini
// contesto que no (clave invalida, limite...), reintentar no cambia nada.
Resultado intentarUnaVez(const uint8_t* jpeg, size_t largo, const String& pregunta,
                         const char* modelo, const String& claveApi, String& respuesta) {
    // El cuerpo del pedido se arma en tres pedazos: principio + foto en base64 + cierre.
    String principio = String("{\"system_instruction\":{\"parts\":[{\"text\":") +
                       comoTextoJson(INSTRUCCION) + "}]},\"contents\":[{\"parts\":[{\"text\":" +
                       comoTextoJson(pregunta) +
                       "},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"";
    String cierre = "\"}}]}]}";
    size_t largoBase64 = 4 * ((largo + 2) / 3);

    WiFiClientSecure red;
    red.setCACert(RAICES_GOOGLE);
    red.setTimeout(30);  // segundos, para conectar y enviar
    // OJO (trampa de esta version de la libreria): WiFiClientSecure guarda ese tiempo en SU
    // propia variable, y las lecturas (readStringUntil, deserializeJson) usan la de Stream,
    // que queda en 1 segundo. Gemini tarda 2-3 s en pensar: la placa cortaba siempre antes de
    // la respuesta. Se fija el tiempo de Stream a mano. (Diagnostico 2026-09-25.)
    static_cast<Stream&>(red).setTimeout(ESPERA_MAXIMA_MS);
    if (!red.connect(SERVIDOR, 443)) {
        char detalle[100];
        red.lastError(detalle, sizeof(detalle));
        Serial.printf("[TLS] fallo al conectar: %s | memoria libre %u, bloque mayor %u\n", detalle,
                      (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
        respuesta = "No se pudo conectar con Google (revisa el WiFi y que la red tenga internet).";
        return Resultado::ERROR_RED;
    }
    // HTTP/1.0: el servidor responde de corrido y cierra; asi leer la respuesta es simple.
    String cabeceras = String("POST /v1beta/models/") + modelo + ":generateContent HTTP/1.0\r\n" +
                       "Host: " + SERVIDOR + "\r\n" +
                       "Content-Type: application/json\r\n" +
                       "x-goog-api-key: " + claveApi + "\r\n" +
                       "Content-Length: " + String(principio.length() + largoBase64 + cierre.length()) +
                       "\r\n\r\n";
    // Cada envio se controla: con senal debil la conexion se puede cortar a mitad de camino.
    bool enviado = red.print(cabeceras) == cabeceras.length() &&
                   red.print(principio) == principio.length() &&
                   enviarBase64(red, jpeg, largo) &&
                   red.print(cierre) == cierre.length();
    if (!enviado) {
        red.stop();
        respuesta = "Se corto la conexion mientras se enviaba la foto (senal WiFi debil?).";
        return Resultado::ERROR_RED;
    }

    // Gemini esta "pensando": se espera a que llegue el primer byte de la respuesta.
    unsigned long t0 = millis();
    while (!red.available() && red.connected() && millis() - t0 < ESPERA_MAXIMA_MS) delay(20);
    if (!red.available()) {
        bool seCorto = !red.connected();
        red.stop();
        respuesta = seCorto ? "Google cerro la conexion sin responder (senal WiFi debil?)."
                            : "Gemini tardo demasiado en responder.";
        return Resultado::ERROR_RED;
    }

    // Respuesta: "HTTP/1.0 200 OK", cabeceras, linea vacia y el JSON.
    String estado = red.readStringUntil('\n');
    int codigo = estado.substring(estado.indexOf(' ') + 1).toInt();
    if (codigo == 0) {
        red.stop();
        respuesta = "Respuesta de Google ilegible: " + estado;
        return Resultado::ERROR_RED;
    }
    while (red.connected() || red.available()) {
        String cabecera = red.readStringUntil('\n');
        if (cabecera == "\r" || cabecera.isEmpty()) break;
    }
    JsonDocument doc;
    DeserializationError fallo = deserializeJson(doc, red);
    red.stop();
    if (fallo) {
        respuesta = "La respuesta de Gemini llego cortada (" + String(fallo.c_str()) + ").";
        return Resultado::ERROR_RED;
    }
    if (codigo != 200) {
        respuesta = explicarError(codigo, doc["error"]["message"] | "", modelo);
        return codigo >= 500 ? Resultado::ERROR_RED : Resultado::ERROR_API;
    }
    respuesta = "";
    for (JsonVariant parte : doc["candidates"][0]["content"]["parts"].as<JsonArray>()) {
        respuesta += parte["text"] | "";
    }
    respuesta.trim();
    if (respuesta.isEmpty()) {
        const char* bloqueo = doc["promptFeedback"]["blockReason"] | "";
        respuesta = *bloqueo ? String("Gemini no quiso responder (") + bloqueo + ")."
                             : "Gemini respondio vacio. Proba con otra pregunta.";
        return Resultado::ERROR_API;
    }
    return Resultado::RESPONDIO;
}

}  // namespace

bool preguntarAGemini(const uint8_t* jpeg, size_t largo, const String& pregunta,
                      const char* modelo, const String& claveApi, String& respuesta) {
    if (claveApi.isEmpty()) {
        respuesta = "Falta tu API key de Gemini: cargala en la tarjeta Agente del panel.";
        return false;
    }
    Resultado r = intentarUnaVez(jpeg, largo, pregunta, modelo, claveApi, respuesta);
    if (r == Resultado::ERROR_RED) {
        // Un solo reintento: las fallas de red suelen ser pasajeras (medido con senal debil).
        Serial.printf("Primer intento fallido (%s). Reintentando...\n", respuesta.c_str());
        delay(500);
        r = intentarUnaVez(jpeg, largo, pregunta, modelo, claveApi, respuesta);
    }
    return r == Resultado::RESPONDIO;
}

