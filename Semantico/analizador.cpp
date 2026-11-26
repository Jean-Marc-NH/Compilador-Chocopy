#include "analizador.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../Comun/diagnostico.hpp"

namespace {

std::string comillas(const std::string& texto) { return "'" + texto + "'"; }

// Largo del subrayado (^~~~) para un nodo: el nombre si es un identificador.
int largoDe(const Nodo& nodo) {
    if (auto* id = dynamic_cast<const Identificador*>(&nodo)) return static_cast<int>(id->nombre.size());
    if (auto* var = dynamic_cast<const VarTipada*>(&nodo)) return static_cast<int>(var->nombre.size());
    return 1;
}

// Como se nombra un destino de asignacion en los mensajes ("asignar ... a 'x'").
std::string describirDestino(const Nodo& destino) {
    if (auto* id = dynamic_cast<const Identificador*>(&destino)) return "a " + comillas(id->nombre);
    if (auto* miembro = dynamic_cast<const Miembro*>(&destino)) return "al atributo " + comillas(miembro->nombre);
    return "al elemento de la lista";
}

// "argumento" / "argumentos"
std::string plural(std::size_t cantidad, const std::string& palabra) {
    return std::to_string(cantidad) + " " + palabra + (cantidad == 1 ? "" : "s");
}

} // namespace

AnalizadorSemantico::AnalizadorSemantico(const std::string& archivo) : archivo(archivo) {
    std::ifstream entrada(archivo, std::ios::binary);
    std::string linea;
    while (std::getline(entrada, linea)) {
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        lineas.push_back(linea);
    }
}

// ============================================================================
// Mensajes
// ============================================================================

void AnalizadorSemantico::error(const Nodo& nodo, const std::string& mensaje) {
    error(nodo.pos, largoDe(nodo), mensaje);
}

// Los mensajes no se imprimen al momento: se guardan y al final se muestran
// ordenados por linea (el analisis no recorre el programa en orden: primero
// declara todo y despues revisa los cuerpos).
void AnalizadorSemantico::error(Posicion pos, int largo, const std::string& mensaje) {
    ++errores;
    std::ostringstream texto;
    const std::string* codigo = pos.linea >= 1 && pos.linea <= static_cast<int>(lineas.size())
                                    ? &lineas[static_cast<std::size_t>(pos.linea - 1)]
                                    : nullptr;
    imprimirDiagnostico(texto, archivo, pos.linea, pos.columna, largo, "error", mensaje, codigo);
    pendientes.push_back({pos, texto.str()});
}

// Una nota se pega al ultimo error (apunta a otro lugar relacionado).
void AnalizadorSemantico::nota(Posicion pos, int largo, const std::string& mensaje) {
    std::ostringstream texto;
    const std::string* codigo = pos.linea >= 1 && pos.linea <= static_cast<int>(lineas.size())
                                    ? &lineas[static_cast<std::size_t>(pos.linea - 1)]
                                    : nullptr;
    imprimirDiagnostico(texto, archivo, pos.linea, pos.columna, largo, "nota", mensaje, codigo);
    if (!pendientes.empty()) pendientes.back().second += texto.str();
}

void AnalizadorSemantico::imprimirErrores() {
    std::stable_sort(pendientes.begin(), pendientes.end(), [](const auto& a, const auto& b) {
        if (a.first.linea != b.first.linea) return a.first.linea < b.first.linea;
        return a.first.columna < b.first.columna;
    });
    for (const auto& mensaje : pendientes) std::cerr << mensaje.second;
    pendientes.clear();
}

// ============================================================================
// Programa completo
// ============================================================================

int AnalizadorSemantico::analizar(Programa& programa) {
    declararPredefinidos();
    actual = &global;

    // Paso 1: clases en la jerarquia (en orden de aparicion).
    for (auto& definicion : programa.definiciones)
        if (auto* clase = dynamic_cast<DefClase*>(definicion.get())) registrarClase(*clase);

    // Paso 2: nombres globales en la tabla global.
    for (auto& definicion : programa.definiciones) declararGlobal(*definicion);

    // Paso 3: cuerpos de las definiciones y sentencias del nivel superior.
    for (auto& definicion : programa.definiciones) revisarDefinicionGlobal(*definicion);
    revisarBloque(programa.sentencias);

    imprimirErrores();
    return errores;
}

// print, len, input y las clases predefinidas (sec. 3.4 y 2.4).
void AnalizadorSemantico::declararPredefinidos() {
    auto funcion = [&](const std::string& nombre, std::vector<TipoValor> parametros, TipoValor retorno) {
        Simbolo simbolo;
        simbolo.nombre = nombre;
        simbolo.categoria = TipoSimbolo::Funcion;
        simbolo.firma.parametros = std::move(parametros);
        simbolo.firma.nombresParametros.assign(simbolo.firma.parametros.size(), "arg");
        simbolo.firma.retorno = retorno;
        simbolo.predefinido = true;
        global.insertar(simbolo);
    };
    for (const TipoValor& clase : {tipos::OBJECT, tipos::INT, tipos::BOOL, tipos::STR}) {
        Simbolo simbolo;
        simbolo.nombre = clase;
        simbolo.categoria = TipoSimbolo::Clase;
        simbolo.tipo = clase;
        simbolo.predefinido = true;
        global.insertar(simbolo);
    }
    funcion("print", {tipos::OBJECT}, tipos::NONE);
    funcion("len", {tipos::OBJECT}, tipos::INT);
    funcion("input", {}, tipos::STR);
}

