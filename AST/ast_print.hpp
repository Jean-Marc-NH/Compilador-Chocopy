#ifndef AST_AST_PRINT_HPP
#define AST_AST_PRINT_HPP

#include <iosfwd>
#include <string>
#include "ast.hpp"

// Volcado de texto indentado, un nodo por linea: "rol: Kind texto".
//
//   Program
//     def: VarDef
//       var: TypedVar x
//         type: Type int
//       value: IntLit 0
void imprimirAST(const Node& raiz, std::ostream& salida);
std::string astATexto(const Node& raiz);

// JSON para el visualizador de Python (Visualizador/ast_grafico.py):
//   {"archivo": "...", "ast": {"kind", "text", "line", "col", "children": [
//       {"role": "...", "kind": ...}, ...]}}
void escribirASTJson(const Node& raiz, const std::string& archivoFuente, std::ostream& salida);

#endif
