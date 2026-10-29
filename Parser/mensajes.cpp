#include "mensajes.hpp"
#include <algorithm>
#include <map>

namespace {

// Terminales con los que puede empezar un valor. Si el parser los espera, es
// que falta una expresion o un literal.
const std::set<std::string> INICIO_VALOR = {"IDENTIFIER", "INTEGER", "STRING", "IDSTRING",
                                            "KW_NONE", "KW_TRUE", "KW_FALSE"};

// Operadores binarios: si el parser los espera, la expresion ya esta completa.
const std::set<std::string> OPERADORES = {"PLUS", "MINUS", "STAR", "SLASH_SLASH", "PERCENT", "EQ_EQ",
                                          "BANG_EQ", "LESS", "GREATER", "LESS_EQ", "GREATER_EQ",
                                          "KW_IS", "KW_AND", "KW_OR"};

bool algunoDe(const std::set<std::string>& conjunto, const std::set<std::string>& grupo) {
    return std::any_of(grupo.begin(), grupo.end(),
                       [&](const std::string& nombre) { return conjunto.count(nombre) > 0; });
}

bool esInicioValor(TokenType tipo) {
    return tipo == TokenType::IDENTIFIER || tipo == TokenType::INTEGER || tipo == TokenType::STRING ||
           tipo == TokenType::IDSTRING || tipo == TokenType::KW_NONE || tipo == TokenType::KW_TRUE ||
           tipo == TokenType::KW_FALSE;
}

bool esOperadorAritmetico(TokenType tipo) {
    return tipo == TokenType::PLUS || tipo == TokenType::MINUS || tipo == TokenType::STAR ||
           tipo == TokenType::SLASH_SLASH || tipo == TokenType::PERCENT;
}

// Operadores y signos que necesitan un valor a su izquierda o derecha.
bool esSigno(TokenType tipo) {
    return esOperadorAritmetico(tipo) || tipo == TokenType::EQ_EQ || tipo == TokenType::BANG_EQ ||
           tipo == TokenType::LESS || tipo == TokenType::GREATER || tipo == TokenType::LESS_EQ ||
           tipo == TokenType::GREATER_EQ || tipo == TokenType::EQUAL || tipo == TokenType::COMMA ||
           tipo == TokenType::COLON || tipo == TokenType::DOT || tipo == TokenType::ARROW ||
           tipo == TokenType::KW_AND || tipo == TokenType::KW_OR || tipo == TokenType::KW_IS ||
           tipo == TokenType::KW_NOT;
}

bool esFinDeLinea(TokenType tipo) {
    return tipo == TokenType::NEWLINE || tipo == TokenType::END_OF_FILE;
}

int largoDe(const Token& token) {
    switch (token.type) {
        case TokenType::NEWLINE:
        case TokenType::INDENT:
        case TokenType::DEDENT:
        case TokenType::END_OF_FILE:
            return 1;
        case TokenType::STRING:
        case TokenType::IDSTRING:
            return static_cast<int>(token.lexeme.size()) + 2; // las comillas
        default:
            return std::max(1, static_cast<int>(token.lexeme.size()));
    }
}

// Como se nombra un terminal de la gramatica cuando se le dice al usuario
// que se esperaba (solo se usa en el mensaje generico).
std::string describirTerminal(const std::string& nombre) {
    static const std::map<std::string, std::string> nombres = {
        {"IDENTIFIER", "un nombre"}, {"INTEGER", "un numero"}, {"STRING", "un texto"},
        {"IDSTRING", "un texto"}, {"NEWLINE", "el fin de la linea"}, {"INDENT", "un bloque indentado"},
        {"DEDENT", "el fin del bloque"}, {"END_OF_FILE", "el fin del archivo"}, {"$", "el fin del archivo"},
        {"PLUS", "'+'"}, {"MINUS", "'-'"}, {"STAR", "'*'"}, {"SLASH_SLASH", "'//'"}, {"PERCENT", "'%'"},
        {"EQ_EQ", "'=='"}, {"BANG_EQ", "'!='"}, {"LESS", "'<'"}, {"GREATER", "'>'"},
        {"LESS_EQ", "'<='"}, {"GREATER_EQ", "'>='"}, {"EQUAL", "'='"}, {"COLON", "':'"}, {"DOT", "'.'"},
        {"COMMA", "','"}, {"LPAREN", "'('"}, {"RPAREN", "')'"}, {"LBRACKET", "'['"},
        {"RBRACKET", "']'"}, {"ARROW", "'->'"}, {"KW_CLASS", "'class'"}, {"KW_DEF", "'def'"},
        {"KW_IF", "'if'"}, {"KW_ELIF", "'elif'"}, {"KW_ELSE", "'else'"}, {"KW_WHILE", "'while'"},
        {"KW_RETURN", "'return'"}, {"KW_FOR", "'for'"}, {"KW_IN", "'in'"}, {"KW_IS", "'is'"},
        {"KW_AND", "'and'"}, {"KW_OR", "'or'"}, {"KW_NOT", "'not'"}, {"KW_NONE", "'None'"},
        {"KW_TRUE", "'True'"}, {"KW_FALSE", "'False'"}, {"KW_PASS", "'pass'"},
        {"KW_GLOBAL", "'global'"}, {"KW_NONLOCAL", "'nonlocal'"}};
    auto it = nombres.find(nombre);
    return it == nombres.end() ? nombre : it->second;
}

// "'+'" para un token de signo; usado en "falta un valor entre '+' y '*'".
std::string entreComillas(const Token& token) { return "'" + token.lexeme + "'"; }

// Version corta para frases como "antes de 'x'" o "sobra 3": sin articulo.
std::string describirCorto(const Token& token) {
    switch (token.type) {
        case TokenType::INTEGER: return token.lexeme;
        case TokenType::STRING:
        case TokenType::IDSTRING: return "\"" + token.lexeme + "\"";
        case TokenType::END_OF_FILE: return "el fin del archivo";
        default: return "'" + token.lexeme + "'";
    }
}

char cierreDe(const Token& apertura) { return apertura.type == TokenType::LPAREN ? ')' : ']'; }

} // namespace

