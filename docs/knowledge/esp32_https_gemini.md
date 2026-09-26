# ESP32 + HTTPS + Gemini: dos trampas medidas (2026-09-25)

Contexto: lección 05 (`lecciones/05_ojos_de_un_agente/src/gemini.cpp`), Arduino core 2.0.17
(`espressif32@6.12.0`), ESP32-CAM llamando a `generateContent` de la API de Gemini.

## 1. `WiFiClientSecure::setTimeout()` no cambia el tiempo de las lecturas

**Síntoma:** todas las preguntas fallaban en ~4 s con "Google no respondió a tiempo", aunque la
conexión, el certificado y el pedido estaban bien.

**Causa:** `WiFiClientSecure` declara **su propio** `int _timeout;` (WiFiClientSecure.h:35), que
tapa al `unsigned long _timeout` de `Stream` (Stream.h:41, que vale 1000 ms). `setTimeout(30)`
escribe en la copia de WiFiClientSecure. `readStringUntil()` y `deserializeJson(stream)` usan la
de `Stream`: **esperan 1 segundo**. Gemini tarda 2-4 s en responder, así que la placa cortaba
siempre antes. (En `WiFiClient`, en cambio, `setTimeout` sí llama a `Client::setTimeout`.)

**Arreglo:** `static_cast<Stream&>(red).setTimeout(40000);` más una espera explícita a que llegue
el primer byte (`while (!red.available() && red.connected() ...)`).

**Cómo se diagnosticó:** el mismo pedido (HTTP/1.0, cabecera `x-goog-api-key`, foto real en
base64) mandado con `curl` desde la PC recibía respuesta en ~40 ms de servidor. Entonces el
problema estaba en la placa, y el tiempo (~4 s = subida + 1 s) apuntó a la lectura.

**Lección de proceso:** una primera versión escribía "Gemini tardó X s" tanto si respondía como si
fallaba, y eso se leyó como "respondió dos veces". Un log que no distingue éxito de fracaso no es
evidencia.

## 2. Google alterna cadenas de certificados: hay que confiar en GTS Root R1 **y** R4

**Síntoma:** con la raíz GTS Root R1 embebida, ~1 de cada 4 conexiones fallaba con
`X509 - Certificate verification failed` (memoria libre: 186 KB, no era memoria).

**Medición:** 20 handshakes con `openssl s_client` contra `generativelanguage.googleapis.com`:
15 entregaron la cadena `WR2 → GTS Root R1` (RSA) y 5 `WE2 → GTS Root R4` (curva elíptica).
Un solo sondeo con `openssl` (el primero que se hizo) mostró solo R1 y engañó.

**Arreglo:** embeber las cuatro raíces GTS (R1–R4, vencen en 2036), descargadas de
`pki.goog/repo/certs/gtsr{1..4}.pem`. Verificado con `openssl verify`: el paquete valida las dos
cadenas reales; R1 sola **no** valida la de WE2 (control negativo).

| Raíz | Huella SHA-256 (inicio) |
|---|---|
| GTS Root R1 | D9:47:43:2A:BD:E7:B7:FA |
| GTS Root R2 | 8D:25:CD:97:22:9D:BF:70 |
| GTS Root R3 | 34:D8:A7:3E:E2:08:D9:BC |
| GTS Root R4 | 34:9D:FA:40:58:C5:E2:63 |

**Regla:** al fijar certificados raíz, muestrear **muchas** conexiones (no una) y embeber todas
las raíces que el proveedor publica para ese servicio.

## 3. Pila de `loop()` (8 KB) insuficiente para TLS

`Stack canary watchpoint triggered (loopTask)` al preguntar desde la página web (camino más
profundo: `handleClient` → handler → TLS). Arreglo: `SET_LOOP_TASK_STACK_SIZE(16 * 1024)` y
buffers grandes como `static`, fuera de la pila.

## Resultado

Tras los tres arreglos: 5/5 preguntas respondidas por WiFi (3.5–4.5 s cada una), 0 reinicios,
y la descripción coincide con la foto que vio el agente.
