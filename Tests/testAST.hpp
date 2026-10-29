#ifndef TEST_AST_HPP
#define TEST_AST_HPP

#include <string>

// Pruebas de Parser + AST: parsea fragmentos de ChocoPy y compara el volcado
// del AST con el texto esperado. Necesita la gramatica y una ruta donde
// guardar la cache de la tabla LR(1) (absolutas, porque los tests corren desde
// otro directorio).
void test_ast(const std::string& rutaGramatica, const std::string& rutaCache);

#endif
