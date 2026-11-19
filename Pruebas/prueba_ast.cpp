#include <fstream>
#include <iostream>
#include <vector>
#include "AST/constructor.hpp"
#include "AST/impresion.hpp"
#include "GeneradorLR1/cache.hpp"
#include "Parser/parser.hpp"
#include "Scanner/lexer.hpp"
#include "prueba_ast.hpp"

namespace {

// Parsea 'fuente' (con un archivo temporal) y devuelve el AST como texto.
// Devuelve false si el programa tiene errores lexicos o sintacticos.
bool analizar(const Gramatica& gramatica, const TablaLR1& tabla,
              const std::string& fuente, std::string& volcado) {
    {
        std::ofstream salida("_tmp_ast.py", std::ios::binary);
        salida << fuente;
    }
    AnalizadorLexico scanner("_tmp_ast.py");
    AnalizadorSintactico parser(scanner, gramatica, tabla);
    if (!parser.parsear()) return false;
    auto arbol = parser.tomarArbol();
    if (!arbol) return false;
    volcado = astATexto(*construirAST(*arbol));
    return true;
}

struct Caso {
    const char* nombre;
    std::string fuente;
    std::string esperado;
};

} // namespace

void prueba_ast(const std::string& rutaGramatica, const std::string& rutaCache) {
    std::cout << "\nPRUEBA DE PARSER + AST\n\n";

    std::string texto = leerArchivo(rutaGramatica);
    Gramatica gramatica = parsearGramatica(texto);
    bool desdeCache = false;
    TablaLR1 tabla = obtenerTabla(gramatica, texto, rutaCache, desdeCache);
    std::cout << "Tabla LR(1): " << tabla.cantidadEstados << " estados, "
              << tabla.conflictos.size() << " conflictos ("
              << (desdeCache ? "de la cache" : "generada") << ")\n\n";

    const std::vector<Caso> casos = {
        {"programa vacio", "", "Programa\n"},
        {"precedencia: * antes que +",
         "a + b * c\n",
         "Programa\n"
         "  sentencia: SentenciaExpr\n"
         "    expr: OpBinaria +\n"
         "      izq: Identificador a\n"
         "      der: OpBinaria *\n"
         "        izq: Identificador b\n"
         "        der: Identificador c\n"},
        {"asociatividad a la izquierda",
         "a - b - c\n",
         "Programa\n"
         "  sentencia: SentenciaExpr\n"
         "    expr: OpBinaria -\n"
         "      izq: OpBinaria -\n"
         "        izq: Identificador a\n"
         "        der: Identificador b\n"
         "      der: Identificador c\n"},
        {"asignacion encadenada con indice y miembro",
         "a = b[0] = c.d = 1\n",
         "Programa\n"
         "  sentencia: Asignacion\n"
         "    objetivo: Identificador a\n"
         "    objetivo: Indice\n"
         "      objeto: Identificador b\n"
         "      indice: LitEntero 0\n"
         "    objetivo: Miembro d\n"
         "      objeto: Identificador c\n"
         "    valor: LitEntero 1\n"},
        {"parentesis, not y menos unario",
         "x = not (a and -b)\n",
         "Programa\n"
         "  sentencia: Asignacion\n"
         "    objetivo: Identificador x\n"
         "    valor: OpUnaria not\n"
         "      operando: OpBinaria and\n"
         "        izq: Identificador a\n"
         "        der: OpUnaria -\n"
         "          operando: Identificador b\n"},
        {"expresion condicional",
         "x = 1 if a else \"no\"\n",
         "Programa\n"
         "  sentencia: Asignacion\n"
         "    objetivo: Identificador x\n"
         "    valor: ExprSi\n"
         "      condicion: Identificador a\n"
         "      entonces: LitEntero 1\n"
         "      sino: LitTexto \"no\"\n"},
        {"if / elif / else: el elif se anida en el sino",
         "if a:\n"
         "    pass\n"
         "elif b:\n"
         "    pass\n"
         "else:\n"
         "    pass\n",
         "Programa\n"
         "  sentencia: Si\n"
         "    condicion: Identificador a\n"
         "    entonces: Pasar\n"
         "    sino: Si\n"
         "      condicion: Identificador b\n"
         "      entonces: Pasar\n"
         "      sino: Pasar\n"},
        {"while y for",
         "while a:\n"
         "    pass\n"
         "for i in [1, 2]:\n"
         "    return\n",
         "Programa\n"
         "  sentencia: Mientras\n"
         "    condicion: Identificador a\n"
         "    cuerpo: Pasar\n"
         "  sentencia: Para i\n"
         "    iterable: LitLista\n"
         "      elem: LitEntero 1\n"
         "      elem: LitEntero 2\n"
         "    cuerpo: Retornar\n"},
        {"llamadas: funcion, metodo y argumentos",
         "print(o.m(1, [2]))\n",
         "Programa\n"
         "  sentencia: SentenciaExpr\n"
         "    expr: Llamada\n"
         "      funcion: Identificador print\n"
         "      arg: Llamada\n"
         "        funcion: Miembro m\n"
         "          objeto: Identificador o\n"
         "        arg: LitEntero 1\n"
         "        arg: LitLista\n"
         "          elem: LitEntero 2\n"},
        {"funcion con parametros, retorno, declaraciones y cuerpo",
         "def f(a: int, b: [str]) -> bool:\n"
         "    global g\n"
         "    x: int = 1\n"
         "    return True\n",
         "Programa\n"
         "  def: DefFuncion f\n"
         "    param: VarTipada a\n"
         "      tipo: Tipo int\n"
         "    param: VarTipada b\n"
         "      tipo: TipoLista\n"
         "        elem: Tipo str\n"
         "    retorno: Tipo bool\n"
         "    decl: DeclGlobal g\n"
         "    decl: DefVariable\n"
         "      var: VarTipada x\n"
         "        tipo: Tipo int\n"
         "      valor: LitEntero 1\n"
         "    cuerpo: Retornar\n"
         "      valor: LitBool True\n"},
        {"clases: cuerpo pass y con miembros",
         "class A(object):\n"
         "    pass\n"
         "class B(A):\n"
         "    n: int = 0\n"
         "    def f(self: \"B\"):\n"
         "        pass\n",
         "Programa\n"
         "  def: DefClase A(object)\n"
         "  def: DefClase B(A)\n"
         "    miembro: DefVariable\n"
         "      var: VarTipada n\n"
         "        tipo: Tipo int\n"
         "      valor: LitEntero 0\n"
         "    miembro: DefFuncion f\n"
         "      param: VarTipada self\n"
         "        tipo: Tipo B\n"
         "      cuerpo: Pasar\n"},
        {"definiciones y luego sentencias",
         "x: [int] = None\n"
         "x = []\n",
         "Programa\n"
         "  def: DefVariable\n"
         "    var: VarTipada x\n"
         "      tipo: TipoLista\n"
         "        elem: Tipo int\n"
         "    valor: LitNone None\n"
         "  sentencia: Asignacion\n"
         "    objetivo: Identificador x\n"
         "    valor: LitLista\n"},
    };

    int fallos = 0;
    for (const auto& caso : casos) {
        std::string obtenido;
        bool aceptado = analizar(gramatica, tabla, caso.fuente, obtenido);
        if (aceptado && obtenido == caso.esperado) {
            std::cout << "OK     " << caso.nombre << "\n";
        } else {
            ++fallos;
            std::cout << "FALLO  " << caso.nombre << "\n";
            if (!aceptado) std::cout << "  el programa fue rechazado\n";
            else std::cout << "--- esperado ---\n" << caso.esperado << "--- obtenido ---\n" << obtenido;
        }
    }

    // Un programa con errores sintacticos no debe producir AST.
    {
        std::string obtenido;
        bool aceptado = analizar(gramatica, tabla, "x = = 1\n", obtenido);
        if (!aceptado) {
            std::cout << "OK     un programa con errores se rechaza y no genera AST\n";
        } else {
            ++fallos;
            std::cout << "FALLO  un programa con errores se acepto\n";
        }
    }

    std::cout << "\nErrores: " << fallos << " (se espera 0)\n";
    std::cout << "\nFIN DE PRUEBAS DE PARSER + AST\n";
}