Location posicionDespuesDe(const Token& token) {
    return Location{token.location.line, token.location.column + largoDe(token)};
}

std::string describirToken(const Token& token) {
    switch (token.type) {
        case TokenType::IDENTIFIER: return "el nombre '" + token.lexeme + "'";
        case TokenType::INTEGER: return "el numero " + token.lexeme;
        case TokenType::STRING:
        case TokenType::IDSTRING: return "el texto \"" + token.lexeme + "\"";
        case TokenType::NEWLINE: return "el fin de la linea";
        case TokenType::INDENT: return "una sangria inesperada";
        case TokenType::DEDENT: return "el fin del bloque";
        case TokenType::END_OF_FILE: return "el fin del archivo";
        case TokenType::KW_CLASS: case TokenType::KW_DEF: case TokenType::KW_IF: case TokenType::KW_ELIF:
        case TokenType::KW_ELSE: case TokenType::KW_WHILE: case TokenType::KW_RETURN: case TokenType::KW_FOR:
        case TokenType::KW_IN: case TokenType::KW_IS: case TokenType::KW_AND: case TokenType::KW_OR:
        case TokenType::KW_NOT: case TokenType::KW_NONE: case TokenType::KW_TRUE: case TokenType::KW_FALSE:
        case TokenType::KW_PASS: case TokenType::KW_GLOBAL: case TokenType::KW_NONLOCAL:
        case TokenType::KW_RESERVED:
            return "la palabra '" + token.lexeme + "'";
        default: return "'" + token.lexeme + "'";
    }
}

