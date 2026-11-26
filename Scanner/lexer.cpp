#include "lexer.hpp"
#include "../Comun/diagnostico.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace {

// Un identificador empieza con letra o '_' (sec. 3.2 del manual).
bool esInicioIdentificador(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// El resto del identificador acepta ademas digitos.
bool esParteIdentificador(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool esDigito(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

// Un contenido de string "tiene sintaxis de identificador" (sec. 3.4.1,
// usado para decidir IDSTRING vs STRING) si, ignorando las comillas,
// cumple la misma regla lexica que un IDENTIFIER: letra o '_' seguido de
// letras/digitos/'_'.
bool tieneFormaDeIdentificador(const std::string& s) {
    if (s.empty() || !esInicioIdentificador(s[0])) return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!esParteIdentificador(s[i])) return false;
    }
    return true;
}

// El manual pide "verificar el tamano maximo permitido" pero no fija un
// numero en el fragmento con el que contamos; elegimos un limite propio
// y lo documentamos aca para que sea facil de ajustar si la catedra da
// un valor especifico.
const size_t MAX_LARGO_CADENA = 4096;

// Maximo valor de un entero de 32 bits con signo (sec. 2.5.1), como
// string, para poder comparar lexemas arbitrariamente largos sin
// desbordar ningun tipo entero de C++.
const std::string MAX_ENTERO_TEXTO = "2147483647";

// Tabla precargada de palabras clave (estrategia "preload the symbol
// table", ver seccion "Two ways to handle keywords" de las diapositivas):
// evita tener un estado del AFD por cada palabra clave y es la forma mas
// simple de mantener el listado completo de las 35 palabras reservadas de
// Python (sec. 3.3), aunque varias de ellas no formen parte de la gramatica
// de ChocoPy (esas caen en KW_RESERVED).
const std::unordered_map<std::string, TipoToken>& tablaPalabrasClave() {
    static const std::unordered_map<std::string, TipoToken> tabla = {
        {"False",    TipoToken::KW_FALSE},
        {"None",     TipoToken::KW_NONE},
        {"True",     TipoToken::KW_TRUE},
        {"and",      TipoToken::KW_AND},
        {"as",       TipoToken::KW_RESERVED},
        {"assert",   TipoToken::KW_RESERVED},
        {"async",    TipoToken::KW_RESERVED},
        {"await",    TipoToken::KW_RESERVED},
        {"break",    TipoToken::KW_RESERVED},
        {"class",    TipoToken::KW_CLASS},
        {"continue", TipoToken::KW_RESERVED},
        {"def",      TipoToken::KW_DEF},
        {"del",      TipoToken::KW_RESERVED},
        {"elif",     TipoToken::KW_ELIF},
        {"else",     TipoToken::KW_ELSE},
        {"except",   TipoToken::KW_RESERVED},
        {"finally",  TipoToken::KW_RESERVED},
        {"for",      TipoToken::KW_FOR},
        {"from",     TipoToken::KW_RESERVED},
        {"global",   TipoToken::KW_GLOBAL},
        {"if",       TipoToken::KW_IF},
        {"import",   TipoToken::KW_RESERVED},
        {"in",       TipoToken::KW_IN},
        {"is",       TipoToken::KW_IS},
        {"lambda",   TipoToken::KW_RESERVED},
        {"nonlocal", TipoToken::KW_NONLOCAL},
        {"not",      TipoToken::KW_NOT},
        {"or",       TipoToken::KW_OR},
        {"pass",     TipoToken::KW_PASS},
        {"raise",    TipoToken::KW_RESERVED},
        {"return",   TipoToken::KW_RETURN},
        {"try",      TipoToken::KW_RESERVED},
        {"while",    TipoToken::KW_WHILE},
        {"with",     TipoToken::KW_RESERVED},
        {"yield",    TipoToken::KW_RESERVED},
    };
    return tabla;
}

} // namespace

AnalizadorLexico::AnalizadorLexico(const std::string& archivo) : archivo(archivo) {
    cargarArchivo(archivo);
}

