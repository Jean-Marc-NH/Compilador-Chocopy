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

bool esInicioValor(TipoToken tipo) {
    return tipo == TipoToken::IDENTIFIER || tipo == TipoToken::INTEGER || tipo == TipoToken::STRING ||
           tipo == TipoToken::IDSTRING || tipo == TipoToken::KW_NONE || tipo == TipoToken::KW_TRUE ||
           tipo == TipoToken::KW_FALSE;
}

bool esOperadorAritmetico(TipoToken tipo) {
    return tipo == TipoToken::PLUS || tipo == TipoToken::MINUS || tipo == TipoToken::STAR ||
           tipo == TipoToken::SLASH_SLASH || tipo == TipoToken::PERCENT;
}

// Operadores y signos que necesitan un valor a su izquierda o derecha.
bool esSigno(TipoToken tipo) {
    return esOperadorAritmetico(tipo) || tipo == TipoToken::EQ_EQ || tipo == TipoToken::BANG_EQ ||
           tipo == TipoToken::LESS || tipo == TipoToken::GREATER || tipo == TipoToken::LESS_EQ ||
           tipo == TipoToken::GREATER_EQ || tipo == TipoToken::EQUAL || tipo == TipoToken::COMMA ||
           tipo == TipoToken::COLON || tipo == TipoToken::DOT || tipo == TipoToken::ARROW ||
           tipo == TipoToken::KW_AND || tipo == TipoToken::KW_OR || tipo == TipoToken::KW_IS ||
           tipo == TipoToken::KW_NOT;
}

bool esFinDeLinea(TipoToken tipo) {
    return tipo == TipoToken::NEWLINE || tipo == TipoToken::END_OF_FILE;
}

