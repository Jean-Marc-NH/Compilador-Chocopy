#include <filesystem>
#include <iostream>
#include "Tests/testTokens.hpp"
#include "Tests/testLexer.hpp"
#include "Tests/testAST.hpp"
#include "Tests/testErrores.hpp"

using namespace std;

int main() {
    // Los tests de lexer escriben archivos .py temporales en el directorio
    // actual; se corre dentro de build/tmp para no ensuciar la raiz del repo.
    // Los tests de parser necesitan la gramatica y la cache de la tabla, asi
    // que sus rutas se fijan antes de cambiar de directorio.
    const auto raiz = filesystem::current_path();
    const string gramatica = (raiz / "Parser/gramatica_chocopy.txt").string();
    const string cacheTabla = (raiz / "build/cache/gramatica_chocopy.lr1").string();
    filesystem::create_directories("build/tmp");
    filesystem::current_path("build/tmp");

    test_token_def();

    test_lexer_basico();
    test_lexer_indentacion();
    test_lexer_identificadores();
    test_lexer_enteros();
    test_lexer_strings();
    test_lexer_operadores();

    test_ast(gramatica, cacheTabla);
    test_errores(gramatica, cacheTabla);

    return 0;
}