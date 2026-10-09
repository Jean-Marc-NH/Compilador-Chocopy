# Laboratorio LR(1) canónico

Generador de tablas y parser dirigido por tablas en **C++17**, con interfaz de escritorio **Python/Tkinter**. La gramática y la entrada se editan en la ventana; Python solo presenta los datos calculados en C++. No requiere paquetes de pip ni acceso a Internet.

Este módulo permite experimentar con gramáticas libres de contexto. No reemplaza el scanner ni integra todavía la gramática completa de ChocoPy. Si una gramática no es LR(1), detecta internamente sus conflictos y bloquea el análisis; no elige una acción arbitrariamente. La interfaz no incluye una vista de conflictos.

## Ejecutar

Desde la raíz del proyecto:

```bash
python3 Parser/gui.py
```

La primera ejecución de **Construir tablas** o **Construir y analizar** compila el motor automáticamente con `g++`. Requiere Python 3 con Tkinter, una sesión gráfica y un compilador C++17. En distribuciones donde Tkinter venga separado, instala el paquete `python3-tk` de tu sistema. `CXX` permite seleccionar otro compilador.

También puedes compilar y lanzar explícitamente:

```bash
make -C Parser
make -C Parser gui
```

En la ventana:

1. Escribe o abre una gramática. Se incluye el ejemplo de `A` y `B` con cierres `)` y `>`.
2. Pulsa **Construir tablas** para ver FIRST, estados, núcleos, cierres, transiciones y tablas.
3. Escribe tokens separados por espacios, por ejemplo `( ( a ) )`, y pulsa **Construir y analizar**.
4. Revisa la traza de seguimiento, o el estado/token donde aparece un error.
5. **Exportar JSON** guarda el resultado calculado junto con la gramática y los parámetros usados. Si editas los campos, un aviso indica que las tablas visibles corresponden a la ejecución anterior.

La pestaña Closure muestra veinte estados por página para evitar una ventana inmanejable con gramáticas grandes. Cada fila incluye todos los GOTO de entrada, núcleo, número de estado y cierre completo. Las tablas tienen desplazamiento horizontal y vertical. La tabla LR combina ACTION (columnas `A:`) y GOTO (columnas `G:`); también se muestran por separado. El seguimiento ocupa una pestaña completa, sin árbol sintáctico.

## Formato de gramática

```text
S -> G
G -> A | B
A -> ( A ) | a
B -> ( B > | b
```

- **Separa símbolos con espacios**, incluyendo puntuación: `( A )`, no `(A)`. `(A)` sin espacios es un único símbolo, no tres. Los nombres como `IDENTIFIER`, `num` o `Expr` pueden tener varios caracteres.
- Cada lado izquierdo define un no terminal. Los símbolos restantes son terminales. No se infiere el tipo por mayúsculas o minúsculas.
- La primera producción define el símbolo inicial.
- `|` separa alternativas. También se aceptan continuaciones `| ...` o `-> ...` sin repetir el lado izquierdo.
- Flechas permitidas: `->` y `→`.
- Epsilon: `ε`, `eps`, `epsilon`, `''`, `""` o una alternativa vacía. Debe ocupar toda la alternativa.
- Usa comillas para terminales especiales: `"|"`, `"->"`, `"epsilon"`, `"two words"`. Las comillas no forman parte del símbolo. `\` permite escapar una comilla o una barra dentro del símbolo.
- `$` está reservado para fin de entrada, incluso entre comillas.
- Las líneas completas que empiezan con `#` son comentarios; no se admiten comentarios al final de una regla.
- Se aceptan etiquetas decorativas como `0.`, `1:` y `(2)`. No fijan los identificadores de las producciones: el motor enumera de nuevo y elimina reglas duplicadas.
- Una nueva regla aumentada se agrega siempre como producción **0**. Si introduces `S -> G`, el motor agrega `S' -> S`; las reglas originales empiezan en 1. Si `S'` ya existe, elige un nombre disponible.
- En la entrada también se separan los tokens con espacios. El campo vacío representa la cadena vacía. No escribas `ε` para simular una entrada vacía. El motor agrega `$`; se permite escribirlo una sola vez al final.

Para el ejemplo anterior se generan **19 estados**, porque se agrega `S' -> S`. Si empiezas directamente con `G -> A | B`, se generan 18. Los estados se numeran de forma determinista, recorriendo las transiciones en orden lexicográfico; los números pueden diferir de otros simuladores sin que cambie el resultado.

