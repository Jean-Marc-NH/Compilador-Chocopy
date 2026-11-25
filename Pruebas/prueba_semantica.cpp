#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "AST/constructor.hpp"
#include "AST/impresion.hpp"
#include "GeneradorLR1/cache.hpp"
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"
#include "Semantico/analizador.hpp"
#include "prueba_semantica.hpp"

namespace {

struct Resultado {
    bool sintaxisOk = false;
    int semanticos = 0;
    std::string mensajes;  // todo lo que se escribio en std::cerr
    std::string ast;       // AST con detalles (linea y tipo)
    std::string tablas;    // tabla de simbolos impresa
};

// Corre scanner + parser + AST + analisis semantico sobre 'fuente',
// capturando los mensajes de error.
Resultado analizar(const Gramatica& gramatica, const TablaLR1& tabla, const std::string& fuente) {
    {
        std::ofstream salida("_tmp_sem.py", std::ios::binary);
        salida << fuente;
    }
    std::ostringstream captura;
    std::streambuf* original = std::cerr.rdbuf(captura.rdbuf());
    Resultado resultado;
    {
        AnalizadorLexico scanner("_tmp_sem.py");
        AnalizadorSintactico parser(scanner, gramatica, tabla, NO_TERMINALES_PANICO_CHOCOPY);
        bool aceptado = parser.parsear();
        auto arbol = parser.tomarArbol();
        resultado.sintaxisOk = aceptado && arbol && scanner.cantidadErrores() == 0 && parser.cantidadErrores() == 0;
        if (resultado.sintaxisOk) {
            auto programa = construirAST(*arbol);
            AnalizadorSemantico semantico("_tmp_sem.py");
            resultado.semanticos = semantico.analizar(*programa);
            resultado.ast = astATexto(*programa, true);
            std::ostringstream texto;
            semantico.tablaGlobal().imprimir(texto);
            resultado.tablas = texto.str();
        }
    }
    std::cerr.rdbuf(original);
    resultado.mensajes = captura.str();
    return resultado;
}

struct Caso {
    const char* nombre;
    std::string fuente;
    int errores;
    std::vector<std::string> debeContener; // fragmentos que deben aparecer en los mensajes
};

} // namespace

