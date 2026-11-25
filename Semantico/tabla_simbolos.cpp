#include "tabla_simbolos.hpp"
#include <iomanip>
#include <ostream>
#include <utility>

std::string tipoSimboloATexto(TipoSimbolo tipo) {
    switch (tipo) {
        case TipoSimbolo::Variable: return "variable";
        case TipoSimbolo::Parametro: return "parametro";
        case TipoSimbolo::Funcion: return "funcion";
        case TipoSimbolo::Clase: return "clase";
        case TipoSimbolo::Global: return "global";
        case TipoSimbolo::Nonlocal: return "nonlocal";
    }
    return "?";
}

Ambito::Ambito(std::string nombre, TipoAmbito tipo, Ambito* padre)
    : nombreAmbito(std::move(nombre)), tipoAmbito(tipo), ambitoPadre(padre) {}

Ambito& Ambito::global() {
    Ambito* actual = this;
    while (actual->ambitoPadre) actual = actual->ambitoPadre;
    return *actual;
}

Ambito& Ambito::crearHijo(const std::string& nombre, TipoAmbito tipo) {
    hijos.push_back(std::make_unique<Ambito>(nombre, tipo, this));
    return *hijos.back();
}

bool Ambito::insertar(const Simbolo& simbolo) {
    if (simbolos.count(simbolo.nombre)) return false;
    simbolos[simbolo.nombre] = simbolo;
    orden.push_back(simbolo.nombre);
    return true;
}

Simbolo* Ambito::buscarLocal(const std::string& nombre) {
    auto it = simbolos.find(nombre);
    return it == simbolos.end() ? nullptr : &it->second;
}

Simbolo* Ambito::buscar(const std::string& nombre, Ambito** donde) {
    for (Ambito* actual = this; actual; actual = actual->ambitoPadre) {
        // Los atributos de una clase no son visibles como nombres sueltos
        // dentro de sus metodos (solo con self.x).
        if (actual->tipoAmbito == TipoAmbito::Clase && actual != this) continue;

        Simbolo* simbolo = actual->buscarLocal(nombre);
        if (!simbolo) continue;

        // 'global x': la variable real esta en la tabla global.
        if (simbolo->categoria == TipoSimbolo::Global) return global().buscar(nombre, donde);
        // 'nonlocal x': la variable real esta en alguna funcion que encierra a esta.
        if (simbolo->categoria == TipoSimbolo::Nonlocal) {
            Ambito* superior = actual->ambitoPadre;
            while (superior && superior->tipoAmbito == TipoAmbito::Clase) superior = superior->ambitoPadre;
            return superior ? superior->buscar(nombre, donde) : nullptr;
        }
        if (donde) *donde = actual;
        return simbolo;
    }
    return nullptr;
}

void Ambito::imprimir(std::ostream& salida, bool conPredefinidos, int nivel) const {
    const std::string sangria(static_cast<std::size_t>(nivel) * 4, ' ');
    salida << sangria << "Tabla \"" << nombreAmbito << "\"";
    if (ambitoPadre) salida << "  (padre: \"" << ambitoPadre->nombreAmbito << "\")";
    else salida << "  (sin padre)";
    salida << '\n';

    bool vacia = true;
    for (const std::string& nombre : orden) {
        const Simbolo& simbolo = simbolos.at(nombre);
        if (simbolo.predefinido && !conPredefinidos) continue;
        vacia = false;
        salida << sangria << "  " << std::left << std::setw(16) << nombre << std::setw(11)
               << tipoSimboloATexto(simbolo.categoria);
        std::string detalle;
        switch (simbolo.categoria) {
            case TipoSimbolo::Funcion: detalle = firmaATexto(simbolo.firma); break;
            case TipoSimbolo::Clase: detalle = ""; break;
            case TipoSimbolo::Global: detalle = simbolo.tipo + " (la global)"; break;
            case TipoSimbolo::Nonlocal: detalle = simbolo.tipo + " (la de afuera)"; break;
            default: detalle = simbolo.tipo;
        }
        salida << std::setw(30) << detalle << ' ';
        if (simbolo.predefinido) salida << "predefinido";
        else salida << "linea " << simbolo.pos.linea;
        salida << std::right << '\n';
    }
    if (vacia) salida << sangria << "  (vacia)\n";
    for (const auto& hijo : hijos) hijo->imprimir(salida, conPredefinidos, nivel + 1);
}
