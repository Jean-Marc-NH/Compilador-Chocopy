#include <fstream>
#include <iostream>
#include <vector>
#include "Scanner/lexer.hpp"
#include "Scanner/tokens.hpp"
#include "prueba_lexer.hpp"

namespace {

// Escribe un archivo temporal con el contenido dado, para no depender de
// tener el .py de prueba ya creado a mano en el disco.
void escribirTemporal(const std::string& ruta, const std::string& contenido) {
    std::ofstream salida(ruta, std::ios::binary);
    salida << contenido;
}

// Que tokens se imprimen en cada caso. END_OF_FILE siempre se imprime.
enum class Filtro {
    TODOS,           // todos los tokens
    SIN_NEWLINE,     // todos menos NEWLINE (en casos de una sola linea no aporta)
    SOLO_ESTRUCTURA  // solo NEWLINE / INDENT / DEDENT (pruebas de indentacion)
};

struct Escaneo {
    std::vector<Token> tokens; // los que pasaron el filtro, sin el END_OF_FILE
    int errores = 0;
};

// Escanea 'ruta' pidiendo como mucho 'maximo' tokens (un tope por si el
// scanner se quedara en un ciclo) e imprime los que pasan el filtro.
Escaneo escanearEImprimir(const std::string& ruta, int maximo, Filtro filtro) {
    Escaneo resultado;
    AnalizadorLexico scanner(ruta);
    for (int i = 0; i < maximo; ++i) {
        Token t = scanner.siguienteToken();
        if (t.tipo == TipoToken::END_OF_FILE) {
            std::cout << tokenATexto(t) << "\n";
            break;
        }
        bool estructura = t.tipo == TipoToken::NEWLINE || t.tipo == TipoToken::INDENT ||
                          t.tipo == TipoToken::DEDENT;
        if (filtro == Filtro::SIN_NEWLINE && t.tipo == TipoToken::NEWLINE) continue;
        if (filtro == Filtro::SOLO_ESTRUCTURA && !estructura) continue;
        std::cout << tokenATexto(t) << "\n";
        resultado.tokens.push_back(t);
    }
    resultado.errores = scanner.cantidadErrores();
    return resultado;
}

} // namespace

