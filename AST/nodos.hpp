#ifndef AST_NODOS_HPP
#define AST_NODOS_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Scanner/tokens.hpp"

// Nodos del AST de ChocoPy. Cada nodo corresponde a una construccion del
// Language Reference (program, var_def, func_def, ...), con el nombre en
// espanol: Programa, DefVariable, DefFuncion, ...
//
// Todos los nodos heredan de Nodo y exponen tres cosas genericas: clase() (el
// nombre del nodo), texto() (el dato propio: nombre, operador, valor) e
// hijos() (sus hijos con el rol que cumplen: "condicion", "entonces",
// "izq"...). Con eso el volcado de texto, el JSON y el grafico de Python
// recorren cualquier nodo sin conocer cada clase; el analisis semantico
// (Semantico/analizador.cpp) distingue cada clase de nodo con dynamic_cast.

struct Nodo;
using NodoPtr = std::unique_ptr<Nodo>;
using ListaNodos = std::vector<NodoPtr>;
using Hijo = std::pair<std::string, const Nodo*>; // (rol, nodo)

struct Nodo {
    Posicion pos; // posicion del primer token del nodo (linea y columna)
    // Tipo que le asigna el analisis semantico (Semantico/analizador.hpp) a
    // cada expresion: "int", "[str]", "<None>"... Vacio en las sentencias y
    // definiciones, y en todo el arbol si todavia no se hizo el analisis.
    std::string tipoInferido;
    virtual ~Nodo() = default;
    virtual std::string clase() const = 0;
    virtual std::string texto() const { return ""; }
    virtual void hijos(std::vector<Hijo>&) const {}
};

// --- Tipos: int, str, "animal", [int], [[bool]] ---

struct Tipo : Nodo {
    std::string nombre;  // "int", "str", "animal"... (vacio en listas)
    NodoPtr elemento;    // tipo de los elementos si es una lista [T]
    std::string clase() const override { return elemento ? "TipoLista" : "Tipo"; }
    std::string texto() const override { return nombre; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// x: T. Es el parametro de una funcion y la cabecera de un DefVariable.
struct VarTipada : Nodo {
    std::string nombre;
    NodoPtr tipo;
    std::string clase() const override { return "VarTipada"; }
    std::string texto() const override { return nombre; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// --- Expresiones ---

struct LitEntero : Nodo {
    std::string valor;
    std::string clase() const override { return "LitEntero"; }
    std::string texto() const override { return valor; }
};

// Cubre STRING e IDSTRING (un string que ademas puede ser un nombre de tipo).
struct LitTexto : Nodo {
    std::string valor; // contenido ya sin comillas ni escapes
    std::string clase() const override { return "LitTexto"; }
    std::string texto() const override { return "\"" + valor + "\""; }
};

struct LitBool : Nodo {
    bool valor = false;
    std::string clase() const override { return "LitBool"; }
    std::string texto() const override { return valor ? "True" : "False"; }
};

struct LitNone : Nodo {
    std::string clase() const override { return "LitNone"; }
    std::string texto() const override { return "None"; }
};

struct Identificador : Nodo {
    std::string nombre;
    std::string clase() const override { return "Identificador"; }
    std::string texto() const override { return nombre; }
};

// Operadores binarios: + - * // % == != < > <= >= is and or
struct OpBinaria : Nodo {
    std::string operador;
    NodoPtr izq, der;
    std::string clase() const override { return "OpBinaria"; }
    std::string texto() const override { return operador; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// Operadores unarios: - y not
struct OpUnaria : Nodo {
    std::string operador;
    NodoPtr operando;
    std::string clase() const override { return "OpUnaria"; }
    std::string texto() const override { return operador; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// entonces if condicion else sino
struct ExprSi : Nodo {
    NodoPtr condicion, entonces, sino;
    std::string clase() const override { return "ExprSi"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// f(args) u obj.metodo(args); 'funcion' es un Identificador o un Miembro.
struct Llamada : Nodo {
    NodoPtr funcion;
    ListaNodos argumentos;
    std::string clase() const override { return "Llamada"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// objeto.nombre
struct Miembro : Nodo {
    NodoPtr objeto;
    std::string nombre;
    std::string clase() const override { return "Miembro"; }
    std::string texto() const override { return nombre; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// objeto[indice]
struct Indice : Nodo {
    NodoPtr objeto, indice;
    std::string clase() const override { return "Indice"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// [a, b, c]
struct LitLista : Nodo {
    ListaNodos elementos;
    std::string clase() const override { return "LitLista"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// --- Sentencias ---

struct Pasar : Nodo {
    std::string clase() const override { return "Pasar"; }
};

struct Retornar : Nodo {
    NodoPtr valor; // nullptr en "return" a secas
    std::string clase() const override { return "Retornar"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// Una expresion usada como sentencia, por ejemplo print(x).
struct SentenciaExpr : Nodo {
    NodoPtr expr;
    std::string clase() const override { return "SentenciaExpr"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// a = b = expr: 'objetivos' guarda [a, b] en orden de escritura.
struct Asignacion : Nodo {
    ListaNodos objetivos;
    NodoPtr valor;
    std::string clase() const override { return "Asignacion"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// if / elif / else. Los elif se representan anidados: el 'sino' de un Si con
// elif es una lista con un unico Si.
struct Si : Nodo {
    NodoPtr condicion;
    ListaNodos entonces, sino;
    std::string clase() const override { return "Si"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// while condicion: cuerpo
struct Mientras : Nodo {
    NodoPtr condicion;
    ListaNodos cuerpo;
    std::string clase() const override { return "Mientras"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// for variable in iterable: cuerpo
struct Para : Nodo {
    std::string variable;
    NodoPtr iterable;
    ListaNodos cuerpo;
    std::string clase() const override { return "Para"; }
    std::string texto() const override { return variable; }
    void hijos(std::vector<Hijo>& salida) const override;
};

// --- Definiciones ---

struct DefVariable : Nodo {
    NodoPtr variable; // VarTipada
    NodoPtr valor;    // literal
    std::string clase() const override { return "DefVariable"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

struct DeclGlobal : Nodo {
    std::string nombre;
    std::string clase() const override { return "DeclGlobal"; }
    std::string texto() const override { return nombre; }
};

struct DeclNonlocal : Nodo {
    std::string nombre;
    std::string clase() const override { return "DeclNonlocal"; }
    std::string texto() const override { return nombre; }
};

struct DefFuncion : Nodo {
    std::string nombre;
    ListaNodos parametros;    // VarTipada
    NodoPtr retorno;          // tipo de retorno o nullptr
    ListaNodos declaraciones; // DeclGlobal, DeclNonlocal, DefVariable, DefFuncion
    ListaNodos cuerpo;
    std::string clase() const override { return "DefFuncion"; }
    std::string texto() const override { return nombre; }
    void hijos(std::vector<Hijo>& salida) const override;
};

struct DefClase : Nodo {
    std::string nombre;
    std::string superclase;
    ListaNodos miembros; // DefVariable y DefFuncion (vacio si el cuerpo es "pass")
    std::string clase() const override { return "DefClase"; }
    std::string texto() const override { return nombre + "(" + superclase + ")"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

struct Programa : Nodo {
    ListaNodos definiciones; // DefVariable, DefFuncion, DefClase
    ListaNodos sentencias;
    std::string clase() const override { return "Programa"; }
    void hijos(std::vector<Hijo>& salida) const override;
};

#endif