void AnalizadorLexico::cargarArchivo(const std::string& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        // Informamos y dejamos 'fuente' vacio: siguienteToken() devolvera
        // END_OF_FILE de inmediato y el parser no tendra nada que analizar.
        imprimirErrorGeneral(std::cerr, ruta, "no se pudo abrir el archivo");
        return;
    }

    std::ostringstream buffer;
    buffer << entrada.rdbuf();
    std::string crudo = buffer.str();

    // Normalizamos las 3 formas de fin de linea (LF, CRLF, CR) a '\n' unico,
    // asi el resto del scanner nunca tiene que preocuparse por '\r'.
    fuente.reserve(crudo.size());
    for (size_t i = 0; i < crudo.size(); ++i) {
        char c = crudo[i];
        if (c == '\r') {
            fuente.push_back('\n');
            if (i + 1 < crudo.size() && crudo[i + 1] == '\n') {
                ++i; // saltamos el '\n' que sigue al '\r' (forma CRLF)
            }
        } else {
            fuente.push_back(c);
        }
    }
}

bool AnalizadorLexico::finDeArchivo() const {
    return pos >= fuente.size();
}

char AnalizadorLexico::verCaracter(int adelante) const {
    size_t indice = pos + static_cast<size_t>(adelante);
    if (indice >= fuente.size()) return '\0';
    return fuente[indice];
}

char AnalizadorLexico::leerCaracter() {
    if (finDeArchivo()) return '\0';

    char c = fuente[pos];
    ++pos;

    if (c == '\n') {
        ++linea;
        columna = 1;
    } else {
        ++columna;
    }

    return c;
}

bool AnalizadorLexico::lineaDeCodigo(int n, std::string& salida) const {
    if (n < 1) return false;
    size_t inicio = 0;
    for (int i = 1; i < n; ++i) {
        size_t salto = fuente.find('\n', inicio);
        if (salto == std::string::npos) return false;
        inicio = salto + 1;
    }
    if (inicio >= fuente.size()) return false; // la linea 'n' esta despues del final
    size_t fin = fuente.find('\n', inicio);
    salida = fuente.substr(inicio, fin == std::string::npos ? std::string::npos : fin - inicio);
    return true;
}

void AnalizadorLexico::reportarError(const std::string& mensaje, int enLinea, int enColumna, int largo) {
    ++errores;
    std::string texto;
    bool hayTexto = lineaDeCodigo(enLinea, texto);
    imprimirDiagnostico(std::cerr, archivo, enLinea, enColumna, largo, "error", mensaje,
                        hayTexto ? &texto : nullptr);
}

void AnalizadorLexico::saltarComentario() {
    // Se asume que verCaracter() == '#'. Consumimos hasta el '\n' o EOF,
    // sin emitir ningun token: los comentarios no existen para el parser.
    while (!finDeArchivo() && verCaracter() != '\n') {
        leerCaracter();
    }
}

void AnalizadorLexico::cerrarIndentacionYFin() {
    // Vaciamos la pila de indentacion (sec. 3.1.5): al llegar a EOF debe
    // generarse un DEDENT por cada nivel que quedo abierto por encima de 0.
    while (pilaIndentacion.size() > 1) {
        pilaIndentacion.pop_back();
        tokensPendientes.push_back(Token{TipoToken::DEDENT, "", Posicion{linea, columna}});
    }
    tokensPendientes.push_back(Token{TipoToken::END_OF_FILE, "", Posicion{linea, columna}});
}

void AnalizadorLexico::procesarInicioDeLinea() {
    // Procesa lineas fisicas una tras otra hasta encontrar contenido real
    // o el fin del archivo. Las lineas en blanco y las de solo-comentario
    // (3.1.4) no generan NEWLINE ni afectan la indentacion.
    for (;;) {
        int n = 0;

        // Medimos espacios de indentacion. Las tabulaciones se rechazan
        // explicitamente (politica elegida para este scanner, sec.
        // "Manejo de la indentacion" del enunciado): se reportan como
        // error y se descartan sin contar hacia el nivel n.
        for (;;) {
            char c = verCaracter();
            if (c == ' ') {
                leerCaracter();
                ++n;
            } else if (c == '\t') {
                reportarError("no se permiten tabuladores para indentar: usa espacios", linea, columna);
                leerCaracter();
            } else {
                break;
            }
        }

        if (finDeArchivo()) {
            cerrarIndentacionYFin();
            return;
        }

        char c = verCaracter();

        if (c == '\n') {
            // Linea en blanco: se ignora por completo.
            leerCaracter();
            continue;
        }

        if (c == '#') {
            saltarComentario();
            if (verCaracter() == '\n') {
                leerCaracter();
            }
            // Si lo que sigue es EOF, la proxima vuelta del for(;;) lo detecta.
            continue;
        }

        // Contenido real: aca se decide si hay INDENT, DEDENT o nada.
        int tope = pilaIndentacion.back();
        if (n > tope) {
            pilaIndentacion.push_back(n);
            tokensPendientes.push_back(Token{TipoToken::INDENT, "", Posicion{linea, columna}});
        } else if (n < tope) {
            while (pilaIndentacion.back() > n) {
                pilaIndentacion.pop_back();
                tokensPendientes.push_back(Token{TipoToken::DEDENT, "", Posicion{linea, columna}});
            }
            if (pilaIndentacion.back() != n) {
                reportarError("la indentacion de esta linea (" + std::to_string(n) +
                                  " espacios) no coincide con la de ningun bloque anterior",
                              linea, columna);
                // Recuperacion: resincronizamos la pila a este nivel nuevo
                // para no arrastrar el mismo error en las lineas siguientes.
                pilaIndentacion.push_back(n);
            }
        }
        // n == tope: incremento de indentacion nulo, no se emite nada.

        inicioDeLinea = false;
        return;
    }
}

