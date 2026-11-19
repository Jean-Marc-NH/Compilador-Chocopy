#include "parser.hpp"
#include "mensajes.hpp"
#include "Comun/diagnostico.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>

const std::vector<std::string> NO_TERMINALES_PANICO_CHOCOPY = {"SENTENCIA", "VARDEF", "FUNCDEF", "CLASEDEF"};

AnalizadorSintactico::AnalizadorSintactico(AnalizadorLexico& scanner, const Gramatica& gramatica,
                                           const TablaLR1& tabla,
                                           const std::vector<std::string>& noTerminalesPanico)
    : scanner(scanner), gramatica(gramatica), tabla(tabla), panico(noTerminalesPanico) {
    if (!tabla.conflictos.empty())
        throw std::runtime_error("La gramatica tiene conflictos; no es LR(1).");

    // Las celdas de ACTION guardan texto ("shift 3", "reduce 7", "accept");
    // lo decodificamos una sola vez para no repetirlo en cada token.
    for (const auto& celda : tabla.tablaAccion) {
        const std::string& texto = *celda.second.begin();
        if (texto == "accept") {
            acciones[celda.first] = {TipoAccion::ACCEPT, 0};
        } else if (texto.compare(0, 6, "shift ") == 0) {
            acciones[celda.first] = {TipoAccion::SHIFT, std::stoi(texto.substr(6))};
        } else {
            acciones[celda.first] = {TipoAccion::REDUCE, std::stoi(texto.substr(7))};
        }
    }

    for (const auto& simbolo : panico)
        if (!gramatica.noTerminales.count(simbolo))
            throw std::runtime_error("No terminal de panico desconocido: " + simbolo);

    if (!panico.empty()) {
        follow = calcularFollow(gramatica, calcularFirst(gramatica));
        // Esquina izquierda: primer simbolo de cada produccion, cerrado
        // transitivamente a traves de los no terminales.
        for (const auto& produccion : gramatica.producciones)
            if (!produccion.derecho.empty()) esquinaIzq[produccion.izquierdo].insert(produccion.derecho.front());
        for (bool cambio = true; cambio;) {
            cambio = false;
            for (auto& fila : esquinaIzq)
                for (const auto& simbolo : std::set<std::string>(fila.second)) {
                    auto interno = esquinaIzq.find(simbolo);
                    if (interno == esquinaIzq.end()) continue;
                    for (const auto& otro : interno->second)
                        if (fila.second.insert(otro).second) cambio = true;
                }
        }
    }
}

std::string AnalizadorSintactico::simboloDe(const Token& token) {
    if (token.tipo == TipoToken::END_OF_FILE) return "$";
    return tipoTokenATexto(token.tipo);
}

void AnalizadorSintactico::avanzar(Token& actual) {
    do {
        actual = scanner.siguienteToken();
        ++posicion;
    } while (actual.tipo == TipoToken::UNKNOWN_ERROR);
}

void AnalizadorSintactico::imprimirSobreCodigo(const std::string& tipo, const std::string& mensaje,
                                               Posicion donde, int largo) const {
    std::string texto;
    bool hayTexto = scanner.lineaDeCodigo(donde.linea, texto);
    imprimirDiagnostico(std::cerr, scanner.nombreArchivo(), donde.linea, donde.columna, largo, tipo,
                        mensaje, hayTexto ? &texto : nullptr);
}

void AnalizadorSintactico::reportarError(const Token& token, int estado, bool detallado) {
    // Un error lexico desde el ultimo token aceptado ya se informo; este es su eco.
    if (scanner.cantidadErrores() > erroresLexicosAlAceptar) return;
    // Un DEDENT inesperado despues de un error lexico es el eco de una sangria inconsistente.
    if (token.tipo == TipoToken::DEDENT && scanner.cantidadErrores() > 0) return;
    // Un segundo error en la misma linea suele ser consecuencia del primero
    // (la recuperacion resincroniza dentro de la linea): se omite.
    if (token.posicion.linea == lineaUltimoError) return;
    lineaUltimoError = token.posicion.linea;

    ++errores;
    std::set<std::string> esperados;
    for (const auto& celda : acciones)
        if (celda.first.first == estado) esperados.insert(celda.first.second);

    ExplicacionError explicacion = explicarErrorSintactico(token, hayAnterior ? &anterior : nullptr,
                                                           esperados, abiertos);
    // Si falta algo al final de la linea, senalamos justo despues del ultimo
    // token y no en el salto de linea (que puede estar despues de un comentario).
    Posicion donde = token.posicion;
    if (token.tipo == TipoToken::NEWLINE && hayAnterior && anterior.posicion.linea == token.posicion.linea)
        donde = posicionDespuesDe(anterior);
    imprimirSobreCodigo("error", explicacion.mensaje, donde, explicacion.largo);
    if (explicacion.hayNota)
        imprimirSobreCodigo("nota", explicacion.notaMensaje, explicacion.notaPos, explicacion.notaLargo);

    if (detallado) {
        std::cout << "DEBUG PARSE - error en el estado " << estado << " con " << tokenATexto(token)
                  << "; terminales esperados:";
        for (const auto& nombre : esperados) std::cout << ' ' << nombre;
        std::cout << "\n";
    }
}

