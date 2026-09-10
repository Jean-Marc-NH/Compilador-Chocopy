#include <iostream> 
#include <multimap>
#include <string>

std::multimap<string,string> gramaticaCompleta; 


// Encargado de pasar un string tipo S > aBa a el map
void parsearGramatica(string gramatica);