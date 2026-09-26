/**
 * Tarjeta "Agente" del panel (leccion 05): guardar la API key de Gemini en la placa y
 * preguntarle al agente. La key viaja solo por USB (comando SECRETO del contrato v5);
 * esta pagina no la guarda en ningun lado.
 */

const $ = (id) => document.getElementById(id);

/**
 * @param {import("./protocolo.js").Protocolo} protocolo
 * @param {() => {conectada: boolean, coincide: boolean, pausada: boolean}} estadoPlaca
 * @param {(texto: string, clase?: string) => void} aviso
 */
export function montarAgente(protocolo, estadoPlaca, aviso) {
  let esperando = false;

  function mostrarRespuesta(texto, esError) {
    const caja = $("respuestaAgente");
    caja.hidden = false;
    caja.className = "respuesta" + (esError ? " error" : "");
    caja.textContent = texto;
  }

  function actualizar() {
    const e = estadoPlaca();
    $("btnClave").disabled = !e.conectada;
    $("btnBorrarClave").disabled = !e.conectada;
    $("btnPreguntar").disabled = !e.conectada || !e.coincide || e.pausada || esperando;
  }

  $("formClave").onsubmit = (ev) => {
    ev.preventDefault();
    const clave = $("claveGemini").value.trim();
    if (!clave) return;
    protocolo.guardarSecreto("GEMINI_KEY", clave);
    $("claveGemini").value = ""; // la clave no queda en la pantalla
  };
  $("btnBorrarClave").onclick = () => protocolo.guardarSecreto("GEMINI_KEY", "");

  $("formPregunta").onsubmit = (ev) => {
    ev.preventDefault();
    esperando = true;
    actualizar();
    mostrarRespuesta("Mirando y pensando... (la foto llega primero, después la respuesta)", false);
    protocolo.preguntarAgente($("preguntaAgente").value.trim());
  };

  protocolo.on("ia", (texto) => { esperando = false; mostrarRespuesta(texto, false); actualizar(); });
  protocolo.on("iaError", (msg) => { esperando = false; mostrarRespuesta(msg, true); actualizar(); });

  return {
    actualizar,
    /** Respuesta OK SECRETO: confirma sin mostrar la clave. */
    secretoGuardado(nombre) {
      if (nombre === "GEMINI_KEY") aviso("API key guardada en la placa (o borrada, si la dejaste vacía).", "ok");
    },
  };
}
