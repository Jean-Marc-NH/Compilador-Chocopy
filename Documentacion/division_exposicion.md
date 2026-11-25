# Guía para la exposición

La defensa es **individual**. A cada uno le pueden pedir:
- que explique el código mostrando una prueba o haciendo un cambio en vivo;
- que responda una pregunta conceptual y la **aplique a un caso concreto**.

La guía tiene dos partes:

- **Parte 1: lo que los tres deben saber sí o sí.** Es lo que hicimos al inicio del proyecto: scanner, generador
  LR(1) y parser, con la recuperación de errores. Todo lo de la entrega final se apoya en esto, y la rúbrica exige que
  "todos los pasos anteriores" estén implementados.
- **Parte 2: lo que nos dividimos.** Son los criterios de la rúbrica final y los temas de la carpeta Final (AST, tablas
  de símbolos y los temas de back end).

Antes de exponer, compilar todo una vez. La primera corrida genera la tabla LR(1) en `build/cache/`.

```powershell
powershell -ExecutionPolicy Bypass -File compilar.ps1 -Pruebas
```

---

# Parte 1: lo que los tres deben saber

## 1.1 El recorrido completo

A cualquiera le pueden preguntar "¿qué pasa cuando corro esto?":

```
programa.py
  -> AnalizadorLexico::siguienteToken()        Scanner/lexer.cpp         (texto -> tokens)
  -> AnalizadorSintactico::parsear()           Parser/parser.cpp         (tokens -> árbol de derivación, con la tabla LR(1))
  -> construirAST()                            AST/constructor.cpp       (árbol de derivación -> AST)
  -> AnalizadorSemantico::analizar()           Semantico/analizador.cpp  (tablas de símbolos + tipos)
```

`main()` en `Herramientas/chocopy.cpp` encadena todo. El parser **le pide** los tokens al scanner uno por uno; no hay
una lista de tokens previa. Si hay errores léxicos o sintácticos no se arma el AST y el semántico no corre.

```powershell
.\build\chocopy.exe Pruebas\programas\Programa_valido.py               # pasa todas las fases: ACEPTADO
.\build\chocopy.exe Pruebas\programas\Programa_prueba.py               # 1 error léxico y 3 sintácticos
.\build\chocopy.exe Pruebas\programas\semantica\errores_semanticos.py  # 24 errores semánticos
```

## 1.2 Scanner (análisis léxico)

**Archivos:** `Scanner/tokens.hpp` y `Scanner/lexer.hpp` / `lexer.cpp`.

- `Token` = tipo (`TipoToken`) + lexema + `Posicion` (línea y columna).
- `siguienteToken()` es el punto de entrada. Mira el carácter actual y decide qué escanear: identificador, entero,
  cadena u operador. Es un AFD escrito a mano.
- `escanearIdentificador()` + `tablaPalabrasClave()`: lee el identificador y decide si es una palabra reservada.
- `escanearEntero()`: el entero más grande es 2147483647.
- `escanearCadena()`: maneja los escapes `\" \\ \n \t`. Una cadena con forma de identificador es `IDSTRING`.
- `procesarInicioDeLinea()`: la **indentación con una pila**.
  - Si la sangría crece, apila el nivel y emite `INDENT`.
  - Si baja, emite un `DEDENT` por cada nivel que desapila.
  - Si no coincide con ningún nivel de la pila, da error.
  - Las líneas en blanco y las de solo comentario se ignoran.
- `reportarError()`: el error léxico se reporta y el scanner sigue; no se detiene.

**Demo:**
```powershell
.\build\escanear.exe Pruebas\programas\Programa_valido.py
```

**Preguntas típicas:**
- Token frente a lexema: en `x = 42`, `IDENTIFIER` es el token y `"x"` es el lexema.
- ¿Por qué una pila para la indentación? Porque los bloques se anidan. Al pasar de la sangría 8 a la 0 se cierran dos
  bloques a la vez, así que se emiten dos `DEDENT`. Mostrarlo con `escanear` sobre un `if` dentro de un `def`.

## 1.3 Gramática y generador LR(1)

**Archivos:** `Gramaticas/chocopy.txt` y la carpeta `GeneradorLR1/`.

