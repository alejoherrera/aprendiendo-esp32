# La ESP32-CAM (AI-Thinker): hardware y conexión

## Qué trae la placa

| Componente | Detalle |
|---|---|
| Microcontrolador | ESP32 de doble núcleo a 240 MHz, WiFi 2.4 GHz y Bluetooth |
| Memoria | 520 KB de RAM interna + **4 MB de PSRAM** + 4 MB de flash |
| Cámara | OV2640 de 2 MP (algunas traen OV3660 de 3 MP) |
| LED rojo | GPIO 33, **lógica invertida** (LOW = encendido). Está en la **cara de abajo**: con el adaptador MB puesto no se ve |
| LED flash (blanco) | GPIO 4. Muy brillante: se calienta y consume mucho |
| microSD | Comparte pines con el flash (GPIO 4, 2, 12, 13, 14, 15) |
| **No trae** | Puerto USB. Hace falta un adaptador para programarla |

## Cómo conectarla a la computadora

### Opción A: adaptador ESP32-CAM-MB (recomendada)

La placa se monta encima del adaptador MB, que trae un micro-USB y el chip **CH340**.

1. Instalá el driver **CH340** (Windows suele necesitarlo; macOS/Linux casi nunca).
2. Montá la ESP32-CAM sobre el MB con la cámara hacia afuera.
3. Para cargar un programa: si PlatformIO dice `Connecting....___`, mantené **IO0**, tocá
   **RST** y soltá IO0.

### Opción B: adaptador USB-serie (FTDI / CP2102)

| Adaptador | ESP32-CAM |
|---|---|
| 5V | 5V |
| GND | GND |
| TX | U0R (GPIO 3) |
| RX | U0T (GPIO 1) |

- **Para cargar:** puente entre **IO0 y GND**, después apretá RST.
- **Para ejecutar:** quitá el puente y apretá RST.
- Alimentá con **5 V**, no 3.3 V: con 3.3 V la cámara provoca reinicios.

## Alimentación: la causa número uno de problemas

La cámara y el WiFi tienen picos de consumo de ~300 mA. Con un cable USB largo o delgado, o un
puerto USB débil, el voltaje cae y el chip se reinicia (`Brownout detector was triggered`).

- Usá un cable USB **corto y bueno** (los de solo carga suelen ser delgados).
- Si persiste: fuente de 5 V / 2 A.
- Las lecciones 03–05 apagan el detector de brownout por software. Es un parche, no una
  solución: si la alimentación es mala, las fotos igual pueden salir corruptas.

## Pines libres para proyectos

Casi todos están ocupados por la cámara. Sin microSD quedan disponibles: **GPIO 2, 12, 13,
14, 15 y 16** (con cuidado: el 12 afecta el arranque si está en HIGH, y el 16 lo usa la PSRAM
en algunas placas).