// ============================================================================
// Pasos 1 y 2: declaraciones
// ============================================================================

void AnalizadorSemantico::registrarClase(const DefClase& clase) {
    if (jerarquia.existe(clase.nombre)) return; // redeclaracion: se reporta en el paso 2

    std::string superclase = clase.superclase;
    if (!jerarquia.existe(superclase)) {
        error(clase.pos, 5, "la superclase " + comillas(superclase) + " de " + comillas(clase.nombre) +
                                " no es una clase definida antes");
        superclase = tipos::OBJECT;
    } else if (tipos::esPrimitivo(superclase)) {
        error(clase.pos, 5, "la clase " + comillas(clase.nombre) + " no puede heredar de " +
                                comillas(superclase) + " (int, bool y str no se pueden extender)");
        superclase = tipos::OBJECT;
    }
    jerarquia.agregar(clase.nombre, superclase);
}



// Tipo escrito en una anotacion (nodo Tipo): "int", "animal", "[[str]]".
TipoValor AnalizadorSemantico::tipoAnotado(const Nodo* anotacion, bool reportar) {
    auto* tipo = dynamic_cast<const Tipo*>(anotacion);
    if (!tipo) return tipos::ERROR;
    if (tipo->elemento) {
        TipoValor elemento = tipoAnotado(tipo->elemento.get(), reportar);
        return tipos::listaDe(elemento);
    }
    std::string nombre = tipo->nombre;
    if (nombre.size() >= 2 && nombre.front() == '"' && nombre.back() == '"') nombre = nombre.substr(1, nombre.size() - 2);
    if (!jerarquia.existe(nombre)) {
        if (reportar)
            error(tipo->pos, static_cast<int>(tipo->nombre.size()), "el tipo " + comillas(nombre) + " no existe");
        return tipos::ERROR;
    }
    return nombre;
}

FirmaFuncion AnalizadorSemantico::construirFirma(const DefFuncion& funcion, const std::string& clase) {
    FirmaFuncion firma;
    for (const auto& parametro : funcion.parametros) {
        auto* var = static_cast<VarTipada*>(parametro.get());
        firma.nombresParametros.push_back(var->nombre);
        firma.parametros.push_back(tipoAnotado(var->tipo.get(), true));
        var->tipoInferido = firma.parametros.back();
    }
    firma.retorno = funcion.retorno ? tipoAnotado(funcion.retorno.get(), true) : tipos::NONE;

    // Un metodo recibe primero al objeto: def m(self: "clase", ...).
    if (!clase.empty() && (firma.parametros.empty() || firma.parametros[0] != clase))
        error(funcion.pos, 3, "el primer parametro del metodo " + comillas(funcion.nombre) +
                                  " debe ser self, de tipo " + comillas(clase));
    firmas[&funcion] = firma;
    return firma;
}

void AnalizadorSemantico::declararGlobal(Nodo& definicion) {
    if (auto* variable = dynamic_cast<DefVariable*>(&definicion)) {
        auto* var = static_cast<VarTipada*>(variable->variable.get());
        Simbolo simbolo{var->nombre, TipoSimbolo::Variable, tipoAnotado(var->tipo.get(), true), {}, var->pos};
        var->tipoInferido = simbolo.tipo;
        declarar(simbolo, *var);
    } else if (auto* funcion = dynamic_cast<DefFuncion*>(&definicion)) {
        Simbolo simbolo{funcion->nombre, TipoSimbolo::Funcion, "", construirFirma(*funcion, ""), funcion->pos};
        declarar(simbolo, *funcion);
    } else if (auto* clase = dynamic_cast<DefClase*>(&definicion)) {
        Simbolo simbolo{clase->nombre, TipoSimbolo::Clase, clase->nombre, {}, clase->pos};
        if (declarar(simbolo, *clase)) declararMiembrosDeClase(*clase);
    }
}

