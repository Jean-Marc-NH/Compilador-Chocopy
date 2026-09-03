#ifndef SCANNER_LEXER_HPP
#define SCANNER_LEXER_HPP

#include <string>
#include <deque>
#include <vector>
#include "tokens.hpp"

// Lexer / Scanner de ChocoPy.
//
// Responsable de:
//   - Leer el archivo fuente completo a memoria.
//   - Exponer getchar_()/peekchar() para el resto del scanner.
//   - Mantener linea:columna actualizadas caracter a caracter.
//   - Encolar tokens que no consumen caracteres (INDENT/DEDENT) para
//     que gettoken() los devuelva antes de seguir leyendo el archivo.
//
// Paso 1 del proyecto: solo el esqueleto (buffer + lectura de caracteres +
// cola). La logica de reconocimiento (identificadores, numeros, strings,
// operadores, indentacion) se agrega en los pasos siguientes.
class Lexer {
public:
    // Carga el codigo fuente desde un archivo en disco.
    explicit Lexer(const std::string& filename);

    // Punto de entrada que el parser LR(1) invocara para pedir el
    // siguiente token. Por ahora es un stub: drena la cola si hay algo
    // pendiente, y si no, devuelve END_OF_FILE.
    Token gettoken();

    // Cantidad de errores lexicos detectados durante el escaneo.
    int errorCount() const { return errors; }

private:
    // --- Estado del buffer de entrada ---
    std::string source;   // contenido completo del archivo (line endings normalizados a '\n')
    size_t pos = 0;        // indice del proximo caracter a leer en 'source'
    int line = 1;
    int column = 1;

    // --- Cola de tokens que no consumen caracteres del input ---
    std::deque<Token> pendingTokens;

    // --- Manejo de indentacion ---
    std::vector<int> indentStack{0}; // pila inicializada con el nivel 0
    bool atLineStart = true;         // true si el proximo caracter a leer inicia una linea logica

    // --- Contadores de diagnostico ---
    int errors = 0;

    // --- Lectura de caracteres ---
    // Devuelve el siguiente caracter y avanza el puntero. Devuelve '\0' si ya no hay mas input.
    char getchar_();

    // Devuelve el caracter que esta 'offset' posiciones adelante del puntero, sin moverlo.
    // peekchar() (offset = 0) mira el proximo caracter a leer.
    char peekchar(int offset = 0) const;

    bool isAtEnd() const;

    // --- Utilidades internas ---
    // Carga 'filename' completo en 'source', normalizando \r\n y \r a \n.
    void loadFile(const std::string& filename);

    // Reporta un error lexico con mensaje, linea y columna. Incrementa 'errors'.
    void reportError(const std::string& message, int atLine, int atColumn);

    // --- Indentacion (paso 3) ---
    // Se invoca cuando atLineStart == true. Recorre, internamente, todas
    // las lineas fisicas en blanco o de solo-comentario hasta encontrar
    // contenido real o EOF. Al encontrar contenido real, compara el nivel
    // de indentacion contra el tope de 'indentStack' y encola INDENT/DEDENT
    // segun corresponda, dejando atLineStart en false. Si llega a EOF sin
    // encontrar contenido, delega en flushIndentToZeroAndEOF().
    void handleLineStart();

    // Consume caracteres desde '#' (asumido ya en peekchar()) hasta el
    // siguiente '\n' o EOF, sin emitir ningun token.
    void skipComment();

    // Desapila 'indentStack' hasta dejar solo el nivel 0, encolando un
    // DEDENT por cada nivel que se cierra, y finalmente encola END_OF_FILE.
    void flushIndentToZeroAndEOF();

    // --- Identificadores y palabras clave (paso 5) ---
    // Se invoca cuando peekchar() ya cumplio la condicion de inicio de
    // identificador (letra o '_'). Consume el resto del lexema (letras,
    // digitos, '_') y decide si es IDENTIFIER o una palabra clave
    // consultando la tabla precargada de keywords.
    Token scanIdentifierOrKeyword();

    // --- Literales enteros (paso 6) ---
    // Se invoca cuando peekchar() ya es un digito. Consume la corrida
    // maxima de digitos y valida: (1) que no tenga ceros a la izquierda
    // salvo que sea "0" solo (sec. 3.4.2), y (2) que el valor entre en
    // el rango de un entero de 32 bits con signo (max 2147483647).
    // La comparacion de rango se hace sobre el lexema como string, para
    // no desbordar ningun tipo entero de C++ si el usuario escribe un
    // numero con muchos digitos.
    Token scanInteger();

    // --- Literales de cadena (paso 6, parte 2) ---
    // Se invoca cuando peekchar() ya es '"'. Consume hasta la comilla de
    // cierre, decodificando los escapes \" \n \t \\ (sec. 3.4.1). El
    // lexema del token resultante es el CONTENIDO ya decodificado (sin
    // las comillas), tal como lo define el manual. Si el contenido tiene
    // sintaxis de identificador, el token es IDSTRING; si no, STRING.
    // Ante cadena sin cerrar (EOF o fin de linea antes de la comilla de
    // cierre) devuelve UNKNOWN_ERROR sin consumir el '\n', para no
    // interferir con el manejo de indentacion/NEWLINE.
    Token scanString();

    // --- Operadores y delimitadores (paso 7) ---
    // Se invoca como ultimo recurso en gettoken(), cuando peekchar() no
    // disparo ninguna de las categorias anteriores (identificador, digito,
    // comilla). Consume 1 o 2 caracteres segun corresponda (sec. 3.5):
    // la mayoria de los simbolos son de un solo caracter, pero '->', '==',
    // '!=', '<=', '>=' y '//' requieren mirar un caracter adelante
    // (peekchar) antes de decidir el token. Si el caracter no pertenece a
    // ningun token valido (p. ej. '@', '$', ';'), o si un prefijo valido
    // no esta seguido de lo que necesita para completarse (un '/' suelto,
    // ya que ChocoPy solo tiene '//'; o un '!' no seguido de '='), reporta
    // el error y devuelve UNKNOWN_ERROR.
    Token scanOperatorOrDelimiter();
};

#endif