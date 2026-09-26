# Lección 05: los ojos de un agente

**Objetivo:** que la ESP32-CAM sea **los ojos de un agente de inteligencia artificial**. La placa
saca una foto, se la manda a **Gemini** (la IA de Google) junto con una pregunta, y Gemini
responde lo que ve. Cada estudiante usa **su propia API key**.

## La idea

```
   ┌────────────┐   foto + pregunta    ┌──────────────┐
   │ ESP32-CAM  │ ───────────────────▶ │   Gemini     │
   │  (los ojos)│ ◀─────────────────── │ (el cerebro) │
   └────────────┘   "Veo una mesa..."  └──────────────┘
         ▲
         │ "¿hay alguien con casco?"
     vos (panel o celular)
```

Un **agente** es un programa que **percibe** algo del mundo, **razona** y **actúa o responde**. La
cámara es la percepción: sin ella, la IA no sabe qué pasa en tu escritorio, en una obra o en una
puerta. Un dispositivo IoT de pocos dólares le da ojos a un modelo que corre en la nube.

Lo interesante es que **la pregunta define al agente**. La misma cámara puede ser:
- un descriptor ("¿qué ves?"),
- un inspector ("¿todos tienen casco?"),
- un contador ("¿cuántas personas hay?"),
- un vigilante ("¿la puerta está abierta?").

## Antes de empezar: tu API key de Gemini