// Atributos y metodos de una clase (sec. 3.1.2): se heredan los de la
// superclase; un atributo no se puede redefinir y un metodo solo se puede
// redefinir con la misma firma (salvo el tipo de self).
void AnalizadorSemantico::declararMiembrosDeClase(DefClase& clase) {
    InfoClase* info = jerarquia.buscar(clase.nombre);
    const InfoClase* superclase = jerarquia.buscar(info->superclase);
    info->atributos = superclase->atributos;
    info->metodos = superclase->metodos;
    info->claseDeMetodo = superclase->claseDeMetodo;

    Ambito* anterior = actual;
    actual = &actual->crearHijo("clase " + clase.nombre, TipoAmbito::Clase);
    tablaDeClase[&clase] = actual;

    for (auto& miembro : clase.miembros) {
        if (auto* variable = dynamic_cast<DefVariable*>(miembro.get())) {
            auto* var = static_cast<VarTipada*>(variable->variable.get());
            Simbolo simbolo{var->nombre, TipoSimbolo::Variable, tipoAnotado(var->tipo.get(), true), {}, var->pos};
            var->tipoInferido = simbolo.tipo;
            bool heredado = !actual->buscarLocal(var->nombre) &&
                            (info->atributos.count(var->nombre) || info->metodos.count(var->nombre));
            if (heredado) {
                error(*var, "el atributo " + comillas(var->nombre) + " ya esta definido en la superclase " +
                                comillas(info->superclase) + "; no se puede redefinir");
            } else if (declarar(simbolo, *var)) {
                info->atributos[var->nombre] = simbolo.tipo;
            }
        } else if (auto* metodo = dynamic_cast<DefFuncion*>(miembro.get())) {
            FirmaFuncion firma = construirFirma(*metodo, clase.nombre);
            Simbolo simbolo{metodo->nombre, TipoSimbolo::Funcion, "", firma, metodo->pos};
            if (!actual->buscarLocal(metodo->nombre) && info->atributos.count(metodo->nombre)) {
                error(metodo->pos, 3, comillas(metodo->nombre) + " ya es un atributo heredado de " +
                                          comillas(info->superclase) + "; no puede ser un metodo");
                continue;
            }
            auto heredado = info->metodos.find(metodo->nombre);
            if (!actual->buscarLocal(metodo->nombre) && heredado != info->metodos.end()) {
                const FirmaFuncion& vieja = heredado->second;
                bool igual = vieja.parametros.size() == firma.parametros.size() && vieja.retorno == firma.retorno;
                for (std::size_t i = 1; igual && i < firma.parametros.size(); ++i)
                    igual = vieja.parametros[i] == firma.parametros[i];
                if (!igual) {
                    // La firma esperada se muestra sin self, que si puede cambiar de tipo.
                    FirmaFuncion sinSelf = vieja;
                    if (!sinSelf.parametros.empty()) sinSelf.parametros.erase(sinSelf.parametros.begin());
                    std::string origen = info->claseDeMetodo[metodo->nombre];
                    error(metodo->pos, 3, "el metodo " + comillas(metodo->nombre) + " redefine al de " +
                                              comillas(origen) + " con otra firma: despues de self se esperaba " +
                                              firmaATexto(sinSelf));
                }
            }
            if (declarar(simbolo, *metodo)) {
                info->metodos[metodo->nombre] = firma;
                info->claseDeMetodo[metodo->nombre] = clase.nombre;
            }
        }
    }
    actual = anterior;
}

// ============================================================================
// Paso 3: cuerpos
// ============================================================================

void AnalizadorSemantico::revisarDefinicionGlobal(Nodo& definicion) {
    if (auto* variable = dynamic_cast<DefVariable*>(&definicion)) {
        revisarValorInicial(*variable);
    } else if (auto* funcion = dynamic_cast<DefFuncion*>(&definicion)) {
        revisarFuncion(*funcion, "funcion " + funcion->nombre);
    } else if (auto* clase = dynamic_cast<DefClase*>(&definicion)) {
        auto tabla = tablaDeClase.find(clase);
        if (tabla == tablaDeClase.end()) return; // clase redeclarada: ya se reporto
        Ambito* anterior = actual;
        actual = tabla->second;
        for (auto& miembro : clase->miembros) {
            if (auto* variable = dynamic_cast<DefVariable*>(miembro.get())) revisarValorInicial(*variable);
            else if (auto* metodo = dynamic_cast<DefFuncion*>(miembro.get()))
                revisarFuncion(*metodo, "metodo " + clase->nombre + "." + metodo->nombre);
        }
        actual = anterior;
    }
}

// x: T = literal  ->  el literal tiene que poder guardarse en T.
void AnalizadorSemantico::revisarValorInicial(DefVariable& variable) {
    auto* var = static_cast<VarTipada*>(variable.variable.get());
    TipoValor declarado = tipoAnotado(var->tipo.get(), false);
    TipoValor valor = tipoDe(*variable.valor);
    if (!jerarquia.esAsignable(valor, declarado))
        error(*variable.valor, "no se puede inicializar " + comillas(var->nombre) + " (de tipo " +
                                   comillas(declarado) + ") con un valor de tipo " + comillas(valor));
}

