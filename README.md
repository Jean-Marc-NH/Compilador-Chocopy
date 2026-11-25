# 🐍 ChocoPy Compiler (C++)

Compilador modular para **ChocoPy** (un subconjunto estáticamente tipado de Python 3 diseñado para la enseñanza de compiladores) desarrollado en **C++17**.

---

## Integrantes del equipo:

- Diaz Alvizuri, Luis Fabian
- Nadeau Huamani, Jean Marc
- Silva Murillo, Rodrigo Antonio

## 🚀 Estado del Proyecto

- [x] **Scanner (Analizador Léxico):** Conversión de texto a tokens, manejo de pila para sangría (`INDENT`/`DEDENT`) e ignorado de comentarios. 
- [x] **Parser (Analizador Sintáctico):** Tabla LR(1) con caché en disco, recuperación de errores en modo pánico y construcción del AST (con visualizador gráfico en Python).
- [x] **Análisis Semántico:** Tabla de símbolos por scope (con shadowing, `global` y `nonlocal`), chequeo de tipos y errores con formato de `g++`.
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
│   ├── nodos.hpp / .cpp         # Nodos de ChocoPy (Programa, DefFuncion, OpBinaria, ...) y tipo inferido de cada expresión
│   ├── constructor.hpp / .cpp   # Convierte el árbol de derivación en el AST
│   └── impresion.hpp / .cpp     # Volcado de texto y exportación a JSON
├── Semantico/                   # Análisis semántico: tablas de símbolos y chequeo de tipos
│   ├── tipos.hpp / .cpp         # Tipos de ChocoPy, jerarquía de clases, subtipo, asignable y join
│   ├── tabla_simbolos.hpp / .cpp # Ambito: una tabla por scope con puntero al padre; búsqueda por la cadena de padres
│   └── analizador.hpp / .cpp    # AnalizadorSemantico: tres pasos, chequeo de tipos y errores semánticos
├── Gramaticas/
│   ├── chocopy.txt              # Gramática de ChocoPy que usa el parser
│   ├── chocopy_espacios.txt     # La misma gramática, con espacios para leerla mejor
│   └── ejemplos/                # Gramáticas pequeñas de ejemplo (con y sin conflictos)
├── Herramientas/                # Programas con su propio main
│   ├── chocopy.cpp              # Compilador completo: scanner + parser + AST + semántico (ejecutable principal)
│   ├── escanear.cpp             # Imprime los tokens de un .py
│   └── lr1.cpp                  # Imprime producciones, estados y tabla de una gramática
├── Pruebas/                     # Pruebas unitarias y programas ChocoPy de ejemplo
│   ├── correr_pruebas.cpp       # main que corre todas las pruebas
│   ├── prueba_tokens.*          # Representación de tokens
│   ├── prueba_lexer.*           # Scanner, un grupo de casos por cada paso
│   ├── prueba_ast.*             # Parser + AST
│   ├── prueba_errores.*         # Mensajes de error
│   ├── prueba_semantica.*       # Tablas de símbolos, scopes y errores semánticos
│   └── programas/               # Programa_valido.py se acepta sin errores; errores/ tiene casos con errores sintácticos;
│                                # semantica/ tiene scopes válidos y programas con errores semánticos
├── Visualizador/
│   └── ast_grafico.py           # Dibuja el AST (HTML/SVG interactivo, solo biblioteca estándar de Python)
├── Documentacion/               # Language Reference de ChocoPy (PDF y texto)
├── compilar.ps1                 # Compila con g++ desde PowerShell, para Windows sin make
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
g++ -std=c++17 -O2 -I. Pruebas/*.cpp Scanner/*.cpp Comun/*.cpp GeneradorLR1/*.cpp Parser/*.cpp AST/*.cpp Semantico/*.cpp -o build/pruebas
g++ -std=c++17 -O2 -I. Herramientas/escanear.cpp Scanner/*.cpp Comun/*.cpp -o build/escanear
g++ -std=c++17 -O2 -I. Herramientas/chocopy.cpp Scanner/*.cpp Comun/*.cpp GeneradorLR1/*.cpp Parser/*.cpp AST/*.cpp Semantico/*.cpp -o build/chocopy
g++ -std=c++17 -O2 -I. Herramientas/lr1.cpp GeneradorLR1/*.cpp -o build/lr1
```

#### En Windows sin make: `compilar.ps1`

Si no está instalado `make`, el script `compilar.ps1` hace lo mismo que el Makefile (usa las mismas listas de fuentes
y genera `build\chocopy.exe`, `build\escanear.exe`, `build\lr1.exe` y `build\pruebas.exe`, enlazados estáticamente):

```powershell
powershell -ExecutionPolicy Bypass -File compilar.ps1            # compila todo en build\
powershell -ExecutionPolicy Bypass -File compilar.ps1 -Pruebas   # compila y corre las pruebas
```

Se detiene en el primer ejecutable que no compila. Necesita `g++` en el PATH (MinGW-w64 o MSYS2).

```bash
# Pruebas unitarias
./build/pruebas

# Solo el scanner: imprime los tokens de un .py
./build/escanear Pruebas/programas/Programa_prueba.py > build/resultado_programa_prueba.txt

# Ejecutable principal: scanner + parser + AST + análisis semántico sobre un .py
./build/chocopy Pruebas/programas/Programa_valido.py      # usa Gramaticas/chocopy.txt
./build/chocopy Pruebas/programas/Programa_valido.py -v   # imprime cada shift/reduce y cada recuperación
./build/chocopy Pruebas/programas/errores/varios_errores.py   # varios errores sintácticos en una corrida
./build/chocopy programa.py --sin-recuperacion            # el parser se detiene en el primer error
./build/chocopy programa.py --regenerar                   # ignora la caché y regenera la tabla
./build/chocopy programa.py --gramatica otra.txt          # usa otra gramática

# La tabla LR(1) se genera solo la primera vez y se guarda en build/cache/ (~1.8 s -> ~0.04 s);
# se regenera sola si cambia el texto de la gramática.

# AST: se imprime si el programa no tiene errores sintácticos (los semánticos no lo impiden)
./build/chocopy Pruebas/programas/Programa_valido.py --ast                      # lo imprime como texto indentado
./build/chocopy Pruebas/programas/Programa_valido.py --ast-json build/ast.json  # lo guarda en JSON

# Tablas de símbolos de cada scope (con --predefinidos se ven también print, len, int, str...)
./build/chocopy Pruebas/programas/Programa_valido.py --tablas

# Gráfico del AST (Python 3, sin instalar nada): genera build/ast.html con zoom y arrastre
python Visualizador/ast_grafico.py build/ast.json --abrir
python Visualizador/ast_grafico.py build/ast.json --solo suma_lista   # solo una función o clase
python Visualizador/ast_grafico.py build/ast.json --profundidad 3     # resumen del programa
python Visualizador/ast_grafico.py build/ast.json -o build/ast.svg    # SVG suelto, para informes

# El JSON se escribe después del análisis semántico, así que también se guarda si hay errores semánticos.
# Si hay errores sintácticos no se escribe un JSON nuevo y el gráfico mostraría el árbol del programa anterior.

# Generador LR(1) por separado (imprime producciones, estados y tabla)
./build/lr1 Gramaticas/ejemplos/gramatica.txt
```

---

## 💡 Ejemplos de uso de `./build/chocopy`

```bash
# Programa válido: pasa todas las fases y termina con "Programa ACEPTADO"
./build/chocopy Pruebas/programas/Programa_valido.py

# Programa con errores semánticos: se reportan todos, ordenados por línea
./build/chocopy Pruebas/programas/semantica/errores_semanticos.py

# Tabla de símbolos de cada scope (scopes, global, nonlocal, shadowing)
./build/chocopy Pruebas/programas/semantica/scopes_valido.py --tablas

# AST con la línea y el tipo de cada nodo
./build/chocopy Pruebas/programas/Programa_valido.py --ast

# AST en JSON para el visualizador
./build/chocopy Pruebas/programas/Programa_valido.py --ast-json build/ast.json
python Visualizador/ast_grafico.py build/ast.json --abrir
```

---

## 🔎 Análisis semántico

El ejecutable `chocopy` encadena las fases en una sola corrida: **scanner -> parser LR(1) -> AST -> análisis
semántico**. Si hay errores léxicos o sintácticos no se puede armar el AST y el análisis semántico no corre. El análisis
se hace en tres pasos (`Semantico/analizador.cpp`, `AnalizadorSemantico::analizar`):

1. **Clases:** se registran en la jerarquía, en orden (la superclase debe estar definida antes). Así una anotación puede
   nombrar una clase que se define más abajo.
2. **Declaraciones globales:** clases (con sus atributos y métodos), funciones (con su firma) y variables globales.
3. **Cuerpos:** valores iniciales, funciones y métodos (cada uno con su propia tabla hija) y las sentencias del nivel
   superior. Aquí se chequean los tipos.

**Tabla de símbolos por scope.** Cada scope (global, función, clase, método) tiene su tabla `Ambito`
(`Semantico/tabla_simbolos.cpp`) con un puntero a la tabla padre. Declarar un nombre lo inserta solo en la tabla actual
(`Ambito::insertar`). Usar un nombre lo busca en la tabla actual y luego en los padres (`Ambito::buscar`). Los scopes de
clase se saltan al buscar desde un método: en ChocoPy los atributos se ven solo como `self.atributo`.

Salida de `--tablas` para un programa pequeño (abreviada):

```text
Tabla "global"  (sin padre)
  x               variable   int                            linea 1
  animal          clase                                     linea 3
  vaca            clase                                     linea 9
  externa         funcion    () -> int                      linea 13
  cambiar_global  funcion    () -> int                      linea 22
    Tabla "clase animal"  (padre: "global")
      sonido          variable   str                            linea 4
      hablar          funcion    (animal) -> str                linea 6
        Tabla "metodo animal.hablar"  (padre: "clase animal")
          self            parametro  animal                         linea 6
    Tabla "funcion externa"  (padre: "global")
      x               variable   str                            linea 14
      interna         funcion    () -> int                      linea 15
        Tabla "funcion externa.interna"  (padre: "funcion externa")
          x               nonlocal   str (la de afuera)             linea 16
    Tabla "funcion cambiar_global"  (padre: "global")
      x               global     int (la global)                linea 23
```

**Shadowing, `global` y `nonlocal`:**

- **Shadowing:** una variable local con el mismo nombre que una global la oculta. Como se busca primero en la tabla
  actual, la local es la que se encuentra (en el ejemplo, `x` dentro de `externa` es `str`, aunque la global sea `int`).
- **`global x`:** agrega a la tabla de la función una entrada que apunta a la variable global. Permite asignarla.
- **`nonlocal x`:** igual, pero apunta a la variable de la función que encierra a esta. Si no existe ninguna, se reporta
  un error.

**Errores que detecta** (los números son la sección del Language Reference):

- *Declaraciones y scopes:* nombre declarado dos veces en el mismo scope (5.2); nombre de clase usado como variable,
  función o parámetro (5.2); tipo de una anotación que no existe (2.4); superclase no definida antes o que es `int`,
  `bool` o `str` (3.1.2); atributo o método que redefine un atributo heredado (3.1.2); método que redefine a otro con
  una firma distinta (3.1.2); método cuyo primer parámetro no es de la clase (3.1.2); `global x` o `nonlocal x` que no
  apuntan a una variable válida (3.1.3, 3.1.4).
- *Sentencias:* uso de un nombre no declarado en ningún scope (5.2); asignar a una variable de afuera sin `global` o
  `nonlocal` (5.2); tipo incompatible en una asignación o en un valor inicial (5.2); condición de `if`, `elif` o `while`
  que no es `bool` (5.2); `for` sobre algo que no es una lista ni un `str` (5.2); `return` fuera de una función o con un
  tipo distinto al declarado (5.2); función que no devuelve nada en algún camino y debía hacerlo (5.2); asignar a un
  carácter de un `str` (los `str` no se pueden modificar).
- *Expresiones:* operadores con tipos inválidos, como `1 + "a"`, `not 3`, `"a" < "b"` o `1 is 2` (5.2); cantidad o tipo
  de los argumentos de una llamada, método o constructor; llamar algo que no es una función o usar una función sin
  llamarla; atributo o método que no existe en la clase; índice que no es `int`.

**Cómo se reportan.** Los errores no detienen el análisis: una expresión con error recibe el tipo interno `<error>`, que
es compatible con todo. Así un solo error (por ejemplo, una variable no declarada) no genera mensajes en cascada en las
expresiones que la contienen. Todos los errores se imprimen al final, ordenados por línea, con el mismo formato que el
scanner y el parser.

Ejemplo de salida (`build/ejemplo_errores.py`, abreviada a cuatro mensajes):

```text
build/ejemplo_errores.py:5:9: error: el operador '+' no se puede usar entre 'int' y 'str' (suma int + int, une str + str o listas + listas)
    5 |     z = 1 + "dos"
      |         ^
build/ejemplo_errores.py:9:3: error: el argumento 1 de la funcion 'f' debe ser de tipo 'int', pero es 'str'
    9 | f("texto")
      |   ^
build/ejemplo_errores.py:10:7: error: 'variable_inexistente' no esta declarado
   10 | print(variable_inexistente)
      |       ^~~~~~~~~~~~~~~~~~~~
build/ejemplo_errores.py:11:4: error: la condicion del 'if' debe ser de tipo 'bool', no 'int'
   11 | if 3:
      |    ^
INFO SEMANTICO - 4 errores semanticos
INFO - Programa RECHAZADO
```

El tipo de cada expresión queda guardado en `Nodo::tipoInferido` (`AST/nodos.hpp`), así que `--ast` y el JSON lo muestran.

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

`Pruebas/programas/Programa_prueba.py` trae **4 errores sintácticos y léxicos intencionales** (marcados con `# ERROR n`)
para mostrar que el compilador los reconoce todos y sigue analizando el resto sin detenerse:

```bash
./build/chocopy Pruebas/programas/Programa_prueba.py
```

```text
Programa_prueba.py:46:16: error: falta ':' al final de la linea
   46 |     elif n == 5          # ERROR 2: falta ":" al final del elif
      |                ^
Programa_prueba.py:65:41: error: el caracter '@' no es valido en ChocoPy
   65 |         resultado = resultado + palabra @ " "  # ERROR 1 (lexico): el caracter '@' no existe en ChocoPy
      |                                         ^
Programa_prueba.py:98:26: error: falta cerrar el '(' con ')' antes del fin de la linea
   98 | resultado_op = (a + b * 2    # ERROR 3: falta cerrar el parentesis
      |                          ^
Programa_prueba.py:98:16: nota: el '(' abierto aqui nunca se cierra
   98 | resultado_op = (a + b * 2    # ERROR 3: falta cerrar el parentesis
      |                ^
Programa_prueba.py:99:9: error: ')' de mas: no hay ningun '(' abierto que cerrar
   99 | print(a))                    # ERROR 4: sobra un parentesis
      |         ^
```

Como hay errores sintácticos, el análisis semántico no llega a correr. Para ver el programa sin errores, borra las 4
líneas marcadas (en el `ERROR 1`, cambia `@` por `+`).

Los errores semánticos se prueban con `Pruebas/programas/semantica/errores_semanticos.py`, donde cada error está marcado
con `# ERROR n`. Ese programa es sintácticamente válido, así que todos sus errores aparecen en una sola corrida:

```bash
./build/chocopy Pruebas/programas/semantica/errores_semanticos.py
```

Opciones útiles: `-v` agrega el detalle técnico (estado del autómata, terminales esperados y cada paso de la
recuperación); `--sin-recuperacion` se detiene en el primer error.

Cada mensaje tiene una prueba automática en `Pruebas/prueba_errores.cpp` (se corre con `./build/pruebas`): errores
léxicos (carácter inválido, cadena sin cerrar, entero demasiado grande, `;`, `/`, sangría inconsistente) y de sintaxis
(paréntesis sin cerrar o de más, falta `:`, falta un valor, `else` sin `if`, `+=`, palabras reservadas de Python,
definiciones después de sentencias...). Un error léxico se informa una sola vez: el error de sintaxis que sería su
consecuencia no se repite. Los errores semánticos tienen sus pruebas en `Pruebas/prueba_semantica.cpp`.

**Limitación conocida:** si el cuerpo de una `def` está sin sangría (`def f():` seguido de `return 1` pegado al margen),
el parser reporta además un error por cada definición de variable que venga después, porque tras resincronizar el resto
del programa se lee como sentencias.

### 🪟 Ver el gráfico del AST en Windows (Anaconda Prompt)

En PowerShell, `python` suele apuntar al atajo de la Microsoft Store ("Python was not found"). Lo más simple es usar el
**Anaconda Prompt**, donde `python` ya funciona. En ese prompt (cmd) las rutas llevan `\`:

```bat
cd C:\Users\TuUsuario\Documents\GitHub\Compilador-Chocopy

build\chocopy.exe Pruebas\programas\Programa_valido.py --ast-json build\ast.json
python Visualizador\ast_grafico.py build\ast.json --abrir
```

- Corre `chocopy` desde la carpeta del proyecto: busca `Gramaticas\chocopy.txt` en la ruta actual.
- Si el dibujo sale muy grande, mira solo una función: `python Visualizador\ast_grafico.py build\ast.json --solo suma_lista --abrir`.
- Alternativa en PowerShell, con la ruta completa a tu Python: `& "C:\Users\TuUsuario\miniconda3\python.exe" Visualizador/ast_grafico.py build/ast.json --abrir`.

---

## 🎤 Guía para la exposición

La división de temas entre los tres integrantes (qué archivos y funciones debe dominar cada uno, demo, cambio en vivo
y preguntas probables) está en [`Documentacion/division_exposicion.md`](Documentacion/division_exposicion.md).

Comandos listos para mostrar cada criterio de la rúbrica (desde la carpeta del proyecto):

**Integración (un solo ejecutable).** Una sola corrida recorre todas las fases y muestra cada una con su prefijo `INFO`.

```bash
./build/chocopy Pruebas/programas/Programa_valido.py
```

Código: `Herramientas/chocopy.cpp`, función `main`.

**Diseño del AST.** Cada nodo muestra su línea y su tipo inferido.

```bash
./build/chocopy Pruebas/programas/Programa_valido.py --ast
```

Código: `AST/nodos.hpp` (`Nodo`, `tipoInferido`), `AST/constructor.cpp` (`construirAST`), `AST/impresion.cpp`
(`imprimirAST`, `escribirASTJson`).

**Tabla de símbolos y scopes.** Una tabla por scope, unida al padre; se ven `global`, `nonlocal` y el shadowing.

```bash
./build/chocopy Pruebas/programas/semantica/scopes_valido.py --tablas
```

Código: `Semantico/tabla_simbolos.cpp` (`Ambito::insertar`, `Ambito::buscar`), `Semantico/analizador.cpp`
(`AnalizadorSemantico::analizar`, `revisarFuncion`).

**Chequeo semántico.** Tipos de expresiones, llamadas, condiciones y ámbitos.

```bash
./build/chocopy Pruebas/programas/semantica/errores_semanticos.py
```

Código: `Semantico/tipos.cpp` (`JerarquiaClases::esSubtipo`, `esAsignable`, `unir`), `Semantico/analizador.cpp`
(`revisarSentencia`, `tipoOpBinaria`, `tipoLlamada`).

**Mensajes de error.** Formato de `g++` con todos los errores de una corrida, ordenados por línea y sin cascada.

```bash
./build/chocopy Pruebas/programas/semantica/errores_semanticos.py
```

Código: `Comun/diagnostico.cpp` (formato de los mensajes), `Semantico/analizador.cpp` (`error`, `nota`,
`imprimirErrores`).

**Pruebas.** Pruebas unitarias de tokens, scanner, parser, AST, errores y análisis semántico.

```bash
./build/pruebas
```

Código: `Pruebas/correr_pruebas.cpp` (`main`), y una prueba por tema en `Pruebas/prueba_*.cpp`.

**Scanner y parser (de apoyo).** Para responder preguntas sobre las fases anteriores al análisis semántico:
`Scanner/lexer.cpp` (`AnalizadorLexico`, indentación con pila), `GeneradorLR1/tabla.cpp` (tabla LR(1)) y
`Parser/parser.cpp` (`AnalizadorSintactico::parsear`, recuperación en modo pánico).
