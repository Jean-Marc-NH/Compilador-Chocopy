# compilar.ps1
#
# Compila el proyecto con g++ cuando no hay make instalado (PowerShell en Windows).
# Genera los mismos ejecutables que el Makefile, dentro de build\:
#
#   build\chocopy.exe     scanner + parser + AST + analisis semantico
#   build\escanear.exe    imprime los tokens de un .py
#   build\lr1.exe         imprime producciones, estados y tabla de una gramatica
#   build\pruebas.exe     pruebas unitarias
#
# Uso (desde cualquier carpeta):
#
#   powershell -ExecutionPolicy Bypass -File compilar.ps1
#   powershell -ExecutionPolicy Bypass -File compilar.ps1 -Pruebas    # ademas corre las pruebas
#
# Se detiene en el primer ejecutable que no compila. Los .exe se enlazan
# estaticos (-static) para que corran fuera de la terminal de MinGW.

param(
    [switch]$Pruebas
)

# Los programas buscan Gramaticas/chocopy.txt y escriben en build/ usando rutas
# relativas, asi que siempre se trabaja desde la carpeta del proyecto.
Set-Location -Path $PSScriptRoot

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Host "No se encontro g++. Instala MinGW-w64 o MSYS2 y agrega su carpeta bin al PATH." -ForegroundColor Red
    exit 1
}

# Mismas banderas que el Makefile (CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra, mas -static).
$banderas = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-I.')

# --- Fuentes de cada modulo (las mismas listas que el Makefile) ---
$scanner   = @('Scanner/tokens.cpp', 'Scanner/lexer.cpp', 'Comun/diagnostico.cpp')
$generador = @('GeneradorLR1/gramatica.cpp', 'GeneradorLR1/conjuntos.cpp', 'GeneradorLR1/tabla.cpp', 'GeneradorLR1/cache.cpp')
$parser    = @('Parser/parser.cpp', 'Parser/mensajes.cpp')
$ast       = @('AST/nodos.cpp', 'AST/constructor.cpp', 'AST/impresion.cpp')
$semantico = @('Semantico/tipos.cpp', 'Semantico/tabla_simbolos.cpp', 'Semantico/analizador.cpp')
# Pruebas/*.cpp incluye todas las pruebas (correr_pruebas.cpp y prueba_*.cpp).
$pruebasFuentes = @(Get-ChildItem -Path 'Pruebas' -Filter '*.cpp' | ForEach-Object { 'Pruebas/' + $_.Name })

# Carpeta de salida (equivale a la regla 'build' del Makefile).
if (-not (Test-Path 'build')) {
    New-Item -ItemType Directory -Path 'build' | Out-Null
}

# Compila un ejecutable: Compilar-Ejecutable <nombre> <fuentes...>
function Compilar-Ejecutable {
    param(
        [string]$Nombre,
        [string[]]$Fuentes
    )
    $salida = "build/$Nombre.exe"
    Write-Host "Compilando $salida ..."
    & g++ @banderas @Fuentes -o $salida -static
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error al compilar $salida (codigo $LASTEXITCODE)." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Compilar-Ejecutable 'escanear' (@('Herramientas/escanear.cpp') + $scanner)
Compilar-Ejecutable 'chocopy'  (@('Herramientas/chocopy.cpp') + $scanner + $generador + $parser + $ast + $semantico)
Compilar-Ejecutable 'lr1'      (@('Herramientas/lr1.cpp') + $generador)
Compilar-Ejecutable 'pruebas'  ($pruebasFuentes + $scanner + $generador + $parser + $ast + $semantico)

Write-Host "Listo: build\chocopy.exe, build\escanear.exe, build\lr1.exe y build\pruebas.exe" -ForegroundColor Green

if ($Pruebas) {
    Write-Host "Corriendo las pruebas ..."
    & ./build/pruebas.exe
    exit $LASTEXITCODE
}
