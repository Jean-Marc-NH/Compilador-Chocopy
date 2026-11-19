#ifndef PRUEBAS_PRUEBA_LEXER_HPP
#define PRUEBAS_PRUEBA_LEXER_HPP

// Pruebas del scanner, una por cada paso con el que se fue armando (ver
// Scanner/lexer.hpp). Imprimen los tokens de cada caso y la cantidad de
// errores junto con la que se espera, para revisarlas a ojo.
void prueba_lexer_basico();
void prueba_lexer_indentacion();
void prueba_lexer_identificadores();
void prueba_lexer_enteros();
void prueba_lexer_cadenas();
void prueba_lexer_operadores();

#endif
