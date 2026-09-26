/**
 * Panel de aprendizaje: une la placa, el protocolo, el visor de codigo y la interfaz.
 */
import { Placa } from "./placa.js";
import { Protocolo } from "./protocolo.js";
import { VisorCodigo } from "./visor_codigo.js";
import { markdownAHtml, seccionesPorParte } from "./markdown.js";
import { montarAgente } from "./agente.js";

const $ = (id) => document.getElementById(id);
const placa = new Placa();
const protocolo = new Protocolo(placa);
const agente = montarAgente(protocolo, () => ({
  conectada: estado.conectada,
  coincide: estado.leccionEnPlaca === estado.leccion?.id,
  pausada: estado.pausada,
}), (t, c) => aviso(t, c));
const visor = new VisorCodigo($("codigo"), (n) => mostrarExplicacion(n, true));

const estado = {
  lecciones: [],
  leccion: null,        // leccion elegida en la lista
  leccionEnPlaca: null, // la que dijo HOLA / INFO
  explicaciones: {},
  conectada: false,
  pausada: false,
  parteActual: 0,
  recibiendoFoto: false,
  ip: null,             // IP que aviso la placa (lecciones con WiFi)
};

// --- Progreso guardado en este navegador ------------------------------------------
function progreso() {
  try { return JSON.parse(localStorage.getItem("progreso") || "{}"); } catch { return {}; }
}
function marcarProbada(id) {
  try { localStorage.setItem("progreso", JSON.stringify({ ...progreso(), [id]: true })); } catch { /* sin almacenamiento */ }
}

// --- Mensajes de estado -------------------------------------------------------------
function aviso(texto, clase = "") {
  $("estado").textContent = texto;
  $("estado").className = clase;
}

function actualizarBotones() {
  const c = estado.conectada;
  const coincide = estado.leccionEnPlaca === estado.leccion?.id;
  $("btnCargar").disabled = !c;
  $("btnReiniciar").disabled = !c;
  $("btnConectar").hidden = c;
  $("btnDesconectar").hidden = !c;
  $("btnPausar").disabled = !c || !coincide || estado.pausada;
  $("btnPaso").disabled = !c || !coincide || !estado.pausada;
  $("btnSeguir").disabled = !c || !coincide || !estado.pausada;
  $("btnFabrica").disabled = !c || !coincide;
  $("btnFoto").disabled = !c || !coincide || estado.pausada || estado.recibiendoFoto;
  $("btnWifi").disabled = !c;
  agente.actualizar();
  $("enPlaca").textContent = estado.leccionEnPlaca
    ? `En la placa: lección ${estado.leccionEnPlaca}` + (coincide ? "" : " (no es la que estás viendo)")
    : c ? "En la placa: programa sin panel (cargá una lección)" : "Placa sin conectar";
  $("estadoEjecucion").textContent = !coincide ? "" : estado.pausada
    ? `⏸ Detenida antes de la Parte ${estado.parteActual}` : `▶ Ejecutando · Parte ${estado.parteActual || "-"}`;
}

// --- Lista de lecciones ---------------------------------------------------------------
function dibujarLista() {
  const hechas = progreso();
  $("lecciones").replaceChildren(...estado.lecciones.map((l) => {
    const li = document.createElement("li");
    li.className = (l.id === estado.leccion?.id ? "elegida " : "") + (l.disponible ? "" : "bloqueada");
    li.innerHTML = `<span class="marca">${hechas[l.id] ? "✓" : l.id.slice(0, 2)}</span>
      <span><b>${l.titulo}</b><small>${l.disponible ? l.resumen : "Próximamente en el panel"}</small></span>`;
    if (l.disponible) li.onclick = () => elegirLeccion(l);
    return li;
  }));
}

async function elegirLeccion(l) {
  estado.leccion = l;
  dibujarLista();
  $("tituloLeccion").textContent = `Lección ${l.id.slice(0, 2)}: ${l.titulo}`;
  const [codigo, readme] = await Promise.all([
    fetch(`/lecciones/${l.id}/src/main.cpp`).then((r) => r.text()),
    fetch(`/lecciones/${l.id}/README.md`).then((r) => r.text()),
  ]);
  visor.mostrar(codigo);
  $("tarjetaFoto").hidden = !l.foto && !l.agente; // el agente muestra la foto que "vio"
  $("btnFoto").hidden = !l.foto;
  $("tarjetaAgente").hidden = !l.agente;
  $("tarjetaWifi").hidden = !l.web;
  mostrarRed();
  $("ayudaEntrada").innerHTML = l.entrada
    ? `<b>Qué entiende esta lección si le escribís:</b> ${l.entrada}.`
    : "Esta lección no lee lo que escribís. Igual podés mandar comandos del panel: PAUSA, PASO, SIGUE, LIST.";
  estado.explicaciones = seccionesPorParte(readme);
  mostrarExplicacion(1, true);
  $("parametros").replaceChildren();
  if (estado.leccionEnPlaca === l.id) protocolo.pedirParametros();
  actualizarBotones();
}

