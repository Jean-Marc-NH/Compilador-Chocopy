CXX := g++
CXXFLAGS := -std=c++17 -Wall -I.

SCANNER_SRC := $(wildcard Scanner/*.cpp)

TEST_BIN := test_tokens
SCAN_BIN := scan_file

FILE ?= Programa_prueba.py
OUT  ?= resultado_programa_prueba.txt

.PHONY: all tests scan run test scan-run clean

all: tests scan

# Compila y corre los tests unitarios del scanner
tests: $(TEST_BIN)

$(TEST_BIN): main.cpp Tests/testTokens.cpp Tests/testLexer.cpp $(SCANNER_SRC)
	$(CXX) $(CXXFLAGS) main.cpp Tests/testTokens.cpp Tests/testLexer.cpp $(SCANNER_SRC) -o $(TEST_BIN)

test: tests
	./$(TEST_BIN)

# Compila el driver que tokeniza un archivo .py
scan: $(SCAN_BIN)

$(SCAN_BIN): scan_file.cpp Scanner/lexer.cpp Scanner/tokens.cpp
	$(CXX) $(CXXFLAGS) scan_file.cpp Scanner/lexer.cpp Scanner/tokens.cpp -o $(SCAN_BIN)

# Ejecuta el scanner sobre FILE (por defecto Programa_prueba.py) y guarda en OUT
scan-run: scan
	./$(SCAN_BIN) $(FILE) > $(OUT)
	@echo "Salida guardada en $(OUT)"

run: scan
	./$(SCAN_BIN) $(FILE)

clean:
	rm -f $(TEST_BIN) $(SCAN_BIN)
