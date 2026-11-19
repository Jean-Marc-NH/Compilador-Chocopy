#include "constructor.hpp"
#include <algorithm>
#include <stdexcept>

// Cada funcion de este archivo corresponde a un no terminal de
// Gramaticas/chocopy.txt y decide el nodo segun la forma de la produccion
// (cantidad e identidad de los hijos). Arriba de cada una esta la regla de
// la gramatica que recorre.

namespace {

[[noreturn]] void fallo(const NodoParse& nodo, const std::string& detalle) {
    throw std::runtime_error("AST: " + detalle + " en el nodo " + nodo.simbolo);
}

const NodoParse& hijo(const NodoParse& nodo, std::size_t indice) {
    if (indice >= nodo.hijos.size()) fallo(nodo, "falta el hijo " + std::to_string(indice));
    return *nodo.hijos[indice];
}

// Lexema del token de una hoja (nombres, operadores, literales).
const std::string& lexema(const NodoParse& hoja) {
    if (!hoja.esHoja) fallo(hoja, "se esperaba un token");
    return hoja.token.lexema;
}

// Posicion del primer token del subarbol (las producciones e no tienen).
Posicion posicionDe(const NodoParse& nodo) {
    if (nodo.esHoja) return nodo.token.posicion;
    for (const auto& h : nodo.hijos) {
        if (h->esHoja || !h->hijos.empty()) return posicionDe(*h);
    }
    return Posicion{};
}

// Crea un nodo del AST con la posicion de 'origen'.
template <class T>
std::unique_ptr<T> nuevo(const NodoParse& origen) {
    auto nodo = std::make_unique<T>();
    nodo->pos = posicionDe(origen);
    return nodo;
}

// Aplana una lista recursiva a la izquierda: L > item | L [sep] item.
std::vector<const NodoParse*> aplanar(const NodoParse& lista) {
    std::vector<const NodoParse*> elementos;
    const NodoParse* actual = &lista;
    while (actual->hijos.size() > 1 && actual->hijos[0]->simbolo == lista.simbolo) {
        elementos.push_back(actual->hijos.back().get());
        actual = actual->hijos[0].get();
    }
    elementos.push_back(actual->hijos.back().get());
    std::reverse(elementos.begin(), elementos.end());
    return elementos;
}

NodoPtr expr(const NodoParse& nodo);
NodoPtr tipo(const NodoParse& nodo);
NodoPtr sentencia(const NodoParse& nodo);
NodoPtr definicion(const NodoParse& nodo);

// ---------- Tipos ----------

// TIPO > IDENTIFIER | IDSTRING | LBRACKET TIPO RBRACKET
NodoPtr tipo(const NodoParse& nodo) {
    auto resultado = nuevo<Tipo>(nodo);
    if (nodo.hijos.size() == 1) resultado->nombre = lexema(hijo(nodo, 0));
    else resultado->elemento = tipo(hijo(nodo, 1));
    return resultado;
}

// VARTIPADA > IDENTIFIER COLON TIPO
NodoPtr variableTipada(const NodoParse& nodo) {
    auto resultado = nuevo<VarTipada>(nodo);
    resultado->nombre = lexema(hijo(nodo, 0));
    resultado->tipo = tipo(hijo(nodo, 2));
    return resultado;
}

// ---------- Expresiones ----------

// LITERAL > KW_NONE | KW_TRUE | KW_FALSE | INTEGER | STRING | IDSTRING
NodoPtr literal(const NodoParse& nodo) {
    const NodoParse& hoja = hijo(nodo, 0);
    const std::string& terminal = hoja.simbolo;
    if (terminal == "KW_NONE") return nuevo<LitNone>(nodo);
    if (terminal == "KW_TRUE" || terminal == "KW_FALSE") {
        auto resultado = nuevo<LitBool>(nodo);
        resultado->valor = terminal == "KW_TRUE";
        return resultado;
    }
    if (terminal == "INTEGER") {
        auto resultado = nuevo<LitEntero>(nodo);
        resultado->valor = lexema(hoja);
        return resultado;
    }
    if (terminal == "STRING" || terminal == "IDSTRING") {
        auto resultado = nuevo<LitTexto>(nodo);
        resultado->valor = lexema(hoja);
        return resultado;
    }
    fallo(nodo, "literal desconocido " + terminal);
}

// ARGUMENTOS / ELEMENTOS > e | ARGLISTA / ELEMLISTA
ListaNodos listaExpresiones(const NodoParse& nodo) {
    ListaNodos resultado;
    if (nodo.hijos.empty()) return resultado;
    for (const NodoParse* elemento : aplanar(hijo(nodo, 0))) resultado.push_back(expr(*elemento));
    return resultado;
}

// Y > Y operador Y: el operador siempre es el hijo del medio.
NodoPtr binaria(const NodoParse& nodo, const std::string& operador) {
    auto resultado = nuevo<OpBinaria>(nodo);
    resultado->operador = operador;
    resultado->izq = expr(hijo(nodo, 0));
    resultado->der = expr(hijo(nodo, 2));
    return resultado;
}

NodoPtr expr(const NodoParse& nodo) {
    const std::string& simbolo = nodo.simbolo;

    // Un IDENTIFIER suelto (por ejemplo como OBJETIVO de una asignacion).
    if (nodo.esHoja) {
        if (simbolo != "IDENTIFIER") fallo(nodo, "token inesperado en expresion");
        auto resultado = nuevo<Identificador>(nodo);
        resultado->nombre = lexema(nodo);
        return resultado;
    }

    // EXPR > ORE | ORE KW_IF ORE KW_ELSE EXPR
    if (simbolo == "EXPR") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<ExprSi>(nodo);
        resultado->entonces = expr(hijo(nodo, 0));
        resultado->condicion = expr(hijo(nodo, 2));
        resultado->sino = expr(hijo(nodo, 4));
        return resultado;
    }

