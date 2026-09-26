# Lección 04: ver la foto en el navegador

**Objetivo:** conectar la ESP32-CAM al WiFi y convertirla en un pequeño **servidor web**: una
página que se abre desde cualquier celular o compu de tu casa y muestra la foto, con un control
para el flash.

## Cómo correrla

**Con el panel (sin instalar nada):**

1. Doble clic en `iniciar.bat` → **Conectar placa** → elegí la lección 04 → **Cargar lección**.
2. En la tarjeta **WiFi**, escribí el nombre de tu red y la clave → **Guardar en la placa**.
   La placa las guarda y se reinicia para conectarse.
3. Cuando se conecta, aparece la tarjeta **Página de la cámara** con el enlace y la opción
   **Ver en vivo**.

**Con VS Code y PlatformIO:** abrí `lecciones/04_servidor_web`, cargá el programa y en el monitor
escribí `WIFI`, un espacio, el nombre de la red, la tecla **TAB** y la clave. Es el mismo mensaje
que manda el panel.

> La ESP32 **solo se conecta a redes de 2.4 GHz**. Si tu router tiene una red "5G", usá la otra.
> La red queda guardada en la placa: la lección 05 la vuelve a usar sin pedírtela.

## Cómo se comunican

```
 Navegador (celular/PC)                    ESP32-CAM (IP 192.168.x.y)
   GET /          ───────────────────────▶  paginaInicio() → la página (HTML)
   GET /foto      ───────────────────────▶  paginaFoto()   → la foto (JPEG)
   GET /flash?brillo=30 ─────────────────▶  paginaFlash()  → prende el flash al 30 %
   GET /video     ───────────────────────▶  paginaVideo()  → cuadros JPEG sin parar (video)
```

Los dos tienen que estar **en la misma red WiFi**.

## El código, parte por parte

### Parte 1: constantes, parámetros y el servidor

**En palabras simples:** anotamos los pines y los tiempos, las variables que se pueden ajustar
desde el panel, y creamos el **servidor web**: la parte del programa que va a atender a los
navegadores que se conecten.

**Conceptos nuevos**

- **Servidor web:** un programa que espera pedidos ("dame la página", "dame la foto") y los
  contesta. Cada vez que abrís una página en internet, hay un servidor del otro lado. Acá el
  servidor es la propia placa.
- **Puerto 80:** una red tiene "puertas" numeradas. La 80 es la que usan por defecto las páginas
  web (`http://`): por eso no hace falta escribirla en la dirección.
- **Objeto (`WebServer servidor(80);`):** una variable que además de datos trae funciones propias.
  `servidor.on(...)`, `servidor.send(...)` son funciones **del** servidor.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `const int LED_FLASH = 4;` | El flash, en el pin 4 (lección 01). |
| `const int CANAL_FLASH = 4;` | Canal de PWM del flash, distinto del 0 de la cámara (lección 02). |
| `const unsigned long WIFI_TIMEOUT_MS = 20000;` | Cuánto esperar la conexión antes de rendirse: 20 segundos. |
| `CALIDAD_JPEG` (parámetro, 12) y `GIRAR_180` (parámetro, 1) | Como en la lección 03: calidad de la foto y giro. Se aplican en la foto siguiente. |
| `WebServer servidor(80);` | Crea el servidor web en el puerto 80. |

### Parte 2: cámara, con dos cambios respecto a la lección 03

**En palabras simples:** se configura la cámara igual que en la lección 03, pero pensando en
mandar fotos por WiFi: más chicas y siempre recién sacadas.

**Conceptos nuevos**

- **SVGA (800 × 600) en vez de UXGA (1600 × 1200):** la foto tiene la cuarta parte de píxeles,
  pesa unas 4 veces menos y viaja mucho más rápido por el aire.
- **Dos buffers + `CAMERA_GRAB_LATEST`:** en la lección 03 tirábamos la foto vieja a mano. Acá el
  driver usa **dos lugares** para guardar fotos: mientras vos usás una, él sigue sacando en el
  otro, y siempre te entrega **la más reciente**. Ideal para ver "en vivo".

**Línea por línea**

| Código | Qué hace |
|---|---|
| `config.pin_... = CAM_PIN_...;` | Los pines de la cámara, igual que en la lección 03. |
| `config.frame_size = FRAMESIZE_SVGA;` | Fotos de 800 × 600. |
| `config.fb_count = 2;` | Dos lugares para fotos. |
| `config.grab_mode = CAMERA_GRAB_LATEST;` | Entregar siempre la más nueva. |
| `return esp_camera_init(&config) == ESP_OK;` | Enciende la cámara y devuelve `true` si salió bien. |

### Parte 3: conectarse al WiFi

**En palabras simples:** la placa lee la red y la clave que le pasaste desde el panel, se conecta
al router y espera, con un tiempo máximo, a que la conexión esté lista.