ExplicacionError explicarErrorSintactico(const Token& encontrado, const Token* anterior,
                                         const std::set<std::string>& esperados,
                                         const std::vector<Token>& abiertos) {
    ExplicacionError r;
    r.largo = largoDe(encontrado);

    const TokenType tipo = encontrado.type;
    const std::string desc = describirToken(encontrado);
    const bool esperaValor = algunoDe(esperados, INICIO_VALOR);
    const bool esperaOperador = algunoDe(esperados, OPERADORES);
    const bool esperaFin = esperados.count("NEWLINE") > 0;
    const bool esperaCierre = esperados.count("RPAREN") > 0 || esperados.count("RBRACKET") > 0;
    const Token* abierto = abiertos.empty() ? nullptr : &abiertos.back();

    // Nota "el '(' abierto aqui nunca se cierra", que apunta al parentesis.
    auto notaAbierto = [&]() {
        if (!abierto) return;
        r.hayNota = true;
        r.notaMensaje = "el '" + abierto->lexeme + "' abierto aqui nunca se cierra";
        r.notaPos = abierto->location;
    };

    // --- Estructura de bloques (indentacion) ---
    if (esperados.count("INDENT")) {
        r.mensaje = "falta el bloque indentado que debe seguir a ':' (la siguiente linea debe ir mas adentro)";
        return r;
    }
    if (tipo == TokenType::INDENT) {
        r.mensaje = "indentacion inesperada: una linea solo se indenta mas que la anterior cuando la "
                    "anterior termina en ':'";
        return r;
    }
    if (tipo == TokenType::DEDENT) {
        r.mensaje = "fin de bloque inesperado: la instruccion anterior esta incompleta";
        return r;
    }

    // --- Palabras clave fuera de lugar ---
    if (tipo == TokenType::KW_ELSE || tipo == TokenType::KW_ELIF) {
        r.mensaje = "'" + encontrado.lexeme + "' sin un 'if' previo: " +
                    (tipo == TokenType::KW_ELSE ? "un 'else'" : "un 'elif'") + " siempre acompana a un 'if'";
        return r;
    }
    if (tipo == TokenType::KW_RESERVED) {
        r.mensaje = "'" + encontrado.lexeme + "' es una palabra reservada de Python que ChocoPy no admite";
        return r;
    }
    if (tipo == TokenType::KW_DEF || tipo == TokenType::KW_CLASS) {
        r.mensaje = "no se puede escribir '" + encontrado.lexeme +
                    "' aqui: las funciones y clases se definen antes de las sentencias";
        return r;
    }

    // --- Parentesis y corchetes ---
    if (tipo == TokenType::RPAREN || tipo == TokenType::RBRACKET) {
        const TokenType apertura = tipo == TokenType::RPAREN ? TokenType::LPAREN : TokenType::LBRACKET;
        bool hayApertura = std::any_of(abiertos.begin(), abiertos.end(),
                                       [&](const Token& t) { return t.type == apertura; });
        if (!hayApertura) {
            r.mensaje = "'" + encontrado.lexeme + "' de mas: no hay ningun '" +
                        (apertura == TokenType::LPAREN ? "(" : "[") + "' abierto que cerrar";
            return r;
        }
    }
    if (esFinDeLinea(tipo) && abierto && esperaCierre) {
        r.mensaje = std::string("falta cerrar el '") + abierto->lexeme + "' con '" + cierreDe(*abierto) +
                    "' antes del fin " + (tipo == TokenType::NEWLINE ? "de la linea" : "del archivo");
        notaAbierto();
        return r;
    }

    // --- La linea o el archivo terminan antes de tiempo ---
    if (esFinDeLinea(tipo)) {
        if (esperados.count("COLON")) {
            r.mensaje = "falta ':' al final de la linea";
        } else if (esperaValor) {
            if (anterior && anterior->type == TokenType::EQUAL)
                r.mensaje = "falta el valor que se asigna despues de '='";
            else if (anterior && anterior->type == TokenType::DOT)
                r.mensaje = "falta el nombre del atributo o metodo despues de '.'";
            else if (anterior && (esSigno(anterior->type) || anterior->type == TokenType::LPAREN ||
                                  anterior->type == TokenType::LBRACKET))
                r.mensaje = "falta un valor despues de " + entreComillas(*anterior);
            else
                r.mensaje = "la expresion esta incompleta: falta un valor";
        } else {
            r.mensaje = tipo == TokenType::NEWLINE
                            ? "la linea termina antes de completar la instruccion"
                            : "el archivo termina antes de completar la instruccion";
        }
        notaAbierto();
        return r;
    }

    // --- Operador mal escrito ---
    // "a += 1": ChocoPy no tiene asignacion compuesta.
    if (tipo == TokenType::EQUAL && anterior && esOperadorAritmetico(anterior->type) &&
        anterior->location.line == encontrado.location.line &&
        anterior->location.column + static_cast<int>(anterior->lexeme.size()) == encontrado.location.column) {
        r.mensaje = "'" + anterior->lexeme + "=' no existe en ChocoPy: escribe la operacion completa, "
                    "por ejemplo 'a = a " + anterior->lexeme + " 1'";
        r.largo = 1;
        return r;
    }

    // ":" donde empezaria una sentencia: "z: int = 5" despues de otras sentencias.
    if (tipo == TokenType::COLON && esperaFin && !esperaValor) {
        r.mensaje = "':' inesperado: las declaraciones de variables (nombre: tipo = valor) deben "
                    "escribirse antes de cualquier sentencia";
        return r;
    }

    // --- Falta un valor ---
    if (esperaValor) {
        const bool soloLiterales = esperados.count("IDENTIFIER") == 0;
        if (soloLiterales) {
            r.mensaje = "el valor inicial de una variable debe ser un literal (un numero, un texto, True, "
                        "False o None), pero se encontro " + desc;
        } else if (anterior && anterior->type == TokenType::DOT) {
            r.mensaje = "se esperaba un nombre despues de '.' pero se encontro " + desc;
        } else if (anterior && anterior->type == TokenType::COMMA &&
                   (tipo == TokenType::RPAREN || tipo == TokenType::RBRACKET)) {
            r.mensaje = "sobra la ',' antes de '" + encontrado.lexeme +
                        "': despues de una coma debe ir otro valor";
        } else if (anterior && esSigno(tipo) && (esSigno(anterior->type) || anterior->type == TokenType::LPAREN ||
                                                 anterior->type == TokenType::LBRACKET)) {
            r.mensaje = "falta un valor entre " + entreComillas(*anterior) + " y " + entreComillas(encontrado);
        } else {
            r.mensaje = "se esperaba un valor (un numero, un nombre, un texto...) pero se encontro " + desc;
        }
        return r;
    }

    // --- Dos cosas juntas sin separador ---
    if (esInicioValor(tipo) && (esperaFin || esperaOperador)) {
        r.mensaje = "falta un operador (como + o ==) o un salto de linea antes de " + describirCorto(encontrado);
        return r;
    }
    if (esperados.size() == 1 && esperados.count("COLON")) {
        if (anterior && anterior->type == TokenType::IDENTIFIER)
            r.mensaje = "se esperaba ':' despues de '" + anterior->lexeme +
                        "': cada variable o parametro se escribe como nombre: tipo";
        else
            r.mensaje = "se esperaba ':' pero se encontro " + desc;
        return r;
    }
    if (esperados.size() == 1 && esperaFin) {
        r.mensaje = "sobra " + describirCorto(encontrado) + ": la sentencia ya estaba completa";
        return r;
    }
    if (tipo == TokenType::COMMA && esperaFin) {
        r.mensaje = "',' inesperada: aqui solo cabe un valor (ChocoPy no tiene tuplas)";
        return r;
    }

    // --- Generico: se esperaba X pero se encontro Y ---
    std::vector<std::string> opciones;
    for (const auto& nombre : esperados) {
        std::string texto = describirTerminal(nombre);
        if (std::find(opciones.begin(), opciones.end(), texto) == opciones.end()) opciones.push_back(texto);
    }
    if (!opciones.empty() && opciones.size() <= 3) {
        std::string lista;
        for (std::size_t i = 0; i < opciones.size(); ++i) {
            if (i > 0) lista += i + 1 == opciones.size() ? " o " : ", ";
            lista += opciones[i];
        }
        r.mensaje = "se esperaba " + lista + " pero se encontro " + desc;
    } else {
        r.mensaje = desc + " no es valido en este lugar";
    }
    return r;
}
