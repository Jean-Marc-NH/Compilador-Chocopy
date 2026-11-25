#include "tipos.hpp"

namespace tipos {
const TipoValor INT = "int";
const TipoValor BOOL = "bool";
const TipoValor STR = "str";
const TipoValor OBJECT = "object";
const TipoValor NONE = "<None>";
const TipoValor EMPTY = "<Empty>";
const TipoValor ERROR = "<error>";

bool esLista(const TipoValor& tipo) { return tipo.size() >= 2 && tipo.front() == '[' && tipo.back() == ']'; }

TipoValor elementoDe(const TipoValor& lista) { return lista.substr(1, lista.size() - 2); }

TipoValor listaDe(const TipoValor& elemento) { return "[" + elemento + "]"; }

bool esPrimitivo(const TipoValor& tipo) { return tipo == INT || tipo == BOOL || tipo == STR; }
} // namespace tipos

std::string firmaATexto(const FirmaFuncion& firma) {
    std::string texto = "(";
    for (std::size_t i = 0; i < firma.parametros.size(); ++i) {
        if (i > 0) texto += ", ";
        texto += firma.parametros[i];
    }
    return texto + ") -> " + firma.retorno;
}

JerarquiaClases::JerarquiaClases() {
    // object es la raiz; su constructor __init__(self) no recibe nada mas.
    InfoClase& object = clases[tipos::OBJECT];
    object.nombre = tipos::OBJECT;
    FirmaFuncion init;
    init.nombresParametros = {"self"};
    init.parametros = {tipos::OBJECT};
    object.metodos["__init__"] = init;
    object.claseDeMetodo["__init__"] = tipos::OBJECT;

    agregar(tipos::INT, tipos::OBJECT);
    agregar(tipos::BOOL, tipos::OBJECT);
    agregar(tipos::STR, tipos::OBJECT);
}

bool JerarquiaClases::existe(const std::string& clase) const { return clases.count(clase) > 0; }

InfoClase* JerarquiaClases::buscar(const std::string& clase) {
    auto it = clases.find(clase);
    return it == clases.end() ? nullptr : &it->second;
}

const InfoClase* JerarquiaClases::buscar(const std::string& clase) const {
    auto it = clases.find(clase);
    return it == clases.end() ? nullptr : &it->second;
}

InfoClase& JerarquiaClases::agregar(const std::string& clase, const std::string& superclase) {
    InfoClase nueva = clases.at(superclase); // hereda atributos y metodos
    nueva.nombre = clase;
    nueva.superclase = superclase;
    return clases[clase] = nueva;
}

bool JerarquiaClases::esTipoValido(const TipoValor& tipo) const {
    if (tipos::esLista(tipo)) return esTipoValido(tipos::elementoDe(tipo));
    return existe(tipo);
}

bool JerarquiaClases::heredaDe(const std::string& clase, const std::string& ancestro) const {
    for (const InfoClase* actual = buscar(clase); actual; actual = buscar(actual->superclase)) {
        if (actual->nombre == ancestro) return true;
        if (actual->superclase.empty()) break;
    }
    return false;
}

// a <= b (sec. 2.4): a y b son el mismo tipo, a es subclase de b o b es object.
// Las listas son invariantes: [int] solo es subtipo de [int] y de object.
bool JerarquiaClases::esSubtipo(const TipoValor& a, const TipoValor& b) const {
    if (a == b || b == tipos::OBJECT) return true;
    if (tipos::esLista(a) || tipos::esLista(b)) return false;
    return heredaDe(a, b);
}

// a <=a b (sec. 2.4): ademas de a <= b,
//   - None se puede guardar en cualquier tipo que no sea int, bool o str;
//   - [] (<Empty>) se puede guardar en cualquier lista;
//   - [None] se puede guardar en una lista [T] si None se puede guardar en T.
bool JerarquiaClases::esAsignable(const TipoValor& a, const TipoValor& b) const {
    if (a == tipos::ERROR || b == tipos::ERROR) return true; // el error ya se reporto
    if (esSubtipo(a, b)) return true;
    if (a == tipos::NONE) return !tipos::esPrimitivo(b);
    if (tipos::esLista(b) && a == tipos::EMPTY) return true;
    if (tipos::esLista(b) && a == tipos::listaDe(tipos::NONE))
        return esAsignable(tipos::NONE, tipos::elementoDe(b));
    return false;
}

// Join (sec. 2.4): si uno se puede guardar en el otro, gana el mas general;
// si no, el ancestro comun mas cercano en la jerarquia (object en el peor caso).
TipoValor JerarquiaClases::unir(const TipoValor& a, const TipoValor& b) const {
    if (a == tipos::ERROR) return b;
    if (b == tipos::ERROR) return a;
    if (esAsignable(a, b)) return b;
    if (esAsignable(b, a)) return a;
    if (tipos::esLista(a) || tipos::esLista(b) || !existe(a) || !existe(b)) return tipos::OBJECT;
    for (const InfoClase* actual = buscar(a); actual; actual = buscar(actual->superclase)) {
        if (heredaDe(b, actual->nombre)) return actual->nombre;
        if (actual->superclase.empty()) break;
    }
    return tipos::OBJECT;
}
