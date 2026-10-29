#include "ast_builder.hpp"
#include <algorithm>
#include <stdexcept>

// Cada funcion de este archivo corresponde a un no terminal de
// Parser/gramatica_chocopy.txt y decide el nodo segun la forma de la
// produccion (cantidad e identidad de los hijos).

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
    return hoja.token.lexeme;
}

// Posicion del primer token del subarbol.
Location posicion(const NodoParse& nodo) {
    if (nodo.esHoja) return nodo.token.location;
    for (const auto& h : nodo.hijos) {
        if (h->esHoja || !h->hijos.empty()) return posicion(*h);
    }
    return Location{};
}

template <class T>
std::unique_ptr<T> nuevo(const NodoParse& origen) {
    auto nodo = std::make_unique<T>();
    nodo->loc = posicion(origen);
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

NodePtr expr(const NodoParse& nodo);
NodePtr tipo(const NodoParse& nodo);
NodePtr sentencia(const NodoParse& nodo);
NodePtr definicion(const NodoParse& nodo);

// ---------- Tipos ----------

// TIPO > IDENTIFIER | IDSTRING | LBRACKET TIPO RBRACKET
NodePtr tipo(const NodoParse& nodo) {
    auto resultado = nuevo<TypeRef>(nodo);
    if (nodo.hijos.size() == 1) resultado->name = lexema(hijo(nodo, 0));
    else resultado->elem = tipo(hijo(nodo, 1));
    return resultado;
}

// VARTIPADA > IDENTIFIER COLON TIPO
NodePtr variableTipada(const NodoParse& nodo) {
    auto resultado = nuevo<TypedVar>(nodo);
    resultado->name = lexema(hijo(nodo, 0));
    resultado->type = tipo(hijo(nodo, 2));
    return resultado;
}

// ---------- Expresiones ----------

// LITERAL > KW_NONE | KW_TRUE | KW_FALSE | INTEGER | STRING | IDSTRING
NodePtr literal(const NodoParse& nodo) {
    const NodoParse& hoja = hijo(nodo, 0);
    const std::string& clase = hoja.simbolo;
    if (clase == "KW_NONE") return nuevo<NoneLit>(nodo);
    if (clase == "KW_TRUE" || clase == "KW_FALSE") {
        auto resultado = nuevo<BoolLit>(nodo);
        resultado->value = clase == "KW_TRUE";
        return resultado;
    }
    if (clase == "INTEGER") {
        auto resultado = nuevo<IntLit>(nodo);
        resultado->value = lexema(hoja);
        return resultado;
    }
    if (clase == "STRING" || clase == "IDSTRING") {
        auto resultado = nuevo<StrLit>(nodo);
        resultado->value = lexema(hoja);
        return resultado;
    }
    fallo(nodo, "literal desconocido " + clase);
}

// ARGUMENTOS / ELEMENTOS > e | ARGLISTA / ELEMLISTA
NodeList listaExpresiones(const NodoParse& nodo) {
    NodeList resultado;
    if (nodo.hijos.empty()) return resultado;
    for (const NodoParse* elemento : aplanar(hijo(nodo, 0))) resultado.push_back(expr(*elemento));
    return resultado;
}

NodePtr binario(const NodoParse& nodo, const std::string& operador) {
    auto resultado = nuevo<Binary>(nodo);
    resultado->op = operador;
    resultado->left = expr(hijo(nodo, 0));
    resultado->right = expr(hijo(nodo, 2));
    return resultado;
}

NodePtr expr(const NodoParse& nodo) {
    const std::string& simbolo = nodo.simbolo;

    // Un IDENTIFIER suelto (por ejemplo como OBJETIVO de una asignacion).
    if (nodo.esHoja) {
        if (simbolo != "IDENTIFIER") fallo(nodo, "token inesperado en expresion");
        auto resultado = nuevo<Id>(nodo);
        resultado->name = lexema(nodo);
        return resultado;
    }

    // EXPR > ORE | ORE KW_IF ORE KW_ELSE EXPR
    if (simbolo == "EXPR") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<IfExpr>(nodo);
        resultado->then = expr(hijo(nodo, 0));
        resultado->cond = expr(hijo(nodo, 2));
        resultado->otherwise = expr(hijo(nodo, 4));
        return resultado;
    }

    // ORE, ANDE, ARIT, TERM > Y | Y operador Y (asociativos a la izquierda)
    if (simbolo == "ORE" || simbolo == "ANDE" || simbolo == "ARIT" || simbolo == "TERM") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        return binario(nodo, lexema(hijo(nodo, 1)));
    }

    // NOTE > CMPE | KW_NOT NOTE
    if (simbolo == "NOTE") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<Unary>(nodo);
        resultado->op = "not";
        resultado->operand = expr(hijo(nodo, 1));
        return resultado;
    }

    // CMPE > ARIT | ARIT CMPOP ARIT   (CMPOP tiene un unico token hijo)
    if (simbolo == "CMPE") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        return binario(nodo, lexema(hijo(hijo(nodo, 1), 0)));
    }

    // UNARIO > POSTF | MINUS UNARIO
    if (simbolo == "UNARIO") {
        if (nodo.hijos.size() == 1) return expr(hijo(nodo, 0));
        auto resultado = nuevo<Unary>(nodo);
        resultado->op = "-";
        resultado->operand = expr(hijo(nodo, 1));
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
        auto resultado = nuevo<ListLit>(nodo);
        resultado->elems = listaExpresiones(hijo(nodo, 1));
        return resultado;
    }

    if (simbolo == "LITERAL") return literal(nodo);

    // MIEMBRO > POSTF DOT IDENTIFIER
    if (simbolo == "MIEMBRO") {
        auto resultado = nuevo<Member>(nodo);
        resultado->object = expr(hijo(nodo, 0));
        resultado->name = lexema(hijo(nodo, 2));
        return resultado;
    }

    // INDICE > POSTF LBRACKET EXPR RBRACKET
    if (simbolo == "INDICE") {
        auto resultado = nuevo<Index>(nodo);
        resultado->object = expr(hijo(nodo, 0));
        resultado->index = expr(hijo(nodo, 2));
        return resultado;
    }

    // LLAMADA > IDENTIFIER LPAREN ARGUMENTOS RPAREN | MIEMBRO LPAREN ARGUMENTOS RPAREN
    if (simbolo == "LLAMADA") {
        auto resultado = nuevo<Call>(nodo);
        resultado->func = expr(hijo(nodo, 0));
        resultado->args = listaExpresiones(hijo(nodo, 2));
        return resultado;
    }

    fallo(nodo, "no es una expresion");
}

