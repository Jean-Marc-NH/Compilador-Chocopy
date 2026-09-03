#ifndef SCANNER_TOKENS_HPP
#define SCANNER_TOKENS_HPP

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

    // Palabras reservadas de Python 3 que ChocoPy reserva por compatibilidad
    // (sec. 3.3) pero que no forman parte de su gramatica: as, assert,
    // async, await, break, continue, del, except, finally, from, global,
    // import, lambda, nonlocal, raise, try, with, yield. El scanner las
    // reconoce como palabra clave (nunca como IDENTIFIER); es el parser
    // quien decidira que su aparicion es un error sintactico.
    KW_RESERVED,

    // Literales e Identificadores
    IDENTIFIER, // Nombres de variables
    INTEGER, // Numeros
    STRING, // Texto
    // String cuyo contenido tiene sintaxis de identificador (sec. 3.4.1).
    // La gramatica de ChocoPy los usa en anotaciones de tipo (type ::= ID
    // | IDSTRING | [type]), asi que el scanner los distingue en el
    // momento de reconocer el literal, no el parser.
    IDSTRING,

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