function mostrarExplicacion(n, elegidaPorUsuario) {
  const md = estado.explicaciones[n];
  $("explicacion").innerHTML = md ? markdownAHtml(md) : "<p>Esta parte no tiene explicación todavía.</p>";
  if (elegidaPorUsuario) visor.marcarElegida(n);
}

// --- Parametros ---------------------------------------------------------------------
function dibujarParametros(lista) {
  if (!lista.length) { $("parametros").innerHTML = "<p class='suave'>Esta lección no tiene variables ajustables.</p>"; return; }
  $("parametros").replaceChildren(...lista.map((p) => {
    const fila = document.createElement("label");
    fila.className = "param";
    fila.innerHTML = `<span><code>${p.nombre}</code><small>${p.descripcion}</small></span>
      <input type="range" min="${p.min}" max="${p.max}" value="${p.valor}">
      <input type="number" min="${p.min}" max="${p.max}" value="${p.valor}">`;
    const [rango, numero] = fila.querySelectorAll("input");
    rango.oninput = () => (numero.value = rango.value);
    const aplicar = (v) => protocolo.cambiar(p.nombre, v);
    rango.onchange = () => aplicar(rango.value);
    numero.onchange = () => { rango.value = numero.value; aplicar(numero.value); };
    fila.dataset.nombre = p.nombre;
    return fila;
  }));
}

// --- Eventos de la placa ---------------------------------------------------------------
// --- WiFi y pagina de la camara (protocolo_serie.v5) -----------------------------------
function calidadSenal(rssi) {
  if (rssi >= -67) return "buena";
  if (rssi >= -80) return "regular";
  return "débil: acercá la placa al router, las fotos pueden cortarse";
}

function mostrarRed() {
  const hay = Boolean(estado.ip) && estado.leccion?.web && estado.leccionEnPlaca === estado.leccion.id;
  $("tarjetaWeb").hidden = !hay;
  if (!hay) { $("enVivo").checked = false; $("vistaVivo").hidden = true; cortarVivo(); return; }
  const url = `http://${estado.ip}/`;
  $("enlaceWeb").href = url;
  $("enlaceWeb").textContent = `Abrir ${url}`;
  $("infoRed").textContent = `Señal WiFi: ${estado.rssi} dBm (${calidadSenal(estado.rssi)}). Se abre desde cualquier celular o compu conectado a la misma red.`;
}

// En vivo: /video es MJPEG, UNA respuesta que trae cuadros sin parar. Se pide una sola vez;
// solo se vuelve a pedir si se corta (error). Apagarlo libera la placa para otros pedidos.
function siguienteVivo() {
  if (!$("enVivo").checked || !estado.ip) return;
  $("vistaVivo").src = `http://${estado.ip}/video?t=${Date.now()}`;
}
function cortarVivo() {
  $("vistaVivo").removeAttribute("src"); // cierra la conexion del video
}
$("vistaVivo").onerror = () => { if ($("enVivo").checked) setTimeout(siguienteVivo, 2000); };
$("enVivo").onchange = () => {
  $("vistaVivo").hidden = !$("enVivo").checked;
  if ($("enVivo").checked) siguienteVivo();
  else cortarVivo();
};

protocolo.on("red", (r) => {
  if (r.ip) {
    estado.ip = r.ip;
    estado.rssi = r.rssi;
    aviso(`La placa se conectó al WiFi: ${r.ip}`, "ok");
  } else if (r.falta) {
    estado.ip = null;
    aviso("La placa espera la red WiFi: cargala en la tarjeta WiFi.", "error");
  } else if (r.noConecto !== undefined) {
    estado.ip = null;
    aviso(`No se pudo conectar a "${r.noConecto}". Revisá la clave y que la red sea de 2.4 GHz.`, "error");
  }
  mostrarRed();
});

$("formWifi").onsubmit = (e) => {
  e.preventDefault();
  const red = $("wifiRed").value.trim();
  if (!red || !estado.conectada) return;
  protocolo.guardarWifi(red, $("wifiClave").value);
  $("wifiClave").value = "";           // la clave no queda en la pantalla
  try { localStorage.setItem("wifiRed", red); } catch { /* sin almacenamiento */ }
  estado.ip = null;
  mostrarRed();
  aviso(`Red "${red}" enviada. La placa la guarda y se reinicia para conectarse...`);
};
try { $("wifiRed").value = localStorage.getItem("wifiRed") || ""; } catch { /* sin almacenamiento */ }