Token AnalizadorLexico::escanearIdentificador() {
    // Se asume que verCaracter() ya cumplio esInicioIdentificador(). Regla de
    // "maximal munch" (sec. 3, parrafo introductorio): se consume la secuencia
    // mas larga posible de letras/digitos/'_'.
    int lineaInicio = linea;
    int columnaInicio = columna;
    std::string lexema;

    while (!finDeArchivo() && esParteIdentificador(verCaracter())) {
        lexema.push_back(leerCaracter());
    }

    const auto& tabla = tablaPalabrasClave();
    auto it = tabla.find(lexema);
    TipoToken tipo = (it != tabla.end()) ? it->second : TipoToken::IDENTIFIER;

    return Token{tipo, lexema, Posicion{lineaInicio, columnaInicio}};
}

Token AnalizadorLexico::escanearEntero() {
    // Se asume que verCaracter() ya es un digito. "Maximal munch": se
    // consume la corrida mas larga posible de digitos antes de decidir
    // si el lexema resultante es un entero valido.
    int lineaInicio = linea;
    int columnaInicio = columna;
    std::string lexema;

    while (!finDeArchivo() && esDigito(verCaracter())) {
        lexema.push_back(leerCaracter());
    }

    Posicion inicio{lineaInicio, columnaInicio};
    int largo = static_cast<int>(lexema.size());

    // Regla de ceros a la izquierda (sec. 3.4.2): "0" solo es valido,
    // pero "0" seguido de mas digitos no lo es (ej. "007").
    if (lexema.size() > 1 && lexema[0] == '0') {
        reportarError("'" + lexema + "' no es un numero valido: un entero no puede empezar con 0",
                      lineaInicio, columnaInicio, largo);
        return Token{TipoToken::UNKNOWN_ERROR, lexema, inicio};
    }

    // Rango de 32 bits con signo: comparamos como string para no
    // desbordar ningun tipo entero de C++ con lexemas muy largos.
    bool fueraDeRango = false;
    if (lexema.size() > MAX_ENTERO_TEXTO.size()) {
        fueraDeRango = true;
    } else if (lexema.size() == MAX_ENTERO_TEXTO.size() && lexema > MAX_ENTERO_TEXTO) {
        fueraDeRango = true;
    }

    if (fueraDeRango) {
        reportarError("el numero " + lexema + " es demasiado grande: el maximo permitido es " +
                          MAX_ENTERO_TEXTO,
                      lineaInicio, columnaInicio, largo);
        return Token{TipoToken::UNKNOWN_ERROR, lexema, inicio};
    }

    return Token{TipoToken::INTEGER, lexema, inicio};
}

