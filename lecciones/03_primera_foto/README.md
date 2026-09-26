# Lección 03: primera foto

**Objetivo:** iniciar la cámara, capturar una foto y entender qué es un *frame buffer*.
Todavía no vemos la imagen: comprobamos que llegó entera. En la lección 04 la vemos en el navegador.

## Cómo funciona la cámara (en 30 segundos)

```
 Sensor OV2640 ──(8 cables de datos + relojes)──▶ ESP32 ──▶ frame buffer (PSRAM)
      ▲                                              │
      └────────── SCCB (tipo I2C): configuración ────┘
```

- El sensor manda la imagen **ya comprimida en JPEG** por un bus paralelo de 8 bits.
- El ESP32 la guarda en un bloque de memoria: el **frame buffer** (`camera_fb_t`).
- Por un bus aparte (SCCB) el ESP32 le cambia la configuración al sensor: brillo, espejo, etc.

## El código, parte por parte

### `camera_pins.h`

Los 16 pines que conectan el sensor. Están **soldados en la placa**: no se eligen, se copian
del esquema de la AI-Thinker. Si algún día usás otra placa (M5Stack, ESP-EYE), este es el
único archivo que cambia. Notá que `XCLK` es el **GPIO 0**, el mismo pin que se pone a GND
para cargar programas.

### Parte 1: configurar e iniciar la cámara

**En palabras simples:** antes de sacar fotos hay que decirle al programa **cómo está conectada
la cámara** y **cómo queremos las fotos** (tamaño, calidad, dónde guardarlas). Después se
enciende la cámara y se comprueba que respondió.

**Conceptos nuevos**

- **Sensor de imagen:** el "ojo" de la cámara (en esta placa, un chip OV2640). Convierte la luz
  en datos. Está unido a la placa por el cable plano dorado.
- **Driver:** un programa ya escrito por el fabricante (Espressif) que sabe hablar con la
  cámara. Nosotros solo lo configuramos y le pedimos fotos.
- **Estructura (`camera_config_t`):** una variable que agrupa muchos datos relacionados, como
  un formulario con campos. `config.frame_size` es el campo "tamaño de foto" del formulario.
- **Píxel y resolución:** la foto es una grilla de puntitos (píxeles). UXGA = 1600 × 1200 píxeles
  (casi 2 millones: "2 megapíxeles").
- **JPEG:** formato que **comprime** la foto para que pese mucho menos. El sensor lo hace solo.
  Una foto sin comprimir de 800 × 600 pesaría ~960 KB; en JPEG, unos 20-60 KB.
- **Calidad JPEG:** número de 0 a 63 donde **menor = mejor calidad** (y archivo más pesado).
  Suena al revés, pero así funciona este sensor.
- **PSRAM:** 4 MB de memoria extra. Una foto grande no entra en la memoria normal del chip
  (~320 KB), por eso con PSRAM se usa 1600 × 1200 y sin ella hay que achicar a 800 × 600.
- **`bool` como resultado:** `iniciarCamara()` devuelve `true` si todo salió bien y `false` si
  falló. Así `setup()` puede avisar el problema en vez de seguir como si nada.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `CALIDAD_JPEG` (parámetro, 10) | Calidad de las fotos. Se cambia desde el panel y se aplica en la foto siguiente. |
| `GIRAR_180` (parámetro, 1) | En esta placa el sensor va montado "de cabeza": sin girar, la foto sale invertida. 1 = girarla, 0 = dejarla como sale. |
| `camera_config_t config = {};` | Crea el "formulario" de configuración, vacío. |
| `config.pin_... = CAM_PIN_...;` (16 líneas) | Le dice al driver a qué pin va cada cable de la cámara. Están soldados en la placa: se copian del archivo `camera_pins.h`. |
| `config.xclk_freq_hz = 20000000;` | La cámara necesita un "reloj" que marque el ritmo: 20 millones de pulsos por segundo. Lo genera el canal 0 de PWM (por eso el flash usa otro). |
| `config.pixel_format = PIXFORMAT_JPEG;` | Pide las fotos ya comprimidas en JPEG. |
| `if (psramFound()) { ... } else { ... }` | **Si** hay PSRAM: fotos grandes (1600 × 1200) guardadas ahí. **Si no**: fotos de 800 × 600 en la memoria normal. |
| `config.fb_count = 1;` | Un solo lugar ("buffer") para guardar la foto. |
| `esp_camera_init(&config)` | Enciende la cámara con esa configuración. Devuelve un código: `ESP_OK` si salió bien. |
| `if (err != ESP_OK) { ... return false; }` | Si falló (`!=` significa "distinto de"), avisa el código de error y devuelve `false`. |
| `s->id.PID == OV2640_PID ? "OV2640" : ...` | Pregunta el modelo del sensor y lo escribe. El `? :` es un "si / si no" en una sola línea. |

