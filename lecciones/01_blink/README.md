# Lección 01: Blink

**Objetivo:** hacer parpadear el LED blanco del flash (al lado de la cámara) y leer mensajes en el
monitor serie.
Si esta lección funciona, tenés todo el entorno listo: cable, driver, compilador y carga.

## Cómo correrla

**Con el panel (sin instalar nada):** doble clic en `iniciar.bat` → **Conectar placa** →
elegí la lección 01 → **Cargar lección**.

**Con VS Code y PlatformIO:**

1. En VS Code: **File → Open Folder…** y elegí `lecciones/01_blink` (la carpeta que tiene el
   `platformio.ini`, no la raíz del repo).
2. Conectá la ESP32-CAM al adaptador ESP32-CAM-MB y al USB (ver [hardware](../../docs/00_hardware_esp32cam.md)).
3. Barra inferior de PlatformIO: **→ Upload**, después **🔌 Monitor**.

## El código, parte por parte

### `platformio.ini`

| Línea | Qué hace |
|---|---|
| `platform = espressif32@6.12.0` | Fija la versión de la plataforma. Sin el `@`, cada persona bajaría la más nueva y el mismo código podría no compilar. |
| `board = esp32cam` | Perfil de la AI-Thinker: 4 MB de flash, PSRAM, pines correctos. |
| `framework = arduino` | Usamos la API de Arduino (`setup`, `loop`, `digitalWrite`…). |
| `monitor_speed = 921600` | Velocidad del monitor. Tiene que coincidir con `Serial.begin(921600)`. |
| `monitor_rts = 0` / `monitor_dtr = 0` | El adaptador ESP32-CAM-MB usa esas líneas para resetear la placa. Si quedan activas, el monitor la deja trabada y no ves nada. |

### Parte 1: constantes y parámetros

**En palabras simples:** antes de que el programa haga nada, le ponemos **nombre** a los
datos que va a usar: en qué pin está conectado el flash y cuánto tiempo queda prendido y apagado.

**Conceptos nuevos**

- **Pin o GPIO:** cada "patita" del chip que se puede conectar a algo del mundo real (un LED,
  un botón, un sensor). Se identifican por número. El flash de esta placa está soldado al **GPIO 4**.
- **Variable:** una cajita con nombre donde el programa guarda un dato. `LED_FLASH` es una
  cajita que guarda el número `4`.
- **Constante (`const`):** una variable que **no se puede cambiar** mientras el programa corre.
  Sirve para datos fijos, como el pin: el flash no se va a mover de lugar.
- **Tipo de dato:** qué clase de dato guarda la cajita. `int` = número entero (sin decimales);
  `long` = número entero que admite valores más grandes.
- **`HIGH` y `LOW`:** las dos órdenes que entiende un pin de salida. `HIGH` le pone voltaje
  (3.3 V) y el LED se enciende; `LOW` lo deja en 0 V y se apaga.
- **Parámetro del panel:** un valor que **podés cambiar desde el panel sin volver a programar la
  placa**. Tiene un valor de fábrica, un mínimo, un máximo y una descripción.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `const int LED_FLASH = 4;` | Crea la constante `LED_FLASH` con el valor 4: "el flash está en el pin 4". |
| `panel::parametro("ENCENDIDO_MS", 200, 20, 3000, ...)` | Declara un parámetro: arranca en 200 ms y se puede mover entre 20 y 3000. |
| `const long& ENCENDIDO_MS = ...` | Guarda ese parámetro con el nombre `ENCENDIDO_MS`. El `&` lo deja "conectado" al panel: cuando lo cambiás ahí, este valor cambia solo. |
| `ms` en los nombres | Milisegundos: 1000 ms = 1 segundo. |

> **¿Por qué ponerle nombre a un número?** `digitalWrite(LED_FLASH, HIGH)` se entiende solo;
> `digitalWrite(4, HIGH)` obliga a recordar qué era el 4. Y si algún día cambia el pin, se
> corrige en un único lugar.

> **¿Y el LED rojo?** La placa trae otro LED, rojo y chiquito, en el **GPIO 33**, pero está en
> la **cara de abajo**: con la placa montada en el adaptador MB no se ve. Lo usamos en un ejercicio.

### Parte 2: `setup()`, la preparación

**En palabras simples:** es lo que la placa hace **una sola vez** al encenderse, como acomodar
todo antes de empezar a trabajar: abre la comunicación con la computadora, se presenta y
prepara el pin del flash.

**Conceptos nuevos**

- **Función:** un bloque de instrucciones con nombre, encerrado entre llaves `{ }`. `setup()` es
  una función especial: la placa la ejecuta sola, una vez, al arrancar.
- **Instrucción:** cada orden del programa. En C++ termina con punto y coma `;`.
- **Monitor serie (`Serial`):** el canal de texto entre la placa y la computadora, por el mismo
  cable USB. Es la forma en que la placa "nos habla". `Serial.println("hola")` escribe una línea.
- **Velocidad en baudios (921600):** qué tan rápido viajan los datos por ese canal: unos 92.000
  caracteres por segundo. Es rápida a propósito: en la lección 03 las fotos viajan por acá. La placa y
  la computadora tienen que usar **la misma**, si no se ven caracteres raros.
- **PSRAM:** memoria extra de la placa. La vamos a necesitar para las fotos (lección 03).

