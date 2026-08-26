# 🐍 ChocoPy Compiler (C++)

Compilador modular para **ChocoPy** (un subconjunto estáticamente tipado de Python 3 diseñado para la enseñanza de compiladores) desarrollado en **C++17**.

---

## 🚀 Estado del Proyecto

- [x] **Scanner (Analizador Léxico):** Conversión de texto a tokens, manejo de pila para sangría (`INDENT`/`DEDENT`) e ignorado de comentarios. *(En desarrollo)*
- [ ] **Parser (Analizador Sintáctico):** Construcción del Árbol de Sintaxis Abstracta (AST).
- [ ] **Análisis Semántico:** Chequeo de tipos, tabla de símbolos y verificación de ámbitos.
- [ ] **Generación de Código:** Traducción a ensamblador objetivo (RISC-V 32-bit / LLVM IR).

---

## 📁 Estructura del Proyecto

```text
.
├── scanner/                     # Módulo del analizador léxico
│   ├── tokens.hpp / .cpp        # Definiciones de Token, TokenType y funciones auxiliares
│   ├── indentacion.hpp / .cpp   # Manejo del algoritmo de sangría basado en pila
│   └── lexer.hpp / .cpp         # Motor del Scanner (AFD y consumo de fuente)
├── Tests/                       # Pruebas unitarias y muestras de código ChocoPy
│   └── testTokens.hpp / .cpp    # Pruebas del tokenizador
├── main.cpp                     # Punto de entrada principal
└── README.md                    # Documentación del proyecto
```

---

## 🛠️ Requisitos e Instalación

### Requisitos Previos
* **Compilador con soporte C++17:** `g++` (>= 8.0), `clang++` (>= 7.0) o MSVC.
* **Sistema Operativo:** Linux, macOS o Windows (MinGW-w64 / MSYS2).

### Compilación y Ejecución

Para compilar el módulo del scanner junto con las pruebas unitarias usando `g++`:

```bash
# Compilar el proyecto
g++ -std=c++17 main.cpp Tests/testTokens.cpp scanner/*.cpp -I. -o test_tokens
#nuevo
g++ -std=c++17 main.cpp Tests/testTokens.cpp Tests/testLexer.cpp Scanner/*.cpp -I. -o test_tokens

# Ejecutar el ejecutable generado
./test_tokens
```
