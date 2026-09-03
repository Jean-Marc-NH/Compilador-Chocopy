# programa_prueba.py
# Programa ChocoPy de ejemplo, pensado para ejercitar al scanner con la
# mayoria de las categorias de tokens: palabras clave, identificadores,
# literales (enteros, strings, IDSTRING, bool, None), operadores,
# delimitadores, comentarios, lineas en blanco e indentacion anidada.

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
        resultado = resultado + palabra + " "
    return resultado

# --- Bloque de nivel superior ---

mylist: [int] = None
mylist = [1, 0, -3, 42, 2147483647]

c: animal = None
c = cow()
c.make_noise()

print(is_zero(mylist, 1))
print(clasifica(-5))
print(clasifica(0))
print(suma_lista(mylist))
print(concatena(["Hola", "ChocoPy"]))

nombre: str = "He\"llo\"\tWorld\\n"
x: object = None
y: object = None
resultado_id: bool = x is y

a: int = 1
b: int = 2
resultado_op: int = (a + b) * 2 // 3 % 4 - 1
resultado_bool: bool = (a < b) and (not (a == b)) or (a != b)