**Conceptos nuevos**

- **WiFi en modo estación (`WIFI_STA`):** la placa se conecta a un router existente, como tu
  celular. (El otro modo, "punto de acceso", es cuando la placa **crea** su propia red.)
- **Dirección IP:** el "número de casa" que el router le da a la placa en la red, por ejemplo
  `192.168.1.47`. Con ese número los demás equipos la encuentran.
- **Las claves no van en el código:** la red y la clave viajaron por el cable USB y quedaron
  guardadas en la memoria de la placa. Si el código se comparte (como este, que es público),
  **nadie ve tu clave**. Regla de oro de la programación.
- **Tiempo límite:** si la clave está mal, `WiFi.status()` nunca dice "conectado". Un `while`
  sin salida dejaría la placa colgada para siempre sin avisar. Por eso se mide el tiempo con
  `millis()` y se abandona a los 20 segundos.
- **RSSI (intensidad de señal):** se mide en dBm, siempre negativo. Más cerca de 0 = mejor.
  Mayor que −67 es buena; menor que −80, débil. La antena de esta placa es chiquita.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `while (!panel::hayWifi()) { ... }` | Mientras no le hayas cargado una red, avisa al panel y espera (atendiendo al panel). |
| `String red = panel::wifiRed();` | Lee el nombre de la red guardada. |
| `WiFi.mode(WIFI_STA);` | Modo cliente del router. |
| `WiFi.setSleep(false);` | Sin ahorro de energía del radio: responde más rápido. |
| `WiFi.begin(red.c_str(), panel::wifiClave().c_str());` | Empieza a conectarse. `c_str()` pasa el texto en el formato que pide la función. |
| `while (WiFi.status() != WL_CONNECTED) { ... }` | Mientras **no** esté conectada, espera medio segundo y escribe un punto. |
| `if (millis() - t0 > WIFI_TIMEOUT_MS) { ... return false; }` | Si pasaron más de 20 s, se rinde y avisa. |
| `WiFi.RSSI()` | Mide la señal y la escribe. |

### Parte 4: las páginas del servidor

**En palabras simples:** son las respuestas que da la placa a cada pedido. Si te piden `/`,
devuelve la página. Si te piden `/foto`, saca una foto y la manda. Si te piden `/video`, manda
fotos sin parar mientras las miren. Si te piden `/flash`, prende el flash con el brillo indicado.

**Conceptos nuevos**

- **HTML:** el lenguaje en que se escriben las páginas web. Dice qué hay (un título, una imagen,
  un botón). La página vive **adentro del programa**, como un texto largo.
- **Raw string `R"html( ... )html"`:** una forma de C++ de escribir un texto largo tal cual, con
  comillas y saltos de línea. El `html` marca dónde termina. (Un `R"( ... )"` simple se
  cortaría en el primer `)"` del HTML, y hay uno en `onclick="nuevaFoto()"`: nos pasó al escribir
  esta lección.)
- **Ruta:** la parte de la dirección después del número: `/`, `/foto`, `/flash`. Cada ruta la
  atiende una función distinta.
- **Parámetro en la dirección (`?brillo=30`):** un dato que viaja en el pedido.
  `servidor.arg("brillo")` lo lee.
- **Caché:** el navegador guarda copias para no pedir dos veces lo mismo. La página agrega
  `?t=<hora>` a la dirección de la foto para que siempre pida una nueva.
- **Video MJPEG:** un video "de fotos": la placa manda un JPEG, después otro, y otro, dentro de
  **una sola respuesta que no termina nunca**. Cada cuadro va separado por la marca
  `--cuadro`, y el navegador reemplaza la imagen con cada cuadro que llega. Según la señal WiFi,
  se ven de 3 a 15 cuadros por segundo.
- **Un espectador a la vez:** mientras alguien mira el video, la función `paginaVideo()` no
  termina, así que `loop()` no corre y **la placa no atiende otras páginas**. Por eso la página
  corta el video antes de mover el flash. Los servidores "de verdad" atienden a muchos a la vez
  usando **tareas en paralelo**, un tema más avanzado.
- **Nunca confiar en lo que llega:** cualquiera en la red puede abrir `/flash?brillo=100`. Por
  eso se recorta con `constrain` a un máximo de 50 %.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `const char PAGINA_HTML[] = R"html(...)html";` | La página entera, guardada como texto. |