// Crea la tabla de la funcion (hija de la actual), declara parametros y
// declaraciones locales, revisa las funciones anidadas y despues el cuerpo.
void AnalizadorSemantico::revisarFuncion(DefFuncion& funcion, const std::string& nombreTabla) {
    const FirmaFuncion firma = firmas[&funcion];

    Ambito* anterior = actual;
    const bool estabaEnFuncion = dentroDeFuncion;
    const TipoValor retornoAnterior = retornoActual;
    //crear hijo crea una tabla por cada scope
    actual = &actual->crearHijo(nombreTabla, TipoAmbito::Funcion);

    for (std::size_t i = 0; i < funcion.parametros.size(); ++i) {
        auto* var = static_cast<VarTipada*>(funcion.parametros[i].get());
        declarar({var->nombre, TipoSimbolo::Parametro, firma.parametros[i], {}, var->pos}, *var);
    }
    for (auto& declaracion : funcion.declaraciones) declararEnFuncion(*declaracion, nombreTabla);

    // Valores iniciales y funciones anidadas (cada una con su tabla hija).
    const std::string ruta = nombreTabla.substr(nombreTabla.find(' ') + 1);
    for (auto& declaracion : funcion.declaraciones) {
        if (auto* variable = dynamic_cast<DefVariable*>(declaracion.get())) revisarValorInicial(*variable);
        else if (auto* anidada = dynamic_cast<DefFuncion*>(declaracion.get()))
            revisarFuncion(*anidada, "funcion " + ruta + "." + anidada->nombre);
    }

    dentroDeFuncion = true;
    retornoActual = firma.retorno;
    revisarBloque(funcion.cuerpo);

    // Si la funcion devuelve int, bool o str, todo camino debe tener return
    // (devolver None, que es lo que pasa al llegar al final, no es valido).
    if (!jerarquia.esAsignable(tipos::NONE, firma.retorno) && !siempreRetorna(funcion.cuerpo))
        error(funcion.pos, 3, "la funcion " + comillas(funcion.nombre) + " debe devolver " +
                                  comillas(firma.retorno) + ", pero no todos sus caminos terminan en 'return'");

    actual = anterior;
    dentroDeFuncion = estabaEnFuncion;
    retornoActual = retornoAnterior;
}

void AnalizadorSemantico::declararEnFuncion(Nodo& declaracion, const std::string& nombreTabla) {
    if (auto* variable = dynamic_cast<DefVariable*>(&declaracion)) {
        auto* var = static_cast<VarTipada*>(variable->variable.get());
        Simbolo simbolo{var->nombre, TipoSimbolo::Variable, tipoAnotado(var->tipo.get(), true), {}, var->pos};
        var->tipoInferido = simbolo.tipo;
        declarar(simbolo, *var);
    } else if (auto* anidada = dynamic_cast<DefFuncion*>(&declaracion)) {
        declarar({anidada->nombre, TipoSimbolo::Funcion, "", construirFirma(*anidada, ""), anidada->pos}, *anidada);
    } else if (auto* decl = dynamic_cast<DeclGlobal*>(&declaracion)) {
        // global x: x tiene que ser una variable de la tabla global.
        Simbolo* externo = global.buscarLocal(decl->nombre);
        if (!externo || externo->categoria != TipoSimbolo::Variable) {
            error(decl->pos, 6, comillas(decl->nombre) + " no es una variable global");
            return;
        }
        declarar({decl->nombre, TipoSimbolo::Global, externo->tipo, {}, decl->pos}, *decl);
    } else if (auto* decl = dynamic_cast<DeclNonlocal*>(&declaracion)) {
        // nonlocal x: x tiene que ser variable de una funcion que encierre a
        // esta (no de la tabla global ni de una clase).
        Simbolo* externo = nullptr;
        for (Ambito* tabla = actual->padre(); tabla && tabla->tipo() != TipoAmbito::Global; tabla = tabla->padre()) {
            if (tabla->tipo() == TipoAmbito::Clase) continue;
            if ((externo = tabla->buscarLocal(decl->nombre))) break;
        }
        if (externo && externo->categoria == TipoSimbolo::Nonlocal) externo = actual->padre()->buscar(decl->nombre);
        if (!externo || (externo->categoria != TipoSimbolo::Variable && externo->categoria != TipoSimbolo::Parametro)) {
            error(decl->pos, 8, comillas(decl->nombre) + " no es una variable de una funcion que encierre a " +
                                    nombreTabla);
            return;
        }
        declarar({decl->nombre, TipoSimbolo::Nonlocal, externo->tipo, {}, decl->pos}, *decl);
    }
}

void AnalizadorSemantico::revisarBloque(ListaNodos& sentencias) {
    for (auto& sentencia : sentencias) revisarSentencia(*sentencia);
}

