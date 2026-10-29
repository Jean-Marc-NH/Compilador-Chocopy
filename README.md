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
├── Scanner/                     # Módulo del analizador léxico
│   ├── tokens.hpp / .cpp        # Definiciones de Token, TokenType y funciones auxiliares
│   ├── indentacion.hpp / .cpp   # Manejo del algoritmo de sangría basado en pila
│   ├── diagnostico.hpp / .cpp   # Formato de los mensajes de error (estilo g++: línea de código y ^)
│   └── lexer.hpp / .cpp         # Motor del Scanner (AFD y consumo de fuente)
├── lr1/                         # Generador de tablas LR(1)
│   ├── generador.hpp / .cpp     # Lee la gramática y construye estados, ACTION y GOTO
│   ├── main.cpp                 # Herramienta para imprimir producciones, estados y tabla
│   └── ejemplos/                # Gramáticas pequeñas de ejemplo (con y sin conflictos)
├── Parser/                      # Analizador sintáctico (Scanner + tabla LR(1))
│   ├── parser.hpp / .cpp        # Shift/reduce que consume tokens del Lexer + recuperación de errores en modo pánico
│   ├── arbol.hpp                # NodoParse: árbol de derivación que arma el parser
│   ├── mensajes.hpp / .cpp      # Traduce los errores de sintaxis a mensajes para el usuario
│   ├── gramatica_chocopy.txt    # Gramática de ChocoPy para el generador
│   ├── gramatica_chocopy_espacios.txt  # La misma gramática, con espacios para leerla mejor
│   └── referencia/              # Language Reference de ChocoPy (PDF y texto)
├── AST/                         # Árbol de sintaxis abstracta
│   ├── ast.hpp / .cpp           # Nodos de ChocoPy (Program, FuncDef, Binary, ...)
│   ├── ast_builder.hpp / .cpp   # Convierte el árbol de derivación en el AST
│   └── ast_print.hpp / .cpp     # Volcado de texto y exportación a JSON
├── Visualizador/
│   └── ast_grafico.py           # Dibuja el AST (HTML/SVG interactivo, solo biblioteca estándar de Python)
├── Drivers/                     # Programas standalone para probar los módulos
│   ├── scan_file.cpp            # Imprime los tokens de un .py
│   └── parse_file.cpp           # Scanner + Parser sobre un .py
├── Tests/                       # Pruebas unitarias y muestras de código ChocoPy
│   ├── testTokens.hpp / .cpp    # Pruebas del tokenizador
│   ├── testLexer.hpp / .cpp     # Pruebas del lexer
│   ├── testAST.hpp / .cpp       # Pruebas de parser + AST
│   ├── testErrores.hpp / .cpp   # Pruebas de los mensajes de error
│   └── programas/               # Programas .py de ejemplo (Programa_valido.py se acepta sin errores;
│                                #   errores/ tiene casos con errores sintácticos)
├── main.cpp                     # Ejecuta las pruebas unitarias
├── build/                       # Ejecutables, caché de la tabla LR(1) y temporales de los tests (ignorado por git)
└── README.md                    # Documentación del proyecto
```

---

## 🛠️ Requisitos e Instalación

### Requisitos Previos
* **Compilador con soporte C++17:** `g++` (>= 8.0), `clang++` (>= 7.0) o MSVC.
* **Sistema Operativo:** Linux, macOS o Windows (MinGW-w64 / MSYS2).

### Compilación y Ejecución

Para compilar el módulo del scanner junto con las pruebas unitarias usando `g++`:

```bash
# Todos los ejecutables se generan en build/ (ignorado por git)
mkdir -p build

