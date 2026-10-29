#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"
#include "lr1/generador.hpp"
#include "testErrores.hpp"

namespace {

struct Resultado {
    int lexicos = 0;
    int sintacticos = 0;
    std::string mensajes; // todo lo que se escribio en std::cerr
};

// Analiza 'fuente' con la recuperacion de errores activada, capturando los
// mensajes de error.
Resultado analizar(const Gramatica& gramatica, const TablaLR1& tabla, const std::string& fuente) {
    {
        std::ofstream out("_tmp_err.py", std::ios::binary);
        out << fuente;
    }
    std::ostringstream captura;
    std::streambuf* original = std::cerr.rdbuf(captura.rdbuf());
    Resultado resultado;
    {
        Lexer lex("_tmp_err.py");
        Parser parser(lex, gramatica, tabla, NO_TERMINALES_PANICO_CHOCOPY);
        parser.parsear();
        resultado.lexicos = lex.errorCount();
        resultado.sintacticos = parser.errorCount();
    }
    std::cerr.rdbuf(original);
    resultado.mensajes = captura.str();
    return resultado;
}

struct Caso {
    const char* nombre;
    std::string fuente;
    int lexicos;
    int sintacticos;
    std::vector<std::string> debeContener; // fragmentos que deben aparecer en los mensajes
};

} // namespace

void test_errores(const std::string& rutaGramatica, const std::string& rutaCache) {
    std::cout << "\nPRUEBA DE MENSAJES DE ERROR\n\n";

    std::string texto = leerArchivo(rutaGramatica);
    Gramatica gramatica = parsearGramatica(texto);
    bool desdeCache = false;
    TablaLR1 tabla = obtenerTabla(gramatica, texto, rutaCache, desdeCache);

    const std::vector<Caso> casos = {
        {"parentesis sin cerrar, con nota sobre el '('",
         "x: int = 0\nprint(x\n", 0, 1,
         {"_tmp_err.py:2:8: error: falta cerrar el '(' con ')' antes del fin de la linea",
          "    2 | print(x\n      |        ^\n",
          "_tmp_err.py:2:6: nota: el '(' abierto aqui nunca se cierra"}},
        {"corchete sin cerrar",
         "x: [int] = None\nx = [1, 2\n", 0, 1, {"falta cerrar el '[' con ']'"}},
        {"parentesis de mas",
         "print(1))\n", 0, 1,
         {"')' de mas: no hay ningun '(' abierto que cerrar", "    1 | print(1))\n      |         ^\n"}},
        {"falta ':' al final de la linea",
         "x: int = 0\nif x < 3\n    pass\n", 0, 1, {"falta ':' al final de la linea"}},
        {"el subrayado va despues del codigo, no despues del comentario",
         "x: int = 0\nif x < 3   # comentario\n    pass\n", 0, 1,
         {"    2 | if x < 3   # comentario\n      |         ^\n"}},
        {"falta un valor entre dos operadores",
         "x: int = 0\nx = x + + 1\n", 0, 1, {"falta un valor entre '+' y '+'"}},
        {"asignacion sin valor",
         "x: int = 0\nx =\n", 0, 1, {"falta el valor que se asigna despues de '='"}},
        {"else sin if",
         "x: int = 0\nelse:\n    pass\n", 0, 1, {"'else' sin un 'if' previo"}},
        {"+= no existe",
         "x: int = 0\nx += 1\n", 0, 1, {"'+=' no existe en ChocoPy"}},
        {"palabra reservada de Python",
         "import os\n", 0, 1, {"'import' es una palabra reservada de Python"}},
        {"falta un separador entre dos sentencias",
         "x: int = 0\nx = 1 x = 2\n", 0, 1, {"falta un operador (como + o ==) o un salto de linea antes de 'x'"}},
        {"cuerpo sin sangria",
         "x: int = 0\nif x < 3:\nprint(x)\n", 0, 1, {"falta el bloque indentado"}},
        {"definicion despues de una sentencia",
         "x: int = 0\nx = 1\ny: int = 5\n", 0, 1, {"deben escribirse antes de cualquier sentencia"}},
        {"parametro sin tipo",
         "def k(a, b: int):\n    pass\n", 0, 1, {"se esperaba ':' despues de 'a'"}},
        {"valor inicial negativo",
         "w: int = -1\n", 0, 1, {"el valor inicial de una variable debe ser un literal"}},
        // Errores lexicos: se informan una vez y no producen un error sintactico de eco.
        {"caracter invalido",
         "x: int = 0\nx = x @ 2\n", 1, 0,
         {"_tmp_err.py:2:7: error: el caracter '@' no es valido en ChocoPy", "    2 | x = x @ 2\n      |       ^\n"}},
        {"cadena sin cerrar",
         "x: str = \"a\"\nx = \"hola\n", 1, 0,
         {"falta la comilla de cierre (\") de esta cadena antes del fin de la linea", "      |     ^~~~~\n"}},
        {"entero demasiado grande",
         "x: int = 0\nx = 2147483648\n", 1, 0, {"el numero 2147483648 es demasiado grande"}},
        {"punto y coma",
         "x: int = 0\nx = 1;\n", 1, 0, {"';' no se usa en ChocoPy"}},
        {"division real",
         "x: int = 0\nx = x / 2\n", 1, 0, {"'/' no existe en ChocoPy: para dividir enteros usa '//'"}},
        {"sangria inconsistente (sin errores de eco)",
         "x: int = 0\nif x < 3:\n        x = 1\n    x = 2\n", 1, 0, {"no coincide con la de ningun bloque anterior"}},
        // Recuperacion: cada error independiente se informa una vez y el analisis sigue.
        {"tres errores en lineas distintas",
         "x: int = 0\nx = = 1\nx = 2 +\nx = (3\nprint(x)\n", 0, 3,
         {"_tmp_err.py:2:", "_tmp_err.py:3:", "_tmp_err.py:4:"}},
        {"un error y su cascada en la misma linea cuentan una vez",
         "l: [int] = None\nfor x l:\n    pass\n", 0, 1, {"se esperaba 'in' pero se encontro el nombre 'l'"}},
    };

    int errores = 0;
    for (const auto& caso : casos) {
        Resultado r = analizar(gramatica, tabla, caso.fuente);
        std::string faltantes;
        for (const auto& fragmento : caso.debeContener)
            if (r.mensajes.find(fragmento) == std::string::npos) faltantes += "    falta en la salida: " + fragmento + "\n";
        bool ok = r.lexicos == caso.lexicos && r.sintacticos == caso.sintacticos && faltantes.empty();
        if (ok) {
            std::cout << "OK     " << caso.nombre << "\n";
        } else {
            ++errores;
            std::cout << "FALLO  " << caso.nombre << ": lexicos " << r.lexicos << " (se espera " << caso.lexicos
                      << "), sintacticos " << r.sintacticos << " (se espera " << caso.sintacticos << ")\n"
                      << faltantes << "--- salida de error ---\n" << r.mensajes << "-----------------------\n";
        }
    }

    std::cout << "\nErrores: " << errores << " (se espera 0)\n";
    std::cout << "\nFIN DE PRUEBAS DE MENSAJES DE ERROR\n";
}
