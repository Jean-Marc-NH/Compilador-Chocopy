#ifndef SCANNER_TOKENS_HPP
#define SCANNER_TOKENS_HPP

#include <string>


enum class TokenType {

    KW_CLASS, KW_DEF, KW_IF, KW_ELIF, KW_ELSE, KW_WHILE,
    KW_RETURN, KW_FOR, KW_IN, KW_IS, KW_AND, KW_OR,
    KW_NOT, KW_NONE, KW_TRUE, KW_FALSE, KW_PASS,

    KW_RESERVED,

    IDENTIFIER, // Nombres de variables
    INTEGER, // Numeros
    STRING, // Texto
    IDSTRING,

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

    NEWLINE,
    INDENT,
    DEDENT,

    END_OF_FILE,
    UNKNOWN_ERROR
};


struct Location {
    int line = 1;
    int column = 1;
};

class Token {
    public:
    TokenType type;
    std::string lexeme;
    Location location;
    void printToken(const Token& token);
    std::string getTokenString(const Token& token);
};




#endif