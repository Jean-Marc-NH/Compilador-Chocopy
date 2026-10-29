#ifndef PARSER_MENSAJES_HPP
#define PARSER_MENSAJES_HPP

#include <set>
#include <string>
#include <vector>
#include "../Scanner/tokens.hpp"

// Traduce un error sintactico (el token que llego y los terminales que el
// parser habria aceptado) a un mensaje que entienda quien escribe el
// programa: "falta cerrar el '('", "')' de mas", "falta ':' al final de la
// linea"... sin hablar de tokens ni de estados del automata.
struct ExplicacionError {
    std::string mensaje;       // mensaje principal, apunta al token que llego
    int largo = 1;             // ancho del subrayado en el codigo
    bool hayNota = false;      // nota secundaria que apunta a otro lugar
    std::string notaMensaje;   // p. ej. "el '(' abierto aqui nunca se cierra"
    Location notaPos{};
    int notaLargo = 1;
};

// 'esperados' son los nombres de terminal de la gramatica que el parser
// acepta en este punto ("$" es el fin de archivo). 'anterior' es el ultimo
// token aceptado (nullptr si no hubo ninguno) y 'abiertos' los '(' y '['
// que siguen sin cerrar, en orden de apertura.
ExplicacionError explicarErrorSintactico(const Token& encontrado, const Token* anterior,
                                         const std::set<std::string>& esperados,
                                         const std::vector<Token>& abiertos);

// "el nombre 'x'", "el numero 3", "'+'", "el fin de la linea"...
std::string describirToken(const Token& token);

// Posicion justo despues de 'token' en su linea. Los mensajes de "falta algo
// al final de la linea" senalan ahi en vez de en el salto de linea, que queda
// lejos cuando la linea termina con un comentario.
Location posicionDespuesDe(const Token& token);

#endif
