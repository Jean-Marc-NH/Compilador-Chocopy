#include "nodos.hpp"

// hijos() de cada nodo: el orden en que se agregan es el orden en que se
// imprimen y se dibujan.

namespace {

// Agrega un hijo con su rol; los opcionales ausentes (nullptr) se omiten.
void agregar(std::vector<Hijo>& salida, const char* rol, const NodoPtr& nodo) {
    if (nodo) salida.push_back({rol, nodo.get()});
}

void agregarTodos(std::vector<Hijo>& salida, const char* rol, const ListaNodos& lista) {
    for (const auto& nodo : lista) salida.push_back({rol, nodo.get()});
}

} // namespace

void Tipo::hijos(std::vector<Hijo>& salida) const { agregar(salida, "elem", elemento); }
void VarTipada::hijos(std::vector<Hijo>& salida) const { agregar(salida, "tipo", tipo); }

void OpBinaria::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "izq", izq);
    agregar(salida, "der", der);
}

void OpUnaria::hijos(std::vector<Hijo>& salida) const { agregar(salida, "operando", operando); }

void ExprSi::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "condicion", condicion);
    agregar(salida, "entonces", entonces);
    agregar(salida, "sino", sino);
}

void Llamada::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "funcion", funcion);
    agregarTodos(salida, "arg", argumentos);
}

void Miembro::hijos(std::vector<Hijo>& salida) const { agregar(salida, "objeto", objeto); }

void Indice::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "objeto", objeto);
    agregar(salida, "indice", indice);
}

void LitLista::hijos(std::vector<Hijo>& salida) const { agregarTodos(salida, "elem", elementos); }

void Retornar::hijos(std::vector<Hijo>& salida) const { agregar(salida, "valor", valor); }
void SentenciaExpr::hijos(std::vector<Hijo>& salida) const { agregar(salida, "expr", expr); }

void Asignacion::hijos(std::vector<Hijo>& salida) const {
    agregarTodos(salida, "objetivo", objetivos);
    agregar(salida, "valor", valor);
}

void Si::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "condicion", condicion);
    agregarTodos(salida, "entonces", entonces);
    agregarTodos(salida, "sino", sino);
}

void Mientras::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "condicion", condicion);
    agregarTodos(salida, "cuerpo", cuerpo);
}

void Para::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "iterable", iterable);
    agregarTodos(salida, "cuerpo", cuerpo);
}

void DefVariable::hijos(std::vector<Hijo>& salida) const {
    agregar(salida, "var", variable);
    agregar(salida, "valor", valor);
}

void DefFuncion::hijos(std::vector<Hijo>& salida) const {
    agregarTodos(salida, "param", parametros);
    agregar(salida, "retorno", retorno);
    agregarTodos(salida, "decl", declaraciones);
    agregarTodos(salida, "cuerpo", cuerpo);
}

void DefClase::hijos(std::vector<Hijo>& salida) const { agregarTodos(salida, "miembro", miembros); }

void Programa::hijos(std::vector<Hijo>& salida) const {
    agregarTodos(salida, "def", definiciones);
    agregarTodos(salida, "sentencia", sentencias);
}
