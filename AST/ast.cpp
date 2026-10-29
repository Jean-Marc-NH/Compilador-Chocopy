#include "ast.hpp"

// Agrega un hijo con su rol; los opcionales ausentes (nullptr) se omiten.
static void add(std::vector<Child>& out, const char* rol, const NodePtr& nodo) {
    if (nodo) out.push_back({rol, nodo.get()});
}

static void addAll(std::vector<Child>& out, const char* rol, const NodeList& lista) {
    for (const auto& nodo : lista) out.push_back({rol, nodo.get()});
}

void TypeRef::children(std::vector<Child>& out) const { add(out, "elem", elem); }
void TypedVar::children(std::vector<Child>& out) const { add(out, "type", type); }

void Binary::children(std::vector<Child>& out) const {
    add(out, "left", left);
    add(out, "right", right);
}

void Unary::children(std::vector<Child>& out) const { add(out, "operand", operand); }

void IfExpr::children(std::vector<Child>& out) const {
    add(out, "cond", cond);
    add(out, "then", then);
    add(out, "else", otherwise);
}

void Call::children(std::vector<Child>& out) const {
    add(out, "func", func);
    addAll(out, "arg", args);
}

void Member::children(std::vector<Child>& out) const { add(out, "object", object); }

void Index::children(std::vector<Child>& out) const {
    add(out, "object", object);
    add(out, "index", index);
}

void ListLit::children(std::vector<Child>& out) const { addAll(out, "elem", elems); }

void Return::children(std::vector<Child>& out) const { add(out, "value", value); }
void ExprStmt::children(std::vector<Child>& out) const { add(out, "expr", expr); }

void Assign::children(std::vector<Child>& out) const {
    addAll(out, "target", targets);
    add(out, "value", value);
}

void If::children(std::vector<Child>& out) const {
    add(out, "cond", cond);
    addAll(out, "then", then);
    addAll(out, "else", otherwise);
}

void While::children(std::vector<Child>& out) const {
    add(out, "cond", cond);
    addAll(out, "body", body);
}

void For::children(std::vector<Child>& out) const {
    add(out, "iter", iter);
    addAll(out, "body", body);
}

void VarDef::children(std::vector<Child>& out) const {
    add(out, "var", var);
    add(out, "value", value);
}

void FuncDef::children(std::vector<Child>& out) const {
    addAll(out, "param", params);
    add(out, "return", ret);
    addAll(out, "decl", decls);
    addAll(out, "body", body);
}

void ClassDef::children(std::vector<Child>& out) const { addAll(out, "member", members); }

void Program::children(std::vector<Child>& out) const {
    addAll(out, "def", defs);
    addAll(out, "stmt", stmts);
}
