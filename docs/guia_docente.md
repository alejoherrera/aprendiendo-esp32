# Guía docente: qué remarcar en cada clase

Resumen de los conceptos clave de cada lección, con una comparación para explicarlos, qué
mostrar en el panel y las confusiones más comunes. Varios ejemplos vienen de problemas reales
que aparecieron al armar el curso: sirven como historias para contar en clase.

---

## Clase 0: la placa

| Concepto | Cómo explicarlo |
|---|---|
| **Microcontrolador** | Un chip con procesador, memoria y pines para **controlar algo** del mundo físico. No es una computadora: sin sistema operativo, corre **un solo programa** desde que se enciende. |
| **Placa vs. chip** | La ESP32-CAM es una placa: el microcontrolador ESP32 (atrás, bajo una tapa metálica) más cámara, flash, microSD y pines. |
| **Adaptador USB-serie (CH340)** | La placa no tiene USB. El adaptador MB traduce el USB de la compu al "idioma serie" de la placa. |
| **Alimentación** | La causa número uno de fallas: la cámara y el WiFi piden picos de corriente. Cable corto y bueno. |

**Confusiones típicas:** creer que la placa "no anda" cuando es el cable (de solo carga) o la
alimentación; buscar el chip del lado de la cámara (está del otro lado).

---

## Clase 1: Blink (el primer programa)

| Concepto | Cómo explicarlo |
|---|---|
| **`setup()` y `loop()`** | `setup` es preparar el escenario (una sola vez); `loop` es la función que se repite para siempre. |
| **Pin / GPIO** | Una "patita" del chip conectada al mundo. Cada una tiene número: el flash es el GPIO 4. |
| **Salida digital: `HIGH` / `LOW`** | Solo dos estados: con voltaje (prendido) o sin voltaje (apagado). |
| **Variable, constante, tipo** | Cajita con nombre; `const` = no cambia; `int`, `long` = números enteros. Nombrar números (`LED_FLASH` en vez de `4`) hace el código legible. |
| **`delay()` bloquea** | Mientras espera, el microcontrolador **no hace nada más**. |
| **Monitor serie y baudios** | El canal de texto placa ↔ compu por el mismo cable. Los dos lados deben usar la misma velocidad, si no se ve basura. |

**Mostrar en el panel:** **Pausar** y **Paso**. Es la clase donde se *ve* que un programa es una
secuencia de instrucciones que se ejecutan en orden, una por una. Cambiar `APAGADO_MS` sin
reprogramar muestra qué es un parámetro.

**Confusiones típicas:** pensar que `loop` corre una vez; confundir el LED rojo (GPIO 33, abajo,
lógica invertida) con el flash.

---

## Clase 2: el flash con PWM

| Concepto | Cómo explicarlo |
|---|---|
| **PWM** | Un pin solo sabe prender o apagar. Para "media luz" se prende y apaga miles de veces por segundo y el ojo ve el promedio. Como las aspas de un ventilador: tan rápido que no se ven. |
| **Duty cycle** | El porcentaje del tiempo que está prendido: 25 % prendido = luz tenue. |
| **Frecuencia** | Cuántas veces por segundo se repite (5000 Hz). Muy baja = se nota el parpadeo. |
| **Resolución (bits)** | Cuántos escalones de brillo: 8 bits = 256, 12 bits = 4096. Más bits = brillos bajos más finos. |
| **Canal (LEDC)** | **El concepto clave de esta clase.** El chip tiene 16 "generadores de PWM" independientes llamados canales. Cada canal genera su propia onda (frecuencia y resolución), y **a un canal se le conecta un pin**. Se configura el canal (`ledcSetup`), se le asigna el pin (`ledcAttachPin`) y se le ordena el brillo (`ledcWrite`). Comparación: el canal es una emisora de radio y el pin es el parlante que la sintoniza. |
| **Por qué el canal 4 y no el 0** | La cámara usa el canal 0 para generar su reloj (XCLK). Si el flash usara el mismo canal, cambiar el brillo le cambiaría el reloj a la cámara y las fotos saldrían dañadas. Dos usos, dos canales. |
| **El ojo no es lineal (gamma)** | Medido con la propia cámara: con apenas **2 %** de PWM la escena ya tiene el doble de luz. Por eso se pide el brillo en "porcentaje percibido" y se corrige con `pow(x, 2.2)`. |
| **Funciones propias, argumentos, `return`** | Herramientas con nombre: se escriben una vez y se usan muchas. |
| **`for`, `if`/`else`, `==` vs `=`** | Repetir contando, decidir, comparar (`==`) vs. guardar (`=`). |
| **`constrain`: no confiar en la entrada** | Si alguien pide 999 %, se recorta. **Y se avisa**: un programa que corrige en silencio confunde (le pasó al docente: escribió 100 y "no cambiaba"). |

