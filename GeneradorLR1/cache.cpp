#include "cache.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {

// Subir este numero cuando cambie el algoritmo de generacion o el formato del
// archivo, para invalidar las caches viejas.
const char* const VERSION_CACHE = "LR1CACHE 1";

} // namespace

std::string huellaGramatica(const std::string& textoGramatica) {
    // FNV-1a de 64 bits: suficiente para detectar que la gramatica cambio.
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char caracter : textoGramatica) {
        hash ^= caracter;
        hash *= 1099511628211ULL;
    }
    std::ostringstream hex;
    hex << std::hex << std::setw(16) << std::setfill('0') << hash;
    return std::string(VERSION_CACHE) + " " + hex.str();
}

// Formato (texto, una celda por linea):
//   <huella>
//   E <cantidadEstados>
//   A <estado> <terminal> <accion con espacios, ej. "shift 3">
//   G <estado> <no terminal> <destino>
//   C <texto del conflicto>
//   FIN
// El FIN final sirve para detectar archivos truncados.
bool guardarTabla(const TablaLR1& tabla, const std::string& ruta, const std::string& huella) {
    std::filesystem::path destino(ruta);
    if (destino.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(destino.parent_path(), error);
    }
    std::ofstream salida(ruta, std::ios::trunc);
    if (!salida) return false;
    salida << huella << "\n";
    salida << "E " << tabla.cantidadEstados << "\n";
    for (const auto& celda : tabla.tablaAccion)
        for (const auto& accion : celda.second)
            salida << "A " << celda.first.first << ' ' << celda.first.second << ' ' << accion << "\n";
    for (const auto& celda : tabla.tablaGoto)
        salida << "G " << celda.first.first << ' ' << celda.first.second << ' ' << celda.second << "\n";
    for (const auto& conflicto : tabla.conflictos) salida << "C " << conflicto << "\n";
    salida << "FIN\n";
    return static_cast<bool>(salida);
}

bool cargarTabla(const std::string& ruta, const std::string& huella, TablaLR1& tabla) {
    std::ifstream entrada(ruta);
    if (!entrada) return false;
    std::string linea;
    if (!std::getline(entrada, linea) || linea != huella) return false;

    // Se lee en una tabla aparte para no dejar 'tabla' a medias si el
    // archivo esta danado.
    TablaLR1 leida;
    bool completa = false;
    while (std::getline(entrada, linea)) {
        if (linea == "FIN") {
            completa = true;
            break;
        }
        std::istringstream campos(linea);
        std::string tipo;
        campos >> tipo;
        if (tipo == "E") {
            if (!(campos >> leida.cantidadEstados)) return false;
        } else if (tipo == "A") {
            int estado;
            std::string simbolo;
            if (!(campos >> estado >> simbolo)) return false;
            std::string accion;
            std::getline(campos >> std::ws, accion);
            if (accion.empty()) return false;
            leida.tablaAccion[{estado, simbolo}].insert(accion);
        } else if (tipo == "G") {
            int estado, destino;
            std::string simbolo;
            if (!(campos >> estado >> simbolo >> destino)) return false;
            leida.tablaGoto[{estado, simbolo}] = destino;
        } else if (tipo == "C") {
            std::string conflicto;
            std::getline(campos >> std::ws, conflicto);
            leida.conflictos.push_back(conflicto);
        } else {
            return false;
        }
    }
    if (!completa) return false;
    tabla = std::move(leida);
    return true;
}

TablaLR1 obtenerTabla(const Gramatica& gramatica, const std::string& textoGramatica,
                      const std::string& rutaCache, bool& desdeCache, bool forzar) {
    const std::string huella = huellaGramatica(textoGramatica);
    TablaLR1 tabla;
    if (!forzar && cargarTabla(rutaCache, huella, tabla)) {
        desdeCache = true;
        return tabla;
    }
    desdeCache = false;
    tabla = generarTabla(gramatica);
    guardarTabla(tabla, rutaCache, huella); // si no se puede escribir, solo se pierde la cache
    return tabla;
}
