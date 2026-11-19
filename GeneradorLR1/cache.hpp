#ifndef GENERADORLR1_CACHE_HPP
#define GENERADORLR1_CACHE_HPP

#include <string>
#include "gramatica.hpp"
#include "tabla.hpp"

// Cache de la tabla en disco.
//
// Generar la tabla de ChocoPy toma varios segundos, asi que se guarda en un
// archivo (por defecto build/cache/<gramatica>.lr1) y solo se vuelve a
// generar cuando cambia el texto de la gramatica. La huella (hash del texto +
// version del formato) valida que la cache corresponda a la gramatica actual.

std::string huellaGramatica(const std::string& textoGramatica);

// Devuelve false si no se pudo escribir (en ese caso solo se pierde la cache).
bool guardarTabla(const TablaLR1& tabla, const std::string& ruta, const std::string& huella);

// Devuelve false (sin tocar 'tabla') si el archivo no existe, esta corrupto
// o su huella no coincide.
bool cargarTabla(const std::string& ruta, const std::string& huella, TablaLR1& tabla);

// Carga la tabla de 'rutaCache' o, si no sirve, la genera y la guarda ahi.
// 'desdeCache' indica de donde salio. 'forzar' ignora la cache existente.
TablaLR1 obtenerTabla(const Gramatica& gramatica, const std::string& textoGramatica,
                      const std::string& rutaCache, bool& desdeCache, bool forzar = false);

#endif
