#ifndef PRUEBAS_PRUEBA_SEMANTICA_HPP
#define PRUEBAS_PRUEBA_SEMANTICA_HPP

#include <string>

// Pruebas del analisis semantico: programas validos (0 errores) y un programa
// por cada error semantico, comprobando la cantidad de errores, el mensaje y
// que no haya errores en cascada. Tambien revisa el AST con tipos y la tabla
// de simbolos. Recibe las mismas rutas que prueba_ast.
void prueba_semantica(const std::string& rutaGramatica, const std::string& rutaCache);

#endif
