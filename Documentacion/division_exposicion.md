# División de temas para la exposición

La defensa es **individual**: a cada uno le pueden pedir que explique su parte mostrando una prueba o haciendo un
cambio en vivo, y le pueden hacer una pregunta conceptual que tiene que **aplicar a un caso concreto**. Por eso cada
persona tiene un bloque del compilador, los archivos y funciones que debe conocer, una demo lista, un cambio en vivo
para practicar y las preguntas más probables.

| | Persona 1: Front-end | Persona 2: Parser, AST e integración | Persona 3: Análisis semántico |
|---|---|---|---|
| Fases | Scanner y generador LR(1) | Parser, recuperación de errores, AST, ejecutable | Tablas de símbolos y chequeo de tipos |
| Carpetas | `Scanner/`, `GeneradorLR1/`, `Gramaticas/` | `Parser/`, `AST/`, `Herramientas/chocopy.cpp`, `Visualizador/`, `Comun/` | `Semantico/` |
| Criterios de la rúbrica | Requisito previo ("todos los pasos anteriores") | Integración (2), Diseño del AST (3), Mensajes de error (2) | Tabla de símbolos y scopes (2), Chequeo semántico (3), Mensajes de error (2) |

Antes de exponer, compilar todo una vez (la primera corrida genera la tabla LR(1) en `build/cache/`):

```powershell
powershell -ExecutionPolicy Bypass -File compilar.ps1 -Pruebas
```

---

## Lo que los tres deben saber

El recorrido completo de un programa, porque cualquiera puede recibir la pregunta "¿qué pasa cuando corro esto?":

```
programa.py
  -> AnalizadorLexico::siguienteToken()        Scanner/lexer.cpp      (texto -> tokens)
  -> AnalizadorSintactico::parsear()           Parser/parser.cpp      (tokens -> árbol de derivación, tabla LR(1))
  -> construirAST()                            AST/constructor.cpp    (árbol de derivación -> AST)
  -> AnalizadorSemantico::analizar()           Semantico/analizador.cpp (tablas de símbolos + tipos)
```

Todo lo encadena `main()` en `Herramientas/chocopy.cpp`. Si hay errores léxicos o sintácticos no se arma el AST y el
semántico no corre. Cada fase reporta todos sus errores con el mismo formato (`Comun/diagnostico.cpp`).

```powershell
.\build\chocopy.exe Pruebas\programas\Programa_valido.py              # ACEPTADO
.\build\chocopy.exe Pruebas\programas\Programa_prueba.py              # errores léxicos y sintácticos
.\build\chocopy.exe Pruebas\programas\semantica\errores_semanticos.py # 24 errores semánticos
```

---

## Persona 1: Scanner y generador LR(1)

### Archivos y funciones
- `Scanner/tokens.hpp`: `TipoToken`, `Token` y `Posicion` (línea y columna).
- `Scanner/lexer.cpp`:
  - `siguienteToken()`: el punto de entrada; decide qué escanear según el carácter actual.
  - `procesarInicioDeLinea()`: indentación con una **pila**. Si la sangría crece emite `INDENT`, si baja emite un
    `DEDENT` por cada nivel que se cierra, y si no coincide con ningún nivel da error.
  - `escanearIdentificador()` + `tablaPalabrasClave()`: identificador o palabra reservada.
  - `escanearEntero()` (máximo 2147483647), `escanearCadena()` (escapes, `IDSTRING`), `escanearOperador()`.
  - `reportarError()`: errores léxicos con línea y columna.
- `GeneradorLR1/`:
  - `gramatica.cpp`: `parsearGramatica()` lee `Gramaticas/chocopy.txt` (aumentada con `S' -> PROGRAMA`).
  - `conjuntos.cpp`: `calcularFirst()`, `firstSecuencia()`, `calcularFollow()`.
  - `tabla.cpp`: `cierre()` (closure de items LR(1)), `generarTabla()` (estados con goto, tablas ACTION/GOTO y
    detección de conflictos).
  - `cache.cpp`: `obtenerTabla()` guarda la tabla en disco con una huella de la gramática (~1.8 s -> ~0.04 s).

### Demo en vivo
```powershell
.\build\escanear.exe Pruebas\programas\Programa_valido.py               # tokens, con INDENT/DEDENT
.\build\lr1.exe Gramaticas\ejemplos\gramatica.txt                       # producciones, estados y tabla
.\build\lr1.exe Gramaticas\ejemplos\gramatica_conflicto.txt             # una gramática con conflicto
.\build\pruebas.exe                                                     # secciones "PRUEBA DE LEXER"
```

