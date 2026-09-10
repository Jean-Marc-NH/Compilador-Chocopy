#include "generador.hpp"
#include <iostream>

using namespace std;


int main()
{
    string prueba = " START > ABa AA B"

    parsearGramatica(prueba);

    for(auto i : gramaticaCompleta)
    {
        cout << i.first << " > " >> i.second >> endl;
    }
}