### Parte 2: tomar una foto e inspeccionarla

**En palabras simples:** tira la foto vieja que el driver tenía guardada, le pide una nueva,
cuenta cuánto pesa y qué tamaño tiene, revisa que el archivo esté completo, si se la pediste
**la manda al panel** para que la veas, y **la devuelve** para que el driver pueda sacar la siguiente.

**Conceptos nuevos**

- **Frame buffer (`camera_fb_t`):** el lugar de memoria donde el driver deja la foto. Trae los
  bytes de la imagen (`buf`), cuánto pesa (`len`), el ancho (`width`) y el alto (`height`).
- **Puntero (`*`):** en vez de copiar la foto entera (pesada), el driver nos da **la dirección**
  de dónde está guardada. `fb` es esa dirección, y `fb->len` se lee "el `len` de lo que está en `fb`".
- **Byte y hexadecimal:** un byte es un número de 0 a 255. En hexadecimal (base 16) se escribe
  con dos símbolos: `FF` = 255, `D8` = 216. Es la forma habitual de mostrar bytes.
- **Firma de archivo:** todo JPEG empieza con los bytes `FF D8` y termina con `FF D9`. Si no,
  la foto llegó cortada.
- **La foto vieja:** el driver siempre tiene una foto **ya sacada**, esperando: la tomó en el
  momento en que devolviste la anterior, que pudo ser hace minutos. Por eso primero se pide esa y
  se tira, y recién la siguiente es una foto de **ahora**.
- **Base64:** el cable serie está pensado para texto, y una foto son bytes de cualquier valor.
  Base64 traduce cada 3 bytes a 4 letras "seguras" (A-Z, a-z, 0-9, + y /). La foto viaja un 33 %
  más pesada, pero llega sin que ningún byte se confunda con un salto de línea. El panel hace la
  traducción inversa y la dibuja.
- **Pedir prestado y devolver:** la foto es **prestada**. Si no se devuelve con
  `esp_camera_fb_return()`, el driver se queda sin lugar y no puede sacar más fotos. Como un
  libro de biblioteca.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `panel::parte(2);` | Marca la Parte 2. Si hay pausa, la placa se detiene **antes de sacar la foto**. |
| `s->set_quality(s, CALIDAD_JPEG);` | Aplica la calidad elegida en el panel. |
| `s->set_vflip(s, GIRAR_180);` | Da vuelta la imagen de arriba a abajo (*flip* vertical). |
| `s->set_hmirror(s, GIRAR_180);` | Y de izquierda a derecha (espejo). Las dos juntas = girar 180 grados. |
| `void tomarFoto(bool mostrar)` | La función recibe un sí/no: `mostrar` = `true` si además hay que mandarla al panel. |
| `esp_camera_fb_return(esp_camera_fb_get());` | Pide la foto vieja y la devuelve en el acto: queda descartada. |
| `unsigned long t0 = millis();` | Anota la hora (en milisegundos desde que arrancó) para medir cuánto tarda. |
| `camera_fb_t* fb = esp_camera_fb_get();` | Pide una foto. `fb` queda apuntando a ella. |
| `if (!fb) { ... return; }` | Si no llegó ninguna foto (`!` = "no"), avisa y termina la función. |
| `Serial.printf("Foto: %ux%u px, %u bytes ...")` | Escribe ancho, alto, peso y tiempo. |
| `bool inicioOk = ... fb->buf[0] == 0xFF && fb->buf[1] == 0xD8;` | ¿Los dos primeros bytes son `FF D8`? `&&` significa "y además". |
| `bool finOk = ...` | ¿Los dos últimos son `FF D9`? |
| `if (mostrar) panel::enviarJpeg(fb->buf, fb->len);` | Si se la pediste, la manda al panel en base64. A 921600 baudios, una foto de 200 KB tarda unos 3 segundos. |
| `esp_camera_fb_return(fb);` | **Devuelve la foto prestada.** Es la línea más importante de la parte. |

> **¿Cuánto tarda sacar una foto?** Entre 90 y 160 ms para 1600 × 1200. Si sacás la línea que
> descarta la foto vieja, vas a ver "0 ms": no es que sea instantánea, es que el driver te da
> la que ya tenía guardada. Probalo como ejercicio.

### Parte 3: `setup()`, la preparación y el *brownout*

**En palabras simples:** al arrancar, desactiva una protección de voltaje que en esta placa
salta de más, abre el monitor, avisa si hay PSRAM y enciende la cámara. Si la cámara no
responde, se queda quieta y te dice qué revisar.

