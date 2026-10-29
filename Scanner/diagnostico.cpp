#include "diagnostico.hpp"
#include <iomanip>
#include <ostream>

void imprimirDiagnostico(std::ostream& salida, const std::string& archivo, int linea, int columna,
                         int largo, const std::string& tipo, const std::string& mensaje,
                         const std::string* textoLinea) {
    salida << archivo << ':' << linea << ':' << columna << ": " << tipo << ": " << mensaje << '\n';
    if (!textoLinea) return;

    salida << std::setw(5) << linea << " | " << *textoLinea << '\n';
    salida << "      | ";
    // Se copian las tabulaciones del codigo para que el ^ quede alineado.
    for (int i = 0; i < columna - 1; ++i) {
        char c = i < static_cast<int>(textoLinea->size()) ? (*textoLinea)[static_cast<std::size_t>(i)] : ' ';
        salida << (c == '\t' ? '\t' : ' ');
    }
    salida << '^';
    for (int i = 1; i < largo; ++i) salida << '~';
    salida << '\n';
}

void imprimirErrorGeneral(std::ostream& salida, const std::string& archivo, const std::string& mensaje) {
    salida << archivo << ": error: " << mensaje << '\n';
}
