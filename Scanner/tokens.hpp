#ifndef SCANNER_TOKENS_HPP
#define SCANNER_TOKENS_HPP
// añadi define por error de redefinition of struct
#include <string>

// Todo esta en el manual del lenguaje https://chocopy.org/chocopy_language_reference.pdf en la seccion 3
// Seccion 3 Lexical Estructure se divide en
/*
    Estructuras de linea (Control de estructura) NewLine, Ident y Dedent
    KeyWords -> Palabras reservadas como else, if, etc
    Operadores -> todos los operadores admitidos (20) + - //, etc
    Literales -> tipo de datos (3) texto numero y variables


*/

enum class TokenType {

    // Palabras clave (Colocamos KW para evitar conflictos de lenguaje)
    KW_CLASS, KW_DEF, KW_IF, KW_ELIF, KW_ELSE, KW_WHILE,
    KW_RETURN, KW_FOR, KW_IN, KW_IS, KW_AND, KW_OR,
    KW_NOT, KW_NONE, KW_TRUE, KW_FALSE, KW_PASS,

    // Literales e Identificadores
    IDENTIFIER, // Nombres de variables
    INTEGER, // Numeros
    STRING, // Texto

    // Operadores y Puntuacion
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH_SLASH,  // // 
    PERCENT,      // %
    EQ_EQ,        // ==
    BANG_EQ,      // !=
    LESS_EQ,      // <=
    GREATER_EQ,   // >=
    LESS,         // <
    GREATER,      // >
    EQUAL,        // =
    COLON,        // :
    DOT,          // .
    COMMA,        // ,
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    ARROW,        // ->

    // Control de Estructura
    NEWLINE,
    INDENT,
    DEDENT,

    // Tokens Especiales
    END_OF_FILE,
    UNKNOWN_ERROR
};

struct Location {
    int line = 1;
    int column = 1;
};

struct Token {
    TokenType type;
    std::string lexeme;
    Location location;
};

// Debug :C
std::string tokenTypeToString(TokenType type);
std::string tokenToString(const Token& token);

#endif