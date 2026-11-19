# 🐍 ChocoPy Compiler (C++)

Compilador modular para **ChocoPy** (un subconjunto estáticamente tipado de Python 3 diseñado para la enseñanza de compiladores) desarrollado en **C++17**.

---

## Integrantes del equipo:

- Diaz Alvizuri, Luis Fabian
- Nadeau Huamani, Jean Marc
- Silva Murillo, Rodrigo Antonio
- Valdivia Castillo, Jose Miguel

## 🚀 Estado del Proyecto

- [x] **Scanner (Analizador Léxico):** Conversión de texto a tokens, manejo de pila para sangría (`INDENT`/`DEDENT`) e ignorado de comentarios. 
- [x] **Parser (Analizador Sintáctico):** Tabla LR(1) con caché en disco, recuperación de errores en modo pánico y construcción del AST (con visualizador gráfico en Python).
- [ ] **Análisis Semántico:** Chequeo de tipos, tabla de símbolos y verificación de ámbitos.
- [ ] **Generación de Código:** Traducción a ensamblador objetivo (RISC-V 32-bit / LLVM IR).

---

## 📁 Estructura del Proyecto

```text
.
├── Scanner/                     # Analizador léxico
│   ├── tokens.hpp / .cpp        # TipoToken, Token y Posicion (+ funciones para imprimirlos)
│   └── lexer.hpp / .cpp         # AnalizadorLexico: AFD, indentación con pila (INDENT/DEDENT) y errores léxicos
├── Comun/
│   └── diagnostico.hpp / .cpp   # Formato de los mensajes de error (estilo g++: línea de código y ^)
├── GeneradorLR1/                # Generador de tablas LR(1)
│   ├── gramatica.hpp / .cpp     # Lee la gramática desde un .txt (producciones, terminales, aumentada)
│   ├── conjuntos.hpp / .cpp     # Conjuntos FIRST y FOLLOW
│   ├── tabla.hpp / .cpp         # Items LR(1), cierre, goto y tablas ACTION / GOTO
│   └── cache.hpp / .cpp         # Guarda la tabla en build/cache/ para no regenerarla cada vez
├── Parser/                      # Analizador sintáctico
│   ├── parser.hpp / .cpp        # AnalizadorSintactico: shift/reduce + recuperación de errores en modo pánico
│   ├── arbol.hpp                # NodoParse: árbol de derivación que arma el parser
│   └── mensajes.hpp / .cpp      # Traduce los errores de sintaxis a mensajes para el usuario
├── AST/                         # Árbol de sintaxis abstracta
│   ├── nodos.hpp / .cpp         # Nodos de ChocoPy (Programa, DefFuncion, OpBinaria, ...)
│   ├── constructor.hpp / .cpp   # Convierte el árbol de derivación en el AST
│   └── impresion.hpp / .cpp     # Volcado de texto y exportación a JSON
├── Gramaticas/
│   ├── chocopy.txt              # Gramática de ChocoPy que usa el parser
│   ├── chocopy_espacios.txt     # La misma gramática, con espacios para leerla mejor
│   └── ejemplos/                # Gramáticas pequeñas de ejemplo (con y sin conflictos)
├── Herramientas/                # Programas con su propio main
│   ├── escanear.cpp             # Imprime los tokens de un .py
│   ├── analizar.cpp             # Scanner + Parser (+ AST) sobre un .py
│   └── lr1.cpp                  # Imprime producciones, estados y tabla de una gramática
├── Pruebas/                     # Pruebas unitarias y programas ChocoPy de ejemplo
│   ├── correr_pruebas.cpp       # main que corre todas las pruebas
│   ├── prueba_tokens.*          # Representación de tokens
│   ├── prueba_lexer.*           # Scanner, un grupo de casos por cada paso
│   ├── prueba_ast.*             # Parser + AST
│   ├── prueba_errores.*         # Mensajes de error
│   └── programas/               # Programa_valido.py se acepta sin errores; errores/ tiene casos con errores sintácticos
├── Visualizador/
│   └── ast_grafico.py           # Dibuja el AST (HTML/SVG interactivo, solo biblioteca estándar de Python)
├── Documentacion/               # Language Reference de ChocoPy (PDF y texto)
├── Makefile
├── build/                       # Ejecutables, caché de la tabla LR(1) y temporales de las pruebas (ignorado por git)
└── README.md
```