Token AnalizadorLexico::escanearCadena() {
    // Se asume que verCaracter() == '"'.
    int lineaInicio = linea;
    int columnaInicio = columna;
    leerCaracter(); // consumimos la comilla de apertura

    std::string contenido; // valor ya decodificado (sin comillas, escapes aplicados)
    bool cerrada = false;
    bool excedioLargo = false;

    for (;;) {
        if (finDeArchivo()) {
            reportarError("falta la comilla de cierre (\") de esta cadena: el archivo termina antes",
                          lineaInicio, columnaInicio, columna - columnaInicio);
            break;
        }

        char c = verCaracter();

        if (c == '"') {
            leerCaracter();
            cerrada = true;
            break;
        }

        if (c == '\n') {
            // No consumimos el '\n': el manejo de NEWLINE/indentacion lo
            // necesita intacto para no romper el conteo de lineas.
            reportarError("falta la comilla de cierre (\") de esta cadena antes del fin de la linea",
                          lineaInicio, columnaInicio, columna - columnaInicio);
            break;
        }

        if (c == '\\') {
            int lineaBarra = linea, columnaBarra = columna; // posicion de la barra invertida
            leerCaracter();
            if (finDeArchivo()) {
                reportarError("la cadena termina con una barra '\\' incompleta y le falta la comilla de cierre (\")",
                              lineaInicio, columnaInicio, columna - columnaInicio);
                break;
            }
            char escape = leerCaracter();
            switch (escape) {
                case '"':  contenido.push_back('"');  break;
                case 'n':  contenido.push_back('\n'); break;
                case 't':  contenido.push_back('\t'); break;
                case '\\': contenido.push_back('\\'); break;
                default:
                    reportarError(std::string("secuencia de escape '\\") + escape +
                                      "' no valida: solo se permiten \\\", \\n, \\t y \\\\",
                                  lineaBarra, columnaBarra, 2);
                    // Recuperacion: descartamos la barra y dejamos el
                    // caracter tal cual, para poder seguir escaneando.
                    contenido.push_back(escape);
                    break;
            }
            continue;
        }

        // Caracter regular: debe estar en el rango ASCII imprimible 32-126 (sec. 3.4.1).
        int codigo = static_cast<unsigned char>(c);
        if (codigo < 32 || codigo > 126) {
            reportarError("caracter no permitido dentro de la cadena: solo se aceptan "
                          "caracteres ASCII imprimibles",
                          linea, columna);
            leerCaracter(); // se descarta y seguimos buscando el cierre
            continue;
        }

        if (contenido.size() >= MAX_LARGO_CADENA) {
            if (!excedioLargo) {
                reportarError("la cadena es demasiado larga: el maximo es " +
                                  std::to_string(MAX_LARGO_CADENA) + " caracteres",
                              lineaInicio, columnaInicio);
                excedioLargo = true;
            }
            leerCaracter(); // seguimos consumiendo para poder llegar al cierre
            continue;
        }

        contenido.push_back(leerCaracter());
    }

    Posicion inicio{lineaInicio, columnaInicio};

    if (!cerrada) {
        return Token{TipoToken::UNKNOWN_ERROR, contenido, inicio};
    }

    TipoToken tipo = tieneFormaDeIdentificador(contenido) ? TipoToken::IDSTRING : TipoToken::STRING;
    return Token{tipo, contenido, inicio};
}