//checkeo 1
TipoValor AnalizadorSemantico::calcularTipo(Nodo& expresion) {
    if (dynamic_cast<LitEntero*>(&expresion)) return tipos::INT;
    if (dynamic_cast<LitBool*>(&expresion)) return tipos::BOOL;
    if (dynamic_cast<LitTexto*>(&expresion)) return tipos::STR;
    if (dynamic_cast<LitNone*>(&expresion)) return tipos::NONE;

    if (auto* id = dynamic_cast<Identificador*>(&expresion)) {
        //checkeo semantico 1 - variable no declarada
        Simbolo* simbolo = actual->buscar(id->nombre);
        if (!simbolo) {
            error(*id, comillas(id->nombre) + " no esta declarado");
            return tipos::ERROR;
        }
        if (simbolo->categoria == TipoSimbolo::Funcion) {
            error(*id, comillas(id->nombre) + " es una funcion: para usarla hay que llamarla, " + id->nombre + "(...)");
            return tipos::ERROR;
        }
        if (simbolo->categoria == TipoSimbolo::Clase) {
            error(*id, comillas(id->nombre) + " es una clase, no un valor: para crear un objeto escribe " +
                           id->nombre + "()");
            return tipos::ERROR;
        }
        return simbolo->tipo;
    }

    if (auto* lista = dynamic_cast<LitLista*>(&expresion)) {
        if (lista->elementos.empty()) return tipos::EMPTY;
        TipoValor elemento = tipoDe(*lista->elementos[0]);
        for (std::size_t i = 1; i < lista->elementos.size(); ++i)
            elemento = jerarquia.unir(elemento, tipoDe(*lista->elementos[i]));
        return tipos::listaDe(elemento);
    }

    if (auto* unaria = dynamic_cast<OpUnaria*>(&expresion)) {
        TipoValor operando = tipoDe(*unaria->operando);
        const TipoValor esperado = unaria->operador == "not" ? tipos::BOOL : tipos::INT;
        if (operando != esperado && operando != tipos::ERROR)
            error(*unaria, "el operador " + comillas(unaria->operador) + " necesita un valor de tipo " +
                               comillas(esperado) + ", no " + comillas(operando));
        return esperado;
    }

    if (auto* binaria = dynamic_cast<OpBinaria*>(&expresion)) return tipoOpBinaria(*binaria);

    if (auto* si = dynamic_cast<ExprSi*>(&expresion)) {
        TipoValor entonces = tipoDe(*si->entonces);
        revisarCondicion(*si->condicion, "if");
        TipoValor sino = tipoDe(*si->sino);
        return jerarquia.unir(entonces, sino);
    }

    if (auto* indice = dynamic_cast<Indice*>(&expresion)) {
        TipoValor objeto = tipoDe(*indice->objeto);
        TipoValor posicion = tipoDe(*indice->indice);
        if (posicion != tipos::INT && posicion != tipos::ERROR)
            error(*indice->indice, "el indice debe ser de tipo 'int', no " + comillas(posicion));
        if (objeto == tipos::ERROR) return tipos::ERROR;
        if (objeto == tipos::STR) return tipos::STR;
        if (tipos::esLista(objeto)) return tipos::elementoDe(objeto);
        error(*indice, "no se puede usar [] sobre un valor de tipo " + comillas(objeto));
        return tipos::ERROR;
    }

    if (auto* miembro = dynamic_cast<Miembro*>(&expresion)) {
        TipoValor objeto = tipoDe(*miembro->objeto);
        const InfoClase* info = tipos::esLista(objeto) ? nullptr : jerarquia.buscar(objeto);
        if (objeto == tipos::ERROR) return tipos::ERROR;
        if (info && info->atributos.count(miembro->nombre)) return info->atributos.at(miembro->nombre);
        if (info && info->metodos.count(miembro->nombre))
            error(*miembro, comillas(miembro->nombre) + " es un metodo: para usarlo hay que llamarlo");
        else
            error(*miembro, "el tipo " + comillas(objeto) + " no tiene el atributo " + comillas(miembro->nombre));
        return tipos::ERROR;
    }

    if (auto* llamada = dynamic_cast<Llamada*>(&expresion)) return tipoLlamada(*llamada);

    return tipos::ERROR;
}

//checkeo 2
// Inserta 'simbolo' en la tabla actual. Reporta (y devuelve false) si el
// nombre es el de una clase o si ya estaba declarado en este mismo scope.
bool AnalizadorSemantico::declarar(const Simbolo& simbolo, const Nodo& donde) {
    if (simbolo.categoria != TipoSimbolo::Clase && jerarquia.existe(simbolo.nombre)) {
        error(donde, comillas(simbolo.nombre) + " es el nombre de una clase y no se puede usar como " +
                         tipoSimboloATexto(simbolo.categoria));
        return false;
    }
    //checkeo semantico 2 - redeclaracion en el mismo scope
    if (Simbolo* previo = actual->buscarLocal(simbolo.nombre)) {
        error(donde, comillas(simbolo.nombre) + " ya fue declarado en este ambito (" + actual->nombre() + ")");
        if (!previo->predefinido)
            nota(previo->pos, static_cast<int>(simbolo.nombre.size()),
                 "declaracion anterior de " + comillas(simbolo.nombre));
        return false;
    }
    actual->insertar(simbolo);
    return true;
}

