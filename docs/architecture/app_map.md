# Mapa de la app

```mermaid
flowchart LR
  subgraph PC["PC del alumno (Windows, nada instalado)"]
    BAT[iniciar.bat] --> SRV[panel/servidor.ps1<br/>PowerShell, localhost:8765]
    SRV -->|sirve| UI[panel/ en Edge<br/>app.js · protocolo.js · visor_codigo.js · markdown.js<br/>fotos por USB (base64, 921600 baudios)]
    UI -->|fetch| FW[(firmware/*.bin<br/>+ manifiesto.json)]
    UI -->|fetch| SRC[(lecciones/*/src/main.cpp<br/>lecciones/*/README.md)]
    UI -->|esptool-js: carga| USB((USB / Web Serial))
    UI <-->|protocolo_serie.v5| USB
  end
  USB --- PLACA[ESP32-CAM<br/>leccion + lib/panel]
  DEV[scripts/compilar_firmware.py<br/>solo mantenedor, requiere PlatformIO] -->|genera y verifica| FW
```

| Componente | Responsabilidad | Estado |
|---|---|---|
| `lecciones/NN_*` | Proyecto PlatformIO por lección; `// --- Parte n` = sección del README | 01-04 con panel · 05 sin panel (fase 3) |
| `lib/panel` | Lado placa del protocolo: traza de partes, pausa/paso, parámetros en NVS, fotos por USB, WiFi guardado en NVS | en uso (01-04) |
| `panel/servidor.ps1` | Sirve el repo solo en localhost; bloquea `.git`, `.pio`, `secrets.h`, traversal | en uso |
| `panel/js/placa.js` | Web Serial: carga con esptool-js y monitor | en uso |
| `panel/js/protocolo.js` | Lado panel del protocolo (eventos y comandos) | en uso |
| `panel/js/visor_codigo.js` | Resaltado C++ y bloques por parte | en uso |
| `panel/js/markdown.js` | Explicaciones: sección `### Parte n` del README → HTML | en uso |
| `panel/js/app.js` | Interfaz: lecciones, progreso (localStorage), variables, monitor | en uso |
| `firmware/` | `.bin` unificados (desde 0x0) + hash del código que los generó | 01-03 |
| `scripts/compilar_firmware.py` | Genera `.bin`; `--verificar` falla si alguno quedó viejo; se niega si hay `secrets.h` | en uso |

**Dependencias externas:** ninguna en ejecución. `panel/vendor/esptool-js` (Apache-2.0) copiado en el repo.
**Contratos:** [`docs/contracts/protocolo_serie.v5.json`](../contracts/protocolo_serie.v5.json).
**Puntos de entrada:** `iniciar.bat` (alumno), `pio run` por lección (quien programa), `scripts/compilar_firmware.py` (mantenedor).
