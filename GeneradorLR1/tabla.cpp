#include "tabla.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <tuple>

bool Item::operator<(const Item& otro) const {
    return std::tie(produccion, punto, anticipacion) <
           std::tie(otro.produccion, otro.punto, otro.anticipacion);
}

Estado cierre(const Estado& estado, const Gramatica& gramatica, const ConjuntosFirst& first) {
    Estado resultado = estado;
    bool cambio = true;
    while (cambio) {
        cambio = false;
        Estado pendientes;
        for (const auto& item : resultado) {
            const auto& derecho = gramatica.producciones[item.produccion].derecho;
            if (item.punto == static_cast<int>(derecho.size())) continue;
            const auto& simbolo = derecho[item.punto];
            if (!gramatica.noTerminales.count(simbolo)) continue;
            // Para [A > alfa . B beta, a], usamos FIRST(beta a).
            std::vector<std::string> resto(derecho.begin() + item.punto + 1, derecho.end());
            resto.push_back(item.anticipacion);
            auto anticipaciones = firstSecuencia(resto, first);
            auto rango = gramatica.reglas.equal_range(simbolo);
            for (auto regla = rango.first; regla != rango.second; ++regla)
                for (const auto& anticipacion : anticipaciones)
                    if (anticipacion != "e") pendientes.insert({regla->second, 0, anticipacion});
        }
        for (const auto& item : pendientes)
            if (resultado.insert(item).second) cambio = true;
    }
    return resultado;
}

Estado calcularGoto(const Estado& estado, const std::string& simbolo,
                    const Gramatica& gramatica, const ConjuntosFirst& first) {
    // Movemos el punto sobre 'simbolo' en los items que lo tienen adelante.
    Estado movidos;
    for (const auto& item : estado) {
        const auto& derecho = gramatica.producciones[item.produccion].derecho;
        if (item.punto < static_cast<int>(derecho.size()) && derecho[item.punto] == simbolo)
            movidos.insert({item.produccion, item.punto + 1, item.anticipacion});
    }
    return cierre(movidos, gramatica, first);
}

TablaLR1 generarTabla(const Gramatica& gramatica) {
    TablaLR1 tabla;
    auto first = calcularFirst(gramatica);

    // I0 = cierre([S' > . S, $]). Cada estado nuevo se numera en el orden en
    // que aparece; 'numeros' sirve para saber si un estado ya existia.
    tabla.estados.push_back(cierre(Estado{{0, 0, "$"}}, gramatica, first));
    std::map<Estado, int> numeros;
    numeros.emplace(tabla.estados.front(), 0);

    // Transiciones: shift sobre terminales y GOTO sobre no terminales.
    for (int numero = 0; numero < static_cast<int>(tabla.estados.size()); ++numero) {
        std::set<std::string> simbolos;
        for (const auto& item : tabla.estados[numero]) {
            const auto& derecho = gramatica.producciones[item.produccion].derecho;
            if (item.punto < static_cast<int>(derecho.size())) simbolos.insert(derecho[item.punto]);
        }
        for (const auto& simbolo : simbolos) {
            auto siguiente = calcularGoto(tabla.estados[numero], simbolo, gramatica, first);
            auto encontrado = numeros.find(siguiente);
            int destino;
            if (encontrado == numeros.end()) {
                destino = static_cast<int>(tabla.estados.size());
                numeros.emplace(siguiente, destino);
                tabla.estados.push_back(siguiente);
            } else {
                destino = encontrado->second;
            }
            tabla.transiciones[{numero, simbolo}] = destino;
            if (gramatica.noTerminales.count(simbolo))
                tabla.tablaGoto[{numero, simbolo}] = destino;
            else
                tabla.tablaAccion[{numero, simbolo}].insert("shift " + std::to_string(destino));
        }
    }

    // Reducciones: [A > alfa ., a] reduce con A > alfa cuando llega 'a'.
    // [S' > S ., $] es el accept.
    for (int numero = 0; numero < static_cast<int>(tabla.estados.size()); ++numero)
        for (const auto& item : tabla.estados[numero]) {
            const auto& derecho = gramatica.producciones[item.produccion].derecho;
            if (item.punto != static_cast<int>(derecho.size())) continue;
            if (item.produccion == 0 && item.anticipacion == "$")
                tabla.tablaAccion[{numero, "$"}].insert("accept");
            else if (item.produccion != 0)
                tabla.tablaAccion[{numero, item.anticipacion}].insert("reduce " + std::to_string(item.produccion));
        }

    for (const auto& celda : tabla.tablaAccion)
        if (celda.second.size() > 1)
            tabla.conflictos.push_back("Conflicto en estado " + std::to_string(celda.first.first) +
                                       ", simbolo " + celda.first.second);
    tabla.cantidadEstados = static_cast<int>(tabla.estados.size());
    return tabla;
}