std::string AnalizadorSintactico::elegirNoTerminal(int estado, const Token& token,
                                                   const std::string& primerSimbolo) const {
    std::vector<std::string> candidatos;
    for (const auto& candidato : panico)
        if (tabla.tablaGoto.count({estado, candidato})) candidatos.push_back(candidato);

    // Si algunos pueden empezar con el primer simbolo que fallo, solo esos.
    if (!primerSimbolo.empty()) {
        std::vector<std::string> compatibles;
        for (const auto& candidato : candidatos) {
            auto esquina = esquinaIzq.find(candidato);
            if (candidato == primerSimbolo || (esquina != esquinaIzq.end() && esquina->second.count(primerSimbolo)))
                compatibles.push_back(candidato);
        }
        if (!compatibles.empty()) candidatos = compatibles;
    }

    const std::string simbolo = simboloDe(token);
    for (const auto& candidato : candidatos)
        if (follow.at(candidato).count(simbolo)) return candidato;
    return candidatos.empty() ? "" : candidatos.front();
}

bool AnalizadorSintactico::recuperar(std::vector<int>& pila, Token& actual, bool detallado) {
    // Guardia contra ciclos: si el error ocurre justo donde termino la
    // recuperacion anterior, no se avanzo nada; se descarta un token antes de
    // repetir. Descartar $ significa que ya no queda entrada.
    if (posicion == posRecuperacion) {
        if (actual.tipo == TipoToken::END_OF_FILE) {
            imprimirErrorGeneral(std::cerr, scanner.nombreArchivo(),
                                 "el archivo termina y no se pudo continuar despues del error anterior");
            return false;
        }
        if (detallado)
            std::cout << "DEBUG PARSE - recuperacion sin avance: se descarta "
                      << tokenATexto(actual) << "\n";
        avanzar(actual);
    }

    // Paso 1: desapilar hasta un estado con GOTO sobre algun A de N.
    std::string elegido;
    std::string primerSimbolo; // lo ultimo que se desapila es lo primero de la construccion que fallo
    int desapilados = 0;
    for (;;) {
        elegido = elegirNoTerminal(pila.back(), actual, primerSimbolo);
        if (!elegido.empty()) break;
        if (pila.size() == 1) {
            imprimirErrorGeneral(std::cerr, scanner.nombreArchivo(),
                                 "no se pudo continuar el analisis despues del error anterior");
            return false;
        }
        primerSimbolo = nodos.back()->simbolo;
        pila.pop_back();
        nodos.pop_back();
        ++desapilados;
    }

    // Paso 2: consumir entrada hasta un token de FOLLOW(A). Dos ajustes para
    // ChocoPy, porque FOLLOW de una sentencia o definicion es muy permisivo
    // (casi cualquier identificador o literal puede empezar otra sentencia):
    //  - Sangria: un INDENT que se descarta obliga a descartar tambien su
    //    DEDENT, y mientras eso este pendiente no se busca el sincronizador;
    //    si no, el bloque saltado dejaria un DEDENT sin su INDENT.
    //  - Inicio de linea: si ya se descarto algun token, el sincronizador debe
    //    ser el primer token de una linea logica (tras NEWLINE, INDENT o
    //    DEDENT). Las sentencias y definiciones siempre empiezan ahi; asi no
    //    se resincroniza en medio de una cabecera como "def f(a: int, ) -> int:"
    //    y no se encadenan errores falsos.
    const std::set<std::string>& sincronizadores = follow.at(elegido);
    int descartados = 0;
    int profundidad = 0;
    bool inicioLinea = false;
    for (;;) {
        if (actual.tipo == TipoToken::END_OF_FILE) {
            if (sincronizadores.count("$")) break;
            imprimirErrorGeneral(std::cerr, scanner.nombreArchivo(),
                                 "el archivo termina y no se pudo continuar despues del error anterior");
            if (detallado)
                std::cout << "DEBUG PARSE - fin de archivo sin encontrar FOLLOW(" << elegido << ")\n";
            return false;
        }
        if (profundidad == 0 && sincronizadores.count(simboloDe(actual)) &&
            (descartados == 0 || inicioLinea))
            break;
        if (actual.tipo == TipoToken::INDENT) ++profundidad;
        else if (actual.tipo == TipoToken::DEDENT && profundidad > 0) --profundidad;
        inicioLinea = actual.tipo == TipoToken::NEWLINE || actual.tipo == TipoToken::INDENT ||
                      actual.tipo == TipoToken::DEDENT;
        avanzar(actual);
        ++descartados;
    }

    // Pasos 3 a 5: apilar A y su GOTO; el parser sigue como si hubiera visto A.
    pila.push_back(tabla.tablaGoto.at({pila.back(), elegido}));
    auto nodoError = std::make_unique<NodoParse>();
    nodoError->simbolo = elegido;
    nodoError->esError = true;
    nodos.push_back(std::move(nodoError));
    posRecuperacion = posicion;
    abiertos.clear(); // se descartaron tokens: los '(' y '[' pendientes ya no cuentan
    hayAnterior = false;
    if (detallado)
        std::cout << "DEBUG PARSE - recuperacion: desapila " << desapilados
                  << " estados, descarta " << descartados << " tokens, resincroniza con "
                  << elegido << " -> estado " << pila.back() << "\n";
    return true;
}