# Compilar y correr las pruebas unitarias
g++ -std=c++17 main.cpp Tests/testTokens.cpp Tests/testLexer.cpp Tests/testAST.cpp Tests/testErrores.cpp Scanner/*.cpp Parser/parser.cpp Parser/mensajes.cpp lr1/generador.cpp AST/*.cpp -I. -o build/test_tokens
./build/test_tokens

# Para probar un archivo .py con el scanner
g++ -std=c++17 -I. Drivers/scan_file.cpp Scanner/lexer.cpp Scanner/tokens.cpp Scanner/diagnostico.cpp -o build/scan_file
./build/scan_file Tests/programas/Programa_prueba.py > build/resultado_programa_prueba.txt


# Scanner + Parser juntos (genera la tabla LR(1) y analiza un .py)
g++ -O2 -static -std=c++17 -I. Drivers/parse_file.cpp Parser/parser.cpp Parser/mensajes.cpp lr1/generador.cpp Scanner/lexer.cpp Scanner/tokens.cpp Scanner/identacion.cpp Scanner/diagnostico.cpp AST/*.cpp -o build/parse_file
./build/parse_file Tests/programas/Programa_prueba.py     # usa Parser/gramatica_chocopy.txt
./build/parse_file Tests/programas/Programa_prueba.py -v  # imprime cada shift/reduce y cada recuperación
./build/parse_file Tests/programas/errores/varios_errores.py   # varios errores sintácticos en una corrida
./build/parse_file programa.py --sin-recuperacion         # se detiene en el primer error
./build/parse_file programa.py --regenerar                # ignora la caché y regenera la tabla

# La tabla LR(1) se genera solo la primera vez y se guarda en build/cache/ (~1.8 s -> ~0.04 s);
# se regenera sola si cambia el texto de la gramática.


# Mensajes de error y los 4 errores de ejemplo: ver la sección "Probar errores" más abajo

# AST: solo se genera si el programa se acepta sin errores
./build/parse_file Tests/programas/Programa_valido.py --ast                  # lo imprime como texto indentado
./build/parse_file Tests/programas/Programa_valido.py --ast-json build/ast.json   # lo guarda en JSON

# Gráfico del AST (Python 3, sin instalar nada): genera build/ast.html con zoom y arrastre
python Visualizador/ast_grafico.py build/ast.json --abrir
python Visualizador/ast_grafico.py build/ast.json --solo suma_lista   # solo una función o clase
python Visualizador/ast_grafico.py build/ast.json --profundidad 3     # resumen del programa
python Visualizador/ast_grafico.py build/ast.json -o build/ast.svg    # SVG suelto, para informes

# El gráfico se genera a partir de build/ast.json: si el programa tiene errores no se escribe un
# JSON nuevo y verías el árbol del programa anterior. Usa un programa válido (Programa_valido.py).


# Generador LR(1) por separado (imprime producciones, estados y tabla)
g++ -std=c++17 -Wall -Wextra -pedantic -I. lr1/main.cpp lr1/generador.cpp -o build/lr1
./build/lr1 lr1/ejemplos/gramatica.txt

```

---

## 🧪 Probar errores

Los mensajes de error siguen el estilo de `g++`: archivo, línea y columna, la línea de código con un `^` bajo el
problema, y notas que apuntan a otro lugar relacionado. Están escritos para quien programa, no para quien diseña
el compilador: no hablan de tokens ni del parser.

`Tests/programas/Programa_prueba.py` trae **4 errores intencionales** (marcados con `# ERROR n`) para mostrar que
el compilador los reconoce todos y sigue analizando el resto sin detenerse:

```bash
./build/parse_file Tests/programas/Programa_prueba.py
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

Cada mensaje tiene una prueba automática en `Tests/testErrores.cpp` (se corre con `./build/test_tokens`): errores
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

build\parse_file.exe Tests\programas\Programa_valido.py --ast-json build\ast.json
python Visualizador\ast_grafico.py build\ast.json --abrir
```

- Corre `parse_file` desde la carpeta del proyecto: busca `Parser\gramatica_chocopy.txt` en la ruta actual.
- Si el dibujo sale muy grande, mira solo una función: `python Visualizador\ast_grafico.py build\ast.json --solo suma_lista --abrir`.
- Alternativa en PowerShell, con la ruta completa a tu Python: `& "C:\Users\TuUsuario\miniconda3\python.exe" Visualizador/ast_grafico.py build/ast.json --abrir`.