namespace {

std::string unirAcciones(const std::set<std::string>& acciones) {
    std::string texto;
    for (const auto& accion : acciones) {
        if (!texto.empty()) texto += " / ";
        texto += accion;
    }
    return texto;
}

} // namespace

void mostrarEstados(const Gramatica& gramatica, const TablaLR1& tabla, std::ostream& salida) {
    salida << "\nESTADOS LR(1)\n";
    for (int numero = 0; numero < static_cast<int>(tabla.estados.size()); ++numero) {
        salida << "I" << numero << ":\n";
        for (const auto& item : tabla.estados[numero]) {
            const auto& produccion = gramatica.producciones[item.produccion];
            salida << "  [" << produccion.izquierdo << " > ";
            for (int posicion = 0; posicion <= static_cast<int>(produccion.derecho.size()); ++posicion) {
                if (posicion == item.punto) salida << ". ";
                if (posicion < static_cast<int>(produccion.derecho.size())) salida << produccion.derecho[posicion] << ' ';
            }
            salida << ", " << item.anticipacion << "]\n";
        }
    }
}

void mostrarTabla(const Gramatica& gramatica, const TablaLR1& tabla, std::ostream& salida) {
    std::vector<std::string> terminales(gramatica.terminales.begin(), gramatica.terminales.end());
    terminales.push_back("$");
    std::vector<std::string> noTerminales;
    for (const auto& simbolo : gramatica.noTerminales)
        if (simbolo != gramatica.producciones.front().izquierdo) noTerminales.push_back(simbolo);

    // Ancho de columna: lo que ocupe la celda o el encabezado mas largo.
    std::size_t ancho = 12;
    for (const auto& celda : tabla.tablaAccion) ancho = std::max(ancho, unirAcciones(celda.second).size() + 2);
    for (const auto& simbolo : terminales) ancho = std::max(ancho, simbolo.size() + 2);
    for (const auto& simbolo : noTerminales) ancho = std::max(ancho, simbolo.size() + 2);

    salida << "\nTABLA LR(1): ACTION (terminales) | GOTO (no terminales)\n";
    salida << std::left << std::setw(8) << "Estado";
    for (const auto& simbolo : terminales) salida << std::setw(ancho) << simbolo;
    salida << "| ";
    for (const auto& simbolo : noTerminales) salida << std::setw(ancho) << simbolo;
    salida << '\n';
    for (int numero = 0; numero < static_cast<int>(tabla.estados.size()); ++numero) {
        salida << std::setw(8) << numero;
        for (const auto& simbolo : terminales) {
            auto celda = tabla.tablaAccion.find({numero, simbolo});
            salida << std::setw(ancho) << (celda == tabla.tablaAccion.end() ? "-" : unirAcciones(celda->second));
        }
        salida << "| ";
        for (const auto& simbolo : noTerminales) {
            auto celda = tabla.tablaGoto.find({numero, simbolo});
            salida << std::setw(ancho) << (celda == tabla.tablaGoto.end() ? "-" : std::to_string(celda->second));
        }
        salida << '\n';
    }

    if (tabla.conflictos.empty()) {
        salida << "\nSin conflictos: la gramatica es LR(1).\n";
    } else {
        salida << "\nLa gramatica tiene conflictos; no es LR(1).\n";
        for (const auto& conflicto : tabla.conflictos) salida << conflicto << '\n';
    }
}
