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
void imprimirAST(const Nodo& raiz, std::ostream& salida);
std::string astATexto(const Nodo& raiz);

// JSON para el visualizador de Python (Visualizador/ast_grafico.py):
//   {"archivo": "...", "ast": {"clase", "texto", "linea", "col", "hijos": [
//       {"rol": "...", "clase": ...}, ...]}}
void escribirASTJson(const Nodo& raiz, const std::string& archivoFuente, std::ostream& salida);

#endif
