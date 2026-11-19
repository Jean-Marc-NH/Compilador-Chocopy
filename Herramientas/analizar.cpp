// analizar.cpp
//
// Programa aparte (con su propio main) que conecta el scanner con el parser
// LR(1): obtiene la tabla a partir de la gramatica de ChocoPy y analiza un
// archivo fuente pidiendo tokens al scanner. Uso:
//
//   ./build/analizar programa.py [gramatica.txt] [-v] [--sin-recuperacion] [--regenerar]
//                    [--ast] [--ast-json salida.json]
//
// Si no se pasa argumento, usa "Pruebas/programas/Programa_prueba.py" y
// "Gramaticas/chocopy.txt" por defecto. Con -v imprime cada shift/reduce y
// cada paso de la recuperacion de errores.
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
//
// Codigo de salida: 0 si el programa se acepta, 1 si tiene errores y 2 si la
// gramatica tiene conflictos.

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "AST/constructor.hpp"
#include "AST/impresion.hpp"
#include "GeneradorLR1/cache.hpp"
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"

namespace {

struct Opciones {
    std::string archivo = "Pruebas/programas/Programa_prueba.py";
    std::string rutaGramatica = "Gramaticas/chocopy.txt";
    bool detallado = false;
    bool recuperacion = true;
    bool regenerar = false;
    bool mostrarAst = false;
    std::string rutaJson; // vacio: no se guarda el JSON
};

// El primer argumento suelto es el programa y el segundo la gramatica.
Opciones leerOpciones(int argc, char** argv) {
    Opciones opciones;
    int posicional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string argumento = argv[i];
        if (argumento == "-v") opciones.detallado = true;
        else if (argumento == "--sin-recuperacion") opciones.recuperacion = false;
        else if (argumento == "--regenerar") opciones.regenerar = true;
        else if (argumento == "--ast") opciones.mostrarAst = true;
        else if (argumento == "--ast-json" && i + 1 < argc) opciones.rutaJson = argv[++i];
        else if (posicional++ == 0) opciones.archivo = argumento;
        else opciones.rutaGramatica = argumento;
    }
    return opciones;
}

// Construye el AST a partir del arbol del parser y lo imprime y/o lo guarda.
void generarAST(AnalizadorSintactico& parser, bool aceptado, const Opciones& opciones) {
    auto arbol = parser.tomarArbol();
    if (!aceptado || !arbol) {
        std::cerr << "INFO AST - No se genera el AST: el programa tiene errores\n";
        return;
    }
    auto programa = construirAST(*arbol);
    if (opciones.mostrarAst) imprimirAST(*programa, std::cout);
    if (!opciones.rutaJson.empty()) {
        std::filesystem::path destino(opciones.rutaJson);
        if (destino.has_parent_path()) std::filesystem::create_directories(destino.parent_path());
        std::ofstream salida(opciones.rutaJson, std::ios::binary);
        if (!salida) throw std::runtime_error("No se pudo escribir: " + opciones.rutaJson);
        escribirASTJson(*programa, opciones.archivo, salida);
        std::cout << "INFO AST - AST guardado en \"" << opciones.rutaJson << "\"\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    Opciones opciones = leerOpciones(argc, argv);

    try {
        // 1) Gramatica y tabla LR(1) (de la cache si sirve).
        std::string textoGramatica = leerArchivo(opciones.rutaGramatica);
        Gramatica gramatica = parsearGramatica(textoGramatica);

        std::string rutaCache = "build/cache/" +
            std::filesystem::path(opciones.rutaGramatica).stem().string() + ".lr1";
        bool desdeCache = false;
        TablaLR1 tabla = obtenerTabla(gramatica, textoGramatica, rutaCache, desdeCache, opciones.regenerar);
        std::cout << "INFO PARSE - Tabla LR(1) de \"" << opciones.rutaGramatica << "\" "
                  << (desdeCache ? "cargada de \"" + rutaCache + "\"" : "generada y guardada en \"" + rutaCache + "\"")
                  << ": " << tabla.cantidadEstados << " estados, "
                  << tabla.conflictos.size() << " conflictos\n";
        if (!tabla.conflictos.empty()) {
            for (const auto& conflicto : tabla.conflictos) std::cerr << conflicto << "\n";
            return 2;
        }

        // 2) Scanner + parser sobre el programa.
        std::cout << "INFO PARSE - Analizando \"" << opciones.archivo << "\"...\n";
        AnalizadorLexico scanner(opciones.archivo);
        AnalizadorSintactico parser(scanner, gramatica, tabla,
                                    opciones.recuperacion ? NO_TERMINALES_PANICO_CHOCOPY
                                                          : std::vector<std::string>{});
        bool aceptado = parser.parsear(opciones.detallado);

        std::cout << "INFO PARSE - Terminado con " << scanner.cantidadErrores() << " errores lexicos y "
                  << parser.cantidadErrores() << " errores sintacticos\n";
        std::cout << (aceptado ? "INFO PARSE - Programa ACEPTADO\n" : "INFO PARSE - Programa RECHAZADO\n");

        // 3) AST, solo si se pidio.
        if (opciones.mostrarAst || !opciones.rutaJson.empty()) generarAST(parser, aceptado, opciones);

        return aceptado ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
