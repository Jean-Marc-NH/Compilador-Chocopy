#include "impresion.hpp"
#include <cstdio>
#include <ostream>
#include <sstream>

namespace {

void imprimir(const Nodo& nodo, const std::string& rol, int nivel, std::ostream& salida) {
    salida << std::string(static_cast<std::size_t>(nivel) * 2, ' ');
    if (!rol.empty()) salida << rol << ": ";
    salida << nodo.clase();
    if (!nodo.texto().empty()) salida << ' ' << nodo.texto();
    salida << '\n';
    std::vector<Hijo> hijos;
    nodo.hijos(hijos);
    for (const auto& hijo : hijos) imprimir(*hijo.second, hijo.first, nivel + 1, salida);
}

// Escapa las comillas, barras y caracteres de control para que el texto sea
// un string JSON valido.
std::string escaparJson(const std::string& texto) {
    std::string resultado;
    for (unsigned char caracter : texto) {
        switch (caracter) {
            case '"': resultado += "\\\""; break;
            case '\\': resultado += "\\\\"; break;
            case '\n': resultado += "\\n"; break;
            case '\r': resultado += "\\r"; break;
            case '\t': resultado += "\\t"; break;
            default:
                if (caracter < 0x20) {
                    char codigo[8];
                    std::snprintf(codigo, sizeof codigo, "\\u%04x", caracter);
                    resultado += codigo;
                } else {
                    resultado += static_cast<char>(caracter); // UTF-8 pasa tal cual
                }
        }
    }
    return resultado;
}

void volcarJson(const Nodo& nodo, const std::string& rol, int nivel, std::ostream& salida) {
    const std::string sangria(static_cast<std::size_t>(nivel) * 2, ' ');
    salida << sangria << "{";
    if (!rol.empty()) salida << "\"rol\": \"" << escaparJson(rol) << "\", ";
    salida << "\"clase\": \"" << nodo.clase() << "\", \"texto\": \"" << escaparJson(nodo.texto())
           << "\", \"linea\": " << nodo.pos.linea << ", \"col\": " << nodo.pos.columna;
    std::vector<Hijo> hijos;
    nodo.hijos(hijos);
    if (hijos.empty()) {
        salida << ", \"hijos\": []}";
        return;
    }
    salida << ", \"hijos\": [\n";
    for (std::size_t i = 0; i < hijos.size(); ++i) {
        volcarJson(*hijos[i].second, hijos[i].first, nivel + 1, salida);
        salida << (i + 1 < hijos.size() ? ",\n" : "\n");
    }
    salida << sangria << "]}";
}

} // namespace

void imprimirAST(const Nodo& raiz, std::ostream& salida) { imprimir(raiz, "", 0, salida); }

std::string astATexto(const Nodo& raiz) {
    std::ostringstream salida;
    imprimirAST(raiz, salida);
    return salida.str();
}

void escribirASTJson(const Nodo& raiz, const std::string& archivoFuente, std::ostream& salida) {
    salida << "{\"archivo\": \"" << escaparJson(archivoFuente) << "\", \"ast\":\n";
    volcarJson(raiz, "", 0, salida);
    salida << "}\n";
}