- `parsearGramatica()` lee la gramática y la aumenta con `S' -> PROGRAMA`.
- `calcularFirst()` / `calcularFollow()` (`conjuntos.cpp`):
  - FIRST(α): los terminales con los que puede empezar α.
  - FOLLOW(A): los terminales que pueden venir después de A.
- **Item LR(1)** `[A -> α · β, a]`: el punto marca cuánto se leyó y `a` es el lookahead. Con el punto al final, se
  reduce solo si el siguiente token es `a`.
- `cierre()` (closure): si hay `[A -> α · B β, a]`, agrega `[B -> · γ, b]` para cada `b` de FIRST(βa).
- `generarTabla()`: arma los estados con goto y llena las tablas.
  - ACTION: shift, reduce o accept.
  - GOTO: a qué estado ir después de reducir a un no terminal.
  - Si una celda tiene dos acciones, hay un **conflicto**.
- `obtenerTabla()` (`cache.cpp`): guarda la tabla en disco junto con una huella de la gramática. La primera vez tarda
  ~1.8 s; desde la caché, ~0.04 s. ChocoPy da 965 estados y 0 conflictos.
- La precedencia se resuelve **estratificando** la gramática (`EXPR > ORE > ANDE > ...`), un nivel por cada fila de la
  tabla de precedencia (sec. 4.1 del Language Reference). Por eso no hay conflictos.

**Demo:**
```powershell
.\build\lr1.exe Gramaticas\ejemplos\gramatica.txt              # producciones, estados y tabla
.\build\lr1.exe Gramaticas\ejemplos\gramatica_conflicto.txt    # cómo se ve un conflicto
```

**Preguntas típicas:**
- LR(1) frente a SLR y LALR: LR(1) lleva un lookahead por item. Por eso tiene más estados, pero menos conflictos.
- Calcular FIRST o FOLLOW de un no terminal de `gramatica.txt` a mano.

## 1.4 Parser y recuperación de errores

**Archivos:** `Parser/parser.cpp`, `Parser/mensajes.cpp`, `Parser/arbol.hpp` y `Comun/diagnostico.cpp`.

- `parsear()` aplica el algoritmo shift/reduce con una pila de estados:
  - **shift**: apila el estado y pide el siguiente token;
  - **reduce A -> β**: desapila |β| estados, crea un `NodoParse` para A y va a `GOTO[tope, A]`;
  - **accept**: el programa es sintácticamente correcto.
- `recuperar()` hace la recuperación en **modo pánico**:
  1. desapila hasta un estado que tenga GOTO sobre un no terminal de sincronización
     (`NO_TERMINALES_PANICO_CHOCOPY`, por ejemplo una sentencia);
  2. descarta tokens hasta uno que pueda seguir.
  Así se reportan varios errores en una corrida.
- `reportarError()` + `mensajes.cpp`: convierten "token inesperado en el estado N" en un mensaje para quien programa,
  por ejemplo "falta ':' al final de la linea" o "falta cerrar el '(' con ')'".
- Formato de todos los errores, en todas las fases: `archivo:linea:columna: error: ...`, la línea de código y un `^`.

**Demo:**
```powershell
.\build\chocopy.exe Pruebas\programas\errores\varios_errores.py   # varios errores en una corrida
.\build\chocopy.exe Pruebas\programas\Programa_prueba.py -v       # cada shift/reduce y la recuperación
.\build\pruebas.exe                                                # todas las pruebas automáticas
```

---

# Parte 2: lo que nos dividimos

| | **Persona 1** | **Persona 2** | **Persona 3** |
|---|---|---|---|
| Rúbrica final | Integración (2) + Diseño del AST (3) | Tabla de símbolos y scopes (2) | Chequeo semántico (3) + Mensajes de error (2) |
| Carpeta Final | *Building the AST* + *Instruction Selection* | *Symbol Tables* + *Register Allocation* + *Graph Coloring Allocation* | *Instruction Scheduling* (scheduling.pdf y diapositivas) + *Software Pipelining* |
| Código | `AST/`, `Herramientas/chocopy.cpp`, `Visualizador/` | `Semantico/tabla_simbolos.*` y la parte de declaraciones de `Semantico/analizador.cpp` | `Semantico/tipos.*` y la parte de chequeo de `Semantico/analizador.cpp` |

