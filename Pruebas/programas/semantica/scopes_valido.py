# scopes_valido.py
# Programa ChocoPy VALIDO: el analisis semantico debe dar 0 errores.
# Muestra scopes (global, funcion, anidada, clase), shadowing, global,
# nonlocal, herencia con redefinicion de metodos, listas y control de flujo.
# Uso: ./build/chocopy Pruebas/programas/semantica/scopes_valido.py --tablas

# ---------- Variables globales ----------
# En ChocoPy todas las definiciones van antes de la primera sentencia.
total: int = 0
x: int = 10
numeros: [int] = None
letra: str = ""
elemento: int = 0
animal: Animal = None

# ---------- Clases ----------
# Atributos con tipo; el primer parametro de cada metodo es self de la clase.
class Animal(object):
    patas: int = 4

    def sonido(self: "Animal") -> str:
        return "..."

    def describir(self: "Animal") -> int:
        # Los atributos se usan con self.atributo y los metodos con self.metodo()
        print(self.sonido())
        return self.patas

# Herencia: Perro redefine sonido con la MISMA firma (sin contar self).
class Perro(Animal):
    def sonido(self: "Perro") -> str:
        return "guau"

# ---------- Funciones ----------
# Shadowing: el parametro x oculta a la x global dentro de esta funcion.
def duplicar(x: int) -> int:
    return x * 2

# Shadowing: la variable local total oculta a la total global.
def suma_hasta(n: int) -> int:
    total: int = 0
    i: int = 0
    while i <= n:
        total = total + i
        i = i + 1
    return total

# global: permite ASIGNAR a la variable global total desde una funcion.
def acumular(k: int):
    global total
    total = total + k

# Funcion anidada con nonlocal: asigna a la variable de la funcion que la encierra.
def contar_pares(limite: int) -> int:
    pares: int = 0
    k: int = 0

    def anotar():
        nonlocal pares
        pares = pares + 1

    while k < limite:
        if k % 2 == 0:
            anotar()
        k = k + 1
    return pares

# if / elif / else: todos los caminos terminan en return.
def signo(n: int) -> int:
    if n > 0:
        return 1
    elif n < 0:
        return 0 - 1
    else:
        return 0

# ---------- Sentencias del programa ----------
# Una clase se puede asignar a una variable de ese tipo, y None tambien.
animal = Perro()
print(animal.describir())
animal = None

print(duplicar(x))
print(suma_hasta(4))
acumular(5)
acumular(7)
print(total)
print(contar_pares(10))
print(signo(0 - 3))

# Listas: literal, concatenacion, indice, asignacion a un elemento y len.
numeros = [1, 2, 3] + [4, 5]
numeros[0] = len(numeros)

# for sobre una lista (la variable del for ya esta declarada arriba).
for elemento in numeros:
    print(elemento)

# for sobre un str: cada elemento es un str de un caracter.
for letra in "hola":
    print(letra)
