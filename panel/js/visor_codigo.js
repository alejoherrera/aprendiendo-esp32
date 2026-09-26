/**
 * Visor del codigo de una leccion: resaltado de sintaxis C++ y bloques por parte.
 *
 * Las partes se detectan por los comentarios "// --- Parte n:" del main.cpp, que son
 * los mismos que usa panel::parte(n) en la placa (protocolo_serie.v5).
 */

const PALABRAS = new Set(("void int long unsigned char bool const static return if else for while " +
  "true false String size_t uint8_t uint32_t struct namespace include define pragma break continue " +
  "auto enum class esp_err_t camera_config_t camera_fb_t sensor_t").split(" "));

const escapar = (t) => t.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

// Orden importa: comentarios y textos primero, para no colorear palabras dentro de ellos.
const TOKEN = /(\/\*[\s\S]*?\*\/|\/\/[^\n]*)|("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])')|(^[ \t]*#\w+)|(\b\d[\w.]*\b)|([A-Za-z_]\w*)/gm;

function resaltar(codigo) {
  let html = "";
  let ultimo = 0;
  codigo.replace(TOKEN, (texto, comentario, cadena, preprocesador, numero, palabra, pos) => {
    html += escapar(codigo.slice(ultimo, pos));
    ultimo = pos + texto.length;
    let clase = null;
    if (comentario) clase = "c-com";
    else if (cadena) clase = "c-str";
    else if (preprocesador) clase = "c-pre";
    else if (numero) clase = "c-num";
    else if (PALABRAS.has(palabra)) clase = "c-kw";
    else if (/^[A-Z][A-Z0-9_]+$/.test(palabra)) clase = "c-const";
    // Un token puede abarcar varias lineas (comentario /* */): se parte en cada salto para
    // que cada linea del visor quede con sus etiquetas balanceadas.
    const partes = escapar(texto).split("\n");
    html += partes.map((p) => (clase ? `<span class="${clase}">${p}</span>` : p)).join("\n");
    return texto;
  });
  return html + escapar(codigo.slice(ultimo));
}

export class VisorCodigo {
  /** @param {HTMLElement} contenedor @param {(n:number)=>void} alElegirParte */
  constructor(contenedor, alElegirParte) {
    this.contenedor = contenedor;
    this.alElegirParte = alElegirParte;
    this.filasPorParte = {};
  }

  mostrar(codigo) {
    const lineas = codigo.replace(/\r/g, "").split("\n");
    const html = resaltar(lineas.join("\n")).split("\n");
    this.filasPorParte = {};
    let parte = 0;
    const filas = lineas.map((texto, i) => {
      const m = texto.match(/\/\/ --- Parte (\d+)/);
      if (m) parte = Number(m[1]);
      const fila = document.createElement("div");
      fila.className = "linea" + (m ? " inicio-parte" : "");
      fila.dataset.parte = parte;
      fila.innerHTML = `<span class="num">${i + 1}</span><span class="txt">${html[i] || " "}</span>`;
      if (parte) {
        (this.filasPorParte[parte] ||= []).push(fila);
        fila.onclick = () => this.alElegirParte(Number(fila.dataset.parte));
      }
      return fila;
    });
    this.contenedor.replaceChildren(...filas);
  }

  /** Resalta la parte n. `pausado` cambia el color y centra la parte en pantalla. */
  resaltarParte(n, pausado) {
    this.contenedor.querySelectorAll(".activa, .pausada").forEach((f) => f.classList.remove("activa", "pausada"));
    const filas = this.filasPorParte[n] || [];
    filas.forEach((f) => f.classList.add(pausado ? "pausada" : "activa"));
    if (pausado && filas[0]) filas[0].scrollIntoView({ block: "center", behavior: "smooth" });
  }

  marcarElegida(n) {
    this.contenedor.querySelectorAll(".elegida").forEach((f) => f.classList.remove("elegida"));
    (this.filasPorParte[n] || []).forEach((f) => f.classList.add("elegida"));
  }
}
