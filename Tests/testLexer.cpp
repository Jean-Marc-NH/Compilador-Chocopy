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
