#include "lexer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cctype>

namespace {

// Un identificador empieza con letra o '_' (sec. 3.2 del manual).
bool isIdentStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// El resto del identificador acepta ademas digitos.
bool isIdentPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isDigitChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

// Un contenido de string "tiene sintaxis de identificador" (sec. 3.4.1,
// usado para decidir IDSTRING vs STRING) si, ignorando las comillas,
// cumple la misma regla lexica que un IDENTIFIER: letra o '_' seguido de
// letras/digitos/'_'.
bool looksLikeIdentifierContent(const std::string& s) {
    if (s.empty() || !isIdentStart(s[0])) return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!isIdentPart(s[i])) return false;
    }
    return true;
}

// El manual pide "verificar el tamano maximo permitido" pero no fija un
// numero en el fragmento con el que contamos; elegimos un limite propio
// y lo documentamos aca para que sea facil de ajustar si la catedra da
// un valor especifico.
const size_t MAX_STRING_LEN = 4096;

// Maximo valor de un entero de 32 bits con signo (sec. 2.5.1), como
// string, para poder comparar lexemas arbitrariamente largos sin
// desbordar ningun tipo entero de C++.
const std::string INT32_MAX_STR = "2147483647";

// Tabla precargada de palabras clave (estrategia "preload the symbol
// table", ver seccion "Two ways to handle keywords" de las diapositivas):
// evita tener un estado de DFA por cada keyword y es la forma mas simple
// de mantener el listado completo de las 35 palabras reservadas de Python
// (sec. 3.3), aunque varias de ellas no formen parte de la gramatica de
// ChocoPy (esas caen en KW_RESERVED).
const std::unordered_map<std::string, TokenType>& keywordTable() {
    static const std::unordered_map<std::string, TokenType> table = {
        {"False",    TokenType::KW_FALSE},
        {"None",     TokenType::KW_NONE},
        {"True",     TokenType::KW_TRUE},
        {"and",      TokenType::KW_AND},
        {"as",       TokenType::KW_RESERVED},
        {"assert",   TokenType::KW_RESERVED},
        {"async",    TokenType::KW_RESERVED},
        {"await",    TokenType::KW_RESERVED},
        {"break",    TokenType::KW_RESERVED},
        {"class",    TokenType::KW_CLASS},
        {"continue", TokenType::KW_RESERVED},
        {"def",      TokenType::KW_DEF},
        {"del",      TokenType::KW_RESERVED},
        {"elif",     TokenType::KW_ELIF},
        {"else",     TokenType::KW_ELSE},
        {"except",   TokenType::KW_RESERVED},
        {"finally",  TokenType::KW_RESERVED},
        {"for",      TokenType::KW_FOR},
        {"from",     TokenType::KW_RESERVED},
        {"global",   TokenType::KW_RESERVED},
        {"if",       TokenType::KW_IF},
        {"import",   TokenType::KW_RESERVED},
        {"in",       TokenType::KW_IN},
        {"is",       TokenType::KW_IS},
        {"lambda",   TokenType::KW_RESERVED},
        {"nonlocal", TokenType::KW_RESERVED},
        {"not",      TokenType::KW_NOT},
        {"or",       TokenType::KW_OR},
        {"pass",     TokenType::KW_PASS},
        {"raise",    TokenType::KW_RESERVED},
        {"return",   TokenType::KW_RETURN},
        {"try",      TokenType::KW_RESERVED},
        {"while",    TokenType::KW_WHILE},
        {"with",     TokenType::KW_RESERVED},
        {"yield",    TokenType::KW_RESERVED},
    };
    return table;
}

} // namespace

Lexer::Lexer(const std::string& filename) {
    loadFile(filename);
}

void Lexer::loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        // No hay parser todavia esperando tokens: informamos y dejamos
        // 'source' vacio, gettoken() devolvera END_OF_FILE de inmediato.
        std::cerr << "ERROR SCAN - No se pudo abrir el archivo: " << filename << "\n";
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string raw = buffer.str();

    // Normalizamos las 3 formas de fin de linea (LF, CRLF, CR) a '\n' unico,
    // asi el resto del scanner nunca tiene que preocuparse por '\r'.
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

Token Lexer::scanIdentifierOrKeyword() {
    // Se asume que peekchar() ya cumplio isIdentStart(). Regla de "maximal
    // munch" (sec. 3, parrafo introductorio): se consume la secuencia mas
    // larga posible de letras/digitos/'_'.
    int startLine = line;
    int startColumn = column;
    std::string lexeme;

    while (!isAtEnd() && isIdentPart(peekchar())) {
        lexeme.push_back(getchar_());
    }

    const auto& table = keywordTable();
    auto it = table.find(lexeme);
    TokenType type = (it != table.end()) ? it->second : TokenType::IDENTIFIER;

    return Token{type, lexeme, Location{startLine, startColumn}};
}

