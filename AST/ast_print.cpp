#include "ast_print.hpp"
#include <cstdio>
#include <ostream>
#include <sstream>

static void imprimir(const Node& nodo, const std::string& rol, int nivel, std::ostream& salida) {
    salida << std::string(static_cast<std::size_t>(nivel) * 2, ' ');
    if (!rol.empty()) salida << rol << ": ";
    salida << nodo.kind();
    if (!nodo.text().empty()) salida << ' ' << nodo.text();
    salida << '\n';
    std::vector<Child> hijos;
    nodo.children(hijos);
    for (const auto& hijo : hijos) imprimir(*hijo.second, hijo.first, nivel + 1, salida);
}

void imprimirAST(const Node& raiz, std::ostream& salida) { imprimir(raiz, "", 0, salida); }

std::string astATexto(const Node& raiz) {
    std::ostringstream salida;
    imprimirAST(raiz, salida);
    return salida.str();
}

static std::string escaparJson(const std::string& texto) {
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
                } else resultado += static_cast<char>(caracter); // UTF-8 pasa tal cual
        }
    }
    return resultado;
}

static void volcarJson(const Node& nodo, const std::string& rol, int nivel, std::ostream& salida) {
    const std::string sangria(static_cast<std::size_t>(nivel) * 2, ' ');
    salida << sangria << "{";
    if (!rol.empty()) salida << "\"role\": \"" << escaparJson(rol) << "\", ";
    salida << "\"kind\": \"" << nodo.kind() << "\", \"text\": \"" << escaparJson(nodo.text())
           << "\", \"line\": " << nodo.loc.line << ", \"col\": " << nodo.loc.column;
    std::vector<Child> hijos;
    nodo.children(hijos);
    if (hijos.empty()) {
        salida << ", \"children\": []}";
        return;
    }
    salida << ", \"children\": [\n";
    for (std::size_t i = 0; i < hijos.size(); ++i) {
        volcarJson(*hijos[i].second, hijos[i].first, nivel + 1, salida);
        salida << (i + 1 < hijos.size() ? ",\n" : "\n");
    }
    salida << sangria << "]}";
}

void escribirASTJson(const Node& raiz, const std::string& archivoFuente, std::ostream& salida) {
    salida << "{\"archivo\": \"" << escaparJson(archivoFuente) << "\", \"ast\":\n";
    volcarJson(raiz, "", 0, salida);
    salida << "}\n";
}