// ---------- Sentencias ----------

// BLOQUE > NEWLINE INDENT SENTLISTA DEDENT   |   SENTLISTA suelta
NodeList sentencias(const NodoParse& lista) {
    NodeList resultado;
    for (const NodoParse* elemento : aplanar(lista)) resultado.push_back(sentencia(*elemento));
    return resultado;
}

NodeList bloque(const NodoParse& nodo) { return sentencias(hijo(nodo, 2)); }

// ASIGNACION > OBJETIVO EQUAL EXPR | OBJETIVO EQUAL ASIGNACION
NodePtr asignacion(const NodoParse& nodo) {
    auto resultado = nuevo<Assign>(nodo);
    const NodoParse* actual = &nodo;
    for (;;) {
        resultado->targets.push_back(expr(hijo(*actual, 0)));
        const NodoParse& derecho = hijo(*actual, 2);
        if (derecho.simbolo == "ASIGNACION") {
            actual = &derecho;
        } else {
            resultado->value = expr(derecho);
            return resultado;
        }
    }
}

// SIMPLE > KW_PASS | KW_RETURN | KW_RETURN EXPR | EXPR | ASIGNACION
NodePtr simple(const NodoParse& nodo) {
    const NodoParse& primero = hijo(nodo, 0);
    if (primero.simbolo == "KW_PASS") return nuevo<Pass>(nodo);
    if (primero.simbolo == "KW_RETURN") {
        auto resultado = nuevo<Return>(nodo);
        if (nodo.hijos.size() == 2) resultado->value = expr(hijo(nodo, 1));
        return resultado;
    }
    if (primero.simbolo == "ASIGNACION") return asignacion(primero);
    auto resultado = nuevo<ExprStmt>(nodo);
    resultado->expr = expr(primero);
    return resultado;
}

// IFSTMT > KW_IF EXPR COLON BLOQUE LISTAELIF OPCIONSINO
NodePtr condicional(const NodoParse& nodo) {
    auto resultado = nuevo<If>(nodo);
    resultado->cond = expr(hijo(nodo, 1));
    resultado->then = bloque(hijo(nodo, 3));

    // LISTAELIF > e | LISTAELIF KW_ELIF EXPR COLON BLOQUE. Recorriendo hacia
    // la izquierda salen del ultimo elif al primero, que es justo el orden
    // para anidarlos desde adentro: cada elif toma como 'else' lo que sigue.
    NodeList resto;
    const NodoParse& opcionSino = hijo(nodo, 5);
    if (!opcionSino.hijos.empty()) resto = bloque(hijo(opcionSino, 2));
    for (const NodoParse* elif = &hijo(nodo, 4); !elif->hijos.empty(); elif = elif->hijos[0].get()) {
        auto anidado = nuevo<If>(hijo(*elif, 1));
        anidado->cond = expr(hijo(*elif, 2));
        anidado->then = bloque(hijo(*elif, 4));
        anidado->otherwise = std::move(resto);
        resto.clear();
        resto.push_back(std::move(anidado));
    }
    resultado->otherwise = std::move(resto);
    return resultado;
}

