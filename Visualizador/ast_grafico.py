#!/usr/bin/env python3
"""Dibuja el AST de ChocoPy como un grafico (de izquierda a derecha).

Lee el JSON que genera el compilador y escribe un HTML autocontenido con el
arbol dibujado en SVG, con zoom (rueda), desplazamiento (arrastrar), y un
tooltip con la posicion de cada nodo. No usa librerias externas: solo la
biblioteca estandar de Python 3.

    ./build/parse_file programa.py --ast-json build/ast.json
    python Visualizador/ast_grafico.py build/ast.json --abrir

Opciones:
    -o SALIDA        archivo de salida: .html (por defecto, junto al JSON) o
                     .svg (solo el dibujo, sin la pagina)
    --solo NOMBRE    dibuja solo el subarbol del primer nodo cuyo nombre o
                     tipo sea NOMBRE (por ejemplo una funcion: --solo suma_lista)
    --profundidad N  limita la profundidad; los nodos recortados salen con "..."
    --abrir          abre el resultado en el navegador
"""
import argparse
import html
import json
import os
import sys
import webbrowser

# Categorias de nodos. Cada una tiene su color (definido en el CSS, con
# variantes clara y oscura) y la lista de tipos de nodo que agrupa.
CATEGORIAS = [
    ("Definiciones", ["Program", "VarDef", "FuncDef", "ClassDef", "TypedVar", "GlobalDecl", "NonlocalDecl"]),
    ("Tipos", ["Type", "ListType"]),
    ("Sentencias", ["Pass", "Return", "ExprStmt", "Assign", "If", "While", "For"]),
    ("Expresiones", ["Binary", "Unary", "IfExpr", "Call", "Member", "Index", "ListLit"]),
    ("Nombres", ["Id"]),
    ("Literales", ["IntLit", "StrLit", "BoolLit", "NoneLit"]),
]
CATEGORIA_DE = {kind: nombre for nombre, kinds in CATEGORIAS for kind in kinds}
CATEGORIA_DEFECTO = "Otros"

# Medidas del dibujo, en pixeles SVG.
TAM_FUENTE = 13
ANCHO_CAR = 7.9          # ancho de un caracter monoespaciado de 13 px
ALTO_CAJA = 30
SEPARACION_Y = 42        # distancia vertical entre hojas
SEPARACION_X = 96        # espacio entre columnas (aqui va el rol de cada arista)
RELLENO_X = 11
MARGEN = 28
MAX_TEXTO = 40


def etiqueta(nodo):
    """(tipo, texto) que se muestra dentro de la caja."""
    texto = nodo.get("text", "").replace("\n", "\\n")
    if len(texto) > MAX_TEXTO:
        texto = texto[:MAX_TEXTO - 3] + "..."
    if nodo.get("_podado"):
        texto = texto + " ..." if texto else "..."
    return nodo["kind"], texto


def buscar(nodo, nombre):
    """Primer nodo (en profundidad) cuyo texto o tipo es 'nombre'."""
    if nodo.get("text") == nombre or nodo["kind"] == nombre:
        return nodo
    for hijo in nodo["children"]:
        encontrado = buscar(hijo, nombre)
        if encontrado:
            return encontrado
    return None


def podar(nodo, limite, profundidad=0):
    if profundidad >= limite and nodo["children"]:
        nodo["children"] = []
        nodo["_podado"] = True
        return
    for hijo in nodo["children"]:
        podar(hijo, limite, profundidad + 1)


def disenar(raiz):
    """Asigna a cada nodo su profundidad '_d', su fila '_y' (las hojas ocupan
    filas consecutivas y cada padre se centra entre sus hijos) y su ancho
    '_w'. Devuelve (nodos, filas, ancho de cada columna)."""
    nodos = []
    contador = [0]

    def recorrer(nodo, profundidad):
        nodo["_d"] = profundidad
        nodos.append(nodo)
        for hijo in nodo["children"]:
            recorrer(hijo, profundidad + 1)
        if nodo["children"]:
            nodo["_y"] = (nodo["children"][0]["_y"] + nodo["children"][-1]["_y"]) / 2
        else:
            nodo["_y"] = contador[0]
            contador[0] += 1

    recorrer(raiz, 0)
    ancho_columna = {}
    for nodo in nodos:
        tipo, texto = etiqueta(nodo)
        nodo["_w"] = (len(tipo) + (1 + len(texto) if texto else 0)) * ANCHO_CAR + 2 * RELLENO_X
        ancho_columna[nodo["_d"]] = max(ancho_columna.get(nodo["_d"], 0), nodo["_w"])
    return nodos, contador[0], ancho_columna


