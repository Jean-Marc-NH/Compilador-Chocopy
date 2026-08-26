#include <iostream>
#include <fstream>
#include "Scanner/lexer.hpp"
#include "Scanner/tokens.hpp"
#include "testLexer.hpp"

void test_lexer_basico()
{
    std::cout << "PRUEBA DE LEXER - Archivo real\n\n";

    const std::string rutaArchivo = "prueba_1.py"; 

    Lexer lex(rutaArchivo);

    std::cout << "-- Tokenizando: " << rutaArchivo << " --\n";
    
    while (true) {
        Token t = lex.gettoken();
        std::cout << tokenToString(t) << "\n";
        
        if (t.type == TokenType::END_OF_FILE) {
            break;
        }
    }

    std::cout << "\nErrores reportados: " << lex.errorCount() << "\n";
    std::cout << "FIN DE PRUEBA\n";
}