protocolo.on("hola", ({ leccion }) => {
  estado.ip = null; // reinicio: la IP se vuelve a avisar al conectarse
  mostrarRed();
  estado.leccionEnPlaca = leccion;
  estado.pausada = false;
  estado.parteActual = 0;
  marcarProbada(leccion);
  dibujarLista();
  if (leccion === estado.leccion?.id) protocolo.pedirParametros();
  actualizarBotones();
});
protocolo.on("info", (i) => {
  estado.leccionEnPlaca = i.leccion;
  estado.pausada = i.pausado;
  estado.parteActual = i.parte;
  if (i.leccion === estado.leccion?.id) {
    protocolo.pedirParametros();
    visor.resaltarParte(i.parte, i.pausado);
  }
  actualizarBotones();
});
protocolo.on("parte", ({ n }) => {
  estado.parteActual = n;
  if (estado.leccionEnPlaca === estado.leccion?.id) visor.resaltarParte(n, false);
  actualizarBotones();
});
protocolo.on("pausa", ({ n }) => {
  estado.pausada = true;
  estado.parteActual = n;
  if (estado.leccionEnPlaca === estado.leccion?.id) {
    visor.resaltarParte(n, true);
    mostrarExplicacion(n, false);
  }
  actualizarBotones();
});
protocolo.on("parametros", dibujarParametros);

// --- Fotos por el cable USB (protocolo_serie.v5) ---------------------------------------
protocolo.on("fotoProgreso", (pct) => {
  estado.recibiendoFoto = true;
  $("progresoFoto").hidden = false;
  $("progresoFoto").value = pct;
  $("infoFoto").textContent = `Recibiendo foto... ${pct} %`;
  actualizarBotones();
});
protocolo.on("foto", ({ datos, segundos }) => {
  estado.recibiendoFoto = false;
  $("progresoFoto").hidden = true;
  const url = URL.createObjectURL(new Blob([datos], { type: "image/jpeg" }));
  const img = $("foto");
  if (img.src.startsWith("blob:")) URL.revokeObjectURL(img.src);
  img.onload = () => {
    $("infoFoto").textContent = `${img.naturalWidth}×${img.naturalHeight} px · ${(datos.length / 1024).toFixed(0)} KB · llegó en ${segundos.toFixed(1)} s. Tocala para verla grande.`;
  };
  img.src = url;
  $("enlaceFoto").href = url;
  $("enlaceFoto").hidden = false;
  actualizarBotones();
});
protocolo.on("fotoError", (motivo) => {
  estado.recibiendoFoto = false;
  $("progresoFoto").hidden = true;
  $("infoFoto").textContent = `No se pudo mostrar: ${motivo}. Probá de nuevo.`;
  actualizarBotones();
});
protocolo.on("ok", ({ comando, detalle }) => {
  if (comando === "SECRETO") agente.secretoGuardado(detalle);
  if (comando === "SIGUE") { estado.pausada = false; actualizarBotones(); }
  if (comando === "RESET") protocolo.pedirParametros();
  if (comando === "SET") {
    const [nombre, valor] = detalle.split(" ");
    const fila = $("parametros").querySelector(`[data-nombre="${nombre}"]`);
    if (fila) fila.querySelectorAll("input").forEach((i) => (i.value = valor));
    aviso(`${nombre} = ${valor} (guardado en la placa)`, "ok");
  }
});
protocolo.on("error", ({ motivo, detalle }) => aviso(`La placa respondió: ${motivo} ${detalle}`, "error"));
protocolo.on("linea", ({ texto, protocolo: esDelPanel }) => {
  if (esDelPanel && !$("verProtocolo").checked) return;
  const m = $("monitor");
  const cerca = m.scrollHeight - m.scrollTop - m.clientHeight < 40;
  // Al arrancar, el chip escribe a 115200 baudios y a 921600 eso llega como basura (�����).
  const basura = (texto.match(/[�\u0000-\u0008\u000E-\u001F]/g) || []).length;
  if (basura > 10 && basura > texto.length / 3) texto = "(mensajes de arranque del chip: se ven como basura porque usan otra velocidad)";
  m.append((esDelPanel ? "· " : "") + texto + "\n");
  if (m.textContent.length > 60000) m.textContent = m.textContent.slice(-40000);
  if (cerca) m.scrollTop = m.scrollHeight;
});