    // ORE, ANDE, ARIT, TERM > Y | Y operador Y (asociativos a la izquierda)
    if (simbolo == "ORE" || simbolo == "ANDE" || simbolo == "ARIT" || simbolo == "TERM") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        return binaria(nodo, lexema(hijo(nodo, 1)));
    }

    // NOTE > CMPE | KW_NOT NOTE
    if (simbolo == "NOTE") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<OpUnaria>(nodo);
        resultado->operador = "not";
        resultado->operando = expr(hijo(nodo, 1));
        return resultado;
    }

    // CMPE > ARIT | ARIT CMPOP ARIT   (CMPOP tiene un unico token hijo)
    if (simbolo == "CMPE") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        return binaria(nodo, lexema(hijo(hijo(nodo, 1), 0)));
    }

    // UNARIO > POSTF | MINUS UNARIO
    if (simbolo == "UNARIO") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<OpUnaria>(nodo);
        resultado->operador = "-";
        resultado->operando = expr(hijo(nodo, 1));
        return resultado;
    }

    // POSTF > PRIM | MIEMBRO | INDICE | LLAMADA
    // OBJETIVO > IDENTIFIER | MIEMBRO | INDICE
    if (simbolo == "POSTF" || simbolo == "OBJETIVO") return expr(hijo(nodo, 0));

    // PRIM > IDENTIFIER | LITERAL | LPAREN EXPR RPAREN | LBRACKET ELEMENTOS RBRACKET
    if (simbolo == "PRIM") {
        const NodoParse& primero = hijo(nodo, 0);
        if (primero.esHoja && primero.simbolo == "IDENTIFIER") return expr(primero);
        if (primero.simbolo == "LITERAL") return literal(primero);
        if (primero.simbolo == "LPAREN") return expr(hijo(nodo, 1));
        auto resultado = nuevo<LitLista>(nodo);
        resultado->elementos = listaExpresiones(hijo(nodo, 1));
        return resultado;
    }

    if (simbolo == "LITERAL") return literal(nodo);

    // MIEMBRO > POSTF DOT IDENTIFIER
    if (simbolo == "MIEMBRO") {
        auto resultado = nuevo<Miembro>(nodo);
        resultado->objeto = expr(hijo(nodo, 0));
        resultado->nombre = lexema(hijo(nodo, 2));
        return resultado;
    }

    // INDICE > POSTF LBRACKET EXPR RBRACKET
    if (simbolo == "INDICE") {
        auto resultado = nuevo<Indice>(nodo);
        resultado->objeto = expr(hijo(nodo, 0));
        resultado->indice = expr(hijo(nodo, 2));
        return resultado;
    }

    // LLAMADA > IDENTIFIER LPAREN ARGUMENTOS RPAREN | MIEMBRO LPAREN ARGUMENTOS RPAREN
    if (simbolo == "LLAMADA") {
        auto resultado = nuevo<Llamada>(nodo);
        resultado->funcion = expr(hijo(nodo, 0));
        resultado->argumentos = listaExpresiones(hijo(nodo, 2));
        return resultado;
    }

    fallo(nodo, "no es una expresion");
}

// ---------- Sentencias ----------

