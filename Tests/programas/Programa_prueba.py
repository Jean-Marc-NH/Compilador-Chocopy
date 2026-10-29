# programa_prueba.py
# Programa ChocoPy de ejemplo, pensado para ejercitar al scanner con la
# mayoria de las categorias de tokens: palabras clave, identificadores,
# literales (enteros, strings, IDSTRING, bool, None), operadores,
# delimitadores, comentarios, lineas en blanco e indentacion anidada.
#
# Este archivo trae 4 ERRORES INTENCIONALES (marcados con "# ERROR n") para mostrar que
# el compilador los detecta todos, explica cada uno con su linea de codigo y sigue
# analizando el resto del programa sin detenerse:
#   ERROR 1: caracter invalido (@)            -> error lexico, lo informa el scanner
#   ERROR 2: elif sin ":" al final de la linea -> error dentro de una funcion
#   ERROR 3: parentesis sin cerrar            -> el mensaje senala el "(" abierto
#   ERROR 4: parentesis de mas                -> ")" sin su "("
# Para ver el programa sin errores, borra las 4 lineas marcadas (en el ERROR 1 cambia
# el "@" por "+"). Ejecucion:  ./build/parse_file Tests/programas/Programa_prueba.py

class animal(object):
    makes_noise: bool = False

    def make_noise(self: "animal") -> object:
        if self.makes_noise:
            print(self.sound())
        else:
            pass

    def sound(self: "animal") -> str:
        return "???"

class cow(animal):
    def __init__(self: "cow"):
        self.makes_noise = True

    def sound(self: "cow") -> str:
        return "moo"

def is_zero(items: [int], idx: int) -> bool:
    val: int = 0        # comentario dentro de una funcion
    val = items[idx]
    return val == 0

def clasifica(n: int) -> str:
    if n < 0:
        return "negativo"
    elif n == 0:
        return "cero"
    elif n == 5          # ERROR 2: falta ":" al final del elif
        return "cinco"
    elif n <= 9:
        return "digito"
    else:
        return "grande"

def suma_lista(nums: [int]) -> int:
    total: int = 0
    i: int = 0
    while i < len(nums):
        total = total + nums[i]
        i = i + 1
    return total

def concatena(palabras: [str]) -> str:
    resultado: str = ""
    for palabra in palabras:
        resultado = resultado + palabra @ " "  # ERROR 1 (lexico): el caracter '@' no existe en ChocoPy
    return resultado

# --- Bloque de nivel superior ---
# ChocoPy exige todas las definiciones (variables, funciones, clases) antes
# de la primera sentencia.

mylist: [int] = None
c: animal = None
nombre: str = "He\"llo\"\tWorld\n"
x: object = None
y: object = None
resultado_id: bool = False
a: int = 1
b: int = 2
resultado_op: int = 0
resultado_bool: bool = False

mylist = [1, 0, -3, 42, 2147483647]
c = cow()
c.make_noise()

print(is_zero(mylist, 1))
print(clasifica(-5))
print(clasifica(0))
print(suma_lista(mylist))
print(concatena(["Hola", "ChocoPy"]))

resultado_id = x is y
resultado_op = (a + b) * 2 // 3 % 4 - 1
resultado_bool = (a < b) and (not (a == b)) or (a != b)

# --- Errores de ejemplo en sentencias (a proposito) ---
resultado_op = (a + b * 2    # ERROR 3: falta cerrar el parentesis
print(a))                    # ERROR 4: sobra un parentesis