// Reglas de los operadores binarios (sec. 5.2).
//Checkeo semantico 3 - tipos incompatibles en expresiones (5 * True)
TipoValor AnalizadorSemantico::tipoOpBinaria(OpBinaria& operacion) {
    const TipoValor izq = tipoDe(*operacion.izq);
    const TipoValor der = tipoDe(*operacion.der);
    const std::string& op = operacion.operador;
    auto reportar = [&](const std::string& detalle) {
        error(operacion, "el operador " + comillas(op) + " no se puede usar entre " + comillas(izq) + " y " +
                             comillas(der) + detalle);
    };

    // Si un operando ya tuvo un error no se reporta otro aqui: se devuelve el
    // tipo que daria el operador con operandos correctos.
    if (izq == tipos::ERROR || der == tipos::ERROR) {
        if (op == "-" || op == "*" || op == "//" || op == "%") return tipos::INT;
        if (op == "+") {
            const TipoValor& otro = izq == tipos::ERROR ? der : izq;
            return otro == tipos::INT || otro == tipos::STR ? otro : tipos::ERROR;
        }
        return tipos::BOOL; // and, or, comparaciones, is
    }

    if (op == "and" || op == "or") {
        if (izq != tipos::BOOL || der != tipos::BOOL) reportar(" (necesita dos 'bool')");
        return tipos::BOOL;
    }
    if (op == "+") {
        if (izq == tipos::INT && der == tipos::INT) return tipos::INT;
        if (izq == tipos::STR && der == tipos::STR) return tipos::STR;
        const bool listaIzq = tipos::esLista(izq) || izq == tipos::EMPTY;
        const bool listaDer = tipos::esLista(der) || der == tipos::EMPTY;
        if (listaIzq && listaDer) {
            if (izq == tipos::EMPTY) return der;
            if (der == tipos::EMPTY) return izq;
            return tipos::listaDe(jerarquia.unir(tipos::elementoDe(izq), tipos::elementoDe(der)));
        }
        reportar(" (suma int + int, une str + str o listas + listas)");
        return izq == tipos::INT || der == tipos::INT ? tipos::INT : tipos::ERROR;
    }
    if (op == "-" || op == "*" || op == "//" || op == "%") {
        if (izq != tipos::INT || der != tipos::INT) reportar(" (necesita dos 'int')");
        return tipos::INT;
    }
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (izq != tipos::INT || der != tipos::INT) reportar(" (solo compara 'int')");
        return tipos::BOOL;
    }
    if (op == "==" || op == "!=") {
        if (izq != der || !tipos::esPrimitivo(izq)) reportar(" (compara dos int, dos bool o dos str)");
        return tipos::BOOL;
    }
    if (op == "is") {
        if (tipos::esPrimitivo(izq) || tipos::esPrimitivo(der)) reportar(" ('is' no se usa con int, bool ni str)");
        return tipos::BOOL;
    }
    return tipos::ERROR;
}

//checkeo semantico 4 - Tipo incompatibles en asignaciones

void AnalizadorSemantico::revisarSentencia(Nodo& sentencia) {
    if (auto* expr = dynamic_cast<SentenciaExpr*>(&sentencia)) {
        tipoDe(*expr->expr);
    } else if (auto* asignacion = dynamic_cast<Asignacion*>(&sentencia)) {
        TipoValor valor = tipoDe(*asignacion->valor);
        if (asignacion->objetivos.size() > 1 && valor == tipos::listaDe(tipos::NONE))
            error(*asignacion->valor, "en una asignacion multiple el valor no puede ser de tipo '[<None>]'");
        for (auto& objetivo : asignacion->objetivos) {
            // x: int = 0 -> x = "hola"
            TipoValor destino = tipoDestino(*objetivo);
            if (!destino.empty() && !jerarquia.esAsignable(valor, destino))
                error(*objetivo, "no se puede asignar un valor de tipo " + comillas(valor) + " " +
                                     describirDestino(*objetivo) + ", que es de tipo " + comillas(destino));
        }
    } else if (auto* si = dynamic_cast<Si*>(&sentencia)) {
        revisarCondicion(*si->condicion, "if");
        revisarBloque(si->entonces);
        revisarBloque(si->sino);
    } else if (auto* mientras = dynamic_cast<Mientras*>(&sentencia)) {
        revisarCondicion(*mientras->condicion, "while");
        revisarBloque(mientras->cuerpo);
    } else if (auto* para = dynamic_cast<Para*>(&sentencia)) {
        TipoValor iterable = tipoDe(*para->iterable);
        TipoValor elemento;
        if (iterable == tipos::STR) elemento = tipos::STR;
        else if (tipos::esLista(iterable)) elemento = tipos::elementoDe(iterable);
        else if (iterable != tipos::EMPTY && iterable != tipos::ERROR)
            //checkeo bucles
            error(*para->iterable, "'for' solo puede recorrer una lista o un str, no un valor de tipo " +
                                       comillas(iterable));
        Identificador variable;
        variable.nombre = para->variable;
        variable.pos = Posicion{para->pos.linea, para->pos.columna + 4}; // "for x": x va despues de "for "
        TipoValor destino = tipoVariableAsignable(para->variable, variable);
        if (!destino.empty() && !elemento.empty() && !jerarquia.esAsignable(elemento, destino))
            error(variable, "la variable " + comillas(para->variable) + " del 'for' es de tipo " +
                                comillas(destino) + ", pero los elementos son de tipo " + comillas(elemento));
        revisarBloque(para->cuerpo);
    } else if (auto* retornar = dynamic_cast<Retornar*>(&sentencia)) {
        TipoValor valor = retornar->valor ? tipoDe(*retornar->valor) : tipos::NONE;
        if (!dentroDeFuncion) {
            error(retornar->pos, 6, "'return' solo se puede usar dentro de una funcion");
        } else if (!jerarquia.esAsignable(valor, retornoActual)) {
            //checkeo semantico 8 - tipo de retorno incorrecto
            if (retornar->valor)
                error(*retornar->valor, "la funcion debe devolver " + comillas(retornoActual) +
                                            ", pero este valor es de tipo " + comillas(valor));
            else
                error(retornar->pos, 6, "falta el valor del 'return': la funcion debe devolver " +
                                            comillas(retornoActual));
        }
    }
    // Pasar: no hay nada que revisar.
}

