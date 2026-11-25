// correr_pruebas.cpp
//
// Corre todas las pruebas del proyecto (tokens, scanner, parser + AST,
// mensajes de error y analisis semantico). Hay que ejecutarlo desde la raiz
// del repositorio:
//
//   ./build/pruebas

#include <filesystem>
#include <iostream>
#include "prueba_ast.hpp"
#include "prueba_errores.hpp"
#include "prueba_lexer.hpp"
#include "prueba_semantica.hpp"
#include "prueba_tokens.hpp"

int main() {
    // Las pruebas del lexer escriben archivos .py temporales en el directorio
    // actual; se corren dentro de build/tmp para no ensuciar la raiz del repo.
    // Las pruebas del parser necesitan la gramatica y la cache de la tabla,
    // asi que sus rutas se fijan antes de cambiar de directorio.
    const auto raiz = std::filesystem::current_path();
    const std::string gramatica = (raiz / "Gramaticas/chocopy.txt").string();
    const std::string cacheTabla = (raiz / "build/cache/chocopy.lr1").string();
    std::filesystem::create_directories("build/tmp");
    std::filesystem::current_path("build/tmp");

    prueba_tokens();

    prueba_lexer_basico();
    prueba_lexer_indentacion();
    prueba_lexer_identificadores();
    prueba_lexer_enteros();
    prueba_lexer_cadenas();
    prueba_lexer_operadores();

    prueba_ast(gramatica, cacheTabla);
    prueba_errores(gramatica, cacheTabla);
    prueba_semantica(gramatica, cacheTabla);

    return 0;
}
