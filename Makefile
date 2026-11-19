# Makefile del compilador de ChocoPy
#
#   make             compila todo en build/
#   make pruebas     compila y corre las pruebas
#   make limpiar     borra build/
#
# En Windows con MinGW se llama mingw32-make. Todos los ejecutables se
# generan en build/ (ignorado por git) y hay que correrlos desde esta carpeta.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
INCLUIR  := -I.

# En Windows enlazamos estatico para que los .exe no dependan de las DLL de
# MinGW (si no, fallan al correrlos fuera de la terminal de MinGW).
ifeq ($(OS),Windows_NT)
    LDFLAGS ?= -static
    EXE := .exe
else
    EXE :=
endif

# --- Fuentes de cada modulo ---
SCANNER   := Scanner/tokens.cpp Scanner/lexer.cpp Comun/diagnostico.cpp
GENERADOR := GeneradorLR1/gramatica.cpp GeneradorLR1/conjuntos.cpp GeneradorLR1/tabla.cpp GeneradorLR1/cache.cpp
PARSER    := Parser/parser.cpp Parser/mensajes.cpp
AST       := AST/nodos.cpp AST/constructor.cpp AST/impresion.cpp
PRUEBAS   := Pruebas/correr_pruebas.cpp Pruebas/prueba_tokens.cpp Pruebas/prueba_lexer.cpp \
             Pruebas/prueba_ast.cpp Pruebas/prueba_errores.cpp

ESCANEAR := build/escanear$(EXE)
ANALIZAR := build/analizar$(EXE)
LR1      := build/lr1$(EXE)
PROBAR   := build/pruebas$(EXE)

.PHONY: todo pruebas limpiar

todo: $(ESCANEAR) $(ANALIZAR) $(LR1) $(PROBAR)

# 'mkdir build' funciona igual en cmd y en sh; solo se llama si no existe.
build:
	mkdir build

$(ESCANEAR): Herramientas/escanear.cpp $(SCANNER) | build
	$(CXX) $(CXXFLAGS) $(INCLUIR) $^ -o $@ $(LDFLAGS)

$(ANALIZAR): Herramientas/analizar.cpp $(SCANNER) $(GENERADOR) $(PARSER) $(AST) | build
	$(CXX) $(CXXFLAGS) $(INCLUIR) $^ -o $@ $(LDFLAGS)

$(LR1): Herramientas/lr1.cpp $(GENERADOR) | build
	$(CXX) $(CXXFLAGS) $(INCLUIR) $^ -o $@ $(LDFLAGS)

$(PROBAR): $(PRUEBAS) $(SCANNER) $(GENERADOR) $(PARSER) $(AST) | build
	$(CXX) $(CXXFLAGS) $(INCLUIR) $^ -o $@ $(LDFLAGS)

pruebas: $(PROBAR)
	$(PROBAR)

limpiar:
ifeq ($(OS),Windows_NT)
	if exist build rmdir /s /q build
else
	rm -rf build
endif
