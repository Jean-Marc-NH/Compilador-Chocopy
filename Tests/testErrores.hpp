#ifndef TEST_ERRORES_HPP
#define TEST_ERRORES_HPP

#include <string>

// Pruebas de los mensajes de error (scanner + parser): comprueba que cada
// programa con un error produzca el mensaje esperado, con su linea de codigo,
// y la cantidad correcta de errores lexicos y sintacticos (sin errores
// duplicados en cascada). Recibe las mismas rutas que test_ast.
void test_errores(const std::string& rutaGramatica, const std::string& rutaCache);

#endif
