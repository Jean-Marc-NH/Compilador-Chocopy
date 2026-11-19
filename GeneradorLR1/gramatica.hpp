#ifndef GENERADORLR1_GRAMATICA_HPP
#define GENERADORLR1_GRAMATICA_HPP

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

// Lectura de la gramatica desde un archivo de texto.
//
// Formato (ver Gramaticas/chocopy.txt y Gramaticas/ejemplos/):
//   - Una regla por linea: IZQUIERDO > alternativa1 alternativa2 ...
//   - Las alternativas se separan con espacios o con |, asi que los simbolos
//     de una misma alternativa van pegados.
//   - Los no terminales van en MAYUSCULAS (pueden terminar en ').
//   - Los terminales son un caracter suelto (a, +, ...) o un nombre largo
//     entre llaves, por ejemplo {KW_IF}.
//   - e es la cadena vacia y # empieza un comentario.
//
// Si la primera regla no esta aumentada (S' > S) se agrega sola: queda como
// produccion 0 y su reduccion es el accept.

struct Produccion {
    std::string izquierdo;
    std::vector<std::string> derecho; // vacio en las producciones e
};

struct Gramatica {
    std::vector<Produccion> producciones;     // el indice identifica cada reduce
    std::multimap<std::string, int> reglas;   // no terminal -> indices de sus producciones
    std::set<std::string> noTerminales;
    std::set<std::string> terminales;
};

// Lee el archivo completo; lanza std::runtime_error si no se puede abrir.
std::string leerArchivo(const std::string& ruta);

// Arma la gramatica a partir del texto. Lanza std::runtime_error con la linea
// del problema si el formato no es valido.
Gramatica parsearGramatica(const std::string& texto);

// Imprime las producciones numeradas (0 es la regla aumentada).
void mostrarProducciones(const Gramatica& gramatica, std::ostream& salida);

#endif