def dibujar_svg(raiz):
    """Devuelve (svg, ancho, alto, nodos)."""
    nodos, filas, ancho_columna = disenar(raiz)

    x, x_columna = MARGEN, {}
    for profundidad in range(max(ancho_columna) + 1):
        x_columna[profundidad] = x
        x += ancho_columna[profundidad] + SEPARACION_X
    ancho = x - SEPARACION_X + MARGEN
    alto = 2 * MARGEN + max(filas - 1, 0) * SEPARACION_Y + ALTO_CAJA

    for nodo in nodos:
        nodo["_px"] = x_columna[nodo["_d"]]
        nodo["_py"] = MARGEN + ALTO_CAJA / 2 + nodo["_y"] * SEPARACION_Y

    aristas, roles, cajas = [], [], []
    for nodo in nodos:
        for hijo in nodo["children"]:
            xa, ya = nodo["_px"] + nodo["_w"], nodo["_py"]
            xb, yb = hijo["_px"], hijo["_py"]
            xm = (xa + xb) / 2
            aristas.append('<path d="M%.1f,%.1f C%.1f,%.1f %.1f,%.1f %.1f,%.1f"/>'
                           % (xa, ya, xm, ya, xm, yb, xb, yb))
            if hijo.get("role"):
                roles.append('<text x="%.1f" y="%.1f" text-anchor="end">%s</text>'
                             % (xb - 7, yb - 5, html.escape(hijo["role"])))

        tipo, texto = etiqueta(nodo)
        categoria = CATEGORIA_DE.get(nodo["kind"], CATEGORIA_DEFECTO)
        pista = "%s%s  (linea %s, columna %s)" % (
            nodo["kind"], " " + nodo.get("text", "") if nodo.get("text") else "",
            nodo.get("line", "?"), nodo.get("col", "?"))
        tx = nodo["_px"] + RELLENO_X
        partes = ['<g class="nodo cat-%s%s"><title>%s</title>'
                  % (categoria, " podado" if nodo.get("_podado") else "", html.escape(pista)),
                  '<rect x="%.1f" y="%.1f" width="%.1f" height="%d" rx="8"/>'
                  % (nodo["_px"], nodo["_py"] - ALTO_CAJA / 2, nodo["_w"], ALTO_CAJA),
                  '<text class="tipo" x="%.1f" y="%.1f" textLength="%.1f" lengthAdjust="spacingAndGlyphs">%s</text>'
                  % (tx, nodo["_py"] + 4.5, len(tipo) * ANCHO_CAR, html.escape(tipo))]
        if texto:
            partes.append('<text class="valor" x="%.1f" y="%.1f" textLength="%.1f" lengthAdjust="spacingAndGlyphs">%s</text>'
                          % (tx + (len(tipo) + 1) * ANCHO_CAR, nodo["_py"] + 4.5,
                             len(texto) * ANCHO_CAR, html.escape(texto)))
        partes.append("</g>")
        cajas.append("".join(partes))

    svg = ('<svg id="arbol" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %.0f %.0f" '
           'width="%.0f" height="%.0f" font-family="ui-monospace, Consolas, \'DejaVu Sans Mono\', monospace" '
           'font-size="%d">\n<g class="aristas">%s</g>\n<g class="roles">%s</g>\n<g class="nodos">\n%s\n</g>\n</svg>'
           % (ancho, alto, ancho, alto, TAM_FUENTE, "\n".join(aristas), "\n".join(roles), "\n".join(cajas)))
    return svg, ancho, alto, nodos