Los temas de back end (selección de instrucciones, registros y scheduling) **no están implementados** en el
compilador. Son teoría para la pregunta conceptual y para `practica-backend.pdf`, así que cada uno debe poder resolver
un ejemplo a mano.

---

## Persona 1: Integración, AST y selección de instrucciones

### Código
- `Herramientas/chocopy.cpp`, `main()`: encadena las fases y lee las opciones (`--ast`, `--ast-json`, `--tablas`,
  `-v`...). Devuelve 0 si el programa es válido y 1 si tiene errores.
- `AST/nodos.hpp`: un nodo por construcción del lenguaje, por ejemplo `Programa`, `DefFuncion`, `DefClase`, `Si`,
  `Mientras`, `Para`, `Asignacion`, `OpBinaria`, `Llamada`, `Miembro` e `Indice`.
  - Todos heredan de `Nodo`, que guarda `pos` (línea y columna) y `tipoInferido` (lo llena el semántico).
  - Todos exponen `clase()`, `texto()` e `hijos()`. Con eso el volcado de texto, el JSON y el gráfico recorren
    cualquier nodo sin conocer su clase.
- `AST/constructor.cpp`, `construirAST()`, con los ayudantes `expr()`, `sentencia()`, `definicionFuncion()` y
  `definicionClase()`:
  - se saltan las producciones unitarias (`EXPR -> ORE -> ANDE -> ...`);
  - los paréntesis y la puntuación no generan nodos;
  - `aplanar()` convierte las listas recursivas (`SENTLISTA -> SENTLISTA SENT`) en vectores;
  - los `elif` se anidan dentro del `sino` del `Si` anterior.
- `AST/impresion.cpp`: `imprimirAST()` imprime en texto con `[linea N, tipo T]` y `escribirASTJson()` genera el JSON.
- `Visualizador/ast_grafico.py`: dibuja el JSON con los tipos.

### Demo
```powershell
.\build\chocopy.exe Pruebas\programas\Programa_valido.py --ast
.\build\chocopy.exe Pruebas\programas\Programa_valido.py --ast-json build\ast.json
& "C:\Users\LuisFabian\miniconda3\python.exe" Visualizador\ast_grafico.py build\ast.json --solo suma_lista --abrir
```

### Cambio en vivo para practicar
- Agregar una opción `--solo-parser` a `chocopy.cpp`, que termine después del parser.
- En `imprimirAST()`, mostrar también la columna: `[linea 12:5, tipo int]`.

### Conceptos
- **Árbol de derivación frente a AST.** Para `x = (a + b)`, el árbol de derivación tiene toda la cadena
  `EXPR -> ORE -> ... -> ATOMO` y los paréntesis. El AST es solo `Asignacion(objetivo: x, valor: OpBinaria +(a, b))`.
  Mostrar las dos cosas con `-v` y con `--ast`.
- **Building the AST.** Las acciones semánticas (`$$ = new BinOp(...)`) construyen el nodo en cada reducción.
  Nosotros armamos el AST en un paso aparte, recorriendo el árbol de derivación. Las listas pueden ser planas o
  anidadas; usamos listas planas (`ListaNodos`).
- **Instruction Selection.** Traducir el IR (ILOC) a instrucciones de la máquina. La optimización **peephole** recorre
  el código con una ventana de 2 o 3 operaciones y reemplaza patrones por otros más baratos. Ejemplos:
  - `storeAI r1 => r0,8` seguido de `loadAI r0,8 => r2` se reemplaza por `i2i r1 => r2`;
  - `addI r1,0 => r2` es `i2i r1 => r2`;
  - `multI r1,2 => r2` es `add r1,r1 => r2`.
  Con una ventana más grande se encuentran más patrones, pero cuesta más. Practicar el ejercicio 1 de
  `practica-backend.pdf`.

---

## Persona 2: Tablas de símbolos, scopes y asignación de registros

### Código
- `Semantico/tabla_simbolos.cpp`, clase `Ambito`: **una tabla por scope** con un puntero a su padre.
  - `insertar()`: declara un nombre solo en la tabla actual. Devuelve false si ya estaba en esa tabla
    (redeclaración).
  - `buscarLocal()`: busca solo en esta tabla.
  - `buscar()`: el **lookup por la cadena de padres**. Busca en esta tabla y luego en las de los padres, hasta la
    global. Se salta los scopes de clase (los atributos solo se ven con `self.x`) y sigue las entradas
    `global`/`nonlocal` hasta la variable real.
  - `crearHijo()`: las tablas no se borran al salir del scope; quedan como hijas para imprimirlas con `--tablas`.