### Cambio en vivo para practicar
- Que el scanner reconozca un operador o palabra nueva: agregar el `TipoToken` en `tokens.hpp`, el caso en
  `escanearOperador()` o en `tablaPalabrasClave()`, y el nombre en `tipoTokenATexto()`.
- Cambiar el texto de un error léxico y mostrarlo con un `.py` que lo provoque.

### Preguntas probables
- **Token y lexema**: en `x = 42`, `IDENTIFIER` es el token y `"x"` el lexema.
- **¿Por qué una pila para la indentación?** Porque los bloques se anidan: al pasar de la sangría 8 a la 0 hay que
  cerrar dos bloques y se emiten dos `DEDENT` (mostrarlo con `escanear` sobre un `if` dentro de un `def`).
- **FIRST y FOLLOW**: para qué sirven y cómo se calculan sobre una producción de `chocopy.txt`.
- **Item LR(1)** `[A -> α · β, a]`: el punto indica cuánto se leyó y `a` es el lookahead, que decide cuándo reducir.
- **LR(1) frente a SLR/LALR**: LR(1) usa un lookahead por item y por eso tiene más estados (965 en ChocoPy) y menos
  conflictos. Un shift/reduce se ve con `gramatica_conflicto.txt`.
- **Precedencia sin conflictos**: la gramática está estratificada (`EXPR > ORE > ANDE > ...`), un nivel por cada
  precedencia de la tabla de la sección 4.1 del Language Reference.

---

## Persona 2: Parser, recuperación de errores, AST e integración

### Archivos y funciones
- `Parser/parser.cpp`:
  - `parsear()`: el algoritmo shift/reduce con la pila de estados, que consulta ACTION y GOTO. En cada reduce arma un
    `NodoParse` (`Parser/arbol.hpp`).
  - `recuperar()` + `elegirNoTerminal()`: recuperación en **modo pánico**. Desapila hasta un estado con GOTO sobre un
    no terminal de sincronización (`NO_TERMINALES_PANICO_CHOCOPY`) y descarta tokens hasta uno que pueda seguir.
  - `reportarError()` y `Parser/mensajes.cpp` (`describirToken()`): pasan del "token inesperado" a un mensaje para quien
    programa ("falta ':' al final de la linea", "falta cerrar el '('...").
- `AST/nodos.hpp`: un nodo por construcción (`Programa`, `DefFuncion`, `Si`, `OpBinaria`, `Llamada`...). Todos heredan de
  `Nodo`, que guarda `pos` (línea y columna) y `tipoInferido`, y exponen `clase()`, `texto()` e `hijos()`.
- `AST/constructor.cpp`: `construirAST()` y sus ayudantes `expr()`, `sentencia()`, `definicionFuncion()`... Se saltan las
  producciones unitarias, `aplanar()` convierte las listas recursivas en vectores y los `elif` se anidan en el `sino`.
- `AST/impresion.cpp`: `imprimirAST()` (texto con `[linea N, tipo T]`) y `escribirASTJson()`.
- `Herramientas/chocopy.cpp`: `main()` encadena las fases; también tiene las opciones de línea de comandos.
- `Visualizador/ast_grafico.py`: dibuja el JSON del AST.

### Demo en vivo
```powershell
.\build\chocopy.exe Pruebas\programas\errores\varios_errores.py         # varios errores sintácticos en una corrida
.\build\chocopy.exe Pruebas\programas\Programa_prueba.py -v             # cada shift/reduce y la recuperación
.\build\chocopy.exe Pruebas\programas\Programa_valido.py --ast          # AST con línea y tipo
.\build\chocopy.exe Pruebas\programas\Programa_valido.py --ast-json build\ast.json
& "C:\Users\LuisFabian\miniconda3\python.exe" Visualizador\ast_grafico.py build\ast.json --solo suma_lista --abrir
```

### Cambio en vivo para practicar
- Agregar una opción nueva a `chocopy.cpp` (por ejemplo `--solo-parser`, que termine después del parser).
- Cambiar el formato de `imprimirAST()` (por ejemplo, mostrar también la columna).
- Agregar un mensaje nuevo en `Parser/mensajes.cpp` para un caso que hoy sale genérico.

### Preguntas probables
- **Árbol de derivación frente a AST**: para `x = (a + b)`, el árbol de derivación trae `EXPR -> ORE -> ... -> ATOMO`
  y los paréntesis. El AST deja solo `Asignacion(objetivo: x, valor: OpBinaria +(a, b))`. Mostrar las dos cosas con `-v`
  y con `--ast`.
- **¿Qué guarda cada nodo?** Línea y columna (para los mensajes) y el tipo que le pone el semántico.
- **Modo pánico**: cómo se sincroniza después de un error. Mostrarlo con `varios_errores.py`, que reporta varios
  errores en vez de detenerse en el primero.
