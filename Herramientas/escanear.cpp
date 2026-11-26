// escanear.cpp
//
// Programa aparte (con su propio main) que corre el scanner sobre un
// archivo ChocoPy real y va imprimiendo cada token a medida que lo
// devuelve siguienteToken(), en el estilo INFO/DEBUG del enunciado del
// proyecto. Uso:
//
//   ./build/escanear Pruebas/programas/Programa_prueba.py
//
// Si no se pasa argumento, usa "Pruebas/programas/Programa_prueba.py" por
// defecto.

#include <iostream>
#include "../Scanner/lexer.hpp"
#include "../Scanner/tokens.hpp"

int main(int argc, char** argv) {
    std::string archivo = (argc > 1) ? argv[1] : "Pruebas/programas/Programa_prueba.py";

    std::cout << "INFO SCAN - Iniciando escaneo de \"" << archivo << "\"...\n";

    AnalizadorLexico scanner(archivo);

    for (;;) {
        Token t = scanner.siguienteToken();
        std::cout << "DEBUG SCAN - " << tokenATexto(t) << "\n";
        if (t.tipo == TipoToken::END_OF_FILE) break;
    }

    std::cout << "INFO SCAN - Terminado con " << scanner.cantidadErrores() << " errores\n";

    return 0;
}
