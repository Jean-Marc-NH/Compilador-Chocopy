#ifndef SEMANTICO_TIPOS_HPP
#define SEMANTICO_TIPOS_HPP

#include <map>
#include <string>
#include <vector>

// Tipos de ChocoPy para el analisis semantico (Language Reference, sec. 2.4,
// 2.5 y 5). Un tipo se guarda como texto, igual a como lo escribe el
// programador, para que se pueda imprimir tal cual en los mensajes de error y
// en el AST:
//
//   "int", "bool", "str", "object"   tipos predefinidos
//   "animal"                          una clase del programa
//   "[int]", "[[str]]"                listas
//   "<None>"                          tipo del literal None
//   "<Empty>"                         tipo de la lista vacia []
//
// Cuando una expresion tiene un error se le da el tipo interno "<error>". Ese
// tipo es compatible con todo, asi un solo error (por ejemplo una variable no
// declarada) no genera otros mensajes en las expresiones que la contienen.
using TipoValor = std::string;

namespace tipos {
extern const TipoValor INT, BOOL, STR, OBJECT, NONE, EMPTY, ERROR;

bool esLista(const TipoValor& tipo);                 // "[...]"
TipoValor elementoDe(const TipoValor& lista);        // "[int]" -> "int"
TipoValor listaDe(const TipoValor& elemento);        // "int"   -> "[int]"
bool esPrimitivo(const TipoValor& tipo);             // int, bool o str (no aceptan None)
} // namespace tipos

// Firma de una funcion o metodo: tipos de los parametros (en los metodos el
// primero es self) y tipo de retorno ("<None>" si no se declara '->').
struct FirmaFuncion {
    std::vector<std::string> nombresParametros;
    std::vector<TipoValor> parametros;
    TipoValor retorno = tipos::NONE;
};

// "(int, str) -> bool": para los mensajes y para imprimir las tablas.
std::string firmaATexto(const FirmaFuncion& firma);

// Lo que se sabe de una clase: su superclase y TODOS sus atributos y metodos,
// incluidos los heredados (sec. 3.1.2: los atributos y metodos se heredan).
struct InfoClase {
    std::string nombre;
    std::string superclase; // vacio solo en object
    std::map<std::string, TipoValor> atributos;
    std::map<std::string, FirmaFuncion> metodos;
    std::map<std::string, std::string> claseDeMetodo; // metodo -> clase que lo define
};

// Jerarquia de clases del programa. Empieza con las clases predefinidas
// (object, int, bool, str) y responde las relaciones entre tipos de la sec. 2.4:
//
//   esSubtipo(a, b)    a <= b   (a es b, a hereda de b o b es object)
//   esAsignable(a, b)  a <=a b  (un valor de tipo a se puede guardar en b)
//   unir(a, b)         a |_| b  (menor tipo que cubre a los dos, "join")
class JerarquiaClases {
public:
    JerarquiaClases();

    bool existe(const std::string& clase) const;
    InfoClase* buscar(const std::string& clase);
    const InfoClase* buscar(const std::string& clase) const;
    // Registra una clase nueva que hereda de 'superclase' (que ya debe existir)
    // copiando los atributos y metodos de la superclase.
    InfoClase& agregar(const std::string& clase, const std::string& superclase);

    // true si 'tipo' es un tipo que se puede escribir en una anotacion: una
    // clase que existe o una lista de un tipo valido.
    bool esTipoValido(const TipoValor& tipo) const;

    bool esSubtipo(const TipoValor& a, const TipoValor& b) const;
    bool esAsignable(const TipoValor& a, const TipoValor& b) const;
    TipoValor unir(const TipoValor& a, const TipoValor& b) const;

private:
    std::map<std::string, InfoClase> clases;
    bool heredaDe(const std::string& clase, const std::string& ancestro) const;
};

#endif
