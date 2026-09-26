/**
 * Comunicacion con la ESP32-CAM por USB desde el navegador (Web Serial).
 *
 * Dos modos que no pueden convivir sobre el mismo puerto:
 *  - cargar(): toma el puerto con esptool-js para escribir el firmware.
 *  - monitor:  lo abre como serie normal para leer lineas y mandar comandos
 *              (protocolo docs/contracts/protocolo_serie.v1.json).
 */
import { ESPLoader, Transport } from "../vendor/esptool-js/bundle.js";

// Monitor rapido (fotos por serie, protocolo_serie.v3). La carga del programa va mas lenta,
// a la velocidad que el adaptador CH340 soporta sin errores.
const BAUDIOS = 921600;
const BAUDIOS_CARGA = 115200;

export class Placa {
  constructor() {
    this.puerto = null;
    this.lector = null;
    this.alRecibirLinea = () => {};
    this.alCambiarEstado = () => {};
  }

  static soportado() {
    return "serial" in navigator;
  }

  /** Abre el dialogo del navegador para elegir el puerto COM. */
  async elegirPuerto() {
    const elegido = await navigator.serial.requestPort();
    // Si se elige otro puerto (o el mismo de nuevo), se suelta el anterior primero.
    if (this.puerto && this.puerto !== elegido) await this.desconectar();
    this.puerto = elegido;
  }

  estaAbierto() {
    return Boolean(this.puerto && this.puerto.readable);
  }

  /** Suelta el puerto para que otra pestaña o programa pueda usarlo. Nunca lanza error. */
  async desconectar() {
    try {
      await this.cerrarMonitor();
    } catch (_) {
      // puerto ya perdido (cable desconectado, otra pestaña): no hay nada que soltar
    }
  }

  /**
   * Escribe un .bin unificado (bootloader+particiones+app, desde 0x0) en la placa.
   * @param {Uint8Array} datos contenido del .bin
   * @param {(pct:number)=>void} alProgreso
   * @param {(txt:string)=>void} alLog mensajes de esptool-js
   */
  async cargar(datos, alProgreso, alLog) {
    await this.cerrarMonitor();
    const terminal = {
      clean() {},
      writeLine: (t) => alLog(t),
      write: (t) => alLog(t),
    };
    const transporte = new Transport(this.puerto, false);
    const cargador = new ESPLoader({ transport: transporte, baudrate: BAUDIOS_CARGA, romBaudrate: BAUDIOS_CARGA, terminal });
    try {
      this.alCambiarEstado("conectando con el chip...");
      const chip = await cargador.main();
      this.alCambiarEstado(`cargando en ${chip}...`);
      await cargador.writeFlash({
        fileArray: [{ data: datos, address: 0x0 }],
        flashMode: "keep",
        flashFreq: "keep",
        flashSize: "keep",
        eraseAll: false,
        compress: true,
        reportProgress: (_i, escrito, total) => alProgreso(Math.round((escrito / total) * 100)),
      });
      await cargador.after("hard_reset");
    } finally {
      await transporte.disconnect();
    }
  }

  /** Abre el puerto como serie normal y entrega cada linea recibida. */
  async abrirMonitor() {
    if (this.estaAbierto()) return; // ya abierto por esta pagina: open() lanzaria "already open"
    // bufferSize: el de fabrica (255 bytes) se llena en ~3 ms a 921600 baudios; si el
    // navegador se demora en leer, se perderian pedazos de la foto.
    await this.puerto.open({ baudRate: BAUDIOS, bufferSize: 1 << 16 });
    // En el adaptador ESP32-CAM-MB, DTR y RTS van a IO0 y RESET: se sueltan para que la
    // placa arranque normal (mismo motivo que monitor_rts/dtr = 0 en platformio.ini).
    await this.puerto.setSignals({ dataTerminalReady: false, requestToSend: false });
    this.leerEnSegundoPlano();
  }

  /** Reinicia la placa con un pulso en RTS (-> EN) sin entrar en modo descarga. */
  async reiniciar() {
    await this.puerto.setSignals({ dataTerminalReady: false, requestToSend: true });
    await new Promise((r) => setTimeout(r, 200));
    await this.puerto.setSignals({ dataTerminalReady: false, requestToSend: false });
  }

  async leerEnSegundoPlano() {
    const decodificador = new TextDecoder();
    let pendiente = "";
    this.lector = this.puerto.readable.getReader();
    try {
      while (true) {
        const { value, done } = await this.lector.read();
        if (done) break;
        pendiente += decodificador.decode(value, { stream: true });
        const lineas = pendiente.split(/\r?\n/);
        pendiente = lineas.pop();
        lineas.forEach((l) => this.alRecibirLinea(l));
      }
    } catch (_) {
      // cancel() al cerrar el monitor termina aca: es normal
    } finally {
      this.lector.releaseLock();
      this.lector = null;
    }
  }

  async enviar(texto) {
    if (!this.estaAbierto()) return;
    const escritor = this.puerto.writable.getWriter();
    await escritor.write(new TextEncoder().encode(texto + "\n"));
    escritor.releaseLock();
  }

  async cerrarMonitor() {
    if (!this.puerto || !this.puerto.readable) return;
    if (this.lector) await this.lector.cancel();
    await new Promise((r) => setTimeout(r, 50));
    await this.puerto.close();
  }
}
