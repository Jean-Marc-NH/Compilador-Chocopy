#ifndef AST_AST_BUILDER_HPP
#define AST_AST_BUILDER_HPP

#include <memory>
#include "../Parser/arbol.hpp"
#include "ast.hpp"

// Convierte el arbol de derivacion que arma el Parser (raiz = PROGRAMA) en el
// AST de ChocoPy. Las producciones unitarias (EXPR > ORE > ANDE > ...) se
// saltan, las listas recursivas (SENTLISTA, ARGLISTA, ...) se aplanan y los
// elif se anidan dentro del else del If anterior. Los parentesis y los
// tokens de puntuacion no generan nodos.
//
// Espera un arbol de un programa aceptado sin errores; lanza
// std::runtime_error si encuentra un nodo de error o una forma que la
// gramatica de Parser/gramatica_chocopy.txt no produce.
std::unique_ptr<Program> construirAST(const NodoParse& raiz);

#endif