Token Lexer::scanInteger() {
    // Se asume que peekchar() ya es un digito. "Maximal munch": se
    // consume la corrida mas larga posible de digitos antes de decidir
    // si el lexema resultante es un entero valido.
    int startLine = line;
    int startColumn = column;
    std::string lexeme;

    while (!isAtEnd() && isDigitChar(peekchar())) {
        lexeme.push_back(getchar_());
    }

    Location loc{startLine, startColumn};

    // Regla de ceros a la izquierda (sec. 3.4.2): "0" solo es valido,
    // pero "0" seguido de mas digitos no lo es (ej. "007").
    if (lexeme.size() > 1 && lexeme[0] == '0') {
        reportError("Literal entero invalido (ceros a la izquierda no permitidos): '" +
                        lexeme + "'",
                    startLine, startColumn);
        return Token{TokenType::UNKNOWN_ERROR, lexeme, loc};
    }

    // Rango de 32 bits con signo: comparamos como string para no
    // desbordar ningun tipo entero de C++ con lexemas muy largos.
    bool outOfRange = false;
    if (lexeme.size() > INT32_MAX_STR.size()) {
        outOfRange = true;
    } else if (lexeme.size() == INT32_MAX_STR.size() && lexeme > INT32_MAX_STR) {
        outOfRange = true;
    }

    if (outOfRange) {
        reportError("Literal entero fuera de rango (maximo " + INT32_MAX_STR + "): '" +
                        lexeme + "'",
                    startLine, startColumn);
        return Token{TokenType::UNKNOWN_ERROR, lexeme, loc};
    }

    return Token{TokenType::INTEGER, lexeme, loc};
}

Token Lexer::scanString() {
    // Se asume que peekchar() == '"'.
    int startLine = line;
    int startColumn = column;
    getchar_(); // consumimos la comilla de apertura

    std::string content; // valor ya decodificado (sin comillas, escapes aplicados)
    bool terminated = false;
    bool exceededLength = false;

    for (;;) {
        if (isAtEnd()) {
            reportError("Cadena sin cerrar: se llego a fin de archivo antes de la comilla de cierre",
                        startLine, startColumn);
            break;
        }

        char c = peekchar();

        if (c == '"') {
            getchar_();
            terminated = true;
            break;
        }

        if (c == '\n') {
            // No consumimos el '\n': el manejo de NEWLINE/indentacion lo
            // necesita intacto para no romper el conteo de lineas.
            reportError("Cadena sin cerrar: se llego a fin de linea antes de la comilla de cierre",
                        startLine, startColumn);
            break;
        }

        if (c == '\\') {
            int escLine = line, escColumn = column; // posicion de la barra invertida
            getchar_();
            if (isAtEnd()) {
                reportError("Cadena sin cerrar: secuencia de escape incompleta al final del archivo",
                            startLine, startColumn);
                break;
            }
            char esc = getchar_();
            switch (esc) {
                case '"':  content.push_back('"');  break;
                case 'n':  content.push_back('\n'); break;
                case 't':  content.push_back('\t'); break;
                case '\\': content.push_back('\\'); break;
                default:
                    reportError(std::string("Secuencia de escape invalida: '\\") + esc +
                                    "' (solo se permiten \\\", \\n, \\t, \\\\)",
                                escLine, escColumn);
                    // Recuperacion: descartamos la barra y dejamos el
                    // caracter tal cual, para poder seguir escaneando.
                    content.push_back(esc);
                    break;
            }
            continue;
        }

        // Caracter regular: debe estar en el rango ASCII imprimible 32-126 (sec. 3.4.1).
        int code = static_cast<unsigned char>(c);
        if (code < 32 || code > 126) {
            reportError("Caracter no permitido dentro de un literal de cadena "
                        "(fuera del rango ASCII 32-126)",
                        line, column);
            getchar_(); // se descarta y seguimos buscando el cierre
            continue;
        }

        if (content.size() >= MAX_STRING_LEN) {
            if (!exceededLength) {
                reportError("Literal de cadena excede el tamano maximo permitido (" +
                                std::to_string(MAX_STRING_LEN) + " caracteres)",
                            startLine, startColumn);
                exceededLength = true;
            }
            getchar_(); // seguimos consumiendo para poder llegar al cierre
            continue;
        }

        content.push_back(getchar_());
    }

    Location loc{startLine, startColumn};

    if (!terminated) {
        return Token{TokenType::UNKNOWN_ERROR, content, loc};
    }

    TokenType type = looksLikeIdentifierContent(content) ? TokenType::IDSTRING : TokenType::STRING;
    return Token{type, content, loc};
}