// SENTENCIA > SIMPLE NEWLINE | IFSTMT | WHILESTMT | FORSTMT
NodePtr sentencia(const NodoParse& nodo) {
    const NodoParse& primero = hijo(nodo, 0);
    if (primero.simbolo == "SIMPLE") return simple(primero);
    if (primero.simbolo == "IFSTMT") return condicional(primero);
    if (primero.simbolo == "WHILESTMT") {
        // KW_WHILE EXPR COLON BLOQUE
        auto resultado = nuevo<While>(primero);
        resultado->cond = expr(hijo(primero, 1));
        resultado->body = bloque(hijo(primero, 3));
        return resultado;
    }
    if (primero.simbolo == "FORSTMT") {
        // KW_FOR IDENTIFIER KW_IN EXPR COLON BLOQUE
        auto resultado = nuevo<For>(primero);
        resultado->var = lexema(hijo(primero, 1));
        resultado->iter = expr(hijo(primero, 3));
        resultado->body = bloque(hijo(primero, 5));
        return resultado;
    }
    fallo(nodo, "sentencia desconocida " + primero.simbolo);
}

// ---------- Definiciones ----------

// VARDEF > VARTIPADA EQUAL LITERAL NEWLINE
NodePtr definicionVariable(const NodoParse& nodo) {
    auto resultado = nuevo<VarDef>(nodo);
    resultado->var = variableTipada(hijo(nodo, 0));
    resultado->value = literal(hijo(nodo, 2));
    return resultado;
}

// FUNCDEF > KW_DEF IDENTIFIER LPAREN PARAMETROS RPAREN RETTIPO COLON NEWLINE
//           INDENT CUERPOFUNC DEDENT
NodePtr definicionFuncion(const NodoParse& nodo) {
    auto resultado = nuevo<FuncDef>(nodo);
    resultado->name = lexema(hijo(nodo, 1));

    const NodoParse& parametros = hijo(nodo, 3); // e | LISTAPARAM
    if (!parametros.hijos.empty())
        for (const NodoParse* parametro : aplanar(hijo(parametros, 0)))
            resultado->params.push_back(variableTipada(*parametro));

    const NodoParse& retorno = hijo(nodo, 5); // e | ARROW TIPO
    if (!retorno.hijos.empty()) resultado->ret = tipo(hijo(retorno, 1));

    // CUERPOFUNC > SENTLISTA | DECLARACIONES SENTLISTA
    const NodoParse& cuerpo = hijo(nodo, 9);
    if (cuerpo.hijos.size() == 2) {
        for (const NodoParse* unaDecl : aplanar(hijo(cuerpo, 0))) {
            // UNADECL > GLOBALDECL | NONLOCALDECL | VARDEF | FUNCDEF
            const NodoParse& declaracion = hijo(*unaDecl, 0);
            if (declaracion.simbolo == "GLOBALDECL") {
                auto global = nuevo<GlobalDecl>(declaracion);
                global->name = lexema(hijo(declaracion, 1));
                resultado->decls.push_back(std::move(global));
            } else if (declaracion.simbolo == "NONLOCALDECL") {
                auto nonlocal = nuevo<NonlocalDecl>(declaracion);
                nonlocal->name = lexema(hijo(declaracion, 1));
                resultado->decls.push_back(std::move(nonlocal));
            } else {
                resultado->decls.push_back(definicion(declaracion));
            }
        }
    }
    resultado->body = sentencias(hijo(cuerpo, cuerpo.hijos.size() - 1));
    return resultado;
}

// CLASEDEF > KW_CLASS IDENTIFIER LPAREN IDENTIFIER RPAREN COLON NEWLINE INDENT
//            CUERPOCLASE DEDENT
NodePtr definicionClase(const NodoParse& nodo) {
    auto resultado = nuevo<ClassDef>(nodo);
    resultado->name = lexema(hijo(nodo, 1));
    resultado->superclass = lexema(hijo(nodo, 3));

    // CUERPOCLASE > KW_PASS NEWLINE | LISTADECL ; DECLCLASE > VARDEF | FUNCDEF
    const NodoParse& cuerpo = hijo(nodo, 8);
    if (cuerpo.simbolo == "CUERPOCLASE" && !hijo(cuerpo, 0).esHoja)
        for (const NodoParse* declaracion : aplanar(hijo(cuerpo, 0)))
            resultado->members.push_back(definicion(hijo(*declaracion, 0)));
    return resultado;
}

// VARDEF | FUNCDEF | CLASEDEF
NodePtr definicion(const NodoParse& nodo) {
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

std::unique_ptr<Program> construirAST(const NodoParse& raiz) {
    if (raiz.simbolo != "PROGRAMA") fallo(raiz, "la raiz debe ser PROGRAMA");
    if (contieneError(raiz)) fallo(raiz, "el arbol contiene nodos de error");

    // PROGRAMA > e | DEFINICIONES | SENTLISTA | DEFINICIONES SENTLISTA
    auto programa = nuevo<Program>(raiz);
    for (const auto& parte : raiz.hijos) {
        if (parte->simbolo == "DEFINICIONES") {
            for (const NodoParse* unaDef : aplanar(*parte)) // UNADEF > VARDEF | FUNCDEF | CLASEDEF
                programa->defs.push_back(definicion(hijo(*unaDef, 0)));
        } else {
            programa->stmts = sentencias(*parte);
        }
    }
    return programa;
}
