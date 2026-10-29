// parse_file.cpp
//
// Driver standalone (con su propio main) que conecta el scanner con el
// parser LR(1): obtiene la tabla a partir de la gramatica de ChocoPy y
// analiza un archivo fuente pidiendo tokens al Lexer. Uso:
//
//   ./build/parse_file programa.py [gramatica.txt] [-v] [--sin-recuperacion] [--regenerar]
//                      [--ast] [--ast-json salida.json]
//
// Si no se pasa argumento, usa "Tests/programas/Programa_prueba.py" y
// "Parser/gramatica_chocopy.txt" por defecto. Con -v imprime cada
// shift/reduce y cada paso de la recuperacion de errores.
//
// Si el programa se acepta sin errores, --ast imprime el AST como texto
// indentado y --ast-json lo guarda en JSON para dibujarlo con
// Visualizador/ast_grafico.py. Un programa con errores no genera AST.
//
// La tabla LR(1) tarda varios segundos en generarse, asi que se guarda en
// "build/cache/<gramatica>.lr1" y solo se regenera cuando cambia el texto de
// la gramatica (o con --regenerar).
//
// Por defecto el parser se recupera de los errores sintacticos en modo panico
// (ver Parser/parser.hpp) para seguir reportando errores; --sin-recuperacion
// lo detiene en el primero.

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "AST/ast_builder.hpp"
#include "AST/ast_print.hpp"
#include "lr1/generador.hpp"
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"

int main(int argc, char** argv) {
    std::string filename = "Tests/programas/Programa_prueba.py";
    std::string gramaticaPath = "Parser/gramatica_chocopy.txt";
    bool verbose = false;
    bool recuperacion = true;
    bool regenerar = false;
    bool mostrarAst = false;
    std::string rutaJson;

    int posicional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v") verbose = true;
        else if (arg == "--sin-recuperacion") recuperacion = false;
        else if (arg == "--regenerar") regenerar = true;
        else if (arg == "--ast") mostrarAst = true;
        else if (arg == "--ast-json" && i + 1 < argc) rutaJson = argv[++i];
        else if (posicional++ == 0) filename = arg;
        else gramaticaPath = arg;
    }

    try {
        std::string textoGramatica = leerArchivo(gramaticaPath);
        Gramatica gramatica = parsearGramatica(textoGramatica);

        std::string rutaCache = "build/cache/" +
            std::filesystem::path(gramaticaPath).stem().string() + ".lr1";
        bool desdeCache = false;
        TablaLR1 tabla = obtenerTabla(gramatica, textoGramatica, rutaCache, desdeCache, regenerar);
        std::cout << "INFO PARSE - Tabla LR(1) de \"" << gramaticaPath << "\" "
                  << (desdeCache ? "cargada de \"" + rutaCache + "\"" : "generada y guardada en \"" + rutaCache + "\"")
                  << ": " << tabla.cantidadEstados << " estados, "
                  << tabla.conflictos.size() << " conflictos\n";
        if (!tabla.conflictos.empty()) {
            for (const auto& conflicto : tabla.conflictos) std::cerr << conflicto << "\n";
            return 2;
        }

        std::cout << "INFO PARSE - Start parsing \"" << filename << "\"...\n";
        Lexer lex(filename);
        Parser parser(lex, gramatica, tabla,
                      recuperacion ? NO_TERMINALES_PANICO_CHOCOPY : std::vector<std::string>{});
        bool aceptado = parser.parsear(verbose);

        std::cout << "INFO PARSE - Completed with " << lex.errorCount() << " errores lexicos y "
                  << parser.errorCount() << " errores sintacticos\n";
        std::cout << (aceptado ? "INFO PARSE - Programa ACEPTADO\n" : "INFO PARSE - Programa RECHAZADO\n");

        if (mostrarAst || !rutaJson.empty()) {
            auto arbol = parser.tomarArbol();
            if (!aceptado || !arbol) {
                std::cerr << "INFO AST - No se genera el AST: el programa tiene errores\n";
            } else {
                auto programa = construirAST(*arbol);
                if (mostrarAst) imprimirAST(*programa, std::cout);
                if (!rutaJson.empty()) {
                    std::filesystem::path destino(rutaJson);
                    if (destino.has_parent_path()) std::filesystem::create_directories(destino.parent_path());
                    std::ofstream salida(rutaJson, std::ios::binary);
                    if (!salida) throw std::runtime_error("No se pudo escribir: " + rutaJson);
                    escribirASTJson(*programa, filename, salida);
                    std::cout << "INFO AST - AST guardado en \"" << rutaJson << "\"\n";
                }
            }
        }
        return aceptado ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
