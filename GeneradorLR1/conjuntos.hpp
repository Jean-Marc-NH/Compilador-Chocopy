#ifndef GENERADORLR1_CONJUNTOS_HPP
#define GENERADORLR1_CONJUNTOS_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include "gramatica.hpp"

// Conjuntos FIRST y FOLLOW. Se calculan por punto fijo: se recorren todas las
// producciones una y otra vez hasta que ningun conjunto cambie.
// "e" representa la cadena vacia y "$" el fin de la entrada.

using ConjuntosFirst = std::map<std::string, std::set<std::string>>;
using ConjuntosFollow = std::map<std::string, std::set<std::string>>;

// FIRST de cada simbolo (los terminales y "$" tienen FIRST = {ellos mismos}).
ConjuntosFirst calcularFirst(const Gramatica& gramatica);

// FIRST de una secuencia de simbolos: contiene "e" solo si todos los
// simbolos pueden ser vacios (o si la secuencia esta vacia).
std::set<std::string> firstSecuencia(const std::vector<std::string>& secuencia,
                                     const ConjuntosFirst& first);

// FOLLOW de cada no terminal; FOLLOW del simbolo inicial aumentado = {$}.
// Lo usa el parser para la recuperacion de errores en modo panico.
ConjuntosFollow calcularFollow(const Gramatica& gramatica, const ConjuntosFirst& first);

#endif
