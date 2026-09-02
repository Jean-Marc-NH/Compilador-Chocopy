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

void Lexer::skipComment() {
    // Se asume que peekchar() == '#'. Consumimos hasta el '\n' o EOF,
    // sin emitir ningun token: los comentarios no existen para el parser.
    while (!isAtEnd() && peekchar() != '\n') {
        getchar_();
    }
}

void Lexer::flushIndentToZeroAndEOF() {
    // Vaciamos la pila de indentacion (sec. 3.1.5): al llegar a EOF debe
    // generarse un DEDENT por cada nivel que quedo abierto por encima de 0.
    while (indentStack.size() > 1) {
        indentStack.pop_back();
        pendingTokens.push_back(Token{TokenType::DEDENT, "", Location{line, column}});
    }
    pendingTokens.push_back(Token{TokenType::END_OF_FILE, "", Location{line, column}});
}

void Lexer::handleLineStart() {
    // Procesa lineas fisicas una tras otra hasta encontrar contenido real
    // o el fin del archivo. Las lineas en blanco y las de solo-comentario
    // (3.1.4) no generan NEWLINE ni afectan la indentacion.
    for (;;) {
        int n = 0;

        // Medimos espacios de indentacion. Las tabulaciones se rechazan
        // explicitamente (politica elegida para este scanner, sec.
        // "Manejo de la indentacion" del enunciado): se reportan como
        // error y se descartan sin contar hacia el nivel n.
        for (;;) {
            char c = peekchar();
            if (c == ' ') {
                getchar_();
                ++n;
            } else if (c == '\t') {
                reportError("Tabulacion no permitida en la indentacion", line, column);
                getchar_();
            } else {
                break;
            }
        }

        if (isAtEnd()) {
            flushIndentToZeroAndEOF();
            return;
        }

        char c = peekchar();

        if (c == '\n') {
            // Linea en blanco: se ignora por completo.
            getchar_();
            continue;
        }

        if (c == '#') {
            skipComment();
            if (peekchar() == '\n') {
                getchar_();
            }
            // Si lo que sigue es EOF, la proxima vuelta del for(;;) lo detecta.
            continue;
        }

        // Contenido real: aca se decide si hay INDENT, DEDENT o nada.
        int top = indentStack.back();
        if (n > top) {
            indentStack.push_back(n);
            pendingTokens.push_back(Token{TokenType::INDENT, "", Location{line, column}});
        } else if (n < top) {
            while (indentStack.back() > n) {
                indentStack.pop_back();
                pendingTokens.push_back(Token{TokenType::DEDENT, "", Location{line, column}});
            }
            if (indentStack.back() != n) {
                reportError("Indentacion inconsistente: el nivel " + std::to_string(n) +
                                " no coincide con ningun nivel de indentacion abierto",
                            line, column);
                // Recuperacion: resincronizamos la pila a este nivel nuevo
                // para no arrastrar el mismo error en las lineas siguientes.
                indentStack.push_back(n);
            }
        }
        // n == top: incremento de indentacion nulo, no se emite nada.

        atLineStart = false;
        return;
    }
}


Token Lexer::gettoken() {
    for (;;) {
        // 1. Si hay tokens pendientes en la cola, los devolvemos antes de tocar el input.
        if (!pendingTokens.empty()) {
            Token t = pendingTokens.front();
            pendingTokens.pop_front();
            return t;
        }

        // 2. Inicio de linea logica: resolver indentacion / lineas en
        //    blanco / lineas de solo-comentario antes de seguir.
        if (atLineStart) {
            handleLineStart();
            continue; // vuelve a revisar la cola en el paso 1
        }

        // 3. Fin de archivo estando a mitad de una linea con contenido:
        //    el EOF actua como terminador implicito (sec. 3.1.1).
        if (isAtEnd()) {
            pendingTokens.push_back(Token{TokenType::NEWLINE, "", Location{line, column}});
            atLineStart = true; // la proxima vuelta disparara el flush de indentacion
            continue;
        }
        char c = peekchar();

        // 4. Espacios/tabs entre tokens (no al inicio de linea): se descartan.
        if (c == ' ' || c == '\t') {
            getchar_();
            continue;
        }

        // 5. Comentario despues de tokens reales en la misma linea.
        if (c == '#') {
            skipComment();
            continue;
        }

        // 6. Fin de la linea logica actual.
        if (c == '\n') {
            int startLine = line, startColumn = column;
            getchar_();
            atLineStart = true;
            return Token{TokenType::NEWLINE, "\n", Location{startLine, startColumn}};
        }

        // 7. Lo que falta: identificadores/keywords, numeros,
        //    strings, operadores y delimitadores. Placeholder temporal:
        //    consumimos un caracter y lo reportamos como no manejado.
        int startLine = line;
        int startColumn = column;
        char ch = getchar_();
        reportError(std::string("Caracter aun no manejado por el scanner: '") + ch + "'",
                    startLine, startColumn);
        return Token{TokenType::UNKNOWN_ERROR, std::string(1, ch), Location{startLine, startColumn}};
    }
}