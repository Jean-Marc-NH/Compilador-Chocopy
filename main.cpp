#include <iostream>
#include "Tests/testTokens.hpp"
#include "Tests/testLexer.hpp"

using namespace std;

int main() {
    test_token_def();

    test_lexer_basico();
    test_lexer_indentacion();
    test_lexer_identificadores();
    test_lexer_enteros();
    test_lexer_strings();
    test_lexer_operadores();

    return 0;
}