1. Entrá a **[aistudio.google.com/apikey](https://aistudio.google.com/apikey)** con tu cuenta de Google.
2. Tocá **Create API key** y copiala. Empieza con `AIza...`.
3. En el panel, lección 05, tarjeta **Agente**: pegala → **Guardar en la placa**.

> **Tu API key es como una contraseña.** Con ella, cualquiera puede usar Gemini a tu nombre.
> No la pegues en el código, no la subas a GitHub, no la mandes por chat. El panel la manda
> **solo por el cable USB** y queda guardada en la memoria de la placa. Si prestás la placa,
> tocá **Borrar clave de la placa**.

El nivel gratuito de Gemini alcanza de sobra para esta lección. Si preguntás muy seguido, puede
aparecer "se agotó el límite gratuito": esperá un minuto.

## Cómo correrla

1. Panel → **Conectar placa** → lección 05 → **Cargar lección**.
2. Si ya cargaste el WiFi en la lección 04, la placa lo recuerda. Si no, cargalo en la tarjeta **WiFi**.
3. Guardá tu API key en la tarjeta **Agente**.
4. Escribí una pregunta (o dejala vacía para que describa) → **👁 Mirar y responder**.
5. Primero llega la foto que vio el agente y después la respuesta de Gemini.

**Sin compu:** abrí la **Página de la cámara** en el celular. Ahí también se puede preguntar: la
placa hace todo sola, conectada al WiFi.

## El código, parte por parte

El código está en **dos archivos**: `main.cpp` (la cámara, el WiFi, el agente y la página) y
`gemini.cpp` (cómo se habla con Gemini). Separar por responsabilidad hace que cada archivo se
entienda solo.

### Parte 1: el agente: modelo, pregunta y parámetros

**En palabras simples:** elegimos con qué "cerebro" va a pensar el agente (el modelo de Gemini),
qué pregunta hace si no le decís nada, y preparamos lugar para recordar la última foto y la
última respuesta.

**Conceptos nuevos**

- **Modelo de IA:** el programa entrenado que "entiende" imágenes y texto. Google tiene varios;
  usamos `gemini-3.5-flash-lite`, el más rápido y barato, con nivel gratuito. Los modelos se
  renuevan: si Google lo retira, se cambia una sola línea.
- **API:** la "ventanilla" por donde un programa le pide cosas a otro por internet. La **API key**
  es tu credencial para usar esa ventanilla.
- **La pila (*stack*):** la memoria donde cada función guarda sus variables mientras corre. Si
  una función llama a otra, que llama a otra..., todas se apilan. Arduino le da a `loop()` solo
  8 KB, y la conexión segura con Google usa mucho: la primera versión de esta lección **se
  reiniciaba** con el error `Stack canary watchpoint triggered` cuando se preguntaba desde la
  página web (ese camino pasa por el servidor y apila más funciones). `SET_LOOP_TASK_STACK_SIZE`
  le da 16 KB.
- **Puntero a memoria dinámica (`ultimaFoto`):** un lugar de memoria que pedimos mientras el
  programa corre (con `ps_malloc`, en la PSRAM) para guardar una copia de la foto.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `SET_LOOP_TASK_STACK_SIZE(16 * 1024);` | Le da a `loop()` 16 KB de pila en vez de 8 KB. |
| `const char* MODELO = "gemini-3.5-flash-lite";` | El modelo de Gemini que se usa. |
| `const char* PREGUNTA_POR_DEFECTO = "Describí lo que ves en la imagen.";` | Qué pregunta si la dejás vacía. |
| `CALIDAD_JPEG`, `GIRAR_180` (parámetros) | Como en la lección 04. |
| `String ultimaRespuesta`, `uint8_t* ultimaFoto` | Recuerdan la última respuesta y la última foto, para mostrarlas en la página. |

### Parte 2: cámara

**En palabras simples:** igual que la lección 04, con fotos de 800 × 600. Para que una IA entienda
una escena no hace falta más, y una foto chica viaja rápido.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `config.frame_size = FRAMESIZE_SVGA;` | 800 × 600 píxeles. |
| `config.fb_count = 2;` y `CAMERA_GRAB_LATEST` | Siempre la foto más reciente (lección 04). |

### Parte 3: conectarse al WiFi

**En palabras simples:** igual que la lección 04. La red que cargaste en el panel quedó guardada
en la placa, así que la usa directamente.

### Parte 4: mirar y preguntar

**En palabras simples:** es el corazón del agente. Saca una foto, se la muestra al panel, se la
manda a Gemini con la pregunta y reparte la respuesta: al panel y a la página web.

**Conceptos nuevos**

- **Percibir → razonar → responder:** la foto es la percepción, Gemini razona, y la respuesta
  vuelve a quien preguntó. Es el ciclo de cualquier agente.
- **`String& respuesta` (el `&`):** la función **escribe** su resultado en la variable que le
  pasás. Así devuelve dos cosas: si salió bien (`true`/`false`) y el texto.
- **Copiar memoria (`memcpy`):** copia los bytes de la foto a nuestra reserva, porque la foto de
  la cámara es prestada y hay que devolverla.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `panel::parte(4);` | Marca la Parte 4: si pausás, la placa se detiene antes de mirar. |
| `if (pregunta.isEmpty()) pregunta = PREGUNTA_POR_DEFECTO;` | Sin pregunta, pide una descripción. |
| `camera_fb_t* fb = esp_camera_fb_get();` | Saca la foto. |
| `ultimaFoto = (uint8_t*)ps_malloc(fb->len);` y `memcpy(...)` | Guarda una copia para la página web. |
| `panel::enviarJpeg(fb->buf, fb->len);` | Te muestra en el panel lo mismo que va a ver el agente. |
| `preguntarAGemini(fb->buf, fb->len, pregunta, MODELO, panel::secreto("GEMINI_KEY"), respuesta)` | Manda foto y pregunta a Gemini con **tu** API key. Tarda unos segundos. |
| `esp_camera_fb_return(fb);` | Devuelve la foto prestada. |
| `panel::anunciarRespuestaIA(respuesta);` | Manda la respuesta al panel (o el error, si falló). |

### Parte 5: la página web: preguntarle al agente desde el celular

**En palabras simples:** la misma idea de la lección 04, pero la página tiene un cuadro para
escribir la pregunta. Al tocar el botón, el celular le pide a la placa `/preguntar`, la placa
mira, le pregunta a Gemini y contesta con el texto. Después muestra la foto que vio el agente.

**Conceptos nuevos**

- **`fetch` y `async`/`await` (en la página):** el celular pide algo a la placa y **espera** la
  respuesta sin congelar la página.
- **`encodeURIComponent`:** convierte la pregunta (con espacios, tildes, signos) en algo que
  puede viajar dentro de una dirección web.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `servidor.on("/preguntar", paginaPreguntar);` | Ruta que dispara al agente. |
| `mirarYPreguntar(servidor.arg("q"), respuesta);` | Toma la pregunta de la dirección (`?q=...`) y llama a la Parte 4. |
| `servidor.send(200, "text/plain; charset=utf-8", respuesta);` | Devuelve la respuesta como texto (con tildes). |
| `servidor.send_P(200, "image/jpeg", ...ultimaFoto...)` | En `/ultima`, devuelve la foto que vio el agente. |

### Parte 6: `setup()` y `loop()`

**En palabras simples:** al arrancar, avisa si falta la API key, prepara la cámara y el WiFi, y
enciende el servidor. En `loop()` atiende la página web y, si desde el panel escribiste
`? tu pregunta`, mira y responde.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `if (panel::secreto("GEMINI_KEY").isEmpty())` | Si no cargaste tu API key, lo avisa. |
| `servidor.on("/", ...)`, `"/preguntar"`, `"/ultima"` | Las tres rutas de la página. |
| `if (panel::leerLinea(linea) && linea.startsWith("?"))` | Una línea que empieza con `?` es una pregunta para el agente. |

### `gemini.cpp`: cómo se habla con Gemini

**En palabras simples:** abre una **conexión segura** con los servidores de Google, manda la foto y
la pregunta en el formato que pide la API, y lee la respuesta.

**Conceptos nuevos**

- **HTTPS y certificados:** HTTPS es HTTP **cifrado**, así nadie en el camino puede leer tu API
  key. Los **certificados raíz** (`RAICES_GOOGLE`, escritos en el código) le permiten a la placa
  comprobar que del otro lado está **Google** y no un impostor. Van **cuatro**, porque Google
  alterna entre cadenas: con uno solo, 1 de cada 4 conexiones fallaba (lo medimos). La opción fácil sería saltarse la
  verificación (`setInsecure()`), pero así cualquiera podría hacerse pasar por Google y robarte la key.
- **JSON:** el formato de texto con que se hablan la mayoría de las APIs:
  `{"campo": "valor"}`. Se usa la librería **ArduinoJson** para leer la respuesta.
- **Base64 (lección 03):** la foto viaja dentro del JSON como texto. Se codifica **de a pedazos**
  mientras se envía, así la placa nunca necesita tener en memoria la foto entera codificada.
- **Cabecera `x-goog-api-key`:** la API key viaja en una cabecera del pedido y no en la
  dirección, para que no quede anotada en registros.
- **Reintentar solo lo que vale la pena:** si se corta la red en el camino, se reintenta una vez
  (suele ser pasajero). Si Gemini contesta "clave inválida", no: probar de nuevo no lo arregla.
- **Códigos HTTP:** 200 = bien; 400 = pedido mal armado (por ejemplo, key inválida); 429 =
  demasiados pedidos; 5xx = problema del servidor. `explicarError()` los traduce a mensajes que
  se entienden.

**Línea por línea (lo esencial)**

| Código | Qué hace |
|---|---|
| `red.setCACert(RAICES_GOOGLE);` | "Confiá solo en servidores certificados por Google Trust Services". |
| `static_cast<Stream&>(red).setTimeout(ESPERA_MAXIMA_MS);` | Cuánto esperar la respuesta (40 s). Una trampa de la librería hacía que esperara solo 1 s: ver el comentario en el código. |
| `while (!red.available() && ...) delay(20);` | Espera a que Gemini termine de "pensar" y empiece a contestar. |
| `red.connect(SERVIDOR, 443)` | Abre la conexión segura (el 443 es el puerto de HTTPS). |
| `red.printf("POST /v1beta/models/%s:generateContent ...")` | Pide al modelo que genere una respuesta. |
| `red.print("x-goog-api-key: " + claveApi ...)` | Presenta tu API key. |
| `red.print(principio); enviarBase64(...); red.print(cierre);` | Manda el JSON: la pregunta, la foto en base64 y el cierre. |
| `deserializeJson(doc, red);` | Lee la respuesta JSON. |
| `doc["candidates"][0]["content"]["parts"]` | Ahí está el texto que escribió Gemini. |

## Qué deberías ver

Salida real (2026-09-25, habitación con poca luz):

```
Preguntando a gemini-3.5-flash-lite: "¿Qué objetos hay?" (foto de 21420 bytes)...
Gemini tardo 3.8 s
```

> *"La imagen es muy oscura y está inclinada. Se distingue la esquina de un mueble oscuro y parte
> de un teclado o dispositivo electrónico iluminado abajo a la derecha. El resto no se ve bien."*

Y en la foto que vio el agente había, efectivamente, un mueble oscuro y un teclado azul
iluminado abajo a la derecha. Fijate que el agente **dice cuando no distingue algo**: es la
instrucción que le dimos en `gemini.cpp`.

## Errores comunes

| Mensaje | Qué hacer |
|---|---|
| "Falta tu API key de Gemini" | Cargala en la tarjeta **Agente**. |
| "La API key no es válida" | Copiala de nuevo desde AI Studio, sin espacios. |
| "Se agotó el límite gratuito" | Esperá un minuto. |
| "No se pudo conectar con Google" | La placa no tiene internet: revisá el WiFi. |
| "Se cortó la conexión..." / "Google cerró la conexión sin responder" | Señal WiFi débil: acercá la placa al router. La placa ya reintentó una vez. |

## Ejercicios

1. **Inspector de seguridad:** preguntá "¿las personas usan casco? Respondé solo SÍ o NO".
   ¿Qué tan confiable es? Probá con distintas fotos.
2. **Contador:** "¿Cuántos objetos hay sobre la mesa? Respondé solo con un número."
3. Cambiá la `INSTRUCCION` de `gemini.cpp` para que el agente responda como un guía de museo.
   (Requiere PlatformIO.)
4. **Agente vigilante:** hacé que `loop()` pregunte cada 30 segundos "¿hay una persona?" y que
   prenda el flash si la respuesta empieza con "Sí". (Requiere PlatformIO.)
5. Probá `CALIDAD_JPEG` en 40: ¿Gemini sigue entendiendo la escena con una foto pixelada?
