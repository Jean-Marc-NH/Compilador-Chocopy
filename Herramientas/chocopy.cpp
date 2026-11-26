// chocopy.cpp
//
// Ejecutable principal del compilador: une todas las fases en una sola
// corrida, sin pasos a mano.
//
//   programa.py -> scanner -> parser LR(1) -> AST -> analisis semantico
//
// Uso:
//
//   ./build/chocopy programa.py [opciones]
//
//   --ast               imprime el AST con la linea y el tipo de cada nodo
//   --ast-json archivo  guarda el AST (con tipos) en JSON para Visualizador/ast_grafico.py
//   --tablas            imprime la tabla de simbolos de cada scope
//   --predefinidos      con --tablas, muestra tambien print, len, input, int, str...
//   -v                  imprime cada shift/reduce y cada paso de la recuperacion de errores
//   --sin-recuperacion  el parser se detiene en el primer error de sintaxis
//   --regenerar         ignora la cache y regenera la tabla LR(1)
//   --gramatica archivo usa otra gramatica (por defecto Gramaticas/chocopy.txt)
//
// Las fases se encadenan: si hay errores lexicos o sintacticos no se puede
// armar el AST y el analisis semantico no corre. Cada fase reporta TODOS sus
// errores (no se detiene en el primero) con archivo, linea, columna y causa.
//
// La tabla LR(1) tarda varios segundos en generarse, asi que se guarda en
// "build/cache/<gramatica>.lr1" y solo se regenera cuando cambia el texto de
// la gramatica (o con --regenerar).
//
// Codigo de salida: 0 si el programa es valido, 1 si tiene errores y 2 si la
// gramatica tiene conflictos.

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../AST/constructor.hpp"
#include "../AST/impresion.hpp"
#include "../GeneradorLR1/cache.hpp"
#include "../Parser/parser.hpp"
#include "../Scanner/lexer.hpp"
#include "../Semantico/analizador.hpp"

namespace {

struct Opciones {
    std::string archivo;
    std::string rutaGramatica = "Gramaticas/chocopy.txt";
    bool detallado = false;
    bool recuperacion = true;
    bool regenerar = false;
    bool mostrarAst = false;
    bool mostrarTablas = false;
    bool conPredefinidos = false;
    std::string rutaJson; // vacio: no se guarda el JSON
};

const char* USO =
    "Uso: chocopy programa.py [--ast] [--ast-json salida.json] [--tablas] [--predefinidos]\n"
    "                         [-v] [--sin-recuperacion] [--regenerar] [--gramatica archivo.txt]\n";

bool leerOpciones(int argc, char** argv, Opciones& opciones) {
    for (int i = 1; i < argc; ++i) {
        std::string argumento = argv[i];
        if (argumento == "-v") opciones.detallado = true;
        else if (argumento == "--sin-recuperacion") opciones.recuperacion = false;
        else if (argumento == "--regenerar") opciones.regenerar = true;
        else if (argumento == "--ast") opciones.mostrarAst = true;
        else if (argumento == "--tablas") opciones.mostrarTablas = true;
        else if (argumento == "--predefinidos") opciones.conPredefinidos = true;
        else if (argumento == "--ast-json" && i + 1 < argc) opciones.rutaJson = argv[++i];
        else if (argumento == "--gramatica" && i + 1 < argc) opciones.rutaGramatica = argv[++i];
        else if (!argumento.empty() && argumento[0] == '-') {
            std::cerr << "Opcion desconocida: " << argumento << "\n";
            return false;
        } else if (opciones.archivo.empty()) opciones.archivo = argumento;
        else return false;
    }
    return !opciones.archivo.empty();
}

void guardarJson(const Programa& programa, const Opciones& opciones) {
    std::filesystem::path destino(opciones.rutaJson);
    if (destino.has_parent_path()) std::filesystem::create_directories(destino.parent_path());
    std::ofstream salida(opciones.rutaJson, std::ios::binary);
    if (!salida) throw std::runtime_error("No se pudo escribir: " + opciones.rutaJson);
    escribirASTJson(programa, opciones.archivo, salida);
    std::cout << "INFO AST - AST guardado en \"" << opciones.rutaJson << "\"\n";
}

} // namespace

int main(int argc, char** argv) {
    Opciones opciones;
    if (!leerOpciones(argc, argv, opciones)) {
        std::cerr << USO;
        return 1;
    }

    try {
        // 0) Gramatica y tabla LR(1) (de la cache si sirve).
        std::string textoGramatica = leerArchivo(opciones.rutaGramatica);
        Gramatica gramatica = parsearGramatica(textoGramatica);
        std::string rutaCache = "build/cache/" +
            std::filesystem::path(opciones.rutaGramatica).stem().string() + ".lr1";
        bool desdeCache = false;
        TablaLR1 tabla = obtenerTabla(gramatica, textoGramatica, rutaCache, desdeCache, opciones.regenerar);
        std::cout << "INFO PARSE - Tabla LR(1) " << (desdeCache ? "cargada de la cache" : "generada") << ": "
                  << tabla.cantidadEstados << " estados, " << tabla.conflictos.size() << " conflictos\n";
        if (!tabla.conflictos.empty()) {
            for (const auto& conflicto : tabla.conflictos) std::cerr << conflicto << "\n";
            return 2;
        }

        // 1) y 2) Scanner + parser: el parser le pide los tokens al scanner.
        std::cout << "INFO PARSE - Analizando \"" << opciones.archivo << "\"...\n";
        AnalizadorLexico scanner(opciones.archivo);
        AnalizadorSintactico parser(scanner, gramatica, tabla,
                                    opciones.recuperacion ? NO_TERMINALES_PANICO_CHOCOPY
                                                          : std::vector<std::string>{});
        bool aceptado = parser.parsear(opciones.detallado);
        std::cout << "INFO PARSE - " << scanner.cantidadErrores() << " errores lexicos y "
                  << parser.cantidadErrores() << " errores sintacticos\n";

        auto arbol = parser.tomarArbol();
        if (!aceptado || !arbol) {
            std::cout << "INFO - Programa RECHAZADO: corrige los errores de sintaxis para seguir con el "
                         "analisis semantico\n";
            return 1;
        }

        // 3) AST.
        auto programa = construirAST(*arbol);

        // 4) Analisis semantico: tablas de simbolos y tipos.
        AnalizadorSemantico semantico(opciones.archivo);
        int erroresSemanticos = semantico.analizar(*programa);
        std::cout << "INFO SEMANTICO - " << erroresSemanticos << " errores semanticos\n";

        if (opciones.mostrarTablas) {
            std::cout << "\n=== Tablas de simbolos ===\n";
            semantico.tablaGlobal().imprimir(std::cout, opciones.conPredefinidos);
        }
        if (opciones.mostrarAst) {
            std::cout << "\n=== AST ===\n";
            imprimirAST(*programa, std::cout, true);
        }
        if (!opciones.rutaJson.empty()) guardarJson(*programa, opciones);

        std::cout << (erroresSemanticos == 0 ? "INFO - Programa ACEPTADO\n" : "INFO - Programa RECHAZADO\n");
        return erroresSemanticos == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
