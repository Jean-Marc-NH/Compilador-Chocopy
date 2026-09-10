#include "generador.hpp"



// Generar la tabla 
// Reglas para la lectura de la gramatica
// Mayusculas -> No terminal
// Minusculas -> Terminal
// Vacio -> e
// Todo lo anterios a > va a ser un simbolo no terminal y mayusculas 
/*

    Ejemplo:

    START' > START
    START > hAh H c
    A > c
    H > h

    No terminales > START' START A H 
    Terminales > h c

*/

// Usaremos un multimap donde la clave es el lado izquierdo y los valores son el lado derecho


void parsearGramatica(string gramatica)
{

    bool derecho = false;

    string tokenDerecho ="";
    string tokenIzquierdo ="";

    for(auto g : gramatica)
    {
        if(!derecho && g > 64 && g < 91)
        {
            tokenDerecho += g
        }

        if(g = '>')
        {
            dercho = !derecho;
        }

        if(derecho && g != ' ')
        {
            tokenIzquierdo += g
        }

        if(g == ' ')
        {
            gramaticaCompleta = {tokenDerecho, tokenIzquierdo}
            tokenIzquierdo = " "

        }
    }
}