- `Semantico/analizador.cpp`, la parte de declaraciones:
  - `declararPredefinidos()`: carga `print`, `len`, `input`, `object`, `int`, `bool` y `str`;
  - `declarar()`: detecta la redeclaración (con una nota que señala la declaración anterior) y el nombre de clase usado
    como variable;
  - `declararMiembrosDeClase()`: crea la tabla de la clase y aplica la herencia de atributos y métodos;
  - `revisarFuncion()`: crea la tabla hija de la función y declara parámetros y variables locales;
  - `declararEnFuncion()`: procesa `global x` (tiene que existir en la tabla global) y `nonlocal x` (tiene que ser una
    variable de una función que encierre a esta);
  - `tipoVariableAsignable()`: no se puede asignar a una variable de afuera sin `global`/`nonlocal`.

### Demo
```powershell
.\build\chocopy.exe Pruebas\programas\semantica\scopes_valido.py --tablas
.\build\chocopy.exe Pruebas\programas\semantica\scopes_valido.py --tablas --predefinidos
```

### Cambio en vivo para practicar
- Agregar una función predefinida en `declararPredefinidos()`: `funcion("abs", {tipos::INT}, tipos::INT);`. Mostrarla
  con `--tablas --predefinidos` y comprobar que `abs("a")` da error.
- En `Ambito::imprimir()`, mostrar también la columna de cada símbolo.

### Conceptos
- **Symbol Tables.** Declarar es insertar en la tabla actual. Usar un nombre es buscarlo en la actual y luego en los
  padres; si no aparece en ninguna, es un error. Al terminar una función, la tabla actual vuelve a ser la del padre.
- **Shadowing.** En `scopes_valido.py`, el parámetro `x` de `duplicar()` oculta a la `x` global, porque `buscar()`
  lo encuentra primero. Es válido.
- **global / nonlocal.** Leer una variable de afuera está permitido, pero asignarle no, porque crearía una local. Con
  `global x` (o `nonlocal x`) la tabla de la función recibe una entrada que apunta a la variable de afuera. En
  `scopes_valido.py` están `acumular()` (`global total`) y `contar_pares()` con `anotar()` (`nonlocal pares`). En
  `errores_semanticos.py`, `sin_global()` muestra el error de asignar sin `global`.
- **Register Allocation.** Decidir qué valores quedan en los k registros físicos y cuáles van a memoria (spill).
  - **Rango vivo**: desde que se define un valor hasta su último uso.
  - **MAXLIVE**: la máxima cantidad de valores vivos a la vez. Si MAXLIVE > k, hay que hacer spill.
  - **Asignación local de Best** (bottom-up): cuando no queda un registro libre, se libera el del valor cuyo próximo
    uso está más lejos.
- **Graph Coloring Allocation.**
  - El **grafo de interferencia** tiene un nodo por rango vivo y una arista entre dos rangos que están vivos al mismo
    tiempo. Colorear el grafo con k colores es asignar k registros.
  - **Chaitin**: Simplify quita nodos con grado < k y los apila. Si todos tienen grado ≥ k, elige uno para spill según
    costo/grado. Select desapila y colorea.
  - **Chaitin-Briggs**: en vez de hacer spill de inmediato, apila el nodo de forma optimista; solo hace spill si al
    desapilar no queda un color libre.
  - **Webs**: unir las definiciones y los usos de una variable que se alcanzan entre sí en el CFG. Cada web es un
    candidato a registro.
  Practicar los ejercicios 4, 5 y 6 de `practica-backend.pdf`.

---

## Persona 3: Chequeo semántico, mensajes de error y scheduling

### Código
- `Semantico/tipos.cpp`, clase `JerarquiaClases`:
  - `esSubtipo(a, b)`: `a <= b` (misma clase, subclase u `object`).
  - `esAsignable(a, b)`: `a <=a b`. Además de `a <= b`, permite `None` en todo lo que no es int, bool ni str, y `[]` en
    cualquier lista.
  - `unir(a, b)`: el join, el ancestro común más cercano. Se usa en `[1, None]`, en `a if c else b` y en `+` de listas.