- **Lista plana o anidada** (diapositivas "Building the AST"): usamos listas planas (`ListaNodos`); `aplanar()`
  convierte `SENTLISTA -> SENTLISTA SENT` en un vector.
- **Integración**: por qué el semántico no corre si hay errores sintácticos (no hay AST confiable).

---

## Persona 3: Análisis semántico

### Archivos y funciones
- `Semantico/tipos.cpp` (`JerarquiaClases`):
  - `esSubtipo(a, b)`: `a <= b` (misma clase, subclase u `object`).
  - `esAsignable(a, b)`: `a <=a b`. Agrega que `None` cabe en todo lo que no es int/bool/str y que `[]` cabe en
    cualquier lista.
  - `unir(a, b)`: el join (ancestro común); se usa en `[1, None]`, en `a if c else b` y en `+` de listas.
- `Semantico/tabla_simbolos.cpp` (`Ambito`):
  - `insertar()` (falla si el nombre ya está en esa tabla: redeclaración).
  - `buscarLocal()` (solo esta tabla) y `buscar()`: el **lookup por la cadena de padres**. Se salta los scopes de clase
    y sigue las entradas `global`/`nonlocal` hasta la variable real.
  - `imprimir()`: lo que se ve con `--tablas`.
- `Semantico/analizador.cpp` (`AnalizadorSemantico`):
  - `analizar()`: los 3 pasos (1. registrar clases, 2. declarar nombres globales, 3. revisar cuerpos).
  - `declarar()`: redeclaración y nombre de clase usado como variable.
  - `declararMiembrosDeClase()`: herencia, atributos redefinidos y métodos con otra firma.
  - `revisarFuncion()` + `declararEnFuncion()`: crea la tabla hija y procesa parámetros, variables locales, `global` y
    `nonlocal`. También revisa que todos los caminos terminen en `return` (`siempreRetorna()`).
  - `revisarSentencia()`: asignaciones, `if`/`while` (condición bool), `for`, `return`.
  - `tipoVariableAsignable()`: no se puede asignar a una variable de afuera sin `global`/`nonlocal`.
  - `calcularTipo()`, `tipoOpBinaria()`, `tipoLlamada()`, `revisarArgumentos()`: los tipos de las expresiones.
  - `error()`, `nota()`, `imprimirErrores()`: los mensajes se guardan y se imprimen ordenados por línea.

### Demo en vivo
```powershell
.\build\chocopy.exe Pruebas\programas\semantica\scopes_valido.py --tablas        # tablas por scope, global, nonlocal
.\build\chocopy.exe Pruebas\programas\semantica\errores_semanticos.py            # 24 errores distintos
.\build\pruebas.exe                                                               # "PRUEBA DE ANALISIS SEMANTICO"
```

### Cambio en vivo para practicar
- **Nuevo chequeo**: error de división entre cero con un literal (`x // 0`). En `tipoOpBinaria()`, si `op` es `//` o
  `%` y `operacion.der` es un `LitEntero` con valor `"0"`, llamar a `error(...)`.
- **Nueva función predefinida**: en `declararPredefinidos()`, agregar `funcion("abs", {tipos::INT}, tipos::INT);` y
  mostrar que `abs("a")` da error de tipo de argumento.

### Preguntas probables
- **Scope y lookup**: en `scopes_valido.py`, al usar una variable dentro de una función anidada, `buscar()` la busca
  primero en la tabla de la función y después en la del padre, hasta la global.
- **Shadowing**: un parámetro con el mismo nombre que una global la oculta, porque `buscar()` la encuentra antes. Es
  válido; asignar a la global sin `global x` no lo es.
- **`global` y `nonlocal`**: agregan una entrada que apunta a la variable de afuera y por eso se le puede asignar.
  `nonlocal` no puede apuntar a una global (mostrar el error).
- **Subtipo frente a asignable**: `None` no es subtipo de `animal`, pero sí se puede asignar a una variable `animal`.
  No se puede asignar a una `int`.
- **¿Por qué varias pasadas?** Para que una función pueda llamar a otra definida más abajo y una anotación pueda nombrar
  una clase que se define después.
- **¿Por qué no hay errores en cascada?** Una expresión con error recibe el tipo interno `<error>`, que es compatible
  con todo. Así `x = noexiste + 1` da 1 error y no 3.

---

## Antes de la exposición (los tres)
- Correr `compilar.ps1 -Pruebas` y ver que todo pase.
- Cada uno prueba su demo y su cambio en vivo, y después lo deshace con `git checkout -- <archivo>`.
- Cada uno lee el comentario de cabecera de sus `.hpp`, que explica qué hace cada función.