**Conceptos nuevos**

- **Brownout:** una caída de voltaje. Cuando la cámara arranca consume de golpe mucha energía;
  con un cable USB largo o flojo, el voltaje baja y el chip, para protegerse, se **reinicia**. A
  veces entra en un bucle de reinicios.
- **Registro:** una posición especial de memoria que controla el hardware.
  `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0)` escribe un 0 en el registro del detector de
  brownout, y así lo apaga.
- **Bucle infinito `while (true)`:** repite para siempre. Acá se usa para **detener** el
  programa a propósito si la cámara falló: no tiene sentido seguir.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);` | Apaga el detector de brownout. **Es un parche**: lo correcto es alimentar la placa con 5 V estables y un buen cable. |
| `Serial.begin(921600);` | Abre el monitor serie, rápido: las fotos viajan por acá. |
| `panel::iniciar(...)` y `panel::parte(3);` | Se presenta al panel y marca la Parte 3. |
| `if (!iniciarCamara()) { ... while (true) delay(1000); }` | Llama a la Parte 1. **Si no** pudo iniciar la cámara, avisa y se queda esperando para siempre. |

### Parte 4: `loop()`, esperar la orden de foto

**En palabras simples:** la placa espera. Si escribís `f`, saca una foto y la inspecciona. Si
escribís `v` (o tocás **Sacar y ver foto** en el panel), además te la manda para verla.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `panel::parte(4);` | Marca la Parte 4. |
| `String linea;` | Cajita para el texto que escribas. |
| `if (panel::leerLinea(linea)) {` | **Si** escribiste algo, lo guarda en `linea`. |
| `if (linea.equalsIgnoreCase("f")) tomarFoto(false);` | Si es `f` (mayúscula o minúscula): foto solo para inspeccionar. |
| `else if (linea.equalsIgnoreCase("v")) tomarFoto(true);` | Si no, y es `v`: foto para ver en el panel. |

**Probalo en el panel**

1. Tocá **Sacar y ver foto**: en unos segundos aparece en la tarjeta **Foto**. Tocala para verla grande.
2. Cambiá `CALIDAD_JPEG` a 40 en **Variables** y sacá otra: pesa mucho menos y se ve
   **pixelada** (cuadriculada), porque el JPEG tira detalle para ahorrar. Volvé a 10 para fotos
   nítidas. Menor número = mejor calidad.
3. Poné `GIRAR_180` en 0 y sacá otra: la foto sale invertida, tal como la ve el sensor.
4. Tocá **Pausar** y después **Sacar y ver foto**: la placa se detiene en la Parte 2, **antes**
   de sacarla. Con **Paso** la ejecuta.

## Qué deberías ver

```
Leccion 03: primera foto
PSRAM: SI
[OK] Camara lista. Sensor PID=0x26 (OV2640)
Escribi 'f' para inspeccionar una foto, o 'v' para verla en el panel.
Foto: 1600x1200 px, 226991 bytes, 89 ms
Firma JPEG: inicio OK, fin OK
```

(Salida real de una ESP32-CAM con OV2640, 2026-09-25. El peso depende mucho de la escena: con
poca luz la foto tiene "ruido" (granulado) y pesa más.)

> La primera foto después de encender puede salir oscura o verdosa: el sensor todavía
> está ajustando la exposición y el balance de blancos. Por eso conviene descartar unas fotos
> al arrancar.

## Errores comunes

| Síntoma | Causa probable |
|---|---|
| `esp_camera_init fallo: 0x105` / `0x20001` | Cable plano de la cámara mal insertado. Levantá la traba negra, metelo derecho hasta el fondo y cerrala. |
| `Brownout detector was triggered` en bucle | Alimentación insuficiente (ver parte 3). |
| `PSRAM: NO` | Placa clon sin PSRAM, o el `board` no es `esp32cam`. |
| La segunda foto da `No se pudo capturar` | Faltó `esp_camera_fb_return()`. |

## Ejercicios

1. Desde el panel, poné `CALIDAD_JPEG` en 4, 10, 30 y 60. Anotá el peso de cada foto y mirá
   dónde empieza a notarse la pérdida de calidad.
2. Probá `FRAMESIZE_QVGA` (320×240). ¿Cuánto más rápida es la captura? ¿Y el envío al panel?
3. Tomá 5 fotos seguidas apenas arranca: ¿cambia el tamaño? (Pista: el JPEG de una imagen
   oscura comprime más, así que pesa menos).
4. Comentá la línea que descarta la foto vieja y sacá fotos con `f` separadas por un minuto.
   ¿Qué pasa con el tiempo de captura? ¿Y con lo que muestra la foto?
