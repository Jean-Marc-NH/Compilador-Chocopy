#ifndef SCANNER_LEXER_HPP
#define SCANNER_LEXER_HPP

#include <string>
#include <deque>
#include <vector>
#include "tokens.hpp"

//- Leer el archivo fuente completo a memoria.
//- getchar_() y peekchar()


class Lexer {
public:
    explicit Lexer(const std::string& filename);

    Token gettoken();

    // Cantidad de errores detectados durante el escaneo
    int errorCount() const { return errors; }

private:
    std::string source;   // contenido completo del archivo
    // indice del proximo caracter a leer
    size_t pos = 0;       
    int line = 1;
    int column = 1;

    //Cola de tokens que no consumen caracteres del input
    std::deque<Token> pendingTokens;

    std::vector<int> indentStack{0}; // pila inicializada con el nivel 0
    bool atLineStart = true;         // true si el proximo caracter a leer inicia una linea logica
    bool eofEmitted = false;         // true una vez que ya se devolvio END_OF_FILE

    int errors = 0;

    //Lectura de caracteres
    // Devuelve el siguiente caracter y avanza el puntero. Devuelve \0 si ya no hay mas
    char getchar_();

    // Devuelve el caracter que esta offset posiciones adelante del puntero

    char peekchar(int offset = 0) const;//mira el proximo caracter a leer.

    bool isAtEnd() const;

    void loadFile(const std::string& filename);

    void reportError(const std::string& message, int atLine, int atColumn);
};

#endif