- `Semantico/analizador.cpp`, la parte de chequeo:
  - `analizar()`: tres pasos: 1) registrar clases, 2) declarar nombres, 3) revisar cuerpos. Con varios pasos, una
    función puede llamar a otra definida más abajo.
  - `revisarSentencia()`: la condición de `if` y `while` debe ser `bool`; también revisa asignaciones, `for` y
    `return` (tipo de retorno, `return` fuera de una función).
  - `siempreRetorna()`: una función que devuelve int, bool o str debe terminar en `return` en todos los caminos.
  - `calcularTipo()`, `tipoOpBinaria()`, `tipoLlamada()` y `revisarArgumentos()`: los tipos de las expresiones, y la
    cantidad y el tipo de los argumentos.
  - `error()`, `nota()` e `imprimirErrores()`: los mensajes se guardan y al final se imprimen ordenados por línea.
  - El tipo interno `<error>` (en `tipos.hpp`) es compatible con todo, y así no hay errores en cascada: `x = noexiste + 1`
    da 1 error y no 3.
- Lista completa de los errores detectados: en el comentario de `Semantico/analizador.hpp`.

### Demo
```powershell
.\build\chocopy.exe Pruebas\programas\semantica\errores_semanticos.py   # 24 errores, uno por cada "# ERROR n"
.\build\pruebas.exe                                                      # sección "PRUEBA DE ANALISIS SEMANTICO"
```

### Cambio en vivo para practicar
- Nuevo chequeo, división entre cero con un literal (`x // 0`): en `tipoOpBinaria()`, si `op` es `//` o `%` y
  `operacion.der` es un `LitEntero` con `valor == "0"`, llamar a `error(operacion, "division entre cero")`.
- Cambiar el texto de un mensaje y mostrarlo con un `.py` que lo provoque.

### Conceptos
- **Subtipo frente a asignable.** `None` no es subtipo de `animal`, pero sí se puede asignar a una variable `animal`.
  No se puede asignar a una `int`.
- **Join.** `[1, None]` es de tipo `[object]`, porque el ancestro común de `int` y `<None>` es `object`.
- **Mensajes de error.** La rúbrica pide línea y causa, que no se caiga y que reporte más de un error. Mostrarlo con
  `errores_semanticos.py`.
- **Instruction Scheduling (local).**
  - El **grafo de dependencias** de un bloque tiene un nodo por operación y aristas para las dependencias:
    - verdaderas: leer después de escribir;
    - anti: escribir después de leer;
    - de salida: escribir después de escribir;
    - de memoria: `load`/`store` sobre la misma dirección.
  - La **prioridad** de un nodo es el camino más largo, ponderado por latencias, desde el nodo hasta el final.
  - **List scheduling hacia adelante**: en cada ciclo, de las operaciones listas (sus predecesores ya terminaron), se
    eligen las de mayor prioridad, tantas como unidades funcionales haya. Hacia atrás es lo mismo, desde el final.
  - `scheduling.pdf` es el laboratorio `./sched <unidades> < bloque.iloc`.
  - *Beyond Basic Blocks*: se aplica lo mismo sobre bloques básicos extendidos (EBB).
- **Software Pipelining.** Solapar iteraciones de un lazo para ocultar latencias.
  - El **ii** (intervalo de iniciación) es cada cuántos ciclos empieza una iteración nueva.
  - Su cota inferior es el máximo entre:
    - **RC** (recursos): operaciones por iteración entre unidades disponibles, redondeado hacia arriba;
    - **DC** (recurrencias): por cada ciclo de dependencias entre iteraciones, la latencia total entre la distancia,
      redondeado hacia arriba.
  - Tiempo total aproximado ≈ prólogo + (n − 1) · ii + epílogo, frente a n · (largo de una iteración) sin pipelining.
  Practicar los ejercicios 2 y 3 de `practica-backend.pdf`.

---

## Antes de la exposición (los tres)
- Correr `compilar.ps1 -Pruebas` y ver que todo pase.
- Repasar la **Parte 1**: a cualquiera le pueden preguntar por el scanner o el parser.
- Cada uno prueba su demo y su cambio en vivo, y después lo deshace con `git checkout -- <archivo>`.
- Cada uno lee el comentario de cabecera de sus `.hpp`, que explica qué hace cada función.