//checkeo semantico 5 - condieciones no booleana
//toda condifcion sea de tipo bool
void AnalizadorSemantico::revisarCondicion(Nodo& condicion, const char* sentencia) {
    TipoValor tipo = tipoDe(condicion);
    if (tipo != tipos::BOOL && tipo != tipos::ERROR)
        error(condicion, std::string("la condicion del '") + sentencia + "' debe ser de tipo 'bool', no " +
                             comillas(tipo));
}

// Revisa cantidad y tipo de los argumentos. 'desde' es 1 en metodos y
// constructores: el primer parametro (self) no se escribe en la llamada.
//Checkeo semantico 6 - cantidad y tipo de argumentos en llamadas a funciones y metodos
void AnalizadorSemantico::revisarArgumentos(Llamada& llamada, const FirmaFuncion& firma, std::size_t desde,
                                            const std::string& quien) {
    std::vector<TipoValor> argumentos;
    for (auto& argumento : llamada.argumentos) argumentos.push_back(tipoDe(*argumento));

    // (un metodo mal declarado, sin self, tiene menos de 'desde' parametros)
    const std::size_t esperados = firma.parametros.size() > desde ? firma.parametros.size() - desde : 0;
    
    //cantidad de argumentos incorrecta
    if (argumentos.size() != esperados) {
        error(llamada, quien + " espera " + plural(esperados, "argumento") + ", pero recibe " +
                           std::to_string(argumentos.size()));
        return;
    }
    for (std::size_t i = 0; i < argumentos.size(); ++i) {
        //tipo o firma de argumentos incorrectos
        const TipoValor& esperado = firma.parametros[desde + i];
        if (!jerarquia.esAsignable(argumentos[i], esperado))
            error(*llamada.argumentos[i], "el argumento " + std::to_string(i + 1) + " de " + quien +
                                              " debe ser de tipo " + comillas(esperado) + ", pero es " +
                                              comillas(argumentos[i]));
    }
}


// true si toda ejecucion de 'sentencias' termina en un return: hay un return
// directo o un if/else cuyas dos ramas siempre retornan.
bool AnalizadorSemantico::siempreRetorna(const ListaNodos& sentencias) const {
    for (const auto& sentencia : sentencias) {
        if (dynamic_cast<const Retornar*>(sentencia.get())) return true;
        if (auto* si = dynamic_cast<const Si*>(sentencia.get()))
            if (!si->sino.empty() && siempreRetorna(si->entonces) && siempreRetorna(si->sino)) return true;
    }
    return false;
}