void prueba_semantica(const std::string& rutaGramatica, const std::string& rutaCache) {
    std::cout << "\nPRUEBA DE ANALISIS SEMANTICO\n\n";

    std::string texto = leerArchivo(rutaGramatica);
    Gramatica gramatica = parsearGramatica(texto);
    bool desdeCache = false;
    TablaLR1 tabla = obtenerTabla(gramatica, texto, rutaCache, desdeCache);

    const std::vector<Caso> casos = {
        // ----- Programas validos: 0 errores -----
        {"valido: shadowing de parametro y de variable local",
         "x: int = 1\n"
         "def f(x: int) -> int:\n"
         "    y: int = 2\n"
         "    return x + y\n"
         "def g() -> int:\n"
         "    x: str = \"a\"\n"
         "    return len(x)\n"
         "print(f(x) + g())\n", 0, {}},
        {"valido: global con asignacion",
         "x: int = 0\n"
         "def inc():\n"
         "    global x\n"
         "    x = x + 1\n"
         "inc()\n", 0, {}},
        {"valido: nonlocal con asignacion en funcion anidada",
         "def f() -> int:\n"
         "    y: int = 0\n"
         "    def g():\n"
         "        nonlocal y\n"
         "        y = y + 1\n"
         "    g()\n"
         "    return y\n"
         "print(f())\n", 0, {}},
        {"valido: herencia con redefinicion correcta de metodo",
         "class A(object):\n"
         "    n: int = 0\n"
         "    def f(self: \"A\", k: int) -> int:\n"
         "        return k\n"
         "class B(A):\n"
         "    def f(self: \"B\", k: int) -> int:\n"
         "        return k + 1\n"
         "a: A = None\n"
         "a = B()\n"
         "print(a.f(2))\n", 0, {}},
        {"valido: listas y concatenacion",
         "a: [int] = None\n"
         "b: [int] = None\n"
         "a = [1, 2]\n"
         "b = a + [3]\n"
         "b[0] = len(b)\n"
         "print(b[1])\n", 0, {}},
        {"valido: for sobre str y sobre lista",
         "c: str = \"\"\n"
         "i: int = 0\n"
         "l: [int] = None\n"
         "l = [1, 2, 3]\n"
         "for c in \"hola\":\n"
         "    print(c)\n"
         "for i in l:\n"
         "    print(i)\n", 0, {}},
        {"valido: None asignado a una clase",
         "class A(object):\n"
         "    pass\n"
         "a: A = None\n"
         "a = None\n"
         "a = A()\n", 0, {}},
        {"valido: clases y metodos llamados con self",
         "class Contador(object):\n"
         "    n: int = 0\n"
         "    def sumar(self: \"Contador\", k: int) -> int:\n"
         "        self.n = self.n + k\n"
         "        return self.n\n"
         "    def doble(self: \"Contador\") -> int:\n"
         "        return self.sumar(self.n)\n"
         "c: Contador = None\n"
         "c = Contador()\n"
         "print(c.sumar(2))\n"
         "print(c.doble())\n", 0, {}},

        // ----- Declaraciones y scopes -----
        {"variable no declarada",
         "print(zzz)\n", 1, {"_tmp_sem.py:1:7: error: 'zzz' no esta declarado"}},
        {"redeclaracion en el mismo scope (con nota)",
         "x: int = 1\nx: int = 2\n", 1,
         {"'x' ya fue declarado en este ambito (global)", "nota: declaracion anterior de 'x'"}},
        {"asignar a global sin 'global'",
         "x: int = 0\n"
         "def f():\n"
         "    x = 1\n"
         "f()\n", 1,
         {"no se puede asignar a 'x': no es una variable de este ambito", "'global x' o 'nonlocal x'"}},
        {"'global z' inexistente",
         "def f():\n    global z\n    pass\n", 1, {"'z' no es una variable global"}},
        {"nonlocal invalido",
         "def f():\n    nonlocal w\n    pass\n", 1, {"'w' no es una variable de una funcion que encierre a"}},
        {"tipo incompatible en asignacion",
         "x: int = 0\nx = \"hola\"\n", 1,
         {"no se puede asignar un valor de tipo 'str' a 'x', que es de tipo 'int'"}},
        {"valor inicial incompatible",
         "x: int = \"a\"\n", 1, {"no se puede inicializar 'x' (de tipo 'int') con un valor de tipo 'str'"}},
        {"condicion de if que no es bool",
         "x: int = 1\nif x:\n    pass\n", 1, {"la condicion del 'if' debe ser de tipo 'bool', no 'int'"}},
        {"condicion de while que no es bool",
         "x: int = 1\nwhile x:\n    x = 0\n", 1, {"la condicion del 'while' debe ser de tipo 'bool', no 'int'"}},
        {"cantidad de argumentos",
         "def f(a: int) -> int:\n    return a\nprint(f(1, 2))\n", 1, {"espera 1 argumento, pero recibe 2"}},
        {"tipo de argumento",
         "def f(a: int) -> int:\n    return a\nprint(f(\"s\"))\n", 1,
         {"el argumento 1 de", "debe ser de tipo 'int', pero es 'str'"}},
        {"tipo de return",
         "def f() -> int:\n    return \"a\"\n", 1, {"la funcion debe devolver 'int', pero este valor es de tipo 'str'"}},
        {"return sin valor en funcion que devuelve int",
         "def f() -> int:\n    return\n", 1, {"falta el valor del 'return': la funcion debe devolver 'int'"}},
        {"funcion sin return en todos los caminos",
         "def f(a: int) -> int:\n    if a > 0:\n        return 1\n", 1,
         {"la funcion 'f' debe devolver 'int', pero no todos sus caminos terminan en 'return'"}},
        {"return fuera de funcion",
         "return 1\n", 1, {"'return' solo se puede usar dentro de una funcion"}},

        // ----- Expresiones -----
        {"operador + con int y str",
         "x: int = 1\nprint(x + \"a\")\n", 1, {"el operador '+' no se puede usar entre 'int' y 'str'"}},
        {"not con int",
         "x: int = 1\nprint(not x)\n", 1, {"el operador 'not' necesita un valor de tipo 'bool', no 'int'"}},
        {"'<' con str",
         "print(\"a\" < \"b\")\n", 1, {"el operador '<' no se puede usar entre 'str' y 'str'"}},
        {"indice que no es int",
         "l: [int] = None\nl = [1]\nprint(l[\"a\"])\n", 1, {"el indice debe ser de tipo 'int', no 'str'"}},
        {"asignar a un caracter de str",
         "s: str = \"abc\"\ns[0] = \"x\"\n", 1, {"los str no se pueden modificar"}},
        {"for sobre int",
         "i: int = 0\nn: int = 3\nfor i in n:\n    pass\n", 1,
         {"'for' solo puede recorrer una lista o un str, no un valor de tipo 'int'"}},
        {"atributo inexistente",
         "class A(object):\n    n: int = 0\na: A = None\na = A()\nprint(a.zz)\n", 1,
         {"el tipo 'A' no tiene el atributo 'zz'"}},
        {"metodo inexistente",
         "class A(object):\n    n: int = 0\na: A = None\na = A()\na.zz()\n", 1,
         {"el tipo 'A' no tiene el metodo 'zz'"}},
        {"usar una funcion sin llamarla",
         "def f() -> int:\n    return 1\nx: int = 0\nx = f\n", 1,
         {"'f' es una funcion: para usarla hay que llamarla"}},
        {"llamar a una variable",
         "x: int = 0\nx()\n", 1, {"'x' no es una funcion: es una variable de tipo 'int'"}},

        // ----- Clases -----
        {"superclase no definida",
         "class B(A):\n    pass\n", 1, {"la superclase 'A' de 'B' no es una clase definida antes"}},
        {"heredar de int",
         "class B(int):\n    pass\n", 1, {"la clase 'B' no puede heredar de 'int'"}},
        {"atributo redefinido en subclase",
         "class A(object):\n    n: int = 0\nclass B(A):\n    n: int = 1\n", 1,
         {"el atributo 'n' ya esta definido en la superclase 'A'"}},
        {"metodo redefinido con otra firma",
         "class A(object):\n"
         "    def f(self: \"A\", k: int) -> int:\n"
         "        return k\n"
         "class B(A):\n"
         "    def f(self: \"B\") -> int:\n"
         "        return 0\n", 1, {"el metodo 'f' redefine al de 'A' con otra firma"}},
        {"primer parametro de metodo que no es la clase",
         "class A(object):\n    def f(self: int) -> int:\n        return 0\n", 1,
         {"el primer parametro del metodo 'f' debe ser self, de tipo 'A'"}},
        {"nombre de clase usado como variable",
         "class A(object):\n    pass\nA: int = 0\n", 1, {"'A' es el nombre de una clase y no se puede usar como variable"}},
        {"tipo anotado inexistente",
         "x: Foo = None\n", 1, {"el tipo 'Foo' no existe"}},

        // ----- Varios errores y sin cascada -----
        {"varios errores en una corrida",
         "x: int = 0\n"
         "x = \"a\"\n"
         "print(noexiste)\n"
         "if x:\n"
         "    pass\n", 3, {"_tmp_sem.py:2:", "_tmp_sem.py:3:", "_tmp_sem.py:4:"}},
        {"sin cascada: variable no declarada dentro de una expresion",
         "x: int = 0\nx = noexiste + 1\n", 1, {"'noexiste' no esta declarado"}},
        {"sin cascada: expresion con error usada en una llamada",
         "def f(a: int) -> int:\n    return a\nx: int = 0\nx = f(noexiste * 2) + 1\n", 1,
         {"'noexiste' no esta declarado"}},
    };

    int fallos = 0;
    for (const auto& caso : casos) {
        Resultado r = analizar(gramatica, tabla, caso.fuente);
        std::string faltantes;
        for (const auto& fragmento : caso.debeContener)
            if (r.mensajes.find(fragmento) == std::string::npos) faltantes += "    falta en la salida: " + fragmento + "\n";
        bool ok = r.sintaxisOk && r.semanticos == caso.errores && faltantes.empty();
        if (ok) {
            std::cout << "OK     " << caso.nombre << "\n";
        } else {
            ++fallos;
            std::cout << "FALLO  " << caso.nombre << ": sintaxis " << (r.sintaxisOk ? "ok" : "con errores")
                      << ", errores semanticos " << r.semanticos << " (se espera " << caso.errores << ")\n"
                      << faltantes << "--- salida de error ---\n" << r.mensajes << "-----------------------\n";
        }
    }

    // El AST con detalles muestra la linea y el tipo de cada expresion.
    {
        Resultado r = analizar(gramatica, tabla, "x: int = 1\nx = x + 2\n");
        bool ok = r.sintaxisOk && r.semanticos == 0 && r.ast.find("[linea 2, tipo int]") != std::string::npos;
        if (ok) {
            std::cout << "OK     el AST con detalles muestra \"[linea N, tipo int]\"\n";
        } else {
            ++fallos;
            std::cout << "FALLO  el AST con detalles no contiene \"[linea 2, tipo int]\"\n" << r.ast << "\n";
        }
    }

    // La tabla de simbolos tiene un scope hijo por funcion, con el simbolo nonlocal.
    {
        Resultado r = analizar(gramatica, tabla,
                               "def f() -> int:\n"
                               "    y: int = 0\n"
                               "    def g():\n"
                               "        nonlocal y\n"
                               "        y = y + 1\n"
                               "    g()\n"
                               "    return y\n"
                               "print(f())\n");
        bool ok = r.sintaxisOk && r.semanticos == 0 &&
                  r.tablas.find("Tabla \"funcion f\"") != std::string::npos &&
                  r.tablas.find("(padre: \"global\")") != std::string::npos &&
                  r.tablas.find("nonlocal") != std::string::npos;
        if (ok) {
            std::cout << "OK     la tabla de simbolos tiene el scope de la funcion y el simbolo nonlocal\n";
        } else {
            ++fallos;
            std::cout << "FALLO  la tabla de simbolos no tiene lo esperado\n" << r.tablas << "\n";
        }
    }

    std::cout << "\nErrores: " << fallos << " (se espera 0)\n";
    std::cout << "\nFIN DE PRUEBAS DE ANALISIS SEMANTICO\n";
}
