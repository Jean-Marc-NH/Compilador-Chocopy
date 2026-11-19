#include "gramatica.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace {

std::string quitarEspacios(const std::string& texto) {
    std::string resultado;
    for (unsigned char caracter : texto)
        if (!std::isspace(caracter)) resultado += caracter;
    return resultado;
}

// Un no terminal son solo mayusculas, opcionalmente con ' (S', S'').
bool esNoTerminal(const std::string& simbolo) {
    if (simbolo.empty() || simbolo.front() < 'A' || simbolo.front() > 'Z')
        return false;
    for (char caracter : simbolo)
        if ((caracter < 'A' || caracter > 'Z') && caracter != '\'') return false;
    return true;
}

// Separa una alternativa en simbolos. Como van pegados, para los no
// terminales se elige el nombre mas largo que coincida (ARGLISTA antes que
// ARG), por eso hace falta conocer antes todos los lados izquierdos.
std::vector<std::string> separarSimbolos(const std::string& texto, const Gramatica& gramatica) {
    if (texto == "e") return {}; // vacio: no contiene un simbolo e
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
        } else if (caracter == '{') {
            // Terminal con nombre largo, p. ej. {KW_IF}: es el mismo texto que
            // devuelve tipoTokenATexto() para el token del scanner.
            auto cierre = texto.find('}', posicion);
            if (cierre == std::string::npos || cierre == posicion + 1)
                throw std::runtime_error("Terminal sin cerrar o vacio en: " + texto);
            simbolos.push_back(texto.substr(posicion + 1, cierre - posicion - 1));
            posicion = cierre + 1;
        } else {
            if (caracter == 'e' || caracter == '$' || caracter == '\'' || caracter == '>')
                throw std::runtime_error("Simbolo reservado o invalido en: " + texto);
            simbolos.push_back(std::string(1, caracter));
            ++posicion;
        }
    }
    return simbolos;
}

} // namespace

std::string leerArchivo(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo) throw std::runtime_error("No se pudo abrir: " + ruta);
    std::ostringstream contenido;
    contenido << archivo.rdbuf();
    return contenido.str();
}

Gramatica parsearGramatica(const std::string& texto) {
    Gramatica gramatica;

    // Primera pasada: juntamos los lados izquierdos, que hacen falta para
    // poder separar los simbolos pegados de cada alternativa.
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

    // Segunda pasada: cada alternativa es una produccion.
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

    // Gramatica aumentada: si la primera regla ya es S' > S (unica, con S' que
    // no aparece a la derecha) se deja; si no, se agrega una nueva al inicio.
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
