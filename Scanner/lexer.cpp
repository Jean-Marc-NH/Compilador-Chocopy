#include "lexer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

Lexer::Lexer(const std::string& filename) {
    loadFile(filename);
}

void Lexer::loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "ERROR SCAN - No se pudo abrir el archivo: " << filename << "\n";
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string raw = buffer.str();

    // Normaliza3 las 3 formas de fin de linea (LF, CRLF, CR) a '\n' unico. (IA)
    source.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (c == '\r') {
            source.push_back('\n');
            if (i + 1 < raw.size() && raw[i + 1] == '\n') {
                ++i; // saltamos el '\n' que sigue al '\r' (forma CRLF)
            }
        } else {
            source.push_back(c);
        }
    }
}

bool Lexer::isAtEnd() const {
    return pos >= source.size();
}

char Lexer::peekchar(int offset) const {
    size_t idx = pos + static_cast<size_t>(offset);
    if (idx >= source.size()) return '\0';
    return source[idx];
}

char Lexer::getchar_() {
    if (isAtEnd()) return '\0';

    char c = source[pos];
    ++pos;

    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }

    return c;
}

void Lexer::reportError(const std::string& message, int atLine, int atColumn) {
    ++errors;
    std::cerr << "ERROR SCAN - " << message
              << " (" << atLine << ":" << atColumn << ")\n";
}

Token Lexer::gettoken() {
    // 1. Si hay tokens pendientes en la cola, los devolvemos antes de tocar el input.
    if (!pendingTokens.empty()) {
        Token t = pendingTokens.front();
        pendingTokens.pop_front();
        return t;
    }

    // 2. Fin de archivo sin logica de reconocimiento aun
    //    devolvemos endoffile una sola ves 
    //A partir del paso 3 aca
    //    tambien se debe vaciar la pila de indentacion antes de emitir este token.
    if (isAtEnd()) {
        if (!eofEmitted) {
            eofEmitted = true;
            return Token{TokenType::END_OF_FILE, "", Location{line, column}};
        }
    }

    // 3. Lo que falta
    //    - saltar espacios/tabs intra-linea
    //    - manejar comentarios (# hasta fin de linea)
    //    - manejar indentacion al inicio de linea logica
    //    - reconocer identificadores/keywords, numeros, strings
    //    - reconocer operadores y delimitadores
    //    temporal: consumimos un caracter y lo reportamos como error para dejar visible que falta implementar

    int startLine = line;
    int startColumn = column;
    char c = getchar_();
    if (c == '\0') {
        return Token{TokenType::END_OF_FILE, "", Location{startLine, startColumn}};
    }

    reportError(std::string("Caracter aun no manejado por el scanner: '") + c + "'",
                startLine, startColumn);
    return Token{TokenType::UNKNOWN_ERROR, std::string(1, c), Location{startLine, startColumn}};
}