| `void paginaInicio() { ... servidor.send(200, "text/html", PAGINA_HTML); }` | Responde a `/` con la página. El 200 significa "todo bien". |
| `panel::parte(4);` (en cada página) | Marca la Parte 4: si pausás desde el panel, el navegador queda **esperando** la respuesta. |
| `s->set_quality(...)`, `set_vflip`, `set_hmirror` | Aplica calidad y giro del panel. |
| `camera_fb_t* fb = esp_camera_fb_get();` | Pide la foto más reciente. |
| `servidor.setContentLength(fb->len);` | Avisa al navegador cuánto va a pesar. |
| `servidor.send(200, "image/jpeg", "");` | Dice "va una foto JPEG". |
| `servidor.sendContent((const char*)fb->buf, fb->len);` | Manda los bytes de la foto directo desde la memoria de la cámara, sin copiarlos. |
| `esp_camera_fb_return(fb);` | Devuelve la foto prestada (lección 03). |
| `void paginaVideo()` | El video: arma la respuesta a mano con `WiFiClient`. |
| `cliente.print("... multipart/x-mixed-replace; boundary=cuadro ...")` | Avisa al navegador: "van muchas imágenes, separadas por `--cuadro`". |
| `while (cliente.connected()) { ... }` | Mientras el navegador siga mirando: saca una foto, la manda con su tamaño y la devuelve. |
| `panel::parte(4);` (dentro del video) | Si pausás desde el panel, el video se congela en el acto. |
| `Serial.printf("Video terminado: %d cuadros ...")` | Al cerrar el video, informa cuántos cuadros por segundo logró. |
| `constrain(servidor.arg("brillo").toInt(), 0, 50)` | Lee el brillo pedido y lo recorta entre 0 y 50 %. |
| `ledcWrite(CANAL_FLASH, round(4095 * pow(... , 2.2)))` | Porcentaje → PWM con la corrección gamma de la lección 02. |

### Parte 5: `setup()` y `loop()`

**En palabras simples:** al arrancar, prepara el flash, la cámara y el WiFi, le dice al servidor
qué función atiende cada ruta, lo enciende y avisa la dirección. Después, en `loop()`, atiende
los pedidos que van llegando, uno por uno, para siempre.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `Serial.begin(921600);` y `panel::iniciar(...)` | Monitor rápido y aviso al panel (lecciones anteriores). |
| `ledcSetup(CANAL_FLASH, 5000, 12);` | PWM del flash con 12 bits (lección 02). |
| `while (!conectarWiFi()) { ... }` | Si no se conectó, avisa al panel y reintenta cada 10 s: así podés corregir la clave sin reiniciar a mano. |
| `servidor.on("/foto", paginaFoto);` | "Cuando alguien pida `/foto`, llamá a `paginaFoto`". |
| `servidor.begin();` | Enciende el servidor. |
| `panel::anunciarRed(ip, WiFi.RSSI());` | Le avisa al panel la IP y la señal, para que muestre el enlace. |
| `servidor.handleClient();` | Atiende **un** pedido si hay alguno esperando, y vuelve. |

> **No pongás `delay()` largos en `loop()`:** mientras la placa espera, nadie atiende la página
> y el navegador se queda cargando.

**Probalo en el panel**

1. Activá **Ver video en vivo** y movete delante de la cámara. Al apagarlo, el monitor dice
   cuántos cuadros por segundo logró.
2. Abrí el enlace desde el **celular** (conectado a la misma red) y mové el control del flash.
3. Tocá **Pausar**: el video se congela, porque la placa se detuvo en la Parte 4. **Continuar**
   lo destraba.
4. Cambiá `CALIDAD_JPEG` a 40 en **Variables** y fijate cómo se pixela la vista en vivo.

## Qué deberías ver

```
Leccion 04: servidor web
Conectando a 'MiCasa'.....
[OK] Conectado. Senal: -58 dBm
Abri en el navegador: http://192.168.1.47/
Foto enviada: 24519 bytes
```

## Errores comunes

| Síntoma | Causa probable |
|---|---|
| "La placa espera la red WiFi" | Todavía no cargaste la red en la tarjeta **WiFi** del panel. |
| "No se pudo conectar" | Clave mal escrita, red de 5 GHz o señal débil. Corregí la red en el panel: la placa reintenta sola. |
| La página no carga | El celular está en otra red (datos móviles, red de invitados). |
| La foto se ve cortada o gris abajo | Señal débil o fuente floja. Acercá la placa al router y usá un buen cable. |

## Seguridad

Este servidor **no tiene contraseña**: cualquiera en tu red puede ver la cámara. Sirve para
aprender en casa o en el aula. No lo expongas a internet (redirigir puertos del router) sin
agregar autenticación.

## Ejercicios

1. Agregá la ruta `/estado` que devuelva en texto la señal WiFi y la memoria libre (`ESP.getFreeHeap()`).
2. Medí cuántos cuadros por segundo logra el video con `CALIDAD_JPEG` en 10, 20 y 40. ¿Por qué cambia?
3. Agregá un botón para encender y apagar el LED rojo (GPIO 33, lógica invertida, cara de abajo).