bool AnalizadorSintactico::parsear(bool detallado) {
    std::vector<int> pila{0};
    nodos.clear();
    raiz.reset();
    abiertos.clear();
    hayAnterior = false;
    erroresLexicosAlAceptar = 0;
    lineaUltimoError = -1;
    Token actual;
    avanzar(actual);

    for (;;) {
        int estado = pila.back();
        auto encontrada = acciones.find({estado, simboloDe(actual)});
        if (encontrada == acciones.end()) {
            reportarError(actual, estado, detallado);
            if (panico.empty() || !recuperar(pila, actual, detallado)) return false;
            continue;
        }

        const Accion& accion = encontrada->second;
        switch (accion.tipo) {
            case TipoAccion::SHIFT: {
                if (detallado)
                    std::cout << "DEBUG PARSE - shift " << tokenATexto(actual)
                              << " -> estado " << accion.valor << "\n";
                pila.push_back(accion.valor);

                auto hoja = std::make_unique<NodoParse>();
                hoja->simbolo = simboloDe(actual);
                hoja->esHoja = true;
                hoja->token = actual;
                nodos.push_back(std::move(hoja));

                // Contexto para explicar un posible error en el token siguiente.
                if (actual.tipo == TipoToken::LPAREN || actual.tipo == TipoToken::LBRACKET)
                    abiertos.push_back(actual);
                else if (!abiertos.empty() &&
                         ((actual.tipo == TipoToken::RPAREN && abiertos.back().tipo == TipoToken::LPAREN) ||
                          (actual.tipo == TipoToken::RBRACKET && abiertos.back().tipo == TipoToken::LBRACKET)))
                    abiertos.pop_back();
                anterior = actual;
                hayAnterior = true;
                erroresLexicosAlAceptar = scanner.cantidadErrores();
                avanzar(actual);
                break;
            }

            case TipoAccion::REDUCE: {
                const Produccion& produccion = gramatica.producciones[accion.valor];
                if (detallado) {
                    std::cout << "DEBUG PARSE - reduce " << produccion.izquierdo << " >";
                    if (produccion.derecho.empty()) std::cout << " e";
                    for (const auto& simbolo : produccion.derecho) std::cout << ' ' << simbolo;
                    std::cout << "\n";
                }
                pila.resize(pila.size() - produccion.derecho.size());

                // Los ultimos |derecho| nodos pasan a ser hijos del nuevo, en orden.
                auto nuevo = std::make_unique<NodoParse>();
                nuevo->simbolo = produccion.izquierdo;
                nuevo->produccion = accion.valor;
                auto primero = nodos.end() - static_cast<std::ptrdiff_t>(produccion.derecho.size());
                for (auto it = primero; it != nodos.end(); ++it) nuevo->hijos.push_back(std::move(*it));
                nodos.erase(primero, nodos.end());
                nodos.push_back(std::move(nuevo));

                auto destino = tabla.tablaGoto.find({pila.back(), produccion.izquierdo});
                if (destino == tabla.tablaGoto.end())
                    throw std::runtime_error("Tabla GOTO incompleta para " + produccion.izquierdo);
                pila.push_back(destino->second);
                break;
            }

            case TipoAccion::ACCEPT:
                // En la pila queda solo el nodo PROGRAMA (la regla aumentada no se reduce).
                if (nodos.size() == 1) raiz = std::move(nodos.back());
                return errores == 0 && scanner.cantidadErrores() == 0;
        }
    }
}