---

## 🛠️ Requisitos e Instalación

### Requisitos Previos
* **Compilador con soporte C++17:** `g++` (>= 8.0), `clang++` (>= 7.0) o MSVC.
* **Sistema Operativo:** Linux, macOS o Windows (MinGW-w64 / MSYS2).
* **Python 3** (opcional), solo para el visualizador del AST.

### Compilación y Ejecución

Todo se compila con el `Makefile` y los ejecutables quedan en `build/`. Hay que correrlos **desde la carpeta del
proyecto**, porque buscan `Gramaticas/chocopy.txt` en la ruta actual. En Windows `make` se llama `mingw32-make`.

```bash
make              # compila todo en build/
make pruebas      # compila y corre las pruebas
make limpiar      # borra build/
```

Sin `make`, los mismos comandos a mano (en Windows agrega `-static` al final):

```bash
mkdir -p build
g++ -std=c++17 -O2 -I. Pruebas/*.cpp Scanner/*.cpp Comun/*.cpp GeneradorLR1/*.cpp Parser/*.cpp AST/*.cpp -o build/pruebas
g++ -std=c++17 -O2 -I. Herramientas/escanear.cpp Scanner/*.cpp Comun/*.cpp -o build/escanear
g++ -std=c++17 -O2 -I. Herramientas/analizar.cpp Scanner/*.cpp Comun/*.cpp GeneradorLR1/*.cpp Parser/*.cpp AST/*.cpp -o build/analizar
g++ -std=c++17 -O2 -I. Herramientas/lr1.cpp GeneradorLR1/*.cpp -o build/lr1
```

```bash
# Pruebas unitarias
./build/pruebas

# Solo el scanner: imprime los tokens de un .py
./build/escanear Pruebas/programas/Programa_prueba.py > build/resultado_programa_prueba.txt

# Scanner + Parser juntos (genera la tabla LR(1) y analiza un .py)
./build/analizar Pruebas/programas/Programa_prueba.py     # usa Gramaticas/chocopy.txt
./build/analizar Pruebas/programas/Programa_prueba.py -v  # imprime cada shift/reduce y cada recuperación
./build/analizar Pruebas/programas/errores/varios_errores.py   # varios errores sintácticos en una corrida
./build/analizar programa.py --sin-recuperacion          # se detiene en el primer error
./build/analizar programa.py --regenerar                 # ignora la caché y regenera la tabla

# La tabla LR(1) se genera solo la primera vez y se guarda en build/cache/ (~1.8 s -> ~0.04 s);
# se regenera sola si cambia el texto de la gramática.

# AST: solo se genera si el programa se acepta sin errores
./build/analizar Pruebas/programas/Programa_valido.py --ast                      # lo imprime como texto indentado
./build/analizar Pruebas/programas/Programa_valido.py --ast-json build/ast.json  # lo guarda en JSON

# Gráfico del AST (Python 3, sin instalar nada): genera build/ast.html con zoom y arrastre
python Visualizador/ast_grafico.py build/ast.json --abrir
python Visualizador/ast_grafico.py build/ast.json --solo suma_lista   # solo una función o clase
python Visualizador/ast_grafico.py build/ast.json --profundidad 3     # resumen del programa
python Visualizador/ast_grafico.py build/ast.json -o build/ast.svg    # SVG suelto, para informes

# El gráfico se genera a partir de build/ast.json: si el programa tiene errores no se escribe un
# JSON nuevo y verías el árbol del programa anterior. Usa un programa válido (Programa_valido.py).

# Generador LR(1) por separado (imprime producciones, estados y tabla)
./build/lr1 Gramaticas/ejemplos/gramatica.txt
```

---

## ✍️ Convenciones del Código

- Todo el código está en español: clases (`AnalizadorLexico`, `AnalizadorSintactico`, `DefFuncion`...), funciones
  (`siguienteToken()`, `parsear()`, `construirAST()`...), variables y comentarios. Los comentarios van sin tildes.
