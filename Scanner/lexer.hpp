#ifndef SCANNER_LEXER_HPP
#define SCANNER_LEXER_HPP

#include <deque>
#include <string>
#include <vector>
#include "tokens.hpp"

// Scanner (analizador lexico) de ChocoPy.
//
// Responsable de:
//   - Leer el archivo fuente completo a memoria.
//   - Exponer leerCaracter()/verCaracter() para el resto del scanner
//     (son el getchar/peekchar de las diapositivas).
//   - Mantener linea:columna actualizadas caracter a caracter.
//   - Encolar tokens que no consumen caracteres (INDENT/DEDENT) para
//     que siguienteToken() los devuelva antes de seguir leyendo el archivo.
//   - Reportar los errores lexicos con la linea de codigo y el subrayado.
//
// El scanner se armo por pasos (ver las pruebas en Pruebas/prueba_lexer.cpp):
//   paso 1: buffer + lectura de caracteres + cola de tokens
//   paso 3: indentacion (NEWLINE / INDENT / DEDENT)
//   paso 5: identificadores y palabras clave
//   paso 6: literales enteros y de cadena
//   paso 7: operadores y delimitadores
class AnalizadorLexico {
public:
    // Carga el codigo fuente desde un archivo en disco.
    explicit AnalizadorLexico(const std::string& archivo);

    // Punto de entrada que el parser LR(1) invoca para pedir el siguiente
    // token (el gettoken() del enunciado). Primero drena la cola de
    // pendientes y despues sigue leyendo el archivo. Al llegar al final
    // devuelve END_OF_FILE las veces que se lo pidan.
    Token siguienteToken();

    // Cantidad de errores lexicos detectados durante el escaneo.
    int cantidadErrores() const { return errores; }

    // Nombre del archivo tal como se paso al constructor (para los mensajes).
    const std::string& nombreArchivo() const { return archivo; }

    // Texto de la linea 'n' (1 = primera) del codigo fuente, sin el salto de
    // linea. Devuelve false si esa linea no existe. Sirve para mostrar el
    // codigo junto a cada mensaje de error.
    bool lineaDeCodigo(int n, std::string& salida) const;

private:
    std::string archivo;

    // --- Estado del buffer de entrada ---
    std::string fuente;    // contenido completo del archivo (fines de linea normalizados a '\n')
    size_t pos = 0;        // indice del proximo caracter a leer en 'fuente'
    int linea = 1;
    int columna = 1;

    // --- Cola de tokens que no consumen caracteres del input ---
    std::deque<Token> tokensPendientes;

    // --- Manejo de indentacion ---
    std::vector<int> pilaIndentacion{0}; // pila inicializada con el nivel 0
    bool inicioDeLinea = true;           // true si el proximo caracter a leer inicia una linea logica

    // --- Contadores de diagnostico ---
    int errores = 0;

    // --- Lectura de caracteres ---
    // Devuelve el siguiente caracter y avanza el puntero. Devuelve '\0' si ya no hay mas input.
    char leerCaracter();

    // Devuelve el caracter que esta 'adelante' posiciones despues del puntero, sin moverlo.
    // verCaracter() (adelante = 0) mira el proximo caracter a leer.
    char verCaracter(int adelante = 0) const;

    bool finDeArchivo() const;

    // --- Utilidades internas ---
    // Carga 'ruta' completo en 'fuente', normalizando \r\n y \r a \n.
    void cargarArchivo(const std::string& ruta);

    // Reporta un error lexico con mensaje, linea y columna, mostrando la linea
    // de codigo con el subrayado de 'largo' caracteres (ver Comun/diagnostico.hpp).
    // Incrementa 'errores'.
    void reportarError(const std::string& mensaje, int enLinea, int enColumna, int largo = 1);

    // --- Indentacion (paso 3) ---
    // Se invoca cuando inicioDeLinea == true. Recorre, internamente, todas
    // las lineas fisicas en blanco o de solo-comentario hasta encontrar
    // contenido real o EOF. Al encontrar contenido real, compara el nivel
    // de indentacion contra el tope de 'pilaIndentacion' y encola INDENT/DEDENT
    // segun corresponda, dejando inicioDeLinea en false. Si llega a EOF sin
    // encontrar contenido, delega en cerrarIndentacionYFin().
    void procesarInicioDeLinea();

    // Consume caracteres desde '#' (asumido ya en verCaracter()) hasta el
    // siguiente '\n' o EOF, sin emitir ningun token.
    void saltarComentario();

    // Desapila 'pilaIndentacion' hasta dejar solo el nivel 0, encolando un
    // DEDENT por cada nivel que se cierra, y finalmente encola END_OF_FILE.
    void cerrarIndentacionYFin();

    // --- Identificadores y palabras clave (paso 5) ---
    // Se invoca cuando verCaracter() ya cumplio la condicion de inicio de
    // identificador (letra o '_'). Consume el resto del lexema (letras,
    // digitos, '_') y decide si es IDENTIFIER o una palabra clave
    // consultando la tabla precargada de palabras clave.
    Token escanearIdentificador();

    // --- Literales enteros (paso 6) ---
    // Se invoca cuando verCaracter() ya es un digito. Consume la corrida
    // maxima de digitos y valida: (1) que no tenga ceros a la izquierda
    // salvo que sea "0" solo (sec. 3.4.2), y (2) que el valor entre en
    // el rango de un entero de 32 bits con signo (max 2147483647).
    // La comparacion de rango se hace sobre el lexema como string, para
    // no desbordar ningun tipo entero de C++ si el usuario escribe un
    // numero con muchos digitos.
    Token escanearEntero();

    // --- Literales de cadena (paso 6, parte 2) ---
    // Se invoca cuando verCaracter() ya es '"'. Consume hasta la comilla de
    // cierre, decodificando los escapes \" \n \t \\ (sec. 3.4.1). El
    // lexema del token resultante es el CONTENIDO ya decodificado (sin
    // las comillas), tal como lo define el manual. Si el contenido tiene
    // sintaxis de identificador, el token es IDSTRING; si no, STRING.
    // Ante cadena sin cerrar (EOF o fin de linea antes de la comilla de
    // cierre) devuelve UNKNOWN_ERROR sin consumir el '\n', para no
    // interferir con el manejo de indentacion/NEWLINE.
    Token escanearCadena();

    // --- Operadores y delimitadores (paso 7) ---
    // Se invoca como ultimo recurso en siguienteToken(), cuando verCaracter()
    // no disparo ninguna de las categorias anteriores (identificador, digito,
    // comilla). Consume 1 o 2 caracteres segun corresponda (sec. 3.5):
    // la mayoria de los simbolos son de un solo caracter, pero '->', '==',
    // '!=', '<=', '>=' y '//' requieren mirar un caracter adelante
    // (verCaracter) antes de decidir el token. Si el caracter no pertenece a
    // ningun token valido (p. ej. '@', '$', ';'), o si un prefijo valido
    // no esta seguido de lo que necesita para completarse (un '/' suelto,
    // ya que ChocoPy solo tiene '//'; o un '!' no seguido de '='), reporta
    // el error y devuelve UNKNOWN_ERROR.
    Token escanearOperador();
};

#endif