Token AnalizadorLexico::escanearOperador() {
    // Se asume que verCaracter() todavia no fue consumido. Igual que en el
    // resto del scanner, "maximal munch": antes de decidir un token de un
    // solo caracter, miramos si el siguiente caracter completa uno de dos
    // (sec. 3.5: -> == != <= >= //).
    int lineaInicio = linea;
    int columnaInicio = columna;
    char c = leerCaracter();

    auto crear = [&](TipoToken tipo, const std::string& lexema) {
        return Token{tipo, lexema, Posicion{lineaInicio, columnaInicio}};
    };
    auto invalido = [&](const std::string& lexema) {
        return Token{TipoToken::UNKNOWN_ERROR, lexema, Posicion{lineaInicio, columnaInicio}};
    };

    switch (c) {
        // --- Delimitadores de un solo caracter, sin ambiguedad ---
        case '(': return crear(TipoToken::LPAREN, "(");
        case ')': return crear(TipoToken::RPAREN, ")");
        case '[': return crear(TipoToken::LBRACKET, "[");
        case ']': return crear(TipoToken::RBRACKET, "]");
        case ',': return crear(TipoToken::COMMA, ",");
        case ':': return crear(TipoToken::COLON, ":");
        case '.': return crear(TipoToken::DOT, ".");

        // --- Operadores de un solo caracter, sin ambiguedad ---
        case '+': return crear(TipoToken::PLUS, "+");
        case '*': return crear(TipoToken::STAR, "*");
        case '%': return crear(TipoToken::PERCENT, "%");

        // --- '-' puede ser MINUS o el inicio de '->' ---
        case '-':
            if (verCaracter() == '>') {
                leerCaracter();
                return crear(TipoToken::ARROW, "->");
            }
            return crear(TipoToken::MINUS, "-");

        // --- '/' solo es valido como parte de '//' (division entera). ---
        // ChocoPy no tiene division real ('/'), a diferencia de Python
        // (sec. 2.6.3), asi que un '/' suelto es un operador mal formado.
        case '/':
            if (verCaracter() == '/') {
                leerCaracter();
                return crear(TipoToken::SLASH_SLASH, "//");
            }
            reportarError("'/' no existe en ChocoPy: para dividir enteros usa '//'",
                          lineaInicio, columnaInicio);
            return invalido("/");

        // --- '=' puede ser EQUAL o el inicio de '==' ---
        case '=':
            if (verCaracter() == '=') {
                leerCaracter();
                return crear(TipoToken::EQ_EQ, "==");
            }
            return crear(TipoToken::EQUAL, "=");

        // --- '<' puede ser LESS o el inicio de '<=' ---
        case '<':
            if (verCaracter() == '=') {
                leerCaracter();
                return crear(TipoToken::LESS_EQ, "<=");
            }
            return crear(TipoToken::LESS, "<");

        // --- '>' puede ser GREATER o el inicio de '>=' ---
        case '>':
            if (verCaracter() == '=') {
                leerCaracter();
                return crear(TipoToken::GREATER_EQ, ">=");
            }
            return crear(TipoToken::GREATER, ">");

        // --- '!' solo es valido como parte de '!=' (no existe '!' solo). ---
        case '!':
            if (verCaracter() == '=') {
                leerCaracter();
                return crear(TipoToken::BANG_EQ, "!=");
            }
            reportarError("'!' solo es valido dentro de '!=' (distinto de); para negar usa 'not'",
                          lineaInicio, columnaInicio);
            return invalido("!");

        // --- Cualquier otro simbolo no pertenece a la especificacion de
        //     ningun token de ChocoPy (p. ej. '@', '$', ';', '&', '|'). ---
        default:
            if (c == ';') {
                reportarError("';' no se usa en ChocoPy: escribe una sola sentencia por linea",
                              lineaInicio, columnaInicio);
            } else if (static_cast<unsigned char>(c) >= 128) {
                reportarError("caracter no ASCII: ChocoPy solo admite caracteres ASCII "
                              "fuera de los comentarios",
                              lineaInicio, columnaInicio);
            } else {
                reportarError(std::string("el caracter '") + c + "' no es valido en ChocoPy",
                              lineaInicio, columnaInicio);
            }
            return invalido(std::string(1, c));
    }
}

Token AnalizadorLexico::siguienteToken() {
    for (;;) {
        // 1) Si hay tokens pendientes (INDENT/DEDENT/NEWLINE sintetico/EOF),
        //    los devolvemos antes de tocar el input.
        if (!tokensPendientes.empty()) {
            Token t = tokensPendientes.front();
            tokensPendientes.pop_front();
            return t;
        }

        // 2) Inicio de linea logica: resolver indentacion / lineas en
        //    blanco / lineas de solo-comentario antes de seguir.
        if (inicioDeLinea) {
            procesarInicioDeLinea();
            continue; // vuelve a revisar la cola en el paso 1
        }

        // 3) Fin de archivo estando a mitad de una linea con contenido:
        //    el EOF actua como terminador implicito (sec. 3.1.1).
        if (finDeArchivo()) {
            tokensPendientes.push_back(Token{TipoToken::NEWLINE, "", Posicion{linea, columna}});
            inicioDeLinea = true; // la proxima vuelta dispara el cierre de la indentacion
            continue;
        }

        char c = verCaracter();

        // 4) Espacios/tabs entre tokens (no al inicio de linea): se descartan.
        if (c == ' ' || c == '\t') {
            leerCaracter();
            continue;
        }

        // 5) Comentario despues de tokens reales en la misma linea.
        if (c == '#') {
            saltarComentario();
            continue;
        }

        // 6) Fin de la linea logica actual.
        if (c == '\n') {
            int lineaInicio = linea, columnaInicio = columna;
            leerCaracter();
            inicioDeLinea = true;
            return Token{TipoToken::NEWLINE, "\n", Posicion{lineaInicio, columnaInicio}};
        }

        // 7) Identificadores y palabras clave (paso 5).
        if (esInicioIdentificador(c)) {
            return escanearIdentificador();
        }

        // 8) Literales enteros (paso 6).
        if (esDigito(c)) {
            return escanearEntero();
        }

        // 9) Literales de cadena (paso 6, parte 2).
        if (c == '"') {
            return escanearCadena();
        }

        // 10) Operadores y delimitadores (paso 7): cualquier otro
        //     caracter que llega hasta aca solo puede ser esto, o un
        //     simbolo invalido; escanearOperador() decide cual.
        return escanearOperador();
    }
}
