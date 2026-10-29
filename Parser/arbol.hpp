#ifndef PARSER_ARBOL_HPP
#define PARSER_ARBOL_HPP

#include <memory>
#include <string>
#include <vector>
#include "../Scanner/tokens.hpp"

// Nodo del arbol de derivacion (parse tree) que arma el Parser mientras hace
// shift/reduce. Es generico: no sabe nada de ChocoPy, solo recuerda que
// produccion se redujo y sus hijos. AST/ast_builder lo convierte despues en
// el AST tipado.
struct NodoParse {
    std::string simbolo;   // terminal (ej. "IDENTIFIER") o no terminal (ej. "EXPR")
    int produccion = -1;   // indice en Gramatica::producciones; -1 en hojas y nodos de error
    bool esHoja = false;   // true: 'token' es el token que lo origino
    bool esError = false;  // true: lo inserto la recuperacion de errores (paso 3)
    Token token{};
    std::vector<std::unique_ptr<NodoParse>> hijos; // vacio en hojas y en producciones e
};

#endif
