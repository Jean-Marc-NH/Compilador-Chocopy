#include "conjuntos.hpp"

std::set<std::string> firstSecuencia(const std::vector<std::string>& secuencia,
                                     const ConjuntosFirst& first) {
    std::set<std::string> resultado;
    bool todosVacios = true;
    for (const auto& simbolo : secuencia) {
        const auto& conjunto = first.at(simbolo);
        for (const auto& terminal : conjunto)
            if (terminal != "e") resultado.insert(terminal);
        // Si este simbolo no puede ser vacio, los que siguen ya no aportan.
        if (!conjunto.count("e")) {
            todosVacios = false;
            break;
        }
    }
    if (todosVacios) resultado.insert("e");
    return resultado;
}

ConjuntosFirst calcularFirst(const Gramatica& gramatica) {
    ConjuntosFirst first;
    for (const auto& simbolo : gramatica.noTerminales) first[simbolo] = {};
    for (const auto& simbolo : gramatica.terminales) first[simbolo] = {simbolo};
    first["$"] = {"$"};

    // Para A > X1 X2 ... Xn, FIRST(A) incluye FIRST(X1 X2 ... Xn).
    bool cambio = true;
    while (cambio) {
        cambio = false;
        for (const auto& produccion : gramatica.producciones) {
            auto nuevos = firstSecuencia(produccion.derecho, first);
            for (const auto& simbolo : nuevos)
                if (first[produccion.izquierdo].insert(simbolo).second) cambio = true;
        }
    }
    return first;
}

ConjuntosFollow calcularFollow(const Gramatica& gramatica, const ConjuntosFirst& first) {
    ConjuntosFollow follow;
    for (const auto& simbolo : gramatica.noTerminales) follow[simbolo] = {};
    follow[gramatica.producciones.front().izquierdo].insert("$");

    bool cambio = true;
    while (cambio) {
        cambio = false;
        for (const auto& produccion : gramatica.producciones) {
            const auto& derecho = produccion.derecho;
            for (std::size_t i = 0; i < derecho.size(); ++i) {
                if (!gramatica.noTerminales.count(derecho[i])) continue;
                // Para A > alfa B beta: FIRST(beta) - {e} va a FOLLOW(B), y si
                // beta puede ser vacio tambien entra FOLLOW(A).
                std::vector<std::string> resto(derecho.begin() + i + 1, derecho.end());
                auto primeros = firstSecuencia(resto, first);
                auto& destino = follow[derecho[i]];
                for (const auto& terminal : primeros)
                    if (terminal != "e" && destino.insert(terminal).second) cambio = true;
                if (primeros.count("e"))
                    for (const auto& terminal : follow[produccion.izquierdo])
                        if (destino.insert(terminal).second) cambio = true;
            }
        }
    }
    return follow;
}
