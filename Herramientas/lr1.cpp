// lr1.cpp
//
// Herramienta del generador LR(1) por separado: lee una gramatica e imprime
// sus producciones, los estados LR(1) y la tabla ACTION/GOTO. Sirve para
// revisar gramaticas chicas a mano. Uso:
//
//   ./build/lr1 Gramaticas/ejemplos/gramatica.txt
//
// Codigo de salida: 0 si la gramatica es LR(1), 2 si tiene conflictos y 1 si
// no se pudo leer.

#include <exception>
#include <iostream>
#include "GeneradorLR1/gramatica.hpp"
#include "GeneradorLR1/tabla.hpp"

int main(int argc, char* argv[]) {
    try {
        std::string ruta = argc > 1 ? argv[1] : "Gramaticas/ejemplos/gramatica.txt";
        auto gramatica = parsearGramatica(leerArchivo(ruta));
        auto tabla = generarTabla(gramatica);
        mostrarProducciones(gramatica, std::cout);
        mostrarEstados(gramatica, tabla, std::cout);
        mostrarTabla(gramatica, tabla, std::cout);
        return tabla.conflictos.empty() ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