**Mostrar en el panel:** mover `PASO_MS` mientras respira; escribir `5`, `20` y `50` para ver
tres brillos claramente distintos.

**Confusiones típicas:** creer que "canal" y "pin" son lo mismo; esperar que 50 y 100 se vean
muy distintos sin corrección gamma.

---

## Clase 3: la primera foto

| Concepto | Cómo explicarlo |
|---|---|
| **Sensor y driver** | El sensor OV2640 es la retina; el driver es el programa del fabricante que sabe hablarle. Nosotros lo configuramos. |
| **Estructura (`camera_config_t`)** | Un formulario con campos: tamaño, calidad, pines... |
| **Píxel y resolución** | La foto es una grilla de puntos: 1600 × 1200 ≈ 2 millones = "2 megapíxeles". |
| **JPEG y calidad** | Compresión: una foto cruda de 800 × 600 pesaría ~960 KB, en JPEG ~20 KB. Calidad: **número menor = mejor** (al revés de lo intuitivo). |
| **PSRAM** | Memoria extra: sin ella, la foto grande no entra. |
| **Frame buffer: prestado** | La foto se **pide prestada** y hay que **devolverla** (`esp_camera_fb_return`), como un libro de biblioteca. Si no, a la segunda foto el driver no tiene dónde escribir. |
| **Puntero** | En vez de copiar la foto (pesada), se pasa la **dirección** donde está. |
| **Byte, hexadecimal, firma** | Un JPEG sano empieza con `FF D8` y termina con `FF D9`: así se detecta una foto cortada. |
| **La foto vieja** | El driver tiene siempre una foto **ya sacada** esperando; puede tener minutos. Por eso se descarta una antes. (Síntoma real: la captura tardaba "0 ms".) |
| **Base64** | El cable serie es para texto; base64 traduce 3 bytes en 4 letras seguras para mandar la foto al panel. |
| **Velocidad del cable** | A 115200 baudios una foto de 220 KB tardaba 27 s; a 921600, 3,5 s. |
| **Brownout** | Caída de voltaje al arrancar la cámara: el chip se reinicia para protegerse. Apagar el detector es un parche; lo correcto es buena alimentación. |

**Mostrar en el panel:** **Sacar y ver foto**; `CALIDAD_JPEG` en 40 (se pixela y pesa menos);
`GIRAR_180` en 0 (la foto sale invertida: el sensor va montado "de cabeza").

**Confusiones típicas:** pensar que más número de calidad es mejor; olvidarse de devolver el buffer.

---

## Clase 4: servidor web y video

