#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../lr1/generador.hpp"
#include "../Scanner/lexer.hpp"
#include "arbol.hpp"

// Parser LR(1) de ChocoPy.
//
// Conecta el Scanner con la tabla generada por lr1/generador: pide tokens al
// Lexer con gettoken() y los consume con el algoritmo shift/reduce. El nombre
// de cada terminal de la gramatica es el mismo texto que devuelve
// tokenTypeToString(), salvo END_OF_FILE que se mapea al "$" del generador.
//
// Recuperacion de errores en modo panico. Se elige de antemano un conjunto N
// de no terminales de panico (en ChocoPy: sentencias y definiciones, que
// tienen delimitador propio). Cuando Action[estado, token] esta vacia:
//   1. Se desapilan estados hasta que el tope tenga Goto con algun A de N.
//   2. Se consume entrada hasta llegar a un token de FOLLOW(A).
//   3. Se inserta A en la pila.
//   4. Se apila Goto[tope, A].
//   5. Se finge que se vio A y se continua.
// Se aborta si el paso 1 vacia la pila o si el paso 2 llega a $ sin encontrar
// FOLLOW(A). Si dos errores seguidos ocurren en la misma posicion de entrada
// (la recuperacion no avanzo), se descarta un token antes de repetir, para no
// ciclar; si ese token es $ tambien se aborta.
// No terminales de panico de ChocoPy: sentencias y definiciones, que tienen
// delimitador propio. Una sentencia o definicion mala se descarta y el
// analisis continua con la siguiente. Se usan unidades chicas: con un N grueso
// (por ejemplo solo PROGRAMA) el parser consumiria hasta el final y ocultaria
// los errores posteriores.
extern const std::vector<std::string> NO_TERMINALES_PANICO_CHOCOPY;

class Parser {
public:
    // Recibe el Lexer ya abierto sobre el archivo fuente, la gramatica y su
    // tabla LR(1). 'noTerminalesPanico' es el conjunto N; si esta vacio no hay
    // recuperacion y el analisis se detiene en el primer error. Lanza
    // std::runtime_error si la tabla tiene conflictos o si algun elemento de N
    // no es un no terminal de la gramatica.
    Parser(Lexer& lexer, const Gramatica& gramatica, const TablaLR1& tabla,
           const std::vector<std::string>& noTerminalesPanico = {});

    // Analiza todo el programa. Devuelve true si fue aceptado sin errores
    // lexicos ni sintacticos (un programa que solo se acepta gracias a la
    // recuperacion devuelve false). Con verbose = true imprime cada
    // shift/reduce y cada paso de la recuperacion.
    bool parsear(bool verbose = false);

    // Cantidad de errores sintacticos detectados.
    int errorCount() const { return errors; }

    // Raiz (simbolo PROGRAMA) del arbol de derivacion del ultimo parsear(), o
    // nullptr si el analisis no llego a aceptar. Entrega la propiedad: solo
    // sirve la primera llamada.
    std::unique_ptr<NodoParse> tomarArbol() { return std::move(raiz); }

private:
    enum class TipoAccion { SHIFT, REDUCE, ACCEPT };

    struct Accion {
        TipoAccion tipo;
        int valor; // estado destino (SHIFT) o indice de produccion (REDUCE)
    };

    Lexer& lexer;
    const Gramatica& gramatica;
    const TablaLR1& tabla;

    // Tabla action ya decodificada: (estado, terminal) -> accion.
    std::map<std::pair<int, std::string>, Accion> acciones;

    // Conjunto N de la recuperacion (en orden de prioridad) y FOLLOW de la gramatica.
    std::vector<std::string> panico;
    ConjuntosFollow follow;

    int errors = 0;

    // Arbol de derivacion. 'nodos' corre en paralelo a la pila de estados: hay
    // un nodo por cada estado salvo el 0 (nodos.size() == pila.size() - 1).
    std::vector<std::unique_ptr<NodoParse>> nodos;
    std::unique_ptr<NodoParse> raiz;

    // Posicion de entrada = cantidad de tokens leidos del Lexer hasta ahora.
    // 'posRecuperacion' es la posicion en la que termino la ultima
    // recuperacion (-1 si no hubo); un error en esa misma posicion significa
    // que no se avanzo nada.
    long posicion = 0;
    long posRecuperacion = -1;

    // Nombre del terminal de la gramatica que corresponde a un token.
    static std::string simboloDe(const Token& token);

    // Lee el siguiente token del Lexer saltando los errores lexicos (ya
    // fueron reportados por el scanner; se omiten para no encadenar errores
    // sintacticos falsos).
    void avanzar(Token& actual);

    // Contexto para explicar los errores: ultimo token aceptado, '(' y '['
    // sin cerrar, y cuantos errores lexicos habia cuando se acepto ese token.
    Token anterior{};
    bool hayAnterior = false;
    std::vector<Token> abiertos;
    int erroresLexicosAlAceptar = 0;
    int lineaUltimoError = -1; // linea del ultimo error sintactico reportado

    // Reporta un error sintactico en lenguaje del usuario (ver mensajes.hpp),
    // con la linea de codigo y el subrayado. Incrementa 'errors'. No reporta
    // nada si hubo un error lexico desde el ultimo token aceptado: el scanner
    // ya lo informo y este error es su consecuencia (por ejemplo, el token
    // que falta porque se descarto un '@'). Con 'verbose' agrega, ademas, el
    // detalle tecnico (estado y terminales esperados).
    void reportError(const Token& token, int estado, bool verbose);

    // Imprime un mensaje sobre el codigo, en la posicion y con el subrayado dados.
    void imprimirSobreCodigo(const std::string& tipo, const std::string& mensaje,
                             Location posicion, int largo) const;

    // Esquina izquierda: esquinaIzq[A] = simbolos X con A =>* X ..., es decir,
    // los que pueden iniciar una derivacion de A (incluye terminales).
    std::map<std::string, std::set<std::string>> esquinaIzq;

    // Elige el A de N con Goto en 'estado'. 'primerSimbolo' es el primer
    // simbolo de la construccion que fallo (el ultimo nodo desapilado, "" si
    // no se desapilo nada): solo se consideran los A que pueden empezar con
    // el, para que un error en la cabecera de una def se resincronice como
    // FUNCDEF y no como SENTENCIA (despues de una sentencia ya no se aceptan
    // definiciones). Entre los que queden prefiere el que ya tiene el token
    // actual en su FOLLOW (no hay que descartar nada). Devuelve "" si ninguno.
    std::string elegirNoTerminal(int estado, const Token& token, const std::string& primerSimbolo) const;

    // Pasos 1 a 5 de la recuperacion. Modifica la pila y consume tokens.
    // Devuelve false si hay que abortar el analisis.
    bool recuperar(std::vector<int>& pila, Token& actual, bool verbose);
};

#endif
