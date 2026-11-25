#ifndef AST_IMPRESION_HPP
#define AST_IMPRESION_HPP

#include <iosfwd>
#include <string>
#include "nodos.hpp"

// Volcado de texto indentado, un nodo por linea: "rol: Clase texto".
//
//   Programa
//     def: DefVariable
//       var: VarTipada x
//         tipo: Tipo int
//       valor: LitEntero 0
//
// Con 'conDetalles' agrega al final de cada linea la linea del codigo y, en
// las expresiones ya analizadas, el tipo que les dio el analisis semantico:
//
//     sentencia: Asignacion  [linea 3]
//       objetivo: Identificador x  [linea 3, tipo int]
//       valor: OpBinaria +  [linea 3, tipo int]
void imprimirAST(const Nodo& raiz, std::ostream& salida, bool conDetalles = false);
std::string astATexto(const Nodo& raiz, bool conDetalles = false);

// JSON para el visualizador de Python (Visualizador/ast_grafico.py):
//   {"archivo": "...", "ast": {"clase", "texto", "linea", "col", "tipo", "hijos": [
//       {"rol": "...", "clase": ...}, ...]}}
// "tipo" es el tipo inferido ("" si el nodo no es una expresion).
void escribirASTJson(const Nodo& raiz, const std::string& archivoFuente, std::ostream& salida);

#endif
