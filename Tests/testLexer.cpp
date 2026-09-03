#include <iostream>
#include <fstream>
#include "Scanner/lexer.hpp"
#include "Scanner/tokens.hpp"
#include "testLexer.hpp"

// Escribe un archivo temporal con contenido dado, para no depender de
// tener el .py de prueba ya creado a mano en el disco.
static void writeTempFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

void test_lexer_basico()
{
    std::cout << "PRUEBA DE LEXER - PASO 1 (buffer + getchar_/peekchar + cola)\n\n";

    // --- Caso 1: normalizacion de fin de linea (LF, CRLF, CR) ---
    // "ab" + CRLF, "cd" + CR, "e" + LF
    writeTempFile("_tmp_mixed.py", "ab\r\ncd\re\n");
    {
        std::cout << "-- Caso 1: fin de linea mixto --\n";
        Lexer lex("_tmp_mixed.py");
        for (int i = 0; i < 12; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "Errores reportados: " << lex.errorCount() << "\n\n";
    }

    // --- Caso 2: gettoken() no debe romperse si se llama de mas tras EOF ---
    {
        std::cout << "-- Caso 2: gettoken() repetido tras EOF --\n";
        Lexer lex("_tmp_mixed.py");
        Token last;
        for (int i = 0; i < 30; ++i) last = lex.gettoken();
        std::cout << "Ultimo token (deberia seguir siendo END_OF_FILE): "
                   << tokenToString(last) << "\n\n";
    }

    // --- Caso 3: archivo inexistente no debe crashear ---
    {
        std::cout << "-- Caso 3: archivo inexistente --\n";
        Lexer lex("_este_archivo_no_existe.py");
        Token t = lex.gettoken();
        std::cout << tokenToString(t) << "\n\n";
    }

    // --- Caso 4: archivo vacio ---
    writeTempFile("_tmp_empty.py", "");
    {
        std::cout << "-- Caso 4: archivo vacio --\n";
        Lexer lex("_tmp_empty.py");
        Token t = lex.gettoken();
        std::cout << tokenToString(t) << "\n\n";
    }

    std::cout << "FIN DE PRUEBAS DE LEXER (paso 1)\n";
}

void test_lexer_indentacion()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 3 (indentacion / NEWLINE / INDENT / DEDENT)\n\n";

    // --- Caso A: el ejemplo oficial del enunciado ---
    // def suma(x: int, y: int) -> int:
    //     return x + y
    // Los IDENTIFIER/KW/etc. individuales todavia salen como UNKNOWN_ERROR
    // (no estan implementados hasta el paso 5+), pero la estructura de
    // NEWLINE/INDENT/DEDENT/EOF debe coincidir exactamente con la del PDF:
    // INDENT en (2:5), DEDENT y EOF en (3:1).
    writeTempFile("_tmp_ejemplo.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso A: ejemplo oficial (solo estructura NEWLINE/INDENT/DEDENT/EOF) --\n";
        Lexer lex("_tmp_ejemplo.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::NEWLINE || t.type == TokenType::INDENT ||
                t.type == TokenType::DEDENT || t.type == TokenType::END_OF_FILE) {
                std::cout << tokenToString(t) << "\n";
            }
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "\n";
    }

    // --- Caso B: multiples niveles de indentacion y varios DEDENT juntos ---
    writeTempFile("_tmp_niveles.py",
        "a\n"
        "    b\n"
        "        c\n"
        "d\n");
    {
        std::cout << "-- Caso B: multiples niveles (un DEDENT debe cerrar 2 niveles de una) --\n";
        Lexer lex("_tmp_niveles.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "\n";
    }

    // --- Caso C: lineas en blanco y comentarios no deben afectar la indentacion ---
    writeTempFile("_tmp_blancos.py",
        "a\n"
        "\n"
        "    # esto es un comentario, no cuenta\n"
        "    b\n"
        "\n"
        "c\n");
    {
        std::cout << "-- Caso C: lineas en blanco / solo-comentario se ignoran --\n";
        Lexer lex("_tmp_blancos.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::NEWLINE || t.type == TokenType::INDENT ||
                t.type == TokenType::DEDENT || t.type == TokenType::END_OF_FILE) {
                std::cout << tokenToString(t) << "\n";
            }
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "\n";
    }

    // --- Caso D: indentacion inconsistente (nivel que no matchea ningun nivel abierto) ---
    writeTempFile("_tmp_inconsistente.py",
        "a\n"
        "    b\n"
        "        c\n"
        "      d\n"  // 6 espacios: no coincide con 0, 4 ni 8
        "e\n");
    {
        std::cout << "-- Caso D: indentacion inconsistente (debe reportar error y resincronizar) --\n";
        Lexer lex("_tmp_inconsistente.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::NEWLINE || t.type == TokenType::INDENT ||
                t.type == TokenType::DEDENT || t.type == TokenType::END_OF_FILE) {
                std::cout << tokenToString(t) << "\n";
            }
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "Errores reportados: " << lex.errorCount()
                   << " (incluye los UNKNOWN_ERROR de contenido sin implementar; "
                      "lo que importa es que 'Indentacion inconsistente' aparezca 1 sola vez)\n\n";
    }

    // --- Caso E: tab en la indentacion (rechazado) ---
    writeTempFile("_tmp_tab.py", "a\n\tb\n");
    {
        std::cout << "-- Caso E: tab en indentacion (debe reportar error) --\n";
        Lexer lex("_tmp_tab.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::NEWLINE || t.type == TokenType::INDENT ||
                t.type == TokenType::DEDENT || t.type == TokenType::END_OF_FILE) {
                std::cout << tokenToString(t) << "\n";
            }
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "Errores reportados: " << lex.errorCount()
                   << " (incluye UNKNOWN_ERROR de contenido; 'Tabulacion no permitida' "
                      "debe aparecer exactamente 1 vez en stderr)\n\n";
    }

    // --- Caso F: archivo sin salto de linea final ---
    writeTempFile("_tmp_sin_newline.py", "    a");  // 4 espacios, "a", SIN \n final
    {
        std::cout << "-- Caso F: sin newline final (EOF debe actuar como terminador implicito) --\n";
        Lexer lex("_tmp_sin_newline.py");
        for (int i = 0; i < 200; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE INDENTACION (paso 3)\n";
}

void test_lexer_identificadores()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 5 (identificadores y palabras clave)\n\n";

    // --- Caso A: todas las keywords que SI tienen TokenType propio en ChocoPy ---
    writeTempFile("_tmp_kw_chocopy.py",
        "False None True and class def elif else for if in is not or pass return while\n");
    {
        std::cout << "-- Caso A: keywords propias de la gramatica de ChocoPy --\n";
        Lexer lex("_tmp_kw_chocopy.py");
        for (int i = 0; i < 30; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::NEWLINE || t.type == TokenType::END_OF_FILE) {
                if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
                continue; // no interesa el NEWLINE aca
            }
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "\n";
    }

    // --- Caso B: keywords reservadas por Python pero fuera de la gramatica de ChocoPy ---
    writeTempFile("_tmp_kw_reservadas.py",
        "as assert async await break continue del except finally from global "
        "import lambda nonlocal raise try with yield\n");
    {
        std::cout << "-- Caso B: reservadas por compatibilidad (deben salir KW_RESERVED, nunca IDENTIFIER) --\n";
        Lexer lex("_tmp_kw_reservadas.py");
        bool anyIdentifier = false;
        for (int i = 0; i < 30; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::IDENTIFIER) anyIdentifier = true;
        }
        std::cout << (anyIdentifier ? "FALLO: alguna aparecio como IDENTIFIER\n\n"
                                     : "OK: ninguna aparecio como IDENTIFIER\n\n");
    }

    // --- Caso C: la regla de "longest match" (una keyword puede ser substring de un identificador) ---
    writeTempFile("_tmp_longest_match.py", "classic ifdef forest _class class_\n");
    {
        std::cout << "-- Caso C: longest match (classic, ifdef, forest, _class, class_ deben ser IDENTIFIER) --\n";
        Lexer lex("_tmp_longest_match.py");
        bool allIdentifiers = true;
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
            if (t.type != TokenType::IDENTIFIER) allIdentifiers = false;
        }
        std::cout << (allIdentifiers ? "OK: todos IDENTIFIER\n\n" : "FALLO: alguno no fue IDENTIFIER\n\n");
    }

    // --- Caso D: identificadores con guion bajo y digitos (nunca empezando con digito) ---
    writeTempFile("_tmp_ident_variados.py", "_foo foo_bar2 x1 __init__ a_b_c_123\n");
    {
        std::cout << "-- Caso D: identificadores variados (con '_' y digitos) --\n";
        Lexer lex("_tmp_ident_variados.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "\n";
    }

    // --- Caso E: el ejemplo oficial del enunciado, ahora con identificadores/keywords reales ---
    // Los simbolos de puntuacion ( ) : , -> + siguen sin implementarse
    // (eso es el paso 7); lo que nos interesa validar aca es que
    // KW_DEF, IDENTIFIER "suma", "x", "int", etc. salgan bien.
    writeTempFile("_tmp_ejemplo_oficial.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso E: ejemplo oficial, identificadores/keywords ya reconocidos --\n";
        Lexer lex("_tmp_ejemplo_oficial.py");
        for (int i = 0; i < 100; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE IDENTIFICADORES Y KEYWORDS (paso 5)\n";
}

void test_lexer_enteros()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 6 (literales enteros)\n\n";

    // --- Caso A: enteros validos, incluido el limite exacto ---
    writeTempFile("_tmp_int_validos.py", "0 1 42 2147483647\n");
    {
        std::cout << "-- Caso A: enteros validos (0, 1, 42, el maximo 2147483647) --\n";
        Lexer lex("_tmp_int_validos.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    // --- Caso B: ceros a la izquierda (invalido salvo "0" solo) ---
    writeTempFile("_tmp_int_ceros.py", "0 007 0123 00\n");
    {
        std::cout << "-- Caso B: ceros a la izquierda (007, 0123, 00 deben ser error; el primer 0 no) --\n";
        Lexer lex("_tmp_int_ceros.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 3)\n\n";
    }

    // --- Caso C: limite exacto de rango (2147483647 valido, 2147483648 invalido) ---
    writeTempFile("_tmp_int_limite.py", "2147483647 2147483648\n");
    {
        std::cout << "-- Caso C: frontera del rango de 32 bits --\n";
        Lexer lex("_tmp_int_limite.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1, solo el segundo)\n\n";
    }

    // --- Caso D: numero muy grande (muchos digitos, no debe crashear ni desbordar) ---
    writeTempFile("_tmp_int_gigante.py", "999999999999999999999999999999999999999999\n");
    {
        std::cout << "-- Caso D: numero con 42 digitos (no debe crashear) --\n";
        Lexer lex("_tmp_int_gigante.py");
        for (int i = 0; i < 5; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso E: entero pegado a un identificador (sin separador) ---
    // El scanner corta en el primer no-digito: INTEGER "123" + IDENTIFIER "x".
    // No es un error lexico en si (el manual no lo prohibe explicitamente
    // a este nivel); sera el parser el que decida si esa secuencia tiene
    // sentido sintactico.
    writeTempFile("_tmp_int_pegado.py", "123x\n");
    {
        std::cout << "-- Caso E: '123x' sin espacio -> INTEGER('123') + IDENTIFIER('x') --\n";
        Lexer lex("_tmp_int_pegado.py");
        for (int i = 0; i < 5; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "FIN DE PRUEBAS DE LITERALES ENTEROS (paso 6)\n";
}

void test_lexer_strings()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 6, PARTE 2 (literales de cadena)\n\n";

    // --- Caso A: ejemplos de la tabla del manual (sec. 3.4.1) ---
    // "Hello"        -> Hello
    // "He\"ll\"o"    -> He"ll"o
    // "He\\\"llo"    -> He\"llo
    writeTempFile("_tmp_str_tabla.py",
        "\"Hello\" \"He\\\"ll\\\"o\" \"He\\\\\\\"llo\"\n");
    {
        std::cout << "-- Caso A: ejemplos de la tabla del manual --\n";
        Lexer lex("_tmp_str_tabla.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    // --- Caso B: IDSTRING vs STRING ---
    // "cow" tiene sintaxis de identificador -> IDSTRING (uso tipico: anotaciones de tipo)
    // "Hello World" y "" (vacio) no la tienen -> STRING
    writeTempFile("_tmp_str_idstring.py", "\"cow\" \"Hello World\" \"\"\n");
    {
        std::cout << "-- Caso B: IDSTRING (\"cow\") vs STRING (\"Hello World\", \"\") --\n";
        Lexer lex("_tmp_str_idstring.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "\n";
    }

    // --- Caso C: cadena sin cerrar por fin de linea (el '\n' no se debe consumir) ---
    writeTempFile("_tmp_str_sin_cerrar_linea.py", "\"abc\nx\n");
    {
        std::cout << "-- Caso C: cadena sin cerrar por NEWLINE (debe seguir tokenizando bien despues) --\n";
        Lexer lex("_tmp_str_sin_cerrar_linea.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso D: cadena sin cerrar por EOF ---
    writeTempFile("_tmp_str_sin_cerrar_eof.py", "\"abc");
    {
        std::cout << "-- Caso D: cadena sin cerrar por EOF --\n";
        Lexer lex("_tmp_str_sin_cerrar_eof.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso E: escape invalido (del ejemplo del manual: "Hell\o") ---
    writeTempFile("_tmp_str_escape_invalido.py", "\"Hell\\o\"\n");
    {
        std::cout << "-- Caso E: escape invalido \\o (debe reportar error y seguir) --\n";
        Lexer lex("_tmp_str_escape_invalido.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso F: caracter de control crudo (fuera de ASCII 32-126) dentro de la cadena ---
    // Un tab literal (byte 0x09) dentro de las comillas, NO como "\t" escapado.
    writeTempFile("_tmp_str_charinvalido.py", std::string("\"a\tb\"\n"));
    {
        std::cout << "-- Caso F: tab crudo dentro de la cadena (fuera de ASCII 32-126) --\n";
        Lexer lex("_tmp_str_charinvalido.py");
        for (int i = 0; i < 6; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso G: cadena que excede el tamano maximo (solo 1 error, no uno por caracter) ---
    {
        std::string huge(5000, 'a');
        writeTempFile("_tmp_str_gigante.py", "\"" + huge + "\"\n");
        std::cout << "-- Caso G: cadena de 5000 caracteres (excede el limite elegido de 4096) --\n";
        Lexer lex("_tmp_str_gigante.py");
        Token t = lex.gettoken();
        std::cout << "Tipo: " << tokenTypeToString(t.type)
                   << ", largo del lexema: " << t.lexeme.size() << "\n";
        std::cout << "Errores: " << lex.errorCount() << " (se espera exactamente 1)\n\n";
    }

    std::cout << "FIN DE PRUEBAS DE LITERALES DE CADENA (paso 6, parte 2)\n";
}

void test_lexer_operadores()
{
    std::cout << "\nPRUEBA DE LEXER - PASO 7 (operadores y delimitadores)\n\n";

    // --- Caso A: todos los operadores/delimitadores de un solo caracter (sec. 3.5) ---
    writeTempFile("_tmp_op_simples.py", "+ * % ( ) [ ] , : .\n");
    {
        std::cout << "-- Caso A: simbolos de un solo caracter --\n";
        Lexer lex("_tmp_op_simples.py");
        for (int i = 0; i < 15; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    // --- Caso B: operadores de dos caracteres (maximal munch) ---
    writeTempFile("_tmp_op_dobles.py", "-> == != <= >= //\n");
    {
        std::cout << "-- Caso B: operadores de dos caracteres --\n";
        Lexer lex("_tmp_op_dobles.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    // --- Caso C: prefijos que se resuelven al caracter corto cuando no matchean ---
    // '-' sin '>' -> MINUS; '=' sin '=' -> EQUAL; '<'/'>' sin '=' -> LESS/GREATER
    writeTempFile("_tmp_op_prefijos.py", "- = < >\n");
    {
        std::cout << "-- Caso C: '-','=','<','>' solos (no deben confundirse con sus formas dobles) --\n";
        Lexer lex("_tmp_op_prefijos.py");
        for (int i = 0; i < 8; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    // --- Caso D: '/' suelto es invalido (ChocoPy solo tiene '//') ---
    writeTempFile("_tmp_op_slash_suelto.py", "a / b\n");
    {
        std::cout << "-- Caso D: '/' suelto (debe ser UNKNOWN_ERROR, y seguir escaneando 'a' y 'b') --\n";
        Lexer lex("_tmp_op_slash_suelto.py");
        for (int i = 0; i < 8; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso E: '!' suelto es invalido (solo existe '!=') ---
    writeTempFile("_tmp_op_bang_suelto.py", "a ! b\n");
    {
        std::cout << "-- Caso E: '!' suelto (debe ser UNKNOWN_ERROR) --\n";
        Lexer lex("_tmp_op_bang_suelto.py");
        for (int i = 0; i < 8; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 1)\n\n";
    }

    // --- Caso F: simbolos que no pertenecen a ningun token de ChocoPy ---
    writeTempFile("_tmp_op_invalidos.py", "@ $ ; &\n");
    {
        std::cout << "-- Caso F: simbolos invalidos (deben reportar error y seguir, uno por caracter) --\n";
        Lexer lex("_tmp_op_invalidos.py");
        for (int i = 0; i < 10; ++i) {
            Token t = lex.gettoken();
            if (t.type == TokenType::END_OF_FILE) { std::cout << tokenToString(t) << "\n"; break; }
            if (t.type == TokenType::NEWLINE) continue;
            std::cout << tokenToString(t) << "\n";
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 4)\n\n";
    }

    // --- Caso G: el ejemplo oficial del enunciado, ahora completo de punta a punta ---
    writeTempFile("_tmp_ejemplo_completo.py",
        "def suma(x: int, y: int) -> int:\n"
        "    return x + y\n");
    {
        std::cout << "-- Caso G: ejemplo oficial, scanner completo (debe coincidir con el PDF) --\n";
        Lexer lex("_tmp_ejemplo_completo.py");
        for (int i = 0; i < 100; ++i) {
            Token t = lex.gettoken();
            std::cout << tokenToString(t) << "\n";
            if (t.type == TokenType::END_OF_FILE) break;
        }
        std::cout << "Errores: " << lex.errorCount() << " (se espera 0)\n\n";
    }

    std::cout << "FIN DE PRUEBAS DE OPERADORES Y DELIMITADORES (paso 7)\n";
}