# Estilo compartido por la pagina y el SVG suelto. Colores con variantes clara/oscura.
ESTILO = """
:root { color-scheme: light dark; --fondo:#ffffff; --texto:#0f172a; --suave:#64748b; --linea:#94a3b8; --panel:#f8fafc; --borde:#e2e8f0;
  --Definiciones-f:#e0e7ff; --Definiciones-b:#4f46e5; --Tipos-f:#ccfbf1; --Tipos-b:#0d9488;
  --Sentencias-f:#fef3c7; --Sentencias-b:#d97706; --Expresiones-f:#dbeafe; --Expresiones-b:#2563eb;
  --Nombres-f:#f1f5f9; --Nombres-b:#64748b; --Literales-f:#dcfce7; --Literales-b:#16a34a; --Otros-f:#f3f4f6; --Otros-b:#6b7280; }
@media (prefers-color-scheme: dark) { :root { --fondo:#0b1220; --texto:#e2e8f0; --suave:#94a3b8; --linea:#475569; --panel:#111a2e; --borde:#1e293b;
  --Definiciones-f:#2b2f66; --Definiciones-b:#a5b4fc; --Tipos-f:#0f3f3b; --Tipos-b:#5eead4;
  --Sentencias-f:#4a3510; --Sentencias-b:#fbbf24; --Expresiones-f:#16305e; --Expresiones-b:#93c5fd;
  --Nombres-f:#1e293b; --Nombres-b:#94a3b8; --Literales-f:#12381f; --Literales-b:#86efac; --Otros-f:#1f2937; --Otros-b:#9ca3af; } }
#arbol .aristas path { fill:none; stroke:var(--linea); stroke-width:1.4; }
#arbol .roles text { fill:var(--suave); font-size:10.5px; font-style:italic; paint-order:stroke; stroke:var(--fondo); stroke-width:4px; stroke-linejoin:round; }
#arbol .nodo rect { stroke-width:1.5; }
#arbol .nodo .tipo { font-weight:700; }
#arbol .nodo .valor { fill:var(--texto); }
#arbol .nodo.podado rect { stroke-dasharray:5 3; }
#arbol .nodo:hover rect { stroke-width:3; filter:brightness(1.05); }
""" + "".join(
    "#arbol .cat-%s rect{fill:var(--%s-f);stroke:var(--%s-b)} #arbol .cat-%s .tipo{fill:var(--%s-b)}\n" % ((n,) * 5)
    for n in [c for c, _ in CATEGORIAS] + [CATEGORIA_DEFECTO])

PAGINA = """<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>AST de ChocoPy - %(archivo)s</title>
<style>
%(estilo)s
html, body { height:100%%; margin:0; }
body { background:var(--fondo); color:var(--texto); font-family:system-ui, -apple-system, "Segoe UI", sans-serif; display:flex; flex-direction:column; }
header { display:flex; flex-wrap:wrap; align-items:center; gap:10px 22px; padding:12px 20px; background:var(--panel); border-bottom:1px solid var(--borde); }
h1 { font-size:17px; margin:0; font-weight:650; }
.meta { color:var(--suave); font-size:13px; }
.leyenda { display:flex; flex-wrap:wrap; gap:6px 14px; margin-left:auto; font-size:12.5px; }
.leyenda span { display:inline-flex; align-items:center; gap:6px; }
.leyenda i { width:14px; height:14px; border-radius:4px; border:1.5px solid; display:inline-block; }
.herramientas { display:flex; gap:6px; }
button { font:inherit; font-size:13px; background:var(--fondo); color:var(--texto); border:1px solid var(--borde); border-radius:7px; padding:4px 11px; cursor:pointer; }
button:hover { border-color:var(--linea); }
main { flex:1; min-height:0; position:relative; cursor:grab; overflow:hidden; }
main.arrastrando { cursor:grabbing; }
#arbol { width:100%%; height:100%%; display:block; user-select:none; }
.ayuda { position:absolute; right:12px; bottom:10px; font-size:12px; color:var(--suave); background:var(--panel); border:1px solid var(--borde); border-radius:7px; padding:4px 10px; pointer-events:none; opacity:.92; }
</style></head>
<body>
<header>
  <h1>AST de ChocoPy &middot; %(archivo)s</h1>
  <span class="meta">%(nodos)d nodos%(extra)s</span>
  <div class="herramientas"><button id="mas">+</button><button id="menos">&minus;</button><button id="ajustar">Ajustar</button></div>
  <div class="leyenda">%(leyenda)s</div>
</header>
<main id="lienzo">
%(svg)s
<div class="ayuda">Rueda: zoom &middot; Arrastrar: mover &middot; Cursor sobre un nodo: l&iacute;nea y columna</div>
</main>
<script>
(function () {
  var svg = document.getElementById('arbol'), lienzo = document.getElementById('lienzo');
  var AN = %(ancho).0f, AL = %(alto).0f, vb = {x:0, y:0, w:AN, h:AL};
  function aplicar() { svg.setAttribute('viewBox', vb.x + ' ' + vb.y + ' ' + vb.w + ' ' + vb.h); }
  function ajustar() {
    var r = lienzo.getBoundingClientRect(), esc = Math.min(1, r.width / AN, r.height / AL);
    vb = {x:0, y:0, w:r.width / esc, h:r.height / esc};
    if (AN * esc < r.width) vb.x = -(vb.w - AN) / 2;
    if (AL * esc < r.height) vb.y = -(vb.h - AL) / 2;
    aplicar();
  }
  function zoom(f, cx, cy) {
    var r = lienzo.getBoundingClientRect();
    var px = vb.x + vb.w * (cx === undefined ? 0.5 : (cx - r.left) / r.width);
    var py = vb.y + vb.h * (cy === undefined ? 0.5 : (cy - r.top) / r.height);
    vb.w *= f; vb.h *= f;
    vb.x = px - (px - vb.x) * f; vb.y = py - (py - vb.y) * f;
    aplicar();
  }
  lienzo.addEventListener('wheel', function (e) { e.preventDefault(); zoom(e.deltaY < 0 ? 0.85 : 1 / 0.85, e.clientX, e.clientY); }, {passive:false});
  var arr = null;
  lienzo.addEventListener('mousedown', function (e) { arr = {x:e.clientX, y:e.clientY, vx:vb.x, vy:vb.y}; lienzo.classList.add('arrastrando'); });
  window.addEventListener('mouseup', function () { arr = null; lienzo.classList.remove('arrastrando'); });
  window.addEventListener('mousemove', function (e) {
    if (!arr) return;
    var r = lienzo.getBoundingClientRect();
    vb.x = arr.vx - (e.clientX - arr.x) * vb.w / r.width;
    vb.y = arr.vy - (e.clientY - arr.y) * vb.h / r.height;
    aplicar();
  });
  document.getElementById('mas').onclick = function () { zoom(0.8); };
  document.getElementById('menos').onclick = function () { zoom(1.25); };
  document.getElementById('ajustar').onclick = ajustar;
  window.addEventListener('resize', ajustar);
  ajustar();
})();
</script>
</body></html>
"""


