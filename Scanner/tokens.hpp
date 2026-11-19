#ifndef SCANNER_TOKENS_HPP
#define SCANNER_TOKENS_HPP

#include <string>

// Todo esta en el manual del lenguaje https://chocopy.org/chocopy_language_reference.pdf en la seccion 3
// (copia en Documentacion/). La seccion 3 "Lexical Structure" se divide en:
/*
    Estructuras de linea (control de estructura) -> NEWLINE, INDENT y DEDENT
    Palabras clave -> palabras reservadas como else, if, etc
    Operadores -> todos los operadores admitidos (20) + - //, etc
    Literales -> tipos de datos (3): texto, numero y variables
*/
//
// Los nombres de los tipos de token (KW_IF, IDENTIFIER, ...) se dejan como
// en el enunciado porque son tambien los terminales de la gramatica
// (Gramaticas/chocopy.txt los escribe entre llaves, por ejemplo {KW_IF}).
// Si se cambia un nombre aca hay que cambiarlo tambien en la gramatica.

enum class TipoToken {

    // Palabras clave (colocamos KW para evitar conflictos con el lenguaje)
    KW_CLASS, KW_DEF, KW_IF, KW_ELIF, KW_ELSE, KW_WHILE,
    KW_RETURN, KW_FOR, KW_IN, KW_IS, KW_AND, KW_OR,
    KW_NOT, KW_NONE, KW_TRUE, KW_FALSE, KW_PASS,

    // global y nonlocal si forman parte de la gramatica (global_decl y
    // nonlocal_decl dentro de func_body, sec. 4), asi que tienen su propio token.
    KW_GLOBAL, KW_NONLOCAL,

    // Palabras reservadas de Python 3 que ChocoPy reserva por compatibilidad
    // (sec. 3.3) pero que no forman parte de su gramatica: as, assert,
    // async, await, break, continue, del, except, finally, from,
    // import, lambda, raise, try, with, yield. El scanner las
    // reconoce como palabra clave (nunca como IDENTIFIER); es el parser
    // quien decide que su aparicion es un error sintactico.
    KW_RESERVED,

    // Literales e identificadores
    IDENTIFIER, // nombres de variables
    INTEGER,    // numeros
    STRING,     // texto
    // String cuyo contenido tiene sintaxis de identificador (sec. 3.4.1).
    // La gramatica de ChocoPy los usa en anotaciones de tipo (type ::= ID
    // | IDSTRING | [type]), asi que el scanner los distingue en el
    // momento de reconocer el literal, no el parser.
    IDSTRING,

    // Operadores y puntuacion
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

    // Control de estructura
    NEWLINE,
    INDENT,
    DEDENT,

    // Tokens especiales
    END_OF_FILE,
    UNKNOWN_ERROR
};

// Linea y columna donde empieza un token (las dos empiezan en 1).
struct Posicion {
    int linea = 1;
    int columna = 1;
};

struct Token {
    TipoToken tipo;
    std::string lexema;
    Posicion posicion;
};

// Para depurar :C
// tipoTokenATexto(KW_DEF) -> "KW_DEF"
// tokenATexto(...)        -> Token(KW_DEF, "def", L1:C1)
std::string tipoTokenATexto(TipoToken tipo);
std::string tokenATexto(const Token& token);

#endif