Token Lexer::scanOperatorOrDelimiter() {
    // Se asume que peekchar() todavia no fue consumido. Igual que en el
    // resto del scanner, "maximal munch": antes de decidir un token de un
    // solo caracter, miramos si el siguiente caracter completa uno de dos
    // (sec. 3.5: -> == != <= >= //).
    int startLine = line;
    int startColumn = column;
    char c = getchar_();

    auto make = [&](TokenType type, const std::string& lexeme) {
        return Token{type, lexeme, Location{startLine, startColumn}};
    };
    auto error = [&](const std::string& lexeme) {
        return Token{TokenType::UNKNOWN_ERROR, lexeme, Location{startLine, startColumn}};
    };

    switch (c) {
        // --- Delimitadores de un solo caracter, sin ambiguedad ---
        case '(': return make(TokenType::LPAREN, "(");
        case ')': return make(TokenType::RPAREN, ")");
        case '[': return make(TokenType::LBRACKET, "[");
        case ']': return make(TokenType::RBRACKET, "]");
        case ',': return make(TokenType::COMMA, ",");
        case ':': return make(TokenType::COLON, ":");
        case '.': return make(TokenType::DOT, ".");

        // --- Operadores de un solo caracter, sin ambiguedad ---
        case '+': return make(TokenType::PLUS, "+");
        case '*': return make(TokenType::STAR, "*");
        case '%': return make(TokenType::PERCENT, "%");

        // --- '-' puede ser MINUS o el inicio de '->' ---
        case '-':
            if (peekchar() == '>') {
                getchar_();
                return make(TokenType::ARROW, "->");
            }
            return make(TokenType::MINUS, "-");

        // --- '/' solo es valido como parte de '//' (division entera). ---
        // ChocoPy no tiene division real ('/'), a diferencia de Python
        // (sec. 2.6.3), asi que un '/' suelto es un operador mal formado.
        case '/':
            if (peekchar() == '/') {
                getchar_();
                return make(TokenType::SLASH_SLASH, "//");
            }
            reportError("Operador mal formado: '/' (ChocoPy solo tiene division entera '//', "
                        "no division real)",
                        startLine, startColumn);
            return error("/");

        // --- '=' puede ser EQUAL o el inicio de '==' ---
        case '=':
            if (peekchar() == '=') {
                getchar_();
                return make(TokenType::EQ_EQ, "==");
            }
            return make(TokenType::EQUAL, "=");

        // --- '<' puede ser LESS o el inicio de '<=' ---
        case '<':
            if (peekchar() == '=') {
                getchar_();
                return make(TokenType::LESS_EQ, "<=");
            }
            return make(TokenType::LESS, "<");

        // --- '>' puede ser GREATER o el inicio de '>=' ---
        case '>':
            if (peekchar() == '=') {
                getchar_();
                return make(TokenType::GREATER_EQ, ">=");
            }
            return make(TokenType::GREATER, ">");

        // --- '!' solo es valido como parte de '!=' (no existe '!' solo). ---
        case '!':
            if (peekchar() == '=') {
                getchar_();
                return make(TokenType::BANG_EQ, "!=");
            }
            reportError("Operador mal formado: '!' no seguido de '=' "
                        "(ChocoPy solo tiene el operador '!=')",
                        startLine, startColumn);
            return error("!");

        // --- Cualquier otro simbolo no pertenece a la especificacion de
        //     ningun token de ChocoPy (p. ej. '@', '$', ';', '&', '|'). ---
        default:
            reportError(std::string("Simbolo no reconocido: '") + c + "'",
                        startLine, startColumn);
            return error(std::string(1, c));
    }
}

Token Lexer::gettoken() {
    for (;;) {
        // 1) Si hay tokens pendientes (INDENT/DEDENT/NEWLINE sintetico/EOF),
        //    los devolvemos antes de tocar el input.
        if (!pendingTokens.empty()) {
            Token t = pendingTokens.front();
            pendingTokens.pop_front();
            return t;
        }

        // 2) Inicio de linea logica: resolver indentacion / lineas en
        //    blanco / lineas de solo-comentario antes de seguir.
        if (atLineStart) {
            handleLineStart();
            continue; // vuelve a revisar la cola en el paso 1
        }

        // 3) Fin de archivo estando a mitad de una linea con contenido:
        //    el EOF actua como terminador implicito (sec. 3.1.1).
        if (isAtEnd()) {
            pendingTokens.push_back(Token{TokenType::NEWLINE, "", Location{line, column}});
            atLineStart = true; // la proxima vuelta disparara el flush de indentacion
            continue;
        }

        char c = peekchar();

        // 4) Espacios/tabs entre tokens (no al inicio de linea): se descartan.
        if (c == ' ' || c == '\t') {
            getchar_();
            continue;
        }

        // 5) Comentario despues de tokens reales en la misma linea.
        if (c == '#') {
            skipComment();
            continue;
        }

        // 6) Fin de la linea logica actual.
        if (c == '\n') {
            int startLine = line, startColumn = column;
            getchar_();
            atLineStart = true;
            return Token{TokenType::NEWLINE, "\n", Location{startLine, startColumn}};
        }

        // 7) Identificadores y palabras clave (paso 5).
        if (isIdentStart(c)) {
            return scanIdentifierOrKeyword();
        }

        // 8) Literales enteros (paso 6).
        if (isDigitChar(c)) {
            return scanInteger();
        }

        // 9) Literales de cadena (paso 6, parte 2).
        if (c == '"') {
            return scanString();
        }

        // 10) Operadores y delimitadores (paso 7): cualquier otro
        //     caracter que llega hasta aca solo puede ser esto, o un
        //     simbolo invalido; scanOperatorOrDelimiter() decide cual.
        return scanOperatorOrDelimiter();
    }
}