// --- Botones -------------------------------------------------------------------------------
// --- Soltar el puerto: el USB solo lo puede usar UNA pestaña/programa a la vez ------------
// Tres situaciones dejaban el puerto tomado (visto en pruebas 2026-09-25, "The port is already open"):
//  1. otra pestaña del panel lo tenia abierto -> la pestaña nueva les pide que lo suelten;
//  2. se cierra la pestaña -> pagehide lo suelta;
//  3. se cierra la ventana del panel (servidor) -> el latido falla y se suelta.
const canal = "BroadcastChannel" in window ? new BroadcastChannel("panel-esp32") : null;

async function desconectar(motivo) {
  await placa.desconectar();
  estado.conectada = false;
  estado.leccionEnPlaca = null;
  estado.pausada = false;
  if (motivo) aviso(motivo, "error");
  actualizarBotones();
}

if (canal) {
  canal.onmessage = (e) => {
    if (e.data === "soltar-puerto" && estado.conectada) {
      desconectar("Otra pestaña del panel tomó la placa. Esta quedó desconectada.");
    }
  };
}
window.addEventListener("pagehide", () => placa.desconectar());

let servidorCaido = false;
setInterval(async () => {
  try {
    // Con tiempo limite: un latido colgado no debe ocupar una de las pocas conexiones
    // que el navegador abre por servidor (trabaria la descarga de las lecciones).
    const r = await fetch("lecciones.json", { method: "HEAD", cache: "no-store", signal: AbortSignal.timeout(2500) });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    if (servidorCaido) { servidorCaido = false; $("panelCerrado").hidden = true; }
  } catch {
    if (servidorCaido) return;
    servidorCaido = true;
    $("panelCerrado").hidden = false;
    if (estado.conectada) desconectar("Se cerró la ventana del panel: se soltó la placa.");
  }
}, 3000);

$("btnConectar").onclick = async () => {
  try {
    if (canal) {
      canal.postMessage("soltar-puerto");
      await new Promise((r) => setTimeout(r, 300)); // tiempo para que las otras lo suelten
    }
    await placa.elegirPuerto();
    await placa.abrirMonitor();
    estado.conectada = true;
    aviso("Placa conectada.", "ok");
    protocolo.info(); // si ya corre una leccion con panel, responde cual es
  } catch (e) {
    const ocupado = /Failed to open|already open|NetworkError/i.test(e.message);
    aviso(ocupado
      ? "El puerto lo está usando otro programa (VS Code/PlatformIO, otro monitor serie). Cerralo y probá de nuevo."
      : "No se conectó: " + e.message, "error");
  }
  actualizarBotones();
};

$("btnDesconectar").onclick = () => desconectar().then(() => aviso("Placa desconectada: el puerto quedó libre."));

$("btnCargar").onclick = async () => {
  const l = estado.leccion;
  $("btnCargar").disabled = true;
  try {
    aviso(`Descargando lección ${l.id}...`);
    const resp = await fetch(`/firmware/${l.id}.bin`);
    if (!resp.ok) throw new Error(`no encontré firmware/${l.id}.bin`);
    const datos = new Uint8Array(await resp.arrayBuffer());
    estado.leccionEnPlaca = null;
    const t0 = performance.now();
    await placa.cargar(datos, (p) => ($("progreso").value = p), (t) => console.log("[esptool]", t));
    aviso(`Lección cargada en ${((performance.now() - t0) / 1000).toFixed(0)} s.`, "ok");
    $("monitor").append("---- placa reiniciada ----\n");
    await placa.abrirMonitor();
    // Por si el HOLA salio antes de reabrir el puerto: la placa guarda el INFO en su
    // buffer y lo responde apenas llega a panel::parte().
    protocolo.info();
  } catch (e) {
    aviso("Falló la carga: " + e.message, "error");
  }
  actualizarBotones();
};

$("btnReiniciar").onclick = () => placa.reiniciar();
$("btnPausar").onclick = () => protocolo.pausar();
$("btnPaso").onclick = () => protocolo.paso();
$("btnSeguir").onclick = () => protocolo.seguir();
$("btnFabrica").onclick = () => protocolo.valoresDeFabrica();
$("btnFoto").onclick = () => {
  $("infoFoto").textContent = "Sacando la foto...";
  placa.enviar(estado.leccion.foto);
};
$("formEnviar").onsubmit = (e) => {
  e.preventDefault();
  if (!estado.conectada) return;
  placa.enviar($("textoEnviar").value);
  $("textoEnviar").value = "";
};

// --- Arranque ---------------------------------------------------------------------------------
if (!Placa.soportado()) $("sinSoporte").hidden = false;
estado.lecciones = await fetch("lecciones.json").then((r) => r.json());
const siguiente = estado.lecciones.find((l) => l.disponible && !progreso()[l.id]) || estado.lecciones[0];
await elegirLeccion(siguiente);
actualizarBotones();