TipoValor AnalizadorSemantico::tipoDestino(Nodo& destino) {
    TipoValor tipo;
    if (auto* id = dynamic_cast<Identificador*>(&destino)) {
        tipo = tipoVariableAsignable(id->nombre, *id);
    } else if (auto* miembro = dynamic_cast<Miembro*>(&destino)) {
        TipoValor objeto = tipoDe(*miembro->objeto);
        const InfoClase* info = tipos::esLista(objeto) ? nullptr : jerarquia.buscar(objeto);
        if (objeto == tipos::ERROR) tipo = tipos::ERROR;
        else if (info && info->atributos.count(miembro->nombre)) tipo = info->atributos.at(miembro->nombre);
        else if (info && info->metodos.count(miembro->nombre))
            error(*miembro, "no se puede asignar a " + comillas(miembro->nombre) + ": es un metodo de " +
                                comillas(objeto));
        else
            error(*miembro, "el tipo " + comillas(objeto) + " no tiene el atributo " + comillas(miembro->nombre));
    } else if (auto* indice = dynamic_cast<Indice*>(&destino)) {
        TipoValor objeto = tipoDe(*indice->objeto);
        TipoValor posicion = tipoDe(*indice->indice);
        if (posicion != tipos::INT && posicion != tipos::ERROR)
            error(*indice->indice, "el indice debe ser de tipo 'int', no " + comillas(posicion));
        if (objeto == tipos::ERROR)
            tipo = tipos::ERROR;
        else if (objeto == tipos::STR)
            error(*indice, "los str no se pueden modificar: no se puede asignar a un caracter");
        else if (tipos::esLista(objeto))
            tipo = tipos::elementoDe(objeto);
        else
            error(*indice, "no se puede asignar con [] a un valor de tipo " + comillas(objeto));
    } else {
        tipoDe(destino);
    }
    destino.tipoInferido = tipo;
    return tipo;
}

// Una variable solo se puede asignar si esta declarada en el scope actual
// (variable local, parametro, o traida con global / nonlocal). Leerla desde un
// scope de adentro esta bien, pero asignarla no (sec. 5.2, "Assign").
TipoValor AnalizadorSemantico::tipoVariableAsignable(const std::string& nombre, const Nodo& donde) {
    Simbolo* local = actual->buscarLocal(nombre);
    if (local) {
        switch (local->categoria) {
            case TipoSimbolo::Variable:
            case TipoSimbolo::Parametro: return local->tipo;
            case TipoSimbolo::Global:
            case TipoSimbolo::Nonlocal: {
                Simbolo* real = actual->buscar(nombre);
                return real ? real->tipo : "";
            }
            default:
                error(donde, comillas(nombre) + " es una " + tipoSimboloATexto(local->categoria) +
                                 ", no se le puede asignar un valor");
                return "";
        }
    }
    if (actual->buscar(nombre))
        error(donde, "no se puede asignar a " + comillas(nombre) + ": no es una variable de este ambito (" +
                         actual->nombre() + "); declarala con 'global " + nombre + "' o 'nonlocal " + nombre + "'");
    else
        error(donde, comillas(nombre) + " no esta declarado");
    return "";
}

// ============================================================================
// Expresiones
// ============================================================================

TipoValor AnalizadorSemantico::tipoDe(Nodo& expresion) {
    expresion.tipoInferido = calcularTipo(expresion);
    return expresion.tipoInferido;
}



TipoValor AnalizadorSemantico::tipoLlamada(Llamada& llamada) {
    auto tiparArgumentos = [&] {
        for (auto& argumento : llamada.argumentos) tipoDe(*argumento);
    };

    // f(...) o Clase(...)
    if (auto* id = dynamic_cast<Identificador*>(llamada.funcion.get())) {
        Simbolo* simbolo = actual->buscar(id->nombre);
        if (!simbolo) {
            error(*id, comillas(id->nombre) + " no esta declarado");
            tiparArgumentos();
            return tipos::ERROR;
        }
        if (simbolo->categoria == TipoSimbolo::Funcion) {
            revisarArgumentos(llamada, simbolo->firma, 0, "la funcion " + comillas(id->nombre));
            return simbolo->firma.retorno;
        }
        if (simbolo->categoria == TipoSimbolo::Clase) {
            const InfoClase* info = jerarquia.buscar(id->nombre);
            revisarArgumentos(llamada, info->metodos.at("__init__"), 1, "el constructor de " + comillas(id->nombre));
            return id->nombre;
        }
        error(*id, comillas(id->nombre) + " no es una funcion: es una " + tipoSimboloATexto(simbolo->categoria) +
                       " de tipo " + comillas(simbolo->tipo));
        tiparArgumentos();
        return tipos::ERROR;
    }

    // objeto.metodo(...)
    if (auto* miembro = dynamic_cast<Miembro*>(llamada.funcion.get())) {
        TipoValor objeto = tipoDe(*miembro->objeto);
        if (objeto == tipos::ERROR) {
            tiparArgumentos();
            return tipos::ERROR;
        }
        const InfoClase* info = tipos::esLista(objeto) ? nullptr : jerarquia.buscar(objeto);
        if (info && info->metodos.count(miembro->nombre)) {
            const FirmaFuncion& firma = info->metodos.at(miembro->nombre);
            revisarArgumentos(llamada, firma, 1, "el metodo " + comillas(objeto + "." + miembro->nombre));
            return firma.retorno;
        }
        if (info && info->atributos.count(miembro->nombre))
            error(*miembro, comillas(miembro->nombre) + " es un atributo de " + comillas(objeto) + ", no un metodo");
        else
            error(*miembro, "el tipo " + comillas(objeto) + " no tiene el metodo " + comillas(miembro->nombre));
        tiparArgumentos();
        return tipos::ERROR;
    }

    tiparArgumentos();
    return tipos::ERROR;
}