def main():
    ap = argparse.ArgumentParser(description="Dibuja el AST de ChocoPy (JSON de parse_file --ast-json).")
    ap.add_argument("json", help="archivo JSON generado con --ast-json")
    ap.add_argument("-o", "--salida", help="archivo de salida (.html o .svg)")
    ap.add_argument("--solo", metavar="NOMBRE", help="dibuja solo el subarbol de ese nodo")
    ap.add_argument("--profundidad", type=int, metavar="N", help="limita la profundidad")
    ap.add_argument("--abrir", action="store_true", help="abre el resultado en el navegador")
    args = ap.parse_args()

    with open(args.json, encoding="utf-8") as archivo:
        datos = json.load(archivo)
    raiz = datos["ast"]
    if args.solo:
        raiz = buscar(raiz, args.solo)
        if raiz is None:
            sys.exit("No se encontro ningun nodo llamado '%s'" % args.solo)
    if args.profundidad is not None:
        podar(raiz, args.profundidad)

    svg, ancho, alto, nodos = dibujar_svg(raiz)
    salida = args.salida or os.path.splitext(args.json)[0] + ".html"
    carpeta = os.path.dirname(salida)
    if carpeta:
        os.makedirs(carpeta, exist_ok=True)

    if salida.lower().endswith(".svg"):
        # SVG suelto: lleva su propio estilo, sin la pagina ni el zoom.
        svg = svg.replace(">", "><style>%s</style>" % ESTILO, 1)
        contenido = '<?xml version="1.0" encoding="UTF-8"?>\n' + svg
    else:
        usadas = {CATEGORIA_DE.get(n["kind"], CATEGORIA_DEFECTO) for n in nodos}
        leyenda = "".join(
            '<span><i style="background:var(--%s-f);border-color:var(--%s-b)"></i>%s</span>' % (n, n, n)
            for n in [c for c, _ in CATEGORIAS] + [CATEGORIA_DEFECTO] if n in usadas)
        extra = " &middot; subárbol: " + html.escape(args.solo) if args.solo else ""
        contenido = PAGINA % dict(
            estilo=ESTILO, svg=svg, ancho=ancho, alto=alto, nodos=len(nodos), leyenda=leyenda, extra=extra,
            archivo=html.escape(os.path.basename(datos.get("archivo", args.json))))

    with open(salida, "w", encoding="utf-8") as archivo:
        archivo.write(contenido)
    print("Grafico guardado en %s (%d nodos, %.0fx%.0f px)" % (salida, len(nodos), ancho, alto))
    if args.abrir:
        webbrowser.open("file:///" + os.path.abspath(salida).replace("\\", "/"))


if __name__ == "__main__":
    main()