// SENTLISTA > SENTENCIA | SENTLISTA SENTENCIA
ListaNodos sentencias(const NodoParse& lista) {
    ListaNodos resultado;
    for (const NodoParse* elemento : aplanar(lista)) resultado.push_back(sentencia(*elemento));
    return resultado;
}

// BLOQUE > NEWLINE INDENT SENTLISTA DEDENT
ListaNodos bloque(const NodoParse& nodo) { return sentencias(hijo(nodo, 2)); }

// ASIGNACION > OBJETIVO EQUAL EXPR | OBJETIVO EQUAL ASIGNACION
NodoPtr asignacion(const NodoParse& nodo) {
    auto resultado = nuevo<Asignacion>(nodo);
    const NodoParse* actual = &nodo;
    for (;;) {
        resultado->objetivos.push_back(expr(hijo(*actual, 0)));
        const NodoParse& derecho = hijo(*actual, 2);
        if (derecho.simbolo == "ASIGNACION") {
            actual = &derecho;
        } else {
            resultado->valor = expr(derecho);
            return resultado;
        }
    }
}

// SIMPLE > KW_PASS | KW_RETURN | KW_RETURN EXPR | EXPR | ASIGNACION
NodoPtr simple(const NodoParse& nodo) {
    const NodoParse& primero = hijo(nodo, 0);
    if (primero.simbolo == "KW_PASS") return nuevo<Pasar>(nodo);
    if (primero.simbolo == "KW_RETURN") {
        auto resultado = nuevo<Retornar>(nodo);
        if (nodo.hijos.size() == 2) resultado->valor = expr(hijo(nodo, 1));
        return resultado;
    }
    if (primero.simbolo == "ASIGNACION") return asignacion(primero);
    auto resultado = nuevo<SentenciaExpr>(nodo);
    resultado->expr = expr(primero);
    return resultado;
}

// IFSTMT > KW_IF EXPR COLON BLOQUE LISTAELIF OPCIONSINO
NodoPtr condicional(const NodoParse& nodo) {
    auto resultado = nuevo<Si>(nodo);
    resultado->condicion = expr(hijo(nodo, 1));
    resultado->entonces = bloque(hijo(nodo, 3));

    // LISTAELIF > e | LISTAELIF KW_ELIF EXPR COLON BLOQUE. Recorriendo hacia
    // la izquierda salen del ultimo elif al primero, que es justo el orden
    // para anidarlos desde adentro: cada elif toma como 'sino' lo que sigue.
    ListaNodos resto;
    const NodoParse& opcionSino = hijo(nodo, 5);
    if (!opcionSino.hijos.empty()) resto = bloque(hijo(opcionSino, 2));
    for (const NodoParse* elif = &hijo(nodo, 4); !elif->hijos.empty(); elif = elif->hijos[0].get()) {
        auto anidado = nuevo<Si>(hijo(*elif, 1));
        anidado->condicion = expr(hijo(*elif, 2));
        anidado->entonces = bloque(hijo(*elif, 4));
        anidado->sino = std::move(resto);
        resto.clear();
        resto.push_back(std::move(anidado));
    }
    resultado->sino = std::move(resto);
    return resultado;
}

// SENTENCIA > SIMPLE NEWLINE | IFSTMT | WHILESTMT | FORSTMT
NodoPtr sentencia(const NodoParse& nodo) {
    const NodoParse& primero = hijo(nodo, 0);
    if (primero.simbolo == "SIMPLE") return simple(primero);
    if (primero.simbolo == "IFSTMT") return condicional(primero);
    if (primero.simbolo == "WHILESTMT") {
        // KW_WHILE EXPR COLON BLOQUE
        auto resultado = nuevo<Mientras>(primero);
        resultado->condicion = expr(hijo(primero, 1));
        resultado->cuerpo = bloque(hijo(primero, 3));
        return resultado;
    }
    if (primero.simbolo == "FORSTMT") {
        // KW_FOR IDENTIFIER KW_IN EXPR COLON BLOQUE
        auto resultado = nuevo<Para>(primero);
        resultado->variable = lexema(hijo(primero, 1));
        resultado->iterable = expr(hijo(primero, 3));
        resultado->cuerpo = bloque(hijo(primero, 5));
        return resultado;
    }
    fallo(nodo, "sentencia desconocida " + primero.simbolo);
}

// ---------- Definiciones ----------

// VARDEF > VARTIPADA EQUAL LITERAL NEWLINE
NodoPtr definicionVariable(const NodoParse& nodo) {
    auto resultado = nuevo<DefVariable>(nodo);
    resultado->variable = variableTipada(hijo(nodo, 0));
    resultado->valor = literal(hijo(nodo, 2));
    return resultado;
}

