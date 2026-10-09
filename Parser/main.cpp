#include "lr1.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
std::size_t limit(const std::string& s, std::size_t maximum) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Los limites deben ser enteros positivos.");
    const auto n = std::stoull(s);
    if (!n || n > maximum) throw std::runtime_error("Limite fuera de rango: " + s);
    return static_cast<std::size_t>(n);
}
std::string read(std::istream& in) {
    std::string text; char c;
    while (in.get(c)) {
        text += c;
        if (text.size() > 65536) throw std::runtime_error("La gramatica supera 64 KiB.");
    }
    if (in.bad()) throw std::runtime_error("Error al leer la gramatica.");
    return text;
}
}
int main(int argc, char** argv) {
    try {
        std::string path = "-", input; bool json = false, parse = false;
        std::size_t states = 1000, steps = 1000;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "Generador LR(1) canonico, C++17\n"
                    "Uso: lr1 [--grammar archivo|-] [--input \"tokens\"] [--json]\n"
                    "         [--max-states 1..5000] [--max-steps 1..10000]\n"
                    "Sin --grammar se lee stdin hasta EOF. La primera regla define el inicio.\n"
                    "Simbolos separados por espacios; alternativas con |; vacio: ε, eps o ''.\n"
                    "Se agrega una produccion inicial automaticamente y EOF ($) a la entrada.\n"
                    "Salida: 0=LR(1)/aceptada, 1=error de uso, 2=conflictos, 3=entrada no aceptada.\n";
                return 0;
            }
            if (arg == "--json") { json = true; continue; }
            if (arg != "--grammar" && arg != "--input" && arg != "--max-states" && arg != "--max-steps")
                throw std::runtime_error("Opcion desconocida: " + arg);
            if (++i == argc) throw std::runtime_error("Falta valor para " + arg);
            if (arg == "--grammar") path = argv[i];
            if (arg == "--input") { input = argv[i]; parse = true; }
            if (arg == "--max-states") states = limit(argv[i], 5000);
            if (arg == "--max-steps") steps = limit(argv[i], 10000);
        }
        std::string grammar;
        if (path == "-") grammar = read(std::cin);
        else { std::ifstream file(path); if (!file) throw std::runtime_error("No se puede abrir: " + path); grammar = read(file); }
        lr1::Parser parser(grammar, states);
        lr1::ParseResult result;
        if (parse) result = parser.parse(input, steps);
        std::cout << (json ? parser.json(parse ? &result : nullptr) : parser.report(parse ? &result : nullptr)) << '\n';
        if (!parser.conflicts.empty()) return 2;
        return parse && !result.accepted ? 3 : 0;
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
