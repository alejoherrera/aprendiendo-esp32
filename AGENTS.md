# Instrucciones para agentes de código (Codex y similares)

Este repo es **material didáctico** para aprender a programar la ESP32-CAM (AI-Thinker) con
PlatformIO. Quien lo usa está aprendiendo: priorizá código claro sobre código ingenioso.

## Estructura

- Cada carpeta de `lecciones/NN_nombre/` es un proyecto PlatformIO **independiente**
  (su propio `platformio.ini`). Se abre esa carpeta, no la raíz del repo.
- Cada lección tiene un `README.md` que explica el código **parte por parte**. Los bloques
  `// --- Parte N: ...` del código se corresponden con las secciones del README: si cambiás
  una, actualizá la otra.

## Cómo se escriben las explicaciones (README de cada lección)

Quien lee está **aprendiendo a programar**: no des por sabido ningún término. Cada sección
`### Parte n` (el panel la muestra al lado del código) tiene, en este orden:

1. **En palabras simples:** qué hace la parte, en una o dos oraciones, sin jerga.
2. **Conceptos nuevos:** cada término que aparece por primera vez en el curso (función,
   variable, GPIO, PWM, puntero...), explicado con una comparación cotidiana. Lo ya explicado
   en una lección anterior no se repite.
3. **Línea por línea:** tabla `Código | Qué hace`. Sin el carácter `|` dentro de las celdas.
4. Opcional: "Probalo en el panel", con algo que se vea en la placa.

## Reglas

- **No cambiar** `platform = espressif32@6.12.0`: fija el Arduino core 2.0.17. En core 3.x
  cambian APIs (por ejemplo `ledcSetup`/`ledcAttachPin` → `ledcAttach`).
- **Lecciones con panel** (`lib/panel`, contrato `docs/contracts/protocolo_serie.v5.json`):
  cada bloque `// --- Parte n` que se ejecuta llama `panel::parte(n)`; los valores ajustables se
  declaran con `panel::parametro(...)`; la entrada del usuario se lee con `panel::leerLinea()`
  (nunca `Serial.read()` directo: se comería los comandos del panel).
- Si cambiás el código de una lección, regenerá su `.bin`: `python scripts/compilar_firmware.py NN_nombre`
  (y `--verificar` antes de commitear).
- El flash usa el **canal LEDC 4**; el 0 es de la cámara (XCLK). No los mezcles.
- Toda foto de `esp_camera_fb_get()` se devuelve con `esp_camera_fb_return()`.
- Credenciales WiFi solo en `src/secrets.h` (está en `.gitignore`). Nunca en el código.
- Comentarios y textos en español; sin tildes en `Serial.print` (el monitor serie de Windows
  puede mostrar mal los acentos).

## Verificar

```bash
pio run -d lecciones/NN_nombre            # compilar
pio run -d lecciones/NN_nombre -t upload  # cargar (placa conectada)
pio device monitor -d lecciones/NN_nombre # monitor serie
```

Para 04 y 05 hace falta `src/secrets.h` (copiar de `secrets.h.example`).
