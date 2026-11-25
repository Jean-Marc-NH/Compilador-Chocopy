# errores_semanticos.py
# Programa sintacticamente VALIDO con muchos errores SEMANTICOS. Cada uno esta
# marcado con "# ERROR n: causa". El analisis no se detiene en el primero:
# los reporta todos, ordenados por linea.
# Uso: ./build/chocopy Pruebas/programas/semantica/errores_semanticos.py --tablas

contador: int = 0
nombre: str = "ana"
lista: [int] = None
contador: int = 1                  # ERROR 1: 'contador' ya fue declarado en este scope
precio: int = "gratis"             # ERROR 2: valor inicial str para una variable int
objeto: Fantasma = None            # ERROR 3: el tipo 'Fantasma' no existe

class Base(object):
    valor: int = 0

    def calcular(self: "Base", k: int) -> int:
        return k

class Hija(Base):
    valor: int = 5                 # ERROR 4: atributo ya definido en la superclase

    def calcular(self: "Hija") -> int:   # ERROR 5: redefine calcular con otra firma
        return 0

class Huerfana(Inexistente):       # ERROR 6: la superclase no esta definida
    pass

def sin_global():
    contador = 5                   # ERROR 7: asigna a la global sin 'global'

def mal_global():
    global fantasma                # ERROR 8: 'fantasma' no es una variable global
    pass

def sin_return(n: int) -> int:     # ERROR 9: no todos los caminos terminan en return
    if n > 0:
        return n

def tipo_return() -> int:
    return "texto"                 # ERROR 10: devuelve str en una funcion que devuelve int

def dos_parametros(a: int, b: int) -> int:
    return a + b

# ---------- Sentencias ----------
contador = "diez"                  # ERROR 11: asigna str a una variable int
print(no_existe)                   # ERROR 12: 'no_existe' no esta declarado
if contador:                       # ERROR 13: la condicion del if debe ser bool
    pass
print(dos_parametros(1))           # ERROR 14: faltan argumentos en la llamada
print(dos_parametros(1, "dos"))    # ERROR 15: el argumento 2 debe ser int, no str
print(nombre + 1)                  # ERROR 16: '+' entre str e int
print(not contador)                # ERROR 17: 'not' necesita un bool
nombre[0] = "A"                    # ERROR 18: los str no se pueden modificar
print(lista["a"])                  # ERROR 19: el indice debe ser int
for contador in 5:                 # ERROR 20: for sobre un int
    pass
print(Base().inexistente)          # ERROR 21: Base no tiene ese atributo
print(dos_parametros)              # ERROR 22: una funcion usada sin llamarla
contador()                         # ERROR 23: contador es una variable, no una funcion
return 1                           # ERROR 24: return fuera de una funcion
