#ifndef SCANNER_DIAGNOSTICO_HPP
#define SCANNER_DIAGNOSTICO_HPP

#include <iosfwd>
#include <string>

// Formato de los mensajes de error, al estilo de g++ / clang:
//
//   programa.py:12:11: error: el caracter '@' no es valido en ChocoPy
//      12 |     x = a @ b
//         |           ^
//
// El subrayado es '^' en la columna del error seguido de '~' hasta cubrir
// 'largo' caracteres. 'tipo' es "error" o "nota" (las notas apuntan a otro
// lugar relacionado, por ejemplo al '(' que quedo sin cerrar).
//
// 'textoLinea' es el codigo de esa linea sin el salto de linea; con nullptr
// solo se imprime la primera linea del mensaje (sirve cuando no hay codigo
// que mostrar, como en el fin de archivo).
void imprimirDiagnostico(std::ostream& salida, const std::string& archivo, int linea, int columna,
                         int largo, const std::string& tipo, const std::string& mensaje,
                         const std::string* textoLinea);

// Error que no esta asociado a una posicion del codigo: "archivo: error: mensaje".
void imprimirErrorGeneral(std::ostream& salida, const std::string& archivo, const std::string& mensaje);

#endif
