#include <fstream>
#include <iostream>
#include "AST/ast_builder.hpp"
#include "AST/ast_print.hpp"
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"
#include "lr1/generador.hpp"
#include "testAST.hpp"

// Parsea 'fuente' (con un archivo temporal) y devuelve el AST como texto.
// Devuelve false si el programa tiene errores lexicos o sintacticos.
static bool analizar(const Gramatica& gramatica, const TablaLR1& tabla,
                     const std::string& fuente, std::string& volcado) {
    {
        std::ofstream out("_tmp_ast.py", std::ios::binary);
        out << fuente;
    }
    Lexer lex("_tmp_ast.py");
    Parser parser(lex, gramatica, tabla);
    if (!parser.parsear()) return false;
    auto arbol = parser.tomarArbol();
    if (!arbol) return false;
    volcado = astATexto(*construirAST(*arbol));
    return true;
}

void test_ast(const std::string& rutaGramatica, const std::string& rutaCache) {
    std::cout << "\nPRUEBA DE PARSER + AST\n\n";

    std::string texto = leerArchivo(rutaGramatica);
    Gramatica gramatica = parsearGramatica(texto);
    bool desdeCache = false;
    TablaLR1 tabla = obtenerTabla(gramatica, texto, rutaCache, desdeCache);
    std::cout << "Tabla LR(1): " << tabla.cantidadEstados << " estados, "
              << tabla.conflictos.size() << " conflictos ("
              << (desdeCache ? "de la cache" : "generada") << ")\n\n";

    struct Caso {
        const char* nombre;
        std::string fuente;
        std::string esperado;
    };
    const std::vector<Caso> casos = {
        {"programa vacio", "", "Program\n"},
        {"precedencia: * antes que +",
         "a + b * c\n",
         "Program\n"
         "  stmt: ExprStmt\n"
         "    expr: Binary +\n"
         "      left: Id a\n"
         "      right: Binary *\n"
         "        left: Id b\n"
         "        right: Id c\n"},
        {"asociatividad a la izquierda",
         "a - b - c\n",
         "Program\n"
         "  stmt: ExprStmt\n"
         "    expr: Binary -\n"
         "      left: Binary -\n"
         "        left: Id a\n"
         "        right: Id b\n"
         "      right: Id c\n"},
        {"asignacion encadenada con indice y miembro",
         "a = b[0] = c.d = 1\n",
         "Program\n"
         "  stmt: Assign\n"
         "    target: Id a\n"
         "    target: Index\n"
         "      object: Id b\n"
         "      index: IntLit 0\n"
         "    target: Member d\n"
         "      object: Id c\n"
         "    value: IntLit 1\n"},
        {"parentesis, not y menos unario",
         "x = not (a and -b)\n",
         "Program\n"
         "  stmt: Assign\n"
         "    target: Id x\n"
         "    value: Unary not\n"
         "      operand: Binary and\n"
         "        left: Id a\n"
         "        right: Unary -\n"
         "          operand: Id b\n"},
        {"expresion condicional",
         "x = 1 if a else \"no\"\n",
         "Program\n"
         "  stmt: Assign\n"
         "    target: Id x\n"
         "    value: IfExpr\n"
         "      cond: Id a\n"
         "      then: IntLit 1\n"
         "      else: StrLit \"no\"\n"},
        {"if / elif / else: el elif se anida en el else",
         "if a:\n"
         "    pass\n"
         "elif b:\n"
         "    pass\n"
         "else:\n"
         "    pass\n",
         "Program\n"
         "  stmt: If\n"
         "    cond: Id a\n"
         "    then: Pass\n"
         "    else: If\n"
         "      cond: Id b\n"
         "      then: Pass\n"
         "      else: Pass\n"},
        {"while y for",
         "while a:\n"
         "    pass\n"
         "for i in [1, 2]:\n"
         "    return\n",
         "Program\n"
         "  stmt: While\n"
         "    cond: Id a\n"
         "    body: Pass\n"
         "  stmt: For i\n"
         "    iter: ListLit\n"
         "      elem: IntLit 1\n"
         "      elem: IntLit 2\n"
         "    body: Return\n"},
        {"llamadas: funcion, metodo y argumentos",
         "print(o.m(1, [2]))\n",
         "Program\n"
         "  stmt: ExprStmt\n"
         "    expr: Call\n"
         "      func: Id print\n"
         "      arg: Call\n"
         "        func: Member m\n"
         "          object: Id o\n"
         "        arg: IntLit 1\n"
         "        arg: ListLit\n"
         "          elem: IntLit 2\n"},
        {"funcion con parametros, retorno, declaraciones y cuerpo",
         "def f(a: int, b: [str]) -> bool:\n"
         "    global g\n"
         "    x: int = 1\n"
         "    return True\n",
         "Program\n"
         "  def: FuncDef f\n"
         "    param: TypedVar a\n"
         "      type: Type int\n"
         "    param: TypedVar b\n"
         "      type: ListType\n"
         "        elem: Type str\n"
         "    return: Type bool\n"
         "    decl: GlobalDecl g\n"
         "    decl: VarDef\n"
         "      var: TypedVar x\n"
         "        type: Type int\n"
         "      value: IntLit 1\n"
         "    body: Return\n"
         "      value: BoolLit True\n"},
        {"clases: cuerpo pass y con miembros",
         "class A(object):\n"
         "    pass\n"
         "class B(A):\n"
         "    n: int = 0\n"
         "    def f(self: \"B\"):\n"
         "        pass\n",
         "Program\n"
         "  def: ClassDef A(object)\n"
         "  def: ClassDef B(A)\n"
         "    member: VarDef\n"
         "      var: TypedVar n\n"
         "        type: Type int\n"
         "      value: IntLit 0\n"
         "    member: FuncDef f\n"
         "      param: TypedVar self\n"
         "        type: Type B\n"
         "      body: Pass\n"},
        {"definiciones y luego sentencias",
         "x: [int] = None\n"
         "x = []\n",
         "Program\n"
         "  def: VarDef\n"
         "    var: TypedVar x\n"
         "      type: ListType\n"
         "        elem: Type int\n"
         "    value: NoneLit None\n"
         "  stmt: Assign\n"
         "    target: Id x\n"
         "    value: ListLit\n"},
    };

    int errores = 0;
    const Gramatica& g = gramatica;
    const TablaLR1& t = tabla;
    for (const auto& caso : casos) {
        std::string obtenido;
        bool ok = analizar(g, t, caso.fuente, obtenido);
        if (ok && obtenido == caso.esperado) {
            std::cout << "OK     " << caso.nombre << "\n";
        } else {
            ++errores;
            std::cout << "FALLO  " << caso.nombre << "\n";
            if (!ok) std::cout << "  el programa fue rechazado\n";
            else std::cout << "--- esperado ---\n" << caso.esperado << "--- obtenido ---\n" << obtenido;
        }
    }

    // Un programa con errores sintacticos no debe producir AST.
    {
        std::string obtenido;
        bool ok = analizar(g, t, "x = = 1\n", obtenido);
        if (!ok) {
            std::cout << "OK     un programa con errores se rechaza y no genera AST\n";
        } else {
            ++errores;
            std::cout << "FALLO  un programa con errores se acepto\n";
        }
    }

    std::cout << "\nErrores: " << errores << " (se espera 0)\n";
    std::cout << "\nFIN DE PRUEBAS DE PARSER + AST\n";
}