| Concepto | Cómo explicarlo |
|---|---|
| **Cliente y servidor** | El navegador **pide**, la placa **responde**. Acá el servidor es la propia placa. |
| **WiFi estación** | La placa se conecta a un router como un celular (el otro modo, punto de acceso, crea su propia red). Solo 2.4 GHz. |
| **Dirección IP y puerto** | La IP es el "número de casa" en la red; el puerto 80 es la "puerta" de las páginas web. |
| **Ruta y parámetros** | `/foto`, `/video`, `/flash?brillo=30`: cada ruta la atiende una función. |
| **HTTP y códigos** | Pedido → respuesta con un código: 200 = bien, 404 = no existe, 500 = error del servidor. |
| **HTML y caché** | La página vive dentro del programa; `?t=<hora>` evita que el navegador muestre una foto vieja guardada. |
| **Credenciales fuera del código** | La red y la clave viajan por USB y se guardan en la placa. **Nunca** escritas en el código que se comparte. |
| **Tiempo límite** | Un `while` sin salida deja la placa colgada si la clave está mal. Siempre con límite. |
| **RSSI** | Intensidad de la señal en dBm: más cerca de 0 = mejor; bajo −80 las fotos se cortan. |
| **Video MJPEG** | Una respuesta que **nunca termina**: una foto tras otra. |
| **Un espectador a la vez** | Mientras alguien mira el video, `loop()` no corre y la placa no atiende otra cosa. Introduce la idea de **tareas en paralelo** (concurrencia). |

**Mostrar en el panel:** **Ver video en vivo** y **Pausar**: el video se congela porque la placa
se detuvo antes de responder. Abrir la página desde el celular.

**Seguridad:** la página no tiene contraseña: cualquiera en la misma red la ve.

---

## Clase 5: los ojos de un agente

| Concepto | Cómo explicarlo |
|---|---|
| **Agente** | Un programa que **percibe → razona → responde o actúa**. La cámara es la percepción; el modelo razona. |
| **La pregunta define al agente** | Mismo hardware, distinto trabajo: describir, inspeccionar cascos, contar personas, vigilar una puerta. Es la idea central del curso. |
| **Modelo de IA** | El "cerebro" en la nube. Los modelos se renuevan: se cambia una línea. |
| **API y API key** | La API es la ventanilla; la key es tu credencial. **Es una contraseña**: no se comparte ni se sube a internet. |
| **HTTPS y certificados** | Conexión cifrada y comprobación de que del otro lado está Google. Historia real: con un solo certificado raíz, 1 de cada 4 conexiones fallaba, porque Google alterna entre dos cadenas. No se "apaga" la verificación: se confía en las raíces correctas. |
| **JSON** | El formato en que se hablan las APIs: `{"campo": "valor"}`. |
| **La pila (stack)** | Memoria donde cada función guarda sus variables. Historia real: la placa se reiniciaba por "stack canary" al preguntar desde el celular, porque la pila de 8 KB no alcanzaba. |
| **Esperar la respuesta** | Historia real: la librería esperaba solo 1 s y Gemini tarda 2-4 s; todas las preguntas fallaban. Leer la documentación y medir los tiempos lo resolvió. |
| **Reintentar solo lo que vale la pena** | Un corte de red se reintenta; una clave inválida no. |
| **Confianza y error** | El modelo puede equivocarse con seguridad. Pregunta para la clase: ¿qué decisiones le dejarías tomar solo? |
| **Privacidad** | La cámara ve todo lo que tiene enfrente, incluida la pantalla de alguien. El límite lo ponen las personas. |

**Mostrar en el panel:** la tarjeta **Agente**, con preguntas distintas sobre la misma escena, y
la foto que "vio" el agente al lado de la respuesta.

---

## Ideas que atraviesan todo el curso

- **Medir antes de creer.** Cada problema del curso se resolvió midiendo, no suponiendo: el 2 %
  de PWM que duplica la luz, la foto de "0 ms", la espera de 1 s, el certificado que fallaba
  1 de cada 4 veces.
- **Leer los mensajes de error completos.** `0x105`, `Stack canary`, `X509 verification failed`:
  cada uno dice exactamente qué pasó.
- **Versiones fijadas.** Todo el grupo compila lo mismo; si una librería cambia, el código no se rompe solo.
- **Los datos privados no van en el código.** WiFi y API key viven en la placa, no en el repositorio.
