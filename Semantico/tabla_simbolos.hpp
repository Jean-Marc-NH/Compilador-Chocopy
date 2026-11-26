#ifndef SEMANTICO_TABLA_SIMBOLOS_HPP
#define SEMANTICO_TABLA_SIMBOLOS_HPP

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "../Scanner/tokens.hpp"
#include "tipos.hpp"

// Tabla de simbolos de ChocoPy: una tabla (Ambito) por cada scope, unidas por
// un puntero a la tabla padre (el scope que la encierra).
//
//   global                       print, len, input, object, int, ..., f, x
//   ├── clase animal             makes_noise, sound
//   │   └── metodo animal.sound  self
//   └── funcion f                n, total, g
//       └── funcion f.g          ...
//
// - Declarar un nombre lo inserta SOLO en la tabla actual (insertar).
// - Usar un nombre lo busca en la tabla actual y despues en los padres
//   (buscar). Por eso una variable local con el mismo nombre que una global
//   la oculta (shadowing): se encuentra primero.
// - Los scopes de clase se saltan al buscar desde un metodo: en ChocoPy (como
//   en Python) los atributos solo se ven con self.atributo (sec. 3.1.2).
// - 'global x' y 'nonlocal x' agregan a la tabla de la funcion una entrada
//   que apunta a la variable de afuera; buscar() la sigue hasta la variable
//   real, y el analizador la acepta como destino de una asignacion.
// - Las tablas no se borran al salir del scope: quedan como hijos de su padre
//   para poder imprimirlas al final (--tablas).

enum class TipoSimbolo { Variable, Parametro, Funcion, Clase, Global, Nonlocal };
enum class TipoAmbito { Global, Funcion, Clase };

std::string tipoSimboloATexto(TipoSimbolo tipo); // "variable", "parametro", ...

struct Simbolo {
    std::string nombre;
    TipoSimbolo categoria = TipoSimbolo::Variable;
    TipoValor tipo;          // variables y parametros (y global/nonlocal: el de la variable de afuera)
    FirmaFuncion firma;      // funciones y metodos
    Posicion pos;            // donde se declaro (linea 0 = predefinido)
    bool predefinido = false;
};

class Ambito {
public:
    Ambito(std::string nombre, TipoAmbito tipo, Ambito* padre);

    const std::string& nombre() const { return nombreAmbito; }
    TipoAmbito tipo() const { return tipoAmbito; }
    Ambito* padre() const { return ambitoPadre; }
    Ambito& global();

    // Crea la tabla de un scope hijo (una funcion, clase o metodo) y la
    // devuelve. La tabla hija vive mientras viva esta.
    Ambito& crearHijo(const std::string& nombre, TipoAmbito tipo);

    // Inserta en ESTA tabla. Devuelve false (y no inserta) si el nombre ya
    // estaba declarado en esta misma tabla: es una redeclaracion.
    bool insertar(const Simbolo& simbolo);

    // Busca solo en esta tabla (nullptr si no esta).
    Simbolo* buscarLocal(const std::string& nombre);

    // LookUp: busca en esta tabla y luego por la cadena de padres, saltando
    // los scopes de clase. Si encuentra una entrada 'global'/'nonlocal' la
    // sigue hasta la variable real. 'donde' recibe la tabla en que se encontro
    // la declaracion. nullptr si el nombre no esta declarado en ningun scope.
    Simbolo* buscar(const std::string& nombre, Ambito** donde = nullptr);

    // Imprime esta tabla y las de sus hijos, con sangria segun la profundidad.
    // Con 'conPredefinidos' en false no muestra print, len, int, str, ...
    void imprimir(std::ostream& salida, bool conPredefinidos = false, int nivel = 0) const;

private:
    std::string nombreAmbito;
    TipoAmbito tipoAmbito;
    Ambito* ambitoPadre;
    std::map<std::string, Simbolo> simbolos;      // busqueda por nombre
    std::vector<std::string> orden;               // orden de declaracion, para imprimir
    std::vector<std::unique_ptr<Ambito>> hijos;
};

#endif