## CLI sin interfaz gráfica

```bash
make -C Parser

# Reporte de FIRST, estados, ACTION/GOTO y seguimiento
Parser/build/lr1 --grammar Parser/examples/parentheses.grammar --input '( a )'

# Leer la gramática por stdin; terminar con EOF (Ctrl+D en Linux)
Parser/build/lr1 --input 'num + num'

# Resultado estructurado para otras interfaces
Parser/build/lr1 --grammar Parser/examples/nullable.grammar --input '' --json

# Ejemplo de conflicto shift/reduce
Parser/build/lr1 --grammar Parser/examples/conflict.grammar --json

# Ayuda y límites configurables
Parser/build/lr1 --help
```

Sin `--input` se construyen las tablas sin ejecutar el parser. `--input ''` sí analiza una entrada vacía. El reporte de consola usa columnas separadas por tabuladores; la GUI muestra tablas con celdas.

Códigos de salida:

| Código | Significado |
|---|---|
| 0 | Gramática LR(1); si se solicitó análisis, entrada aceptada |
| 1 | Gramática mal escrita, opción inválida, error de archivo o límite de construcción |
| 2 | Conflictos en la gramática; las tablas siguen disponibles |
| 3 | Entrada rechazada o límite de pasos alcanzado |

El JSON incluye `productions`, `terminals`, `nonterminals`, `first`, `nullable`, `states`, `action`, `goto`, `conflicts` y, si se analiza una entrada, `parse`. Cada celda ACTION contiene una lista: varias acciones representan un conflicto. `parse` contiene tokens, traza, resultado y mensaje; no genera árbol sintáctico.

## Algoritmo

1. Leer y aumentar la gramática; identificar terminales y no terminales.
2. Calcular anulables y FIRST hasta un punto fijo.
3. Crear `I0 = closure({[S' -> • S, $]})`.
4. Para `[A -> α • B β, a]`, agregar `[B -> • γ, b]` para cada producción de B y cada `b` de `FIRST(βa)`. Si todo β es anulable, se hereda `a`.
5. Construir GOTO moviendo el punto sobre cada símbolo y calculando el cierre resultante. Reutilizar un estado únicamente si coinciden todos sus ítems **incluyendo los lookaheads**. No fusionar estados como en LALR.
6. Llenar ACTION con shifts sobre terminales, reducciones sobre el lookahead del ítem completo y aceptación para la regla aumentada. Llenar GOTO con transiciones sobre no terminales.
7. Conservar y reportar conflictos shift/reduce, reduce/reduce o de aceptación.
8. Analizar con pila de estados/símbolos y registrar el seguimiento. Una reducción por ε no desapila símbolos. Las reducciones no consumen entrada.

El lookahead de un ítem indica lo que puede seguir al no terminal completo, no necesariamente el siguiente token a la derecha del punto. Consulta especialmente los estados posteriores a `(` en el ejemplo: el A interior recibe `)` y el B interior recibe `>`.

## Límites

LR(1) canónico puede generar muchos estados. Valores por defecto: 1000 estados y 1000 pasos; configurables hasta 5000 y 10000. Además: 64 KiB de gramática/entrada, 512 producciones originales, 128 símbolos por producción, 4096 tokens de entrada, 50000 ítems por cierre y 250000 ítems entre todos los estados. La GUI da hasta 120 segundos a la compilación y 30 segundos a cada ejecución del motor. Si se alcanza un límite, lo informa; no afirma que una gramática sea inválida por ello.

## Pruebas

```bash
make -C Parser test
```

Las pruebas cubren la gramática del ejemplo, propagación de lookaheads por sufijos anulables, ε, recursión izquierda, precedencia, traza, gramáticas LR(1) que no son SLR/LALR, conflictos, errores y límites. Además enumeran cadenas cortas para comparar el parser con el lenguaje independiente `a* b*`.

Archivos:

- `lr1.hpp` / `lr1.cpp`: motor reutilizable, cálculo y análisis.
- `main.cpp`: entrada de consola y salida texto/JSON.
- `gui.py`: interfaz Tkinter y ejecución del motor en segundo plano.
- `examples/`: gramáticas de ejemplo.
- `../Tests/testLR1.py`: pruebas de caja negra.