void prueba_lexer_basico()
{
    std::cout << "PRUEBA DE LEXER - PASO 1 (buffer + leerCaracter/verCaracter + cola)\n\n";

    // --- Caso 1: normalizacion de fin de linea (LF, CRLF, CR) ---
    // "ab" + CRLF, "cd" + CR, "e" + LF
    escribirTemporal("_tmp_mixed.py", "ab\r\ncd\re\n");
    {
        std::cout << "-- Caso 1: fin de linea mixto --\n";
        Escaneo e = escanearEImprimir("_tmp_mixed.py", 12, Filtro::TODOS);
        std::cout << "Errores reportados: " << e.errores << "\n\n";
    }

    // --- Caso 2: siguienteToken() no debe romperse si se llama de mas tras EOF ---
    {
        std::cout << "-- Caso 2: siguienteToken() repetido tras EOF --\n";
        AnalizadorLexico scanner("_tmp_mixed.py");
        Token ultimo;
        for (int i = 0; i < 30; ++i) ultimo = scanner.siguienteToken();
        std::cout << "Ultimo token (deberia seguir siendo END_OF_FILE): "
                  << tokenATexto(ultimo) << "\n\n";
    }

    // --- Caso 3: archivo inexistente no debe crashear ---
    {
        std::cout << "-- Caso 3: archivo inexistente --\n";
        escanearEImprimir("_este_archivo_no_existe.py", 1, Filtro::TODOS);
        std::cout << "\n";
    }

    // --- Caso 4: archivo vacio ---
    escribirTemporal("_tmp_empty.py", "");
    {
        std::cout << "-- Caso 4: archivo vacio --\n";
        escanearEImprimir("_tmp_empty.py", 1, Filtro::TODOS);
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE LEXER (paso 1)\n";
}

void prueba_lexer_indentacion()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 3 (indentacion / NEWLINE / INDENT / DEDENT)\n\n";

    // --- Caso A: el ejemplo oficial del enunciado ---
    // def suma(x: int, y: int) -> int:
    //     return x + y
    // Solo miramos la estructura: NEWLINE/INDENT/DEDENT/EOF debe coincidir
    // exactamente con la del PDF: INDENT en (2:5), DEDENT y EOF en (3:1).
    escribirTemporal("_tmp_ejemplo.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso A: ejemplo oficial (solo estructura NEWLINE/INDENT/DEDENT/EOF) --\n";
        escanearEImprimir("_tmp_ejemplo.py", 200, Filtro::SOLO_ESTRUCTURA);
        std::cout << "\n";
    }

    // --- Caso B: multiples niveles de indentacion y varios DEDENT juntos ---
    escribirTemporal("_tmp_niveles.py",
        "a\n"
        "    b\n"
        "        c\n"
        "d\n");
    {
        std::cout << "-- Caso B: multiples niveles (un DEDENT debe cerrar 2 niveles de una) --\n";
        escanearEImprimir("_tmp_niveles.py", 200, Filtro::TODOS);
        std::cout << "\n";
    }

    // --- Caso C: lineas en blanco y comentarios no deben afectar la indentacion ---
    escribirTemporal("_tmp_blancos.py",
        "a\n"
        "\n"
        "    # esto es un comentario, no cuenta\n"
        "    b\n"
        "\n"
        "c\n");
    {
        std::cout << "-- Caso C: lineas en blanco / solo-comentario se ignoran --\n";
        escanearEImprimir("_tmp_blancos.py", 200, Filtro::SOLO_ESTRUCTURA);
        std::cout << "\n";
    }

    // --- Caso D: indentacion inconsistente (nivel que no coincide con ningun nivel abierto) ---
    escribirTemporal("_tmp_inconsistente.py",
        "a\n"
        "    b\n"
        "        c\n"
        "      d\n"  // 6 espacios: no coincide con 0, 4 ni 8
        "e\n");
    {
        std::cout << "-- Caso D: indentacion inconsistente (debe reportar error y resincronizar) --\n";
        Escaneo e = escanearEImprimir("_tmp_inconsistente.py", 200, Filtro::SOLO_ESTRUCTURA);
        std::cout << "Errores reportados: " << e.errores
                  << " (se espera 1: el de la linea 4, y una sola vez)\n\n";
    }

    // --- Caso E: tab en la indentacion (rechazado) ---
    escribirTemporal("_tmp_tab.py", "a\n\tb\n");
    {
        std::cout << "-- Caso E: tab en indentacion (debe reportar error) --\n";
        Escaneo e = escanearEImprimir("_tmp_tab.py", 200, Filtro::SOLO_ESTRUCTURA);
        std::cout << "Errores reportados: " << e.errores
                  << " (se espera 1: el del tabulador, una sola vez en stderr)\n\n";
    }

    // --- Caso F: archivo sin salto de linea final ---
    escribirTemporal("_tmp_sin_newline.py", "    a");  // 4 espacios, "a", SIN \n final
    {
        std::cout << "-- Caso F: sin newline final (EOF debe actuar como terminador implicito) --\n";
        escanearEImprimir("_tmp_sin_newline.py", 200, Filtro::TODOS);
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE INDENTACION (paso 3)\n";
}

void prueba_lexer_identificadores()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 5 (identificadores y palabras clave)\n\n";

    // --- Caso A: todas las palabras clave que SI tienen TipoToken propio en ChocoPy ---
    escribirTemporal("_tmp_kw_chocopy.py",
        "False None True and class def elif else for if in is not or pass return while\n");
    {
        std::cout << "-- Caso A: palabras clave propias de la gramatica de ChocoPy --\n";
        escanearEImprimir("_tmp_kw_chocopy.py", 30, Filtro::SIN_NEWLINE);
        std::cout << "\n";
    }

    // --- Caso B: palabras reservadas por Python pero fuera de la gramatica de ChocoPy ---
    // (global y nonlocal si estan en la gramatica, asi que salen con su propio token)
    escribirTemporal("_tmp_kw_reservadas.py",
        "as assert async await break continue del except finally from global "
        "import lambda nonlocal raise try with yield\n");
    {
        std::cout << "-- Caso B: reservadas por compatibilidad (deben salir KW_RESERVED, nunca IDENTIFIER) --\n";
        Escaneo e = escanearEImprimir("_tmp_kw_reservadas.py", 30, Filtro::SIN_NEWLINE);
        bool algunIdentificador = false;
        for (const Token& t : e.tokens)
            if (t.tipo == TipoToken::IDENTIFIER) algunIdentificador = true;
        std::cout << (algunIdentificador ? "FALLO: alguna aparecio como IDENTIFIER\n\n"
                                         : "OK: ninguna aparecio como IDENTIFIER\n\n");
    }

    // --- Caso C: la regla de "longest match" (una palabra clave puede ser parte de un identificador) ---
    escribirTemporal("_tmp_longest_match.py", "classic ifdef forest _class class_\n");
    {
        std::cout << "-- Caso C: longest match (classic, ifdef, forest, _class, class_ deben ser IDENTIFIER) --\n";
        Escaneo e = escanearEImprimir("_tmp_longest_match.py", 10, Filtro::SIN_NEWLINE);
        bool todosIdentificadores = true;
        for (const Token& t : e.tokens)
            if (t.tipo != TipoToken::IDENTIFIER) todosIdentificadores = false;
        std::cout << (todosIdentificadores ? "OK: todos IDENTIFIER\n\n" : "FALLO: alguno no fue IDENTIFIER\n\n");
    }

    // --- Caso D: identificadores con guion bajo y digitos (nunca empezando con digito) ---
    escribirTemporal("_tmp_ident_variados.py", "_foo foo_bar2 x1 __init__ a_b_c_123\n");
    {
        std::cout << "-- Caso D: identificadores variados (con '_' y digitos) --\n";
        escanearEImprimir("_tmp_ident_variados.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "\n";
    }

    // --- Caso E: el ejemplo oficial del enunciado con identificadores y palabras clave ---
    // Lo que interesa validar aca es que KW_DEF, IDENTIFIER "suma", "x",
    // "int", etc. salgan bien; el caso completo esta en las pruebas del paso 7.
    escribirTemporal("_tmp_ejemplo_oficial.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso E: ejemplo oficial, identificadores y palabras clave --\n";
        escanearEImprimir("_tmp_ejemplo_oficial.py", 100, Filtro::TODOS);
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE IDENTIFICADORES Y PALABRAS CLAVE (paso 5)\n";
}

void prueba_lexer_enteros()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 6 (literales enteros)\n\n";

    // --- Caso A: enteros validos, incluido el limite exacto ---
    escribirTemporal("_tmp_int_validos.py", "0 1 42 2147483647\n");
    {
        std::cout << "-- Caso A: enteros validos (0, 1, 42, el maximo 2147483647) --\n";
        Escaneo e = escanearEImprimir("_tmp_int_validos.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    // --- Caso B: ceros a la izquierda (invalido salvo "0" solo) ---
    escribirTemporal("_tmp_int_ceros.py", "0 007 0123 00\n");
    {
        std::cout << "-- Caso B: ceros a la izquierda (007, 0123, 00 deben ser error; el primer 0 no) --\n";
        Escaneo e = escanearEImprimir("_tmp_int_ceros.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 3)\n\n";
    }

    // --- Caso C: limite exacto de rango (2147483647 valido, 2147483648 invalido) ---
    escribirTemporal("_tmp_int_limite.py", "2147483647 2147483648\n");
    {
        std::cout << "-- Caso C: frontera del rango de 32 bits --\n";
        Escaneo e = escanearEImprimir("_tmp_int_limite.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1, solo el segundo)\n\n";
    }

    // --- Caso D: numero muy grande (muchos digitos, no debe crashear ni desbordar) ---
    escribirTemporal("_tmp_int_gigante.py", "999999999999999999999999999999999999999999\n");
    {
        std::cout << "-- Caso D: numero con 42 digitos (no debe crashear) --\n";
        Escaneo e = escanearEImprimir("_tmp_int_gigante.py", 5, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso E: entero pegado a un identificador (sin separador) ---
    // El scanner corta en el primer no-digito: INTEGER "123" + IDENTIFIER "x".
    // No es un error lexico en si (el manual no lo prohibe explicitamente
    // a este nivel); es el parser el que decide si esa secuencia tiene
    // sentido sintactico.
    escribirTemporal("_tmp_int_pegado.py", "123x\n");
    {
        std::cout << "-- Caso E: '123x' sin espacio -> INTEGER('123') + IDENTIFIER('x') --\n";
        escanearEImprimir("_tmp_int_pegado.py", 5, Filtro::SIN_NEWLINE);
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE LITERALES ENTEROS (paso 6)\n";
}

void prueba_lexer_cadenas()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 6, PARTE 2 (literales de cadena)\n\n";

    // --- Caso A: ejemplos de la tabla del manual (sec. 3.4.1) ---
    // "Hello"        -> Hello
    // "He\"ll\"o"    -> He"ll"o
    // "He\\\"llo"    -> He\"llo
    escribirTemporal("_tmp_str_tabla.py",
        "\"Hello\" \"He\\\"ll\\\"o\" \"He\\\\\\\"llo\"\n");
    {
        std::cout << "-- Caso A: ejemplos de la tabla del manual --\n";
        Escaneo e = escanearEImprimir("_tmp_str_tabla.py", 6, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    // --- Caso B: IDSTRING vs STRING ---
    // "cow" tiene sintaxis de identificador -> IDSTRING (uso tipico: anotaciones de tipo)
    // "Hello World" y "" (vacio) no la tienen -> STRING
    escribirTemporal("_tmp_str_idstring.py", "\"cow\" \"Hello World\" \"\"\n");
    {
        std::cout << "-- Caso B: IDSTRING (\"cow\") vs STRING (\"Hello World\", \"\") --\n";
        escanearEImprimir("_tmp_str_idstring.py", 6, Filtro::SIN_NEWLINE);
        std::cout << "\n";
    }

    // --- Caso C: cadena sin cerrar por fin de linea (el '\n' no se debe consumir) ---
    escribirTemporal("_tmp_str_sin_cerrar_linea.py", "\"abc\nx\n");
    {
        std::cout << "-- Caso C: cadena sin cerrar por NEWLINE (debe seguir tokenizando bien despues) --\n";
        Escaneo e = escanearEImprimir("_tmp_str_sin_cerrar_linea.py", 6, Filtro::TODOS);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso D: cadena sin cerrar por EOF ---
    escribirTemporal("_tmp_str_sin_cerrar_eof.py", "\"abc");
    {
        std::cout << "-- Caso D: cadena sin cerrar por EOF --\n";
        Escaneo e = escanearEImprimir("_tmp_str_sin_cerrar_eof.py", 6, Filtro::TODOS);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso E: escape invalido (del ejemplo del manual: "Hell\o") ---
    escribirTemporal("_tmp_str_escape_invalido.py", "\"Hell\\o\"\n");
    {
        std::cout << "-- Caso E: escape invalido \\o (debe reportar error y seguir) --\n";
        Escaneo e = escanearEImprimir("_tmp_str_escape_invalido.py", 6, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso F: caracter de control crudo (fuera de ASCII 32-126) dentro de la cadena ---
    // Un tab literal (byte 0x09) dentro de las comillas, NO como "\t" escapado.
    escribirTemporal("_tmp_str_charinvalido.py", std::string("\"a\tb\"\n"));
    {
        std::cout << "-- Caso F: tab crudo dentro de la cadena (fuera de ASCII 32-126) --\n";
        Escaneo e = escanearEImprimir("_tmp_str_charinvalido.py", 6, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso G: cadena que excede el tamano maximo (solo 1 error, no uno por caracter) ---
    {
        std::string enorme(5000, 'a');
        escribirTemporal("_tmp_str_gigante.py", "\"" + enorme + "\"\n");
        std::cout << "-- Caso G: cadena de 5000 caracteres (excede el limite elegido de 4096) --\n";
        AnalizadorLexico scanner("_tmp_str_gigante.py");
        Token t = scanner.siguienteToken();
        std::cout << "Tipo: " << tipoTokenATexto(t.tipo)
                  << ", largo del lexema: " << t.lexema.size() << "\n";
        std::cout << "Errores: " << scanner.cantidadErrores() << " (se espera exactamente 1)\n\n";
    }

    std::cout << "FIN DE PRUEBAS DE LITERALES DE CADENA (paso 6, parte 2)\n";
}

void prueba_lexer_operadores()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 7 (operadores y delimitadores)\n\n";

    // --- Caso A: todos los operadores/delimitadores de un solo caracter (sec. 3.5) ---
    escribirTemporal("_tmp_op_simples.py", "+ * % ( ) [ ] , : .\n");
    {
        std::cout << "-- Caso A: simbolos de un solo caracter --\n";
        Escaneo e = escanearEImprimir("_tmp_op_simples.py", 15, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    // --- Caso B: operadores de dos caracteres (maximal munch) ---
    escribirTemporal("_tmp_op_dobles.py", "-> == != <= >= //\n");
    {
        std::cout << "-- Caso B: operadores de dos caracteres --\n";
        Escaneo e = escanearEImprimir("_tmp_op_dobles.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    // --- Caso C: prefijos que se resuelven al caracter corto cuando no hay segundo caracter ---
    // '-' sin '>' -> MINUS; '=' sin '=' -> EQUAL; '<'/'>' sin '=' -> LESS/GREATER
    escribirTemporal("_tmp_op_prefijos.py", "- = < >\n");
    {
        std::cout << "-- Caso C: '-','=','<','>' solos (no deben confundirse con sus formas dobles) --\n";
        Escaneo e = escanearEImprimir("_tmp_op_prefijos.py", 8, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    // --- Caso D: '/' suelto es invalido (ChocoPy solo tiene '//') ---
    escribirTemporal("_tmp_op_slash_suelto.py", "a / b\n");
    {
        std::cout << "-- Caso D: '/' suelto (debe ser UNKNOWN_ERROR, y seguir escaneando 'a' y 'b') --\n";
        Escaneo e = escanearEImprimir("_tmp_op_slash_suelto.py", 8, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso E: '!' suelto es invalido (solo existe '!=') ---
    escribirTemporal("_tmp_op_bang_suelto.py", "a ! b\n");
    {
        std::cout << "-- Caso E: '!' suelto (debe ser UNKNOWN_ERROR) --\n";
        Escaneo e = escanearEImprimir("_tmp_op_bang_suelto.py", 8, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 1)\n\n";
    }

    // --- Caso F: simbolos que no pertenecen a ningun token de ChocoPy ---
    escribirTemporal("_tmp_op_invalidos.py", "@ $ ; &\n");
    {
        std::cout << "-- Caso F: simbolos invalidos (deben reportar error y seguir, uno por caracter) --\n";
        Escaneo e = escanearEImprimir("_tmp_op_invalidos.py", 10, Filtro::SIN_NEWLINE);
        std::cout << "Errores: " << e.errores << " (se espera 4)\n\n";
    }

    // --- Caso G: el ejemplo oficial del enunciado, completo de punta a punta ---
    escribirTemporal("_tmp_ejemplo_completo.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso G: ejemplo oficial, scanner completo (debe coincidir con el PDF) --\n";
        Escaneo e = escanearEImprimir("_tmp_ejemplo_completo.py", 100, Filtro::TODOS);
        std::cout << "Errores: " << e.errores << " (se espera 0)\n\n";
    }

    std::cout << "FIN DE PRUEBAS DE OPERADORES Y DELIMITADORES (paso 7)\n";
}
