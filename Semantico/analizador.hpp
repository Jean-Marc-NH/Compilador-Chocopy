#ifndef SEMANTICO_ANALIZADOR_HPP
#define SEMANTICO_ANALIZADOR_HPP

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "../AST/nodos.hpp"
#include "tabla_simbolos.hpp"
#include "tipos.hpp"

// Analisis semantico de ChocoPy (Language Reference, sec. 5): recorre el AST,
// arma la tabla de simbolos de cada scope y revisa los tipos.
//
// Trabaja en tres pasos sobre el programa:
//
//   1. Registra las clases en la jerarquia, en orden (la superclase tiene que
//      estar definida antes). Asi las anotaciones de tipo pueden nombrar una
//      clase que se define mas abajo.
//   2. Declara en la tabla global, en orden, las clases (con sus atributos y
//      metodos), las funciones (con su firma) y las variables globales.
//   3. Revisa los cuerpos: valores iniciales, funciones y metodos (cada uno
//      con su propia tabla hija) y las sentencias del nivel superior.
//
// A cada expresion le guarda su tipo en Nodo::tipoInferido, y a cada
// VarTipada el tipo declarado, para que el AST impreso los muestre.
//
// Los errores se imprimen en std::cerr con el mismo formato que los del
// scanner y el parser (archivo:linea:columna, la linea de codigo y un ^). El
// analisis NO se detiene en el primer error: una expresion con error recibe el
// tipo "object" y se sigue revisando el resto del programa.
//
// Errores que detecta (entre parentesis, la seccion del Language Reference):
//   Declaraciones y scopes
//     - nombre declarado dos veces en el mismo scope (5.2)
//     - nombre de clase usado como variable, funcion o parametro (5.2)
//     - tipo de una anotacion que no existe (2.4)
//     - superclase no definida antes o que es int, bool o str (3.1.2)
//     - atributo o metodo que redefine un atributo heredado (3.1.2)
//     - metodo que redefine a otro con una firma distinta (3.1.2)
//     - metodo cuyo primer parametro no es de la clase (3.1.2)
//     - 'global x' que no es una variable global, 'nonlocal x' que no es una
//       variable de una funcion que encierre a esta (3.1.3, 3.1.4)
//   Sentencias
//     - uso de un nombre no declarado en ningun scope (5.2)
//     - asignar a una variable de afuera sin 'global'/'nonlocal' (5.2)
//     - tipo incompatible en una asignacion o en un valor inicial (5.2)
//     - condicion de if / elif / while que no es bool (5.2)
//     - for sobre algo que no es una lista ni un str (5.2)
//     - return fuera de una funcion, o con un tipo distinto al declarado (5.2)
//     - funcion que no devuelve nada en algun camino y debia hacerlo (5.2)
//     - asignar a un caracter de un str (los str no se pueden modificar)
//   Expresiones
//     - operadores con tipos invalidos: 1 + "a", not 3, "a" < "b", 1 is 2 (5.2)
//     - cantidad o tipo de los argumentos de una llamada, metodo o constructor
//     - llamar a algo que no es una funcion; usar una funcion sin llamarla
//     - atributo o metodo que no existe en la clase; indice que no es int
class AnalizadorSemantico {
public:
    // 'archivo' es la ruta del .py: se usa en los mensajes y para mostrar la
    // linea de codigo donde esta cada error.
    explicit AnalizadorSemantico(const std::string& archivo);

    // Analiza el programa y devuelve la cantidad de errores semanticos.
    int analizar(Programa& programa);
    int cantidadErrores() const { return errores; }

    // Tabla del scope global; sus hijos son las tablas de cada clase, funcion
    // y metodo. Se puede imprimir con tablaGlobal().imprimir(std::cout).
    const Ambito& tablaGlobal() const { return global; }
    const JerarquiaClases& jerarquiaClases() const { return jerarquia; }

private:
    std::string archivo;
    std::vector<std::string> lineas; // codigo fuente, para mostrar en los errores
    int errores = 0;

    JerarquiaClases jerarquia;
    Ambito global{"global", TipoAmbito::Global, nullptr};

    // Contexto de lo que se esta revisando ahora.
    Ambito* actual = &global;
    bool dentroDeFuncion = false;
    TipoValor retornoActual;

    std::map<const DefFuncion*, FirmaFuncion> firmas;   // firma de cada funcion y metodo
    std::map<const DefClase*, Ambito*> tablaDeClase;    // tabla de cada clase valida

    // --- Mensajes (se acumulan y se imprimen al final ordenados por linea) ---
    std::vector<std::pair<Posicion, std::string>> pendientes;
    void error(const Nodo& nodo, const std::string& mensaje);
    void error(Posicion pos, int largo, const std::string& mensaje);
    void nota(Posicion pos, int largo, const std::string& mensaje);
    void imprimirErrores();

    // --- Pasos 1 y 2: declaraciones ---
    void declararPredefinidos();
    void registrarClase(const DefClase& clase);
    void declararGlobal(Nodo& definicion);
    void declararMiembrosDeClase(DefClase& clase);
    bool declarar(const Simbolo& simbolo, const Nodo& donde);
    TipoValor tipoAnotado(const Nodo* anotacion, bool reportar);
    FirmaFuncion construirFirma(const DefFuncion& funcion, const std::string& clase);

    // --- Paso 3: cuerpos ---
    void revisarDefinicionGlobal(Nodo& definicion);
    void revisarValorInicial(DefVariable& variable);
    void revisarFuncion(DefFuncion& funcion, const std::string& nombreTabla);
    void declararEnFuncion(Nodo& declaracion, const std::string& nombreTabla);
    void revisarBloque(ListaNodos& sentencias);
    void revisarSentencia(Nodo& sentencia);
    void revisarCondicion(Nodo& condicion, const char* sentencia);
    bool siempreRetorna(const ListaNodos& sentencias) const;

    // Tipo del destino de una asignacion (x, obj.attr, lista[i]); "" si tiene
    // un error que ya se reporto.
    TipoValor tipoDestino(Nodo& destino);
    TipoValor tipoVariableAsignable(const std::string& nombre, const Nodo& donde);

    // --- Expresiones ---
    TipoValor tipoDe(Nodo& expresion); // calcula el tipo y lo guarda en el nodo
    TipoValor calcularTipo(Nodo& expresion);
    TipoValor tipoOpBinaria(OpBinaria& operacion);
    TipoValor tipoLlamada(Llamada& llamada);
    void revisarArgumentos(Llamada& llamada, const FirmaFuncion& firma, std::size_t desde,
                           const std::string& quien);
};

#endif
