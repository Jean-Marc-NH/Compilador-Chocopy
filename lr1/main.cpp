#include "generador.hpp"
#include <exception>
#include <iostream>

int main(int cantidad, char* argumentos[]) {
    try {
        std::string ruta = cantidad > 1 ? argumentos[1] : "lr1/ejemplos/gramatica.txt";
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
