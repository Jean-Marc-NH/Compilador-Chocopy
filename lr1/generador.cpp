#include "generador.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <tuple>

bool Item::operator<(const Item& otro) const {
    return std::tie(produccion, punto, anticipacion) <
           std::tie(otro.produccion, otro.punto, otro.anticipacion);
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo) throw std::runtime_error("No se pudo abrir: " + ruta);
    std::ostringstream contenido;
    contenido << archivo.rdbuf();
    return contenido.str();
}

static std::string quitarEspacios(const std::string& texto) {
    std::string resultado;
    for (unsigned char caracter : texto)
        if (!std::isspace(caracter)) resultado += caracter;
    return resultado;
}

static bool esNoTerminal(const std::string& simbolo) {
    if (simbolo.empty() || simbolo.front() < 'A' || simbolo.front() > 'Z')
        return false;
    for (char caracter : simbolo)
        if ((caracter < 'A' || caracter > 'Z') && caracter != '\'') return false;
    return true;
}

static std::vector<std::string> separarSimbolos(const std::string& texto,
                                              const Gramatica& gramatica) {
    if (texto == "e") return {}; // Vacio: no contiene un simbolo e.
    std::vector<std::string> simbolos;
    for (std::size_t posicion = 0; posicion < texto.size();) {
        char caracter = texto[posicion];
        if (caracter >= 'A' && caracter <= 'Z') {
            std::string elegido;
            for (const auto& nombre : gramatica.noTerminales)
                if (texto.compare(posicion, nombre.size(), nombre) == 0 &&
                    nombre.size() > elegido.size()) elegido = nombre;
            if (elegido.empty())
                throw std::runtime_error("No terminal sin produccion en: " + texto);
            simbolos.push_back(elegido);
            posicion += elegido.size();
        } else {
            if (caracter == 'e' || caracter == '$' || caracter == '\'' || caracter == '>')
                throw std::runtime_error("Simbolo reservado o invalido en: " + texto);
            simbolos.push_back(std::string(1, caracter));
            ++posicion;
        }
    }
    return simbolos;
}

Gramatica parsearGramatica(const std::string& texto) {
    Gramatica gramatica;
    std::vector<std::pair<std::string, std::string>> lineas;
    std::istringstream entrada(texto);
    std::string linea;
    int numero = 0;
    while (std::getline(entrada, linea)) {
        ++numero;
        auto comentario = linea.find('#');
        if (comentario != std::string::npos) linea.erase(comentario);
        if (quitarEspacios(linea).empty()) continue;
        auto separador = linea.find('>');
        if (separador == std::string::npos || linea.find('>', separador + 1) != std::string::npos)
            throw std::runtime_error("Se esperaba un > en la linea " + std::to_string(numero));
        std::string izquierdo = quitarEspacios(linea.substr(0, separador));
        if (!esNoTerminal(izquierdo))
            throw std::runtime_error("Lado izquierdo invalido en linea " + std::to_string(numero));
        gramatica.noTerminales.insert(izquierdo);
        lineas.push_back({izquierdo, linea.substr(separador + 1)});
    }
    if (lineas.empty()) throw std::runtime_error("La gramatica esta vacia.");
    for (const auto& fila : lineas) {
        std::string derecho = fila.second;
        // Espacios y | separan alternativas, como en el ejemplo del trabajo.
        // Con | se exigen alternativas no vacias; el vacio se escribe e.
        if (derecho.find('|') != std::string::npos) {
            std::size_t inicio = 0;
            do {
                auto fin = derecho.find('|', inicio);
                if (quitarEspacios(derecho.substr(inicio, fin - inicio)).empty())
                    throw std::runtime_error("Alternativa vacia: escribe e.");
                if (fin == std::string::npos) break;
                inicio = fin + 1;
            } while (true);
        }
        std::replace(derecho.begin(), derecho.end(), '|', ' ');
        std::istringstream alternativas(derecho);
        std::string alternativa;
        bool tieneRegla = false;
        while (alternativas >> alternativa) {
            auto simbolos = separarSimbolos(alternativa, gramatica);
            gramatica.producciones.push_back({fila.first, simbolos});
            tieneRegla = true;
            for (const auto& simbolo : simbolos)
                if (!gramatica.noTerminales.count(simbolo)) gramatica.terminales.insert(simbolo);
        }
        if (!tieneRegla) throw std::runtime_error("Falta el lado derecho de " + fila.first);
    }
    const auto primera = gramatica.producciones.front();
    int cantidadInicial = 0;
    bool apareceDerecha = false;
    for (const auto& produccion : gramatica.producciones) {
        if (produccion.izquierdo == primera.izquierdo) ++cantidadInicial;
        for (const auto& simbolo : produccion.derecho)
            if (simbolo == primera.izquierdo) apareceDerecha = true;
    }
    bool yaAumentada = primera.izquierdo.back() == '\'' && cantidadInicial == 1 &&
                      !apareceDerecha && primera.derecho.size() == 1 &&
                      gramatica.noTerminales.count(primera.derecho.front());
    if (!yaAumentada) {
        std::string nuevoInicio = primera.izquierdo + "'";
        while (gramatica.noTerminales.count(nuevoInicio)) nuevoInicio += "'";
        gramatica.noTerminales.insert(nuevoInicio);
        gramatica.producciones.insert(gramatica.producciones.begin(),
                                     {nuevoInicio, {primera.izquierdo}});
    }
    for (int indice = 0; indice < static_cast<int>(gramatica.producciones.size()); ++indice)
        gramatica.reglas.emplace(gramatica.producciones[indice].izquierdo, indice);
    return gramatica;
}

