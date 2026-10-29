#ifndef GENERADOR_HPP
#define GENERADOR_HPP

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Produccion {
    std::string izquierdo;
    std::vector<std::string> derecho;
};

struct Item {
    int produccion;
    int punto;
    std::string anticipacion;
    bool operator<(const Item& otro) const;
};

struct Gramatica {
    std::vector<Produccion> producciones; // El indice identifica cada reduce.
    std::multimap<std::string, int> reglas; // No terminal -> indices de sus reglas.
    std::set<std::string> noTerminales;
    std::set<std::string> terminales;
};

using Estado = std::set<Item>;
using ConjuntosFirst = std::map<std::string, std::set<std::string>>;
using ConjuntosFollow = std::map<std::string, std::set<std::string>>;

struct TablaLR1 {
    // 'estados' y 'transiciones' solo existen si la tabla se acaba de generar;
    // al cargarla de la cache solo se restauran action, tablaGoto, conflictos
    // y cantidadEstados (lo unico que necesita el parser).
    int cantidadEstados = 0;
    std::vector<Estado> estados;
    std::map<std::pair<int, std::string>, int> transiciones;
    std::map<std::pair<int, std::string>, std::set<std::string>> action;
    std::map<std::pair<int, std::string>, int> tablaGoto;
    std::vector<std::string> conflictos;
};

std::string leerArchivo(const std::string& ruta);
Gramatica parsearGramatica(const std::string& texto);
ConjuntosFirst calcularFirst(const Gramatica& gramatica);
std::set<std::string> firstSecuencia(const std::vector<std::string>& secuencia,
                                   const ConjuntosFirst& first);
// FOLLOW de cada no terminal; FOLLOW del simbolo inicial aumentado = {$}.
ConjuntosFollow calcularFollow(const Gramatica& gramatica, const ConjuntosFirst& first);
Estado cierre(const Estado& estado, const Gramatica& gramatica,
              const ConjuntosFirst& first);
// goto es palabra reservada en C++; por eso la funcion se llama calcularGoto.
Estado calcularGoto(const Estado& estado, const std::string& simbolo,
                    const Gramatica& gramatica, const ConjuntosFirst& first);
TablaLR1 generarTabla(const Gramatica& gramatica);

// --- Cache de la tabla en disco ---
// Generar la tabla de ChocoPy toma varios segundos, asi que se guarda en un
// archivo y solo se vuelve a generar cuando cambia el texto de la gramatica.
// La huella (hash del texto + version del formato) valida que la cache
// corresponda a la gramatica actual.
std::string huellaGramatica(const std::string& textoGramatica);
bool guardarTabla(const TablaLR1& tabla, const std::string& ruta, const std::string& huella);
// Devuelve false (sin tocar 'tabla') si el archivo no existe, esta corrupto
// o su huella no coincide.
bool cargarTabla(const std::string& ruta, const std::string& huella, TablaLR1& tabla);
// Carga la tabla de 'rutaCache' o, si no sirve, la genera y la guarda ahi.
// 'desdeCache' indica de donde salio. 'forzar' ignora la cache existente.
TablaLR1 obtenerTabla(const Gramatica& gramatica, const std::string& textoGramatica,
                      const std::string& rutaCache, bool& desdeCache, bool forzar = false);

void mostrarProducciones(const Gramatica& gramatica, std::ostream& salida);
void mostrarEstados(const Gramatica& gramatica, const TablaLR1& tabla,
                    std::ostream& salida);
void mostrarTabla(const Gramatica& gramatica, const TablaLR1& tabla,
                  std::ostream& salida);

#endif
