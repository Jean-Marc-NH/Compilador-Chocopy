#include <iostream>
#include <vector>
#include "Scanner/tokens.hpp"
#include "prueba_tokens.hpp"

void prueba_tokens()
{
    // Simular def sumar(a: int) -> int:\n    return a + 1
    // Lo que esta en {} es la fila, columna donde inicia cada token

    std::vector<Token> tokens = {
        {TipoToken::KW_DEF, "def", {1, 1}},
        {TipoToken::IDENTIFIER, "sumar", {1, 5}},
        {TipoToken::LPAREN, "(", {1, 10}},
        {TipoToken::IDENTIFIER, "a", {1, 11}},
        {TipoToken::COLON, ":", {1, 12}},
        {TipoToken::IDENTIFIER, "int", {1, 14}},
        {TipoToken::RPAREN, ")", {1, 17}},
        {TipoToken::ARROW, "->", {1, 19}},
        {TipoToken::IDENTIFIER, "int", {1, 22}},
        {TipoToken::COLON, ":", {1, 25}},
        {TipoToken::NEWLINE, "\n", {1, 26}},
        {TipoToken::INDENT, "    ", {2, 1}},
        {TipoToken::KW_RETURN, "return", {2, 5}},
        {TipoToken::IDENTIFIER, "a", {2, 12}},
        {TipoToken::PLUS, "+", {2, 14}},
        {TipoToken::INTEGER, "1", {2, 16}},
        {TipoToken::NEWLINE, "\n", {2, 17}},
        {TipoToken::DEDENT, "", {3, 1}},
        {TipoToken::END_OF_FILE, "", {3, 1}}
    };

    std::cout << "PRUEBA DE REPRESENTACION DE TOKENS\n\n";
    for (const auto& token : tokens) {
        std::cout << tokenATexto(token) << "\n";
    }
}
