/**
 * Lado panel del protocolo serie (docs/contracts/protocolo_serie.v5.json).
 *
 * Traduce las lineas que manda la placa a eventos (hola, parte, pausa, parametros...)
 * y ofrece los comandos como metodos. No sabe nada de la interfaz.
 */

const PREFIJOS_PROTOCOLO = ["HOLA ", "PARAM ", "END LIST", "OK ", "ERR ", "INFO ", "[PARTE ", "[PAUSA ", "FOTO ", "RED ", "IA "];

export class Protocolo {
  /** @param {import("./placa.js").Placa} placa */
  constructor(placa) {
    this.placa = placa;
    this.eventos = {};
    this.paramsEnCurso = [];
    this.foto = null; // { bytes, partes[], caracteres } mientras llega una foto
    this.respuestaIA = null; // lineas de la respuesta del agente mientras llega
    placa.alRecibirLinea = (linea) => this.procesar(linea);
  }

  /** Suscribe un manejador: hola, parte, pausa, parametros, ok, error, info, texto. */
  on(evento, manejador) {
    this.eventos[evento] = manejador;
  }

  emitir(evento, datos) {
    if (this.eventos[evento]) this.eventos[evento](datos);
  }

  static esDelProtocolo(linea) {
    return PREFIJOS_PROTOCOLO.some((p) => linea.startsWith(p));
  }

  procesar(linea) {
    // Las lineas F: de una foto son miles: se procesan aparte y nunca van al monitor.
    if (linea.startsWith("F:")) {
      if (!this.foto) return;
      this.foto.partes.push(linea.slice(2));
      this.foto.caracteres += linea.length - 2;
      const esperados = Math.ceil(this.foto.bytes / 3) * 4;
      if (this.foto.partes.length % 40 === 0) this.emitir("fotoProgreso", Math.min(100, Math.round((this.foto.caracteres / esperados) * 100)));
      return;
    }
    // Respuesta del agente (v5): IA INICIO, una o mas "IA:<linea>", IA FIN.
    if (linea.startsWith("IA:") && this.respuestaIA !== null) {
      this.respuestaIA.push(linea.slice(3));
      return;
    }
    if (linea === "IA INICIO") { this.respuestaIA = []; return; }
    if (linea === "IA FIN" && this.respuestaIA !== null) {
      this.emitir("ia", this.respuestaIA.join("\n").trim());
      this.respuestaIA = null;
      return;
    }
    if (linea.startsWith("IA ERROR ")) { this.emitir("iaError", linea.slice(9)); return; }
    let m;
    if ((m = linea.match(/^FOTO INICIO (\d+)/))) {
      this.foto = { bytes: Number(m[1]), partes: [], caracteres: 0, inicio: performance.now() };
      this.emitir("fotoProgreso", 0);
    } else if (linea.startsWith("FOTO FIN") && this.foto) {
      this.terminarFoto();
    }
    // HOLA sin "^": tras un reinicio puede llegar pegado a la basura del arranque del chip.
    if ((m = linea.match(/HOLA leccion=(\S+)/))) this.emitir("hola", { leccion: m[1] });
    else if ((m = linea.match(/^\[PARTE (\d+)\]/))) this.emitir("parte", { n: Number(m[1]) });
    else if ((m = linea.match(/^\[PAUSA (\d+)\]/))) this.emitir("pausa", { n: Number(m[1]) });
    else if ((m = linea.match(/^PARAM (\S+) (-?\d+) (-?\d+) (-?\d+) ?(.*)$/))) {
      this.paramsEnCurso.push({ nombre: m[1], valor: +m[2], min: +m[3], max: +m[4], descripcion: m[5] });
    } else if (linea.startsWith("END LIST")) {
      this.emitir("parametros", this.paramsEnCurso);
      this.paramsEnCurso = [];
    } else if ((m = linea.match(/^OK (\S+) ?(.*)$/))) this.emitir("ok", { comando: m[1], detalle: m[2] });
    else if ((m = linea.match(/^ERR (\S+) ?(.*)$/))) this.emitir("error", { motivo: m[1], detalle: m[2] });
    else if ((m = linea.match(/^RED ip=(\S+) rssi=(-?\d+)/))) this.emitir("red", { ip: m[1], rssi: Number(m[2]) });
    else if (linea.startsWith("RED sin_wifi")) this.emitir("red", { falta: true });
    else if ((m = linea.match(/^RED no_conecto red=(.*)$/))) this.emitir("red", { noConecto: m[1] });
    else if ((m = linea.match(/^INFO leccion=(\S+) pausado=(\d) parte=(\d+)/))) {
      this.emitir("info", { leccion: m[1], pausado: m[2] === "1", parte: Number(m[3]) });
    }
    this.emitir("linea", { texto: linea, protocolo: Protocolo.esDelProtocolo(linea) });
  }

  /** Decodifica el base64 y valida largo y firma JPEG (regla del contrato v3). */
  terminarFoto() {
    const { bytes, partes, inicio } = this.foto;
    this.foto = null;
    let datos;
    try {
      const binario = atob(partes.join(""));
      datos = Uint8Array.from(binario, (c) => c.charCodeAt(0));
    } catch {
      this.emitir("fotoError", "la foto llegó dañada (base64 inválido)");
      return;
    }
    const n = datos.length;
    const firmaOk = n > 4 && datos[0] === 0xff && datos[1] === 0xd8 && datos[n - 2] === 0xff && datos[n - 1] === 0xd9;
    if (n !== bytes || !firmaOk) {
      this.emitir("fotoError", `la foto llegó incompleta (${n} de ${bytes} bytes)`);
      return;
    }
    this.emitir("foto", { datos, segundos: (performance.now() - inicio) / 1000 });
  }

  pedirParametros() { this.paramsEnCurso = []; return this.placa.enviar("LIST"); }
  cambiar(nombre, valor) { return this.placa.enviar(`SET ${nombre} ${valor}`); }
  valoresDeFabrica() { return this.placa.enviar("RESET"); }
  pausar() { return this.placa.enviar("PAUSA"); }
  paso() { return this.placa.enviar("PASO"); }
  seguir() { return this.placa.enviar("SIGUE"); }
  info() { return this.placa.enviar("INFO"); }
  /** La clave viaja solo por el cable USB; la placa la guarda y se reinicia. */
  guardarWifi(red, clave) { return this.placa.enviar(`WIFI ${red}\t${clave}`); }
  /** Dato privado (ej. API key): viaja solo por USB y se guarda en la placa. Vacio = borrar. */
  guardarSecreto(nombre, valor) { return this.placa.enviar(`SECRETO ${nombre}\t${valor}`); }
  /** Pide al agente de la leccion 05 que mire y responda. */
  preguntarAgente(pregunta) { return this.placa.enviar(`? ${pregunta}`); }
}