- Se mantienen los términos del curso: token, scanner, parser, AST, FIRST/FOLLOW, ACTION/GOTO, shift/reduce.
- Los nombres de los tokens (`KW_IF`, `IDENTIFIER`, `NEWLINE`...) se dejan como en el enunciado porque también son los
  terminales de la gramática (`Gramaticas/chocopy.txt` los escribe entre llaves, por ejemplo `{KW_IF}`). Si se cambia
  uno en `Scanner/tokens.hpp` hay que cambiarlo también en la gramática.
- Cada `.hpp` explica qué hace cada función y cita la sección del Language Reference cuando aplica (sec. 3.4.1, ...).
- Los `#include` se escriben desde la raíz del proyecto (`#include "Scanner/lexer.hpp"`) y se compila con `-I.`.

---

## 🧪 Probar errores

Los mensajes de error siguen el estilo de `g++`: archivo, línea y columna, la línea de código con un `^` bajo el
problema, y notas que apuntan a otro lugar relacionado. Están escritos para quien programa, no para quien diseña
el compilador: no hablan de tokens ni del parser.

`Pruebas/programas/Programa_prueba.py` trae **4 errores intencionales** (marcados con `# ERROR n`) para mostrar que
el compilador los reconoce todos y sigue analizando el resto sin detenerse:

```bash
./build/analizar Pruebas/programas/Programa_prueba.py
```

```text
Programa_prueba.py:46:16: error: falta ':' al final de la linea
   46 |     elif n == 5          # ERROR 2: falta ":" al final del elif
      |                ^
Programa_prueba.py:64:41: error: el caracter '@' no es valido en ChocoPy
   64 |         resultado = resultado + palabra @ " "  # ERROR 1 (lexico): ...
      |                                         ^
Programa_prueba.py:97:26: error: falta cerrar el '(' con ')' antes del fin de la linea
   97 | resultado_op = (a + b * 2    # ERROR 3: falta cerrar el parentesis
      |                          ^
Programa_prueba.py:97:16: nota: el '(' abierto aqui nunca se cierra
   97 | resultado_op = (a + b * 2    # ERROR 3: falta cerrar el parentesis
      |                ^
Programa_prueba.py:98:9: error: ')' de mas: no hay ningun '(' abierto que cerrar
   98 | print(a))                    # ERROR 4: sobra un parentesis
      |         ^
```

Para ver el programa sin errores, borra las 4 líneas marcadas (en el `ERROR 1`, cambia `@` por `+`).

Opciones útiles: `-v` agrega el detalle técnico (estado del autómata, terminales esperados y cada paso de la
recuperación); `--sin-recuperacion` se detiene en el primer error.

Cada mensaje tiene una prueba automática en `Pruebas/prueba_errores.cpp` (se corre con `./build/pruebas`): errores
léxicos (carácter inválido, cadena sin cerrar, entero demasiado grande, `;`, `/`, sangría inconsistente) y de sintaxis
(paréntesis sin cerrar o de más, falta `:`, falta un valor, `else` sin `if`, `+=`, palabras reservadas de Python,
definiciones después de sentencias...). Un error léxico se informa una sola vez: el error de sintaxis que sería su
consecuencia no se repite.

**Limitación conocida:** si el cuerpo de una `def` está sin sangría (`def f():` seguido de `return 1` pegado al margen),
el parser reporta además un error por cada definición de variable que venga después, porque tras resincronizar el resto
del programa se lee como sentencias.

### 🪟 Ver el gráfico del AST en Windows (Anaconda Prompt)

En PowerShell, `python` suele apuntar al atajo de la Microsoft Store ("Python was not found"). Lo más simple es usar el
**Anaconda Prompt**, donde `python` ya funciona. En ese prompt (cmd) las rutas llevan `\`:

```bat
cd C:\Users\TuUsuario\Documents\GitHub\Compilador-Chocopy

build\analizar.exe Pruebas\programas\Programa_valido.py --ast-json build\ast.json
python Visualizador\ast_grafico.py build\ast.json --abrir
```

- Corre `analizar` desde la carpeta del proyecto: busca `Gramaticas\chocopy.txt` en la ruta actual.
- Si el dibujo sale muy grande, mira solo una función: `python Visualizador\ast_grafico.py build\ast.json --solo suma_lista --abrir`.
- Alternativa en PowerShell, con la ruta completa a tu Python: `& "C:\Users\TuUsuario\miniconda3\python.exe" Visualizador/ast_grafico.py build/ast.json --abrir`.
