# Aprendiendo ESP32 (con ESP32-CAM)

Curso práctico y gratuito para aprender a programar una **ESP32-CAM**: una placa de unos pocos
dólares con WiFi y cámara. Empieza por hacer parpadear una luz y termina con la cámara
funcionando como **los ojos de una inteligencia artificial** (Gemini) que describe lo que ve.

- **Sin instalar nada:** en Windows, un doble clic abre un panel en el navegador desde el que se
  cargan las lecciones en la placa.
- **El código explicado parte por parte**, para quien nunca programó.
- **Mirás el código mientras corre:** podés pausar la placa y ver qué parte se está ejecutando.

**Empezá por la bienvenida:** [¿Quién está mirando?](https://claude.ai/artifact/7S42xzxDEa3cG8YWyaorq4),
una introducción interactiva: del ojo humano al microcontrolador, a la cámara y a la mirada de un
agente de IA. También se abre desde el panel (enlace "Bienvenida", arriba).

## Qué vas a necesitar

**La placa** (se consigue en tiendas de electrónica o en línea):

| Qué | Detalle |
|---|---|
| ESP32-CAM AI-Thinker | Con la cámara OV2640 incluida |
| Adaptador **ESP32-CAM-MB** | La base con conector USB donde se enchufa la placa. Muy recomendado |
| Cable micro-USB **de datos** | Corto y de buena calidad (los cables "solo carga" no sirven) |
| Red WiFi de **2.4 GHz** | Para las lecciones 04 y 05. Las redes "5G" no son compatibles |

**En la computadora:** Windows 10 u 11 con **Microsoft Edge** (ya viene instalado) o Google
Chrome. **No hace falta instalar nada más.**

## Empezar en 5 pasos

1. **Descargá el curso:** botón verde **Code → Download ZIP** en esta página.
2. **Descomprimí el ZIP** (clic derecho → *Extraer todo*). No lo abras desde adentro del ZIP.
3. En la carpeta descomprimida, hacé **doble clic en `iniciar.bat`**. Se abre una ventana negra
   (dejala abierta mientras usás el panel) y el panel en Edge.
   > Si Windows muestra "Windows protegió su PC", tocá **Más información → Ejecutar de todas formas**.
   > El archivo solo abre el panel en tu propia computadora.
4. Enchufá la placa por USB y en el panel tocá **Conectar placa**. Elegí el puerto que dice
   **USB-SERIAL CH340**.
5. Elegí la **lección 01** → **Cargar lección**. Cuando termine, el flash de la cámara empieza a
   parpadear. ¡Listo!

Cada lección tiene su explicación en el panel. Tocá cualquier parte del código para leer qué hace.

## Las lecciones

| # | Lección | Qué aprendés |
|---|---|---|
| 00 | [Hardware y conexión](docs/00_hardware_esp32cam.md) | Qué trae la placa, cómo conectarla, por qué importa el cable |
| 01 | [Blink](lecciones/01_blink/) | Cómo está hecho un programa, pines, mensajes por el cable |
| 02 | [Flash con PWM](lecciones/02_flash_pwm/) | Controlar el brillo de una luz; cómo ve el ojo humano |
| 03 | [Primera foto](lecciones/03_primera_foto/) | La cámara, las fotos JPEG y la memoria |
| 04 | [Servidor web](lecciones/04_servidor_web/) | WiFi y una página web con foto y video en vivo, también en el celular |
| 05 | [Ojos de un agente](lecciones/05_ojos_de_un_agente/) | La cámara como ojos de una IA: Gemini responde lo que le preguntes sobre lo que ve |

Cada lección usa lo aprendido en la anterior: conviene hacerlas en orden.

### Para la lección 05: tu API key de Gemini

La lección 05 usa **Gemini**, la IA de Google. Cada estudiante usa **su propia clave (API key)**,
gratis:

1. Entrá a **[aistudio.google.com/apikey](https://aistudio.google.com/apikey)** con tu cuenta de Google.
2. **Create API key** → copiala.
3. En el panel, lección 05, tarjeta **Agente**: pegala → **Guardar en la placa**.

Tu API key es como una contraseña: no la compartas ni la subas a internet. El panel la manda solo
por el cable USB a tu placa.

## Si algo no funciona

| Problema | Solución |
|---|---|
| No aparece ningún puerto al tocar "Conectar placa" | Probá otro cable (que sea de datos). Si sigue sin aparecer, instalá el driver **CH340** (buscá "CH340 driver" en el sitio del fabricante del adaptador). |
| "El puerto lo está usando otro programa" | Cerrá VS Code, Arduino u otra ventana del panel. |
| La placa se reinicia sola o la cámara falla | Casi siempre es la **alimentación**: usá un cable USB corto y bueno, conectado directo a la computadora. Ver [hardware](docs/00_hardware_esp32cam.md). |
| "La cámara no inició" (error `0x105`) | El cable plano de la cámara está flojo: desenchufá el USB, abrí la traba negra, insertalo derecho hasta el fondo y cerrala. |
| No se conecta al WiFi | La red tiene que ser de **2.4 GHz**. Revisá la clave y acercá la placa al router. |
| Se cerró la ventana negra | Volvé a hacer doble clic en `iniciar.bat` y recargá el panel. |
| Uso Mac o Linux | El panel (`iniciar.bat`) es para Windows. En Mac/Linux se puede seguir el curso con VS Code + PlatformIO (ver abajo). |

## ¿Querés modificar el código?

El panel alcanza para hacer todas las lecciones. Para **cambiar el código y probar tus propias
versiones** hace falta instalar:

1. [Visual Studio Code](https://code.visualstudio.com/).
2. La extensión **PlatformIO IDE** (en VS Code: Extensiones → "PlatformIO IDE" → Instalar). La
   primera vez que compilás descarga sola el compilador y las librerías (~500 MB), siempre en
   las mismas versiones, para que todo el grupo compile lo mismo.
3. Opcional: la extensión **Codex** de OpenAI, un asistente de programación. Lee el archivo
   [`AGENTS.md`](AGENTS.md) de este repo, así que ya conoce las reglas del curso.

Después: VS Code → **File → Open Folder…** → elegí la carpeta de una lección (por ejemplo
`lecciones/01_blink`) → barra de abajo: **✓** compilar, **→** cargar, **🔌** monitor.

## Privacidad y seguridad

- El panel funciona **solo en tu computadora** (`localhost`): nadie de tu red ni de internet
  puede entrar.
- Tu red WiFi y tu API key se guardan **en la memoria de tu placa**, no en el código ni en
  internet. Si prestás la placa, borrá la API key desde la tarjeta Agente.
- Las páginas web de las lecciones 04 y 05 **no tienen contraseña**: cualquiera en tu misma red
  WiFi puede verlas. Son para aprender en casa o en el aula; no las publiques en internet.

## Para docentes y curiosos

- **[Guía docente](docs/guia_docente.md): qué remarcar en cada clase**, con comparaciones para explicar cada concepto y confusiones típicas.
- `lecciones/`: una carpeta por lección (código + explicación).
- `panel/`: el panel web que abre `iniciar.bat`, incluida la bienvenida (`panel/bienvenida.html`).
- `firmware/`: las lecciones ya compiladas, las que carga el panel.
- `docs/`: la placa, decisiones de diseño y aprendizajes técnicos
  ([por ejemplo](docs/knowledge/esp32_https_gemini.md), cómo se depuró la conexión con Gemini).

## Créditos

El código de cámara parte del firmware de campo de **MIVISOR IoT**, cámaras ESP32 que
fotografían obras de construcción, y de la configuración de imagen del proyecto **dIAra**.

## Licencia

[MIT](LICENSE): usalo, copialo y adaptalo para tus clases. Excepción: la foto de la ESP32-CAM
(`panel/img/`) es de Nowforever en Wikimedia Commons y tiene licencia CC BY-SA 4.0
([créditos](panel/img/CREDITOS.md)).
