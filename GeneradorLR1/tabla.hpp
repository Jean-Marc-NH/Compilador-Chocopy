#ifndef GENERADORLR1_TABLA_HPP
#define GENERADORLR1_TABLA_HPP

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "conjuntos.hpp"
#include "gramatica.hpp"

// Construccion de la coleccion canonica de items LR(1) y de las tablas
// ACTION y GOTO (algoritmo del libro del dragon, sec. 4.7).

// Item LR(1): [A > alfa . beta, a]
struct Item {
    int produccion;           // indice en Gramatica::producciones
    int punto;                // cuantos simbolos del lado derecho ya se vieron
    std::string anticipacion; // el terminal "a" (lookahead)
    bool operator<(const Item& otro) const;
};

using Estado = std::set<Item>;

struct TablaLR1 {
    // 'estados' y 'transiciones' solo existen si la tabla se acaba de generar;
    // al cargarla de la cache solo se restauran tablaAccion, tablaGoto,
    // conflictos y cantidadEstados (lo unico que necesita el parser).
    int cantidadEstados = 0;
    std::vector<Estado> estados;
    std::map<std::pair<int, std::string>, int> transiciones;

    // ACTION[estado, terminal]: texto "shift N", "reduce N" o "accept". Es un
    // conjunto para poder detectar conflictos (mas de una accion en la celda).
    std::map<std::pair<int, std::string>, std::set<std::string>> tablaAccion;
    // GOTO[estado, no terminal] = estado destino.
    std::map<std::pair<int, std::string>, int> tablaGoto;

    std::vector<std::string> conflictos; // vacio si la gramatica es LR(1)
};

// cierre(I): agrega [B > . gamma, b] por cada [A > alfa . B beta, a] de I y
// cada b en FIRST(beta a), hasta que no se agregue nada.
Estado cierre(const Estado& estado, const Gramatica& gramatica, const ConjuntosFirst& first);

// goto(I, X). goto es palabra reservada en C++; por eso se llama calcularGoto.
Estado calcularGoto(const Estado& estado, const std::string& simbolo,
                    const Gramatica& gramatica, const ConjuntosFirst& first);

// Genera todos los estados y llena ACTION y GOTO. Los conflictos no detienen
// la construccion: se anotan en 'conflictos'.
TablaLR1 generarTabla(const Gramatica& gramatica);

// Impresion para la herramienta lr1 (Herramientas/lr1.cpp).
void mostrarEstados(const Gramatica& gramatica, const TablaLR1& tabla, std::ostream& salida);
void mostrarTabla(const Gramatica& gramatica, const TablaLR1& tabla, std::ostream& salida);

#endif