int largoDe(const Token& token) {
    switch (token.tipo) {
        case TipoToken::NEWLINE:
        case TipoToken::INDENT:
        case TipoToken::DEDENT:
        case TipoToken::END_OF_FILE:
            return 1;
        case TipoToken::STRING:
        case TipoToken::IDSTRING:
            return static_cast<int>(token.lexema.size()) + 2; // las comillas
        default:
            return std::max(1, static_cast<int>(token.lexema.size()));
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
std::string entreComillas(const Token& token) { return "'" + token.lexema + "'"; }

// Version corta para frases como "antes de 'x'" o "sobra 3": sin articulo.
std::string describirCorto(const Token& token) {
    switch (token.tipo) {
        case TipoToken::INTEGER: return token.lexema;
        case TipoToken::STRING:
        case TipoToken::IDSTRING: return "\"" + token.lexema + "\"";
        case TipoToken::END_OF_FILE: return "el fin del archivo";
        default: return "'" + token.lexema + "'";
    }
}

char cierreDe(const Token& apertura) { return apertura.tipo == TipoToken::LPAREN ? ')' : ']'; }

} // namespace

Posicion posicionDespuesDe(const Token& token) {
    return Posicion{token.posicion.linea, token.posicion.columna + largoDe(token)};
}

std::string describirToken(const Token& token) {
    switch (token.tipo) {
        case TipoToken::IDENTIFIER: return "el nombre '" + token.lexema + "'";
        case TipoToken::INTEGER: return "el numero " + token.lexema;
        case TipoToken::STRING:
        case TipoToken::IDSTRING: return "el texto \"" + token.lexema + "\"";
        case TipoToken::NEWLINE: return "el fin de la linea";
        case TipoToken::INDENT: return "una sangria inesperada";
        case TipoToken::DEDENT: return "el fin del bloque";
        case TipoToken::END_OF_FILE: return "el fin del archivo";
        case TipoToken::KW_CLASS: case TipoToken::KW_DEF: case TipoToken::KW_IF: case TipoToken::KW_ELIF:
        case TipoToken::KW_ELSE: case TipoToken::KW_WHILE: case TipoToken::KW_RETURN: case TipoToken::KW_FOR:
        case TipoToken::KW_IN: case TipoToken::KW_IS: case TipoToken::KW_AND: case TipoToken::KW_OR:
        case TipoToken::KW_NOT: case TipoToken::KW_NONE: case TipoToken::KW_TRUE: case TipoToken::KW_FALSE:
        case TipoToken::KW_PASS: case TipoToken::KW_GLOBAL: case TipoToken::KW_NONLOCAL:
        case TipoToken::KW_RESERVED:
            return "la palabra '" + token.lexema + "'";
        default: return "'" + token.lexema + "'";
    }
}

ExplicacionError explicarErrorSintactico(const Token& encontrado, const Token* anterior,
                                         const std::set<std::string>& esperados,
                                         const std::vector<Token>& abiertos) {
    ExplicacionError r;
    r.largo = largoDe(encontrado);

    const TipoToken tipo = encontrado.tipo;
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
        r.notaMensaje = "el '" + abierto->lexema + "' abierto aqui nunca se cierra";
        r.notaPos = abierto->posicion;
    };

    // --- Estructura de bloques (indentacion) ---
    if (esperados.count("INDENT")) {
        r.mensaje = "falta el bloque indentado que debe seguir a ':' (la siguiente linea debe ir mas adentro)";
        return r;
    }
    if (tipo == TipoToken::INDENT) {
        r.mensaje = "indentacion inesperada: una linea solo se indenta mas que la anterior cuando la "
                    "anterior termina en ':'";
        return r;
    }
    if (tipo == TipoToken::DEDENT) {
        r.mensaje = "fin de bloque inesperado: la instruccion anterior esta incompleta";
        return r;
    }

    // --- Palabras clave fuera de lugar ---
    if (tipo == TipoToken::KW_ELSE || tipo == TipoToken::KW_ELIF) {
        r.mensaje = "'" + encontrado.lexema + "' sin un 'if' previo: " +
                    (tipo == TipoToken::KW_ELSE ? "un 'else'" : "un 'elif'") + " siempre acompana a un 'if'";
        return r;
    }
    if (tipo == TipoToken::KW_RESERVED) {
        r.mensaje = "'" + encontrado.lexema + "' es una palabra reservada de Python que ChocoPy no admite";
        return r;
    }
    if (tipo == TipoToken::KW_DEF || tipo == TipoToken::KW_CLASS) {
        r.mensaje = "no se puede escribir '" + encontrado.lexema +
                    "' aqui: las funciones y clases se definen antes de las sentencias";
        return r;
    }

    // --- Parentesis y corchetes ---
    if (tipo == TipoToken::RPAREN || tipo == TipoToken::RBRACKET) {
        const TipoToken apertura = tipo == TipoToken::RPAREN ? TipoToken::LPAREN : TipoToken::LBRACKET;
        bool hayApertura = std::any_of(abiertos.begin(), abiertos.end(),
                                       [&](const Token& t) { return t.tipo == apertura; });
        if (!hayApertura) {
            r.mensaje = "'" + encontrado.lexema + "' de mas: no hay ningun '" +
                        (apertura == TipoToken::LPAREN ? "(" : "[") + "' abierto que cerrar";
            return r;
        }
    }
    if (esFinDeLinea(tipo) && abierto && esperaCierre) {
        r.mensaje = std::string("falta cerrar el '") + abierto->lexema + "' con '" + cierreDe(*abierto) +
                    "' antes del fin " + (tipo == TipoToken::NEWLINE ? "de la linea" : "del archivo");
        notaAbierto();
        return r;
    }

    // --- La linea o el archivo terminan antes de tiempo ---
    if (esFinDeLinea(tipo)) {
        if (esperados.count("COLON")) {
            r.mensaje = "falta ':' al final de la linea";
        } else if (esperaValor) {
            if (anterior && anterior->tipo == TipoToken::EQUAL)
                r.mensaje = "falta el valor que se asigna despues de '='";
            else if (anterior && anterior->tipo == TipoToken::DOT)
                r.mensaje = "falta el nombre del atributo o metodo despues de '.'";
            else if (anterior && (esSigno(anterior->tipo) || anterior->tipo == TipoToken::LPAREN ||
                                  anterior->tipo == TipoToken::LBRACKET))
                r.mensaje = "falta un valor despues de " + entreComillas(*anterior);
            else
                r.mensaje = "la expresion esta incompleta: falta un valor";
        } else {
            r.mensaje = tipo == TipoToken::NEWLINE
                            ? "la linea termina antes de completar la instruccion"
                            : "el archivo termina antes de completar la instruccion";
        }
        notaAbierto();
        return r;
    }

    // --- Operador mal escrito ---
    // "a += 1": ChocoPy no tiene asignacion compuesta.
    if (tipo == TipoToken::EQUAL && anterior && esOperadorAritmetico(anterior->tipo) &&
        anterior->posicion.linea == encontrado.posicion.linea &&
        anterior->posicion.columna + static_cast<int>(anterior->lexema.size()) == encontrado.posicion.columna) {
        r.mensaje = "'" + anterior->lexema + "=' no existe en ChocoPy: escribe la operacion completa, "
                    "por ejemplo 'a = a " + anterior->lexema + " 1'";
        r.largo = 1;
        return r;
    }

    // ":" donde empezaria una sentencia: "z: int = 5" despues de otras sentencias.
    if (tipo == TipoToken::COLON && esperaFin && !esperaValor) {
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
        } else if (anterior && anterior->tipo == TipoToken::DOT) {
            r.mensaje = "se esperaba un nombre despues de '.' pero se encontro " + desc;
        } else if (anterior && anterior->tipo == TipoToken::COMMA &&
                   (tipo == TipoToken::RPAREN || tipo == TipoToken::RBRACKET)) {
            r.mensaje = "sobra la ',' antes de '" + encontrado.lexema +
                        "': despues de una coma debe ir otro valor";
        } else if (anterior && esSigno(tipo) && (esSigno(anterior->tipo) || anterior->tipo == TipoToken::LPAREN ||
                                                 anterior->tipo == TipoToken::LBRACKET)) {
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
        if (anterior && anterior->tipo == TipoToken::IDENTIFIER)
            r.mensaje = "se esperaba ':' despues de '" + anterior->lexema +
                        "': cada variable o parametro se escribe como nombre: tipo";
        else
            r.mensaje = "se esperaba ':' pero se encontro " + desc;
        return r;
    }
    if (esperados.size() == 1 && esperaFin) {
        r.mensaje = "sobra " + describirCorto(encontrado) + ": la sentencia ya estaba completa";
        return r;
    }
    if (tipo == TipoToken::COMMA && esperaFin) {
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
