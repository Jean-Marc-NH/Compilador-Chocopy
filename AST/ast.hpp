#ifndef AST_AST_HPP
#define AST_AST_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "../Scanner/tokens.hpp"

// AST de ChocoPy. Los nombres de los nodos siguen el Language Reference
// (Program, VarDef, FuncDef, ...).
//
// Todos los nodos heredan de Node y exponen tres cosas genericas: kind() (el
// nombre del nodo), text() (el dato propio: nombre, operador, valor) y
// children() (sus hijos con el rol que cumplen: "cond", "then", "left"...).
// Con eso el volcado de texto, el JSON y el grafico de Python recorren
// cualquier nodo sin conocer cada clase; el analisis semantico puede usar
// dynamic_cast o agregar un Visitor mas adelante.

struct Node;
using NodePtr = std::unique_ptr<Node>;
using NodeList = std::vector<NodePtr>;
using Child = std::pair<std::string, const Node*>; // (rol, nodo)

struct Node {
    Location loc; // posicion del primer token del nodo
    virtual ~Node() = default;
    virtual std::string kind() const = 0;
    virtual std::string text() const { return ""; }
    virtual void children(std::vector<Child>&) const {}
};

// --- Tipos: int, str, "animal", [int], [[bool]] ---

struct TypeRef : Node {
    std::string name;   // "int", "str", "animal"... (vacio en listas)
    NodePtr elem;       // tipo de los elementos si es una lista [T]
    std::string kind() const override { return elem ? "ListType" : "Type"; }
    std::string text() const override { return name; }
    void children(std::vector<Child>& out) const override;
};

// x: T. Es el parametro de una funcion y la cabecera de un VarDef.
struct TypedVar : Node {
    std::string name;
    NodePtr type;
    std::string kind() const override { return "TypedVar"; }
    std::string text() const override { return name; }
    void children(std::vector<Child>& out) const override;
};

// --- Expresiones ---

struct IntLit : Node {
    std::string value;
    std::string kind() const override { return "IntLit"; }
    std::string text() const override { return value; }
};

// Cubre STRING e IDSTRING (un string que ademas puede ser un nombre de tipo).
struct StrLit : Node {
    std::string value; // contenido ya sin comillas ni escapes
    std::string kind() const override { return "StrLit"; }
    std::string text() const override { return "\"" + value + "\""; }
};

struct BoolLit : Node {
    bool value = false;
    std::string kind() const override { return "BoolLit"; }
    std::string text() const override { return value ? "True" : "False"; }
};

struct NoneLit : Node {
    std::string kind() const override { return "NoneLit"; }
    std::string text() const override { return "None"; }
};

struct Id : Node {
    std::string name;
    std::string kind() const override { return "Id"; }
    std::string text() const override { return name; }
};

// Operadores binarios: + - * // % == != < > <= >= is and or
struct Binary : Node {
    std::string op;
    NodePtr left, right;
    std::string kind() const override { return "Binary"; }
    std::string text() const override { return op; }
    void children(std::vector<Child>& out) const override;
};

// Operadores unarios: - y not
struct Unary : Node {
    std::string op;
    NodePtr operand;
    std::string kind() const override { return "Unary"; }
    std::string text() const override { return op; }
    void children(std::vector<Child>& out) const override;
};

// then if cond else otherwise
struct IfExpr : Node {
    NodePtr cond, then, otherwise;
    std::string kind() const override { return "IfExpr"; }
    void children(std::vector<Child>& out) const override;
};

// f(args) o obj.metodo(args); 'func' es un Id o un Member.
struct Call : Node {
    NodePtr func;
    NodeList args;
    std::string kind() const override { return "Call"; }
    void children(std::vector<Child>& out) const override;
};

struct Member : Node {
    NodePtr object;
    std::string name;
    std::string kind() const override { return "Member"; }
    std::string text() const override { return name; }
    void children(std::vector<Child>& out) const override;
};

struct Index : Node {
    NodePtr object, index;
    std::string kind() const override { return "Index"; }
    void children(std::vector<Child>& out) const override;
};

struct ListLit : Node {
    NodeList elems;
    std::string kind() const override { return "ListLit"; }
    void children(std::vector<Child>& out) const override;
};

// --- Sentencias ---

struct Pass : Node {
    std::string kind() const override { return "Pass"; }
};

struct Return : Node {
    NodePtr value; // nullptr en "return" a secas
    std::string kind() const override { return "Return"; }
    void children(std::vector<Child>& out) const override;
};

// Una expresion usada como sentencia, por ejemplo print(x).
struct ExprStmt : Node {
    NodePtr expr;
    std::string kind() const override { return "ExprStmt"; }
    void children(std::vector<Child>& out) const override;
};

// a = b = expr: 'targets' guarda [a, b] en orden de escritura.
struct Assign : Node {
    NodeList targets;
    NodePtr value;
    std::string kind() const override { return "Assign"; }
    void children(std::vector<Child>& out) const override;
};

// Los elif se representan anidados: el 'else' de un If con elif es una lista
// con un unico If.
struct If : Node {
    NodePtr cond;
    NodeList then, otherwise;
    std::string kind() const override { return "If"; }
    void children(std::vector<Child>& out) const override;
};

struct While : Node {
    NodePtr cond;
    NodeList body;
    std::string kind() const override { return "While"; }
    void children(std::vector<Child>& out) const override;
};

struct For : Node {
    std::string var;
    NodePtr iter;
    NodeList body;
    std::string kind() const override { return "For"; }
    std::string text() const override { return var; }
    void children(std::vector<Child>& out) const override;
};

// --- Definiciones ---

struct VarDef : Node {
    NodePtr var;     // TypedVar
    NodePtr value;   // literal
    std::string kind() const override { return "VarDef"; }
    void children(std::vector<Child>& out) const override;
};

struct GlobalDecl : Node {
    std::string name;
    std::string kind() const override { return "GlobalDecl"; }
    std::string text() const override { return name; }
};

struct NonlocalDecl : Node {
    std::string name;
    std::string kind() const override { return "NonlocalDecl"; }
    std::string text() const override { return name; }
};

struct FuncDef : Node {
    std::string name;
    NodeList params;  // TypedVar
    NodePtr ret;      // tipo de retorno o nullptr
    NodeList decls;   // GlobalDecl, NonlocalDecl, VarDef, FuncDef
    NodeList body;
    std::string kind() const override { return "FuncDef"; }
    std::string text() const override { return name; }
    void children(std::vector<Child>& out) const override;
};

struct ClassDef : Node {
    std::string name;
    std::string superclass;
    NodeList members; // VarDef y FuncDef (vacio si el cuerpo es "pass")
    std::string kind() const override { return "ClassDef"; }
    std::string text() const override { return name + "(" + superclass + ")"; }
    void children(std::vector<Child>& out) const override;
};

struct Program : Node {
    NodeList defs;   // VarDef, FuncDef, ClassDef
    NodeList stmts;
    std::string kind() const override { return "Program"; }
    void children(std::vector<Child>& out) const override;
};

#endif
