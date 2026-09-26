/**
 * Conversor minimo de Markdown a HTML para las explicaciones de los README.
 *
 * Cubre lo que usan las lecciones: parrafos, listas, tablas, bloques de codigo, citas,
 * negrita, cursiva, codigo en linea y enlaces. Sin dependencias (el panel funciona
 * sin internet). Todo el texto se escapa antes de dar formato.
 */

const escapar = (t) => t.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function enLinea(texto) {
  // El codigo en linea se aparta primero: un `*` o `_` adentro de `...` no debe
  // confundirse con negrita o cursiva (ej: **Puntero (`*`):**).
  const codigos = [];
  const sinCodigo = escapar(texto).replace(/`([^`]+)`/g, (_, c) => `\u0000${codigos.push(c) - 1}\u0000`);
  return sinCodigo
    .replace(/\*\*(.+?)\*\*/g, "<strong>$1</strong>")
    .replace(/(^|[^*])\*([^*\s][^*]*)\*/g, "$1<em>$2</em>")
    .replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, '<a href="$2" target="_blank" rel="noopener">$1</a>')
    .replace(/\u0000(\d+)\u0000/g, (_, i) => `<code>${codigos[Number(i)]}</code>`);
}

function tabla(lineas) {
  const celdas = (l) => l.trim().replace(/^\||\|$/g, "").split("|").map((c) => enLinea(c.trim()));
  const [cabecera, , ...filas] = lineas;
  const th = celdas(cabecera).map((c) => `<th>${c}</th>`).join("");
  const tr = filas.map((f) => `<tr>${celdas(f).map((c) => `<td>${c}</td>`).join("")}</tr>`).join("");
  return `<table><thead><tr>${th}</tr></thead><tbody>${tr}</tbody></table>`;
}

/** @param {string} md @returns {string} HTML */
export function markdownAHtml(md) {
  const lineas = md.replace(/\r/g, "").split("\n");
  const salida = [];
  let i = 0;
  while (i < lineas.length) {
    const l = lineas[i];
    if (l.startsWith("```")) {
      const bloque = [];
      for (i++; i < lineas.length && !lineas[i].startsWith("```"); i++) bloque.push(lineas[i]);
      salida.push(`<pre><code>${escapar(bloque.join("\n"))}</code></pre>`);
      i++;
    } else if (/^#{1,4} /.test(l)) {
      const nivel = l.match(/^#+/)[0].length;
      salida.push(`<h${nivel + 1}>${enLinea(l.replace(/^#+ /, ""))}</h${nivel + 1}>`);
      i++;
    } else if (l.trim().startsWith("|")) {
      const bloque = [];
      for (; i < lineas.length && lineas[i].trim().startsWith("|"); i++) bloque.push(lineas[i]);
      salida.push(tabla(bloque));
    } else if (/^\s*([-*]|\d+\.) /.test(l)) {
      const ordenada = /^\s*\d+\. /.test(l);
      const items = [];
      for (; i < lineas.length && /^\s*([-*]|\d+\.) |^\s{2,}\S/.test(lineas[i]); i++) {
        if (/^\s*([-*]|\d+\.) /.test(lineas[i])) items.push(lineas[i].replace(/^\s*([-*]|\d+\.) /, ""));
        else items[items.length - 1] += " " + lineas[i].trim();
      }
      const tag = ordenada ? "ol" : "ul";
      salida.push(`<${tag}>${items.map((t) => `<li>${enLinea(t)}</li>`).join("")}</${tag}>`);
    } else if (l.startsWith(">")) {
      const bloque = [];
      for (; i < lineas.length && lineas[i].startsWith(">"); i++) bloque.push(lineas[i].replace(/^>\s?/, ""));
      salida.push(`<blockquote>${enLinea(bloque.join(" "))}</blockquote>`);
    } else if (l.trim() === "") {
      i++;
    } else {
      const bloque = [];
      for (; i < lineas.length && lineas[i].trim() !== "" && !/^(#|```|\||>|\s*([-*]|\d+\.) )/.test(lineas[i]); i++) {
        bloque.push(lineas[i]);
      }
      salida.push(`<p>${enLinea(bloque.join(" "))}</p>`);
    }
  }
  return salida.join("\n");
}

/**
 * Separa el README en secciones "### Parte n". Devuelve { n: markdown }.
 * Cada seccion va hasta el siguiente encabezado ## o ###.
 */
export function seccionesPorParte(readme) {
  const partes = {};
  // JS no tiene "fin de texto" (\Z): se agrega un encabezado centinela al final.
  const re = /^### Parte (\d+)[^\n]*\n[\s\S]*?(?=^#{2,3} )/gm;
  const texto = readme.replace(/\r/g, "") + "\n## fin\n";
  let m;
  while ((m = re.exec(texto))) partes[Number(m[1])] = m[0].trim();
  return partes;
}