**Línea por línea**

| Código | Qué hace |
|---|---|
| `void setup() {` | Empieza la función `setup`. `void` significa que no devuelve ningún resultado. |
| `Serial.begin(921600);` | Abre el canal de texto con la computadora a 921600 baudios. |
| `delay(1000);` | Espera 1 segundo, para que te dé tiempo a abrir el monitor. |
| `panel::iniciar("01_blink");` | Le avisa al panel qué lección está corriendo. |
| `panel::parte(2);` | Marca "empieza la Parte 2". Así el panel sabe qué código se ejecuta y puede **detenerse** acá. |
| `Serial.println(...)` | Escribe el saludo en el monitor. |
| `Serial.printf("Chip: %s ...", ...)` | Escribe texto con datos adentro: cada `%s` o `%d` se reemplaza por un valor (el modelo del chip, los núcleos...). |
| `pinMode(LED_FLASH, OUTPUT);` | Configura el pin del flash como **salida**: la placa le va a dar órdenes (en vez de leerlo). |

### Parte 3: encender

**En palabras simples:** prende el flash, lo cuenta por el monitor y espera un rato con el
flash prendido.

**Conceptos nuevos**

- **`loop()`:** la otra función especial. Cuando `setup()` termina, la placa ejecuta `loop()`
  **una y otra vez, para siempre**, mientras tenga energía. Por eso el flash parpadea sin parar.
- **`digitalWrite(pin, valor)`:** da la orden a un pin de salida: `HIGH` = prendido, `LOW` = apagado.
- **`delay(ms)`:** espera esa cantidad de milisegundos **sin hacer nada más**. Mientras espera,
  la placa no atiende nada: por eso, si tocás Pausar, primero termina de esperar.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `void loop() {` | Empieza la función que se repite para siempre. |
| `panel::parte(3);` | "Empieza la Parte 3". Si pediste pausa, la placa se detiene acá, **antes** de prender el flash. |
| `digitalWrite(LED_FLASH, HIGH);` | Prende el flash. |
| `Serial.println("LED encendido");` | Lo avisa en el monitor. |
| `delay(ENCENDIDO_MS);` | Espera lo que diga el parámetro `ENCENDIDO_MS` (de fábrica, 200 ms = un quinto de segundo). |

### Parte 4: apagar

**En palabras simples:** apaga el flash, lo avisa y espera. Después `loop()` vuelve a empezar
desde la Parte 3, y así se forma el parpadeo.

**Línea por línea**

| Código | Qué hace |
|---|---|
| `panel::parte(4);` | "Empieza la Parte 4". Si hay pausa, la placa se detiene acá, **con el flash todavía prendido**, porque la Parte 3 ya se ejecutó. |
| `digitalWrite(LED_FLASH, LOW);` | Apaga el flash. |
| `Serial.println("LED apagado");` | Lo avisa en el monitor. |
| `delay(APAGADO_MS);` | Espera lo que diga `APAGADO_MS` (de fábrica, 1 segundo). |
| `}` | Termina `loop()`. La placa vuelve a llamarla desde el principio. |

**Probalo en el panel**

1. Tocá **Pausar**: el flash deja de parpadear y se resalta en amarillo la parte donde se detuvo.
2. Tocá **Paso** varias veces: cada paso ejecuta una parte. Mirá cómo el flash se prende en la
   Parte 3 y se apaga en la Parte 4.
3. Tocá **Continuar**, y en **Variables** cambiá `APAGADO_MS` a 200: el parpadeo se acelera al
   instante, sin volver a programar la placa.

## Qué deberías ver

```
================================
  ESP32-CAM - Leccion 01: Blink
================================
Chip: ESP32-D0WDQ6 rev 1, 2 nucleos, 240 MHz
PSRAM: SI (4194252 bytes)
[PARTE 3]
LED encendido
[PARTE 4]
LED apagado
```

Las líneas `[PARTE n]` y `HOLA` son mensajes para el panel. Si usás el monitor de PlatformIO
también las vas a ver, y podés escribir `PAUSA`, `PASO`, `SIGUE`, `LIST` o
`SET APAGADO_MS 300` a mano.

## Errores comunes

| Síntoma | Causa probable |
|---|---|
| `Failed to connect to ESP32: No serial data received` | La placa no entró en modo descarga. Con el MB: mantené **IO0**, tocá **RST**, soltá IO0. Con FTDI: puente IO0 → GND. |
| El monitor no muestra nada | Falta `monitor_rts = 0` / `monitor_dtr = 0`, o te quedó el puente IO0 → GND puesto. Quitalo y apretá RST. |
| Caracteres raros (`⸮⸮⸮`) | La velocidad del monitor no coincide con `Serial.begin`. |
| No aparece puerto COM | Falta el driver **CH340** del adaptador MB. |

## Ejercicios

1. Desde el panel, cambiá `ENCENDIDO_MS` y `APAGADO_MS` para que parpadee 5 veces por segundo.
2. Hacé que el LED haga "SOS" en código Morse (· · · — — — · · ·).
3. Imprimí cuántos milisegundos lleva encendida la placa con `millis()`.
4. Hacé parpadear el LED **rojo** (GPIO 33, lógica invertida) al mismo tiempo que el flash.
   Para verlo tenés que sacar la placa del adaptador MB: está en la cara de abajo.
