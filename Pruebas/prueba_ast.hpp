#ifndef PRUEBAS_PRUEBA_AST_HPP
#define PRUEBAS_PRUEBA_AST_HPP

#include <string>

// Pruebas de parser + AST: parsea fragmentos de ChocoPy y compara el volcado
// del AST con el texto esperado. Necesita la gramatica y una ruta donde
// guardar la cache de la tabla LR(1) (absolutas, porque las pruebas corren
// desde otro directorio).
void prueba_ast(const std::string& rutaGramatica, const std::string& rutaCache);

#endif