std::set<std::string> firstSecuencia(const std::vector<std::string>& secuencia,
                                   const ConjuntosFirst& first) {
    std::set<std::string> resultado;
    bool todosVacios = true;
    for (const auto& simbolo : secuencia) {
        const auto& conjunto = first.at(simbolo);
        for (const auto& terminal : conjunto)
            if (terminal != "e") resultado.insert(terminal);
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

Estado cierre(const Estado& estado, const Gramatica& gramatica,
              const ConjuntosFirst& first) {
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
    tabla.estados.push_back(cierre(Estado{{0, 0, "$"}}, gramatica, first));
    std::map<Estado, int> numeros;
    numeros.emplace(tabla.estados.front(), 0);
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
            } else destino = encontrado->second;
            tabla.transiciones[{numero, simbolo}] = destino;
            if (gramatica.noTerminales.count(simbolo)) tabla.tablaGoto[{numero, simbolo}] = destino;
            else tabla.action[{numero, simbolo}].insert("shift " + std::to_string(destino));
        }
    }
    for (int numero = 0; numero < static_cast<int>(tabla.estados.size()); ++numero)
        for (const auto& item : tabla.estados[numero]) {
            const auto& derecho = gramatica.producciones[item.produccion].derecho;
            if (item.punto != static_cast<int>(derecho.size())) continue;
            if (item.produccion == 0 && item.anticipacion == "$")
                tabla.action[{numero, "$"}].insert("accept");
            else if (item.produccion != 0)
                tabla.action[{numero, item.anticipacion}].insert("reduce " + std::to_string(item.produccion));
        }
    for (const auto& celda : tabla.action)
        if (celda.second.size() > 1)
            tabla.conflictos.push_back("Conflicto en estado " + std::to_string(celda.first.first) +
                                      ", simbolo " + celda.first.second);
    return tabla;
}

static std::string unirAcciones(const std::set<std::string>& acciones) {
    std::string texto;
    for (const auto& accion : acciones) {
        if (!texto.empty()) texto += " / ";
        texto += accion;
    }
    return texto;
}

void mostrarProducciones(const Gramatica& gramatica, std::ostream& salida) {
    salida << "PRODUCCIONES (0 es la regla aumentada)\n";
    for (int indice = 0; indice < static_cast<int>(gramatica.producciones.size()); ++indice) {
        const auto& produccion = gramatica.producciones[indice];
        salida << indice << ": " << produccion.izquierdo << " > ";
        if (produccion.derecho.empty()) salida << "e";
        for (const auto& simbolo : produccion.derecho) salida << simbolo << ' ';
        salida << '\n';
    }
}

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
    std::size_t ancho = 12;
    for (const auto& celda : tabla.action) ancho = std::max(ancho, unirAcciones(celda.second).size() + 2);
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
            auto celda = tabla.action.find({numero, simbolo});
            salida << std::setw(ancho) << (celda == tabla.action.end() ? "-" : unirAcciones(celda->second));
        }
        salida << "| ";
        for (const auto& simbolo : noTerminales) {
            auto celda = tabla.tablaGoto.find({numero, simbolo});
            salida << std::setw(ancho) << (celda == tabla.tablaGoto.end() ? "-" : std::to_string(celda->second));
        }
        salida << '\n';
    }
    if (tabla.conflictos.empty()) salida << "\nSin conflictos: la gramatica es LR(1).\n";
    else {
        salida << "\nLa gramatica tiene conflictos; no es LR(1).\n";
        for (const auto& conflicto : tabla.conflictos) salida << conflicto << '\n';
    }
}