// FUNCDEF > KW_DEF IDENTIFIER LPAREN PARAMETROS RPAREN RETTIPO COLON NEWLINE
//           INDENT CUERPOFUNC DEDENT
NodoPtr definicionFuncion(const NodoParse& nodo) {
    auto resultado = nuevo<DefFuncion>(nodo);
    resultado->nombre = lexema(hijo(nodo, 1));

    const NodoParse& parametros = hijo(nodo, 3); // e | LISTAPARAM
    if (!parametros.hijos.empty())
        for (const NodoParse* parametro : aplanar(hijo(parametros, 0)))
            resultado->parametros.push_back(variableTipada(*parametro));

    const NodoParse& retorno = hijo(nodo, 5); // e | ARROW TIPO
    if (!retorno.hijos.empty()) resultado->retorno = tipo(hijo(retorno, 1));

    // CUERPOFUNC > SENTLISTA | DECLARACIONES SENTLISTA
    const NodoParse& cuerpo = hijo(nodo, 9);
    if (cuerpo.hijos.size() == 2) {
        for (const NodoParse* unaDecl : aplanar(hijo(cuerpo, 0))) {
            // UNADECL > GLOBALDECL | NONLOCALDECL | VARDEF | FUNCDEF
            const NodoParse& declaracion = hijo(*unaDecl, 0);
            if (declaracion.simbolo == "GLOBALDECL") {
                auto global = nuevo<DeclGlobal>(declaracion);
                global->nombre = lexema(hijo(declaracion, 1));
                resultado->declaraciones.push_back(std::move(global));
            } else if (declaracion.simbolo == "NONLOCALDECL") {
                auto nonlocal = nuevo<DeclNonlocal>(declaracion);
                nonlocal->nombre = lexema(hijo(declaracion, 1));
                resultado->declaraciones.push_back(std::move(nonlocal));
            } else {
                resultado->declaraciones.push_back(definicion(declaracion));
            }
        }
    }
    resultado->cuerpo = sentencias(hijo(cuerpo, cuerpo.hijos.size() - 1));
    return resultado;
}

// CLASEDEF > KW_CLASS IDENTIFIER LPAREN IDENTIFIER RPAREN COLON NEWLINE INDENT
//            CUERPOCLASE DEDENT
NodoPtr definicionClase(const NodoParse& nodo) {
    auto resultado = nuevo<DefClase>(nodo);
    resultado->nombre = lexema(hijo(nodo, 1));
    resultado->superclase = lexema(hijo(nodo, 3));

    // CUERPOCLASE > KW_PASS NEWLINE | LISTADECL ; DECLCLASE > VARDEF | FUNCDEF
    const NodoParse& cuerpo = hijo(nodo, 8);
    if (cuerpo.simbolo == "CUERPOCLASE" && !hijo(cuerpo, 0).esHoja)
        for (const NodoParse* declaracion : aplanar(hijo(cuerpo, 0)))
            resultado->miembros.push_back(definicion(hijo(*declaracion, 0)));
    return resultado;
}

// VARDEF | FUNCDEF | CLASEDEF
NodoPtr definicion(const NodoParse& nodo) {
    if (nodo.simbolo == "VARDEF") return definicionVariable(nodo);
    if (nodo.simbolo == "FUNCDEF") return definicionFuncion(nodo);
    if (nodo.simbolo == "CLASEDEF") return definicionClase(nodo);
    fallo(nodo, "definicion desconocida");
}

bool contieneError(const NodoParse& nodo) {
    if (nodo.esError) return true;
    return std::any_of(nodo.hijos.begin(), nodo.hijos.end(),
                       [](const auto& h) { return contieneError(*h); });
}

} // namespace

std::unique_ptr<Programa> construirAST(const NodoParse& raiz) {
    if (raiz.simbolo != "PROGRAMA") fallo(raiz, "la raiz debe ser PROGRAMA");
    if (contieneError(raiz)) fallo(raiz, "el arbol contiene nodos de error");

    // PROGRAMA > e | DEFINICIONES | SENTLISTA | DEFINICIONES SENTLISTA
    auto programa = nuevo<Programa>(raiz);
    for (const auto& parte : raiz.hijos) {
        if (parte->simbolo == "DEFINICIONES") {
            for (const NodoParse* unaDef : aplanar(*parte)) // UNADEF > VARDEF | FUNCDEF | CLASEDEF
                programa->definiciones.push_back(definicion(hijo(*unaDef, 0)));
        } else {
            programa->sentencias = sentencias(*parte);
        }
    }
    return programa;
}
