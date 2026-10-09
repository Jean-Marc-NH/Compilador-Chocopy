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

struct TablaLR1 {
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
Estado cierre(const Estado& estado, const Gramatica& gramatica,
              const ConjuntosFirst& first);
// goto es palabra reservada en C++; por eso la funcion se llama calcularGoto.
Estado calcularGoto(const Estado& estado, const std::string& simbolo,
                    const Gramatica& gramatica, const ConjuntosFirst& first);
TablaLR1 generarTabla(const Gramatica& gramatica);
void mostrarProducciones(const Gramatica& gramatica, std::ostream& salida);
void mostrarEstados(const Gramatica& gramatica, const TablaLR1& tabla,
                    std::ostream& salida);
void mostrarTabla(const Gramatica& gramatica, const TablaLR1& tabla,
                  std::ostream& salida);

#endif
