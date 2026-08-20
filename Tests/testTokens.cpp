#include <iostream>
#include <vector>
#include "Scanner/tokens.hpp"
#include "testTokens.hpp"

void test_token_def()
{
    // Simular def sumar(a: int) -> int:\n    return a + 1
    // Lo que esta en {} es la fila, columna donde inicia la declaracion

    std::vector<Token> Tokens = {
        {TokenType::KW_DEF, "def", {1, 1}},
        {TokenType::IDENTIFIER, "sumar", {1, 5}},
        {TokenType::LPAREN, "(", {1, 10}},
        {TokenType::IDENTIFIER, "a", {1, 11}},
        {TokenType::COLON, ":", {1, 12}},
        {TokenType::IDENTIFIER, "int", {1, 14}},
        {TokenType::RPAREN, ")", {1, 17}},
        {TokenType::ARROW, "->", {1, 19}},
        {TokenType::IDENTIFIER, "int", {1, 22}},
        {TokenType::COLON, ":", {1, 25}},
        {TokenType::NEWLINE, "\n", {1, 26}},
        {TokenType::INDENT, "    ", {2, 1}},
        {TokenType::KW_RETURN, "return", {2, 5}},
        {TokenType::IDENTIFIER, "a", {2, 12}},
        {TokenType::PLUS, "+", {2, 14}},
        {TokenType::INTEGER, "1", {2, 16}},
        {TokenType::NEWLINE, "\n", {2, 17}},
        {TokenType::DEDENT, "", {3, 1}},
        {TokenType::END_OF_FILE, "", {3, 1}}
    };

    std::cout << "PRUEBA DE REPRESENTACIÓN DE TOKENS\n\n";
    for (const auto& token : Tokens) {
        std::cout << tokenToString(token) << "\n";
    }
}