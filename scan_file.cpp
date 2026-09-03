// scan_file.cpp
//
// Driver standalone (con su propio main) que corre el scanner sobre un
// archivo ChocoPy real y va imprimiendo cada token a medida que lo
// devuelve gettoken(), en el estilo INFO/DEBUG del enunciado del
// proyecto. Uso:
//
//   ./scan_file programa_prueba.py
//
// Si no se pasa argumento, usa "programa_prueba.py" por defecto.

#include <iostream>
#include "Scanner/lexer.hpp"
#include "Scanner/tokens.hpp"

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "programa_prueba.py";

    std::cout << "INFO SCAN - Start scanning \"" << filename << "\"...\n";

    Lexer lex(filename);

    for (;;) {
        Token t = lex.gettoken();
        std::cout << "DEBUG SCAN - " << tokenToString(t) << "\n";
        if (t.type == TokenType::END_OF_FILE) break;
    }

    std::cout << "INFO SCAN - Completed with " << lex.errorCount() << " errors\n";

    return 0;
}