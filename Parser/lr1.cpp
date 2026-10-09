#include "lr1.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace lr1 {
namespace {
    
struct Lexeme { 
    std::string text; 
    bool quoted = false;
};

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
// "A -> ( A ) | a" -> ["A", "->", "(", "A", ")", "|", "a"]
std::vector<Lexeme> lex(const std::string& text, bool grammar) {
    std::vector<Lexeme> out;
    std::size_t i = 0;
    
    while (i < text.size()) {
        if (std::isspace(static_cast<unsigned char>(text[i]))) { 
            ++i; continue;
        }
        if (text[i] == '\'' || text[i] == '"') {
            const char quote = text[i++];
            std::string value;
            bool closed = false;

            while (i < text.size()) {
                char c = text[i++];
                if (c == quote) {
                    closed = true;
                    break;
                }
                if (c == '\\') {
                    if (i == text.size()) throw std::runtime_error("Escape incompleto en simbolo entre comillas.");
                    c = text[i++];
                }
                value += c;
            }
            if (!closed) throw std::runtime_error("Comillas sin cerrar.");
            out.push_back({
                value,
                true
            });

        } else if (grammar && (text.compare(i, 2, "->") == 0 || text.compare(i, 3, "→") == 0)) {
            out.push_back({"->", false}); i += text[i] == '-' ? 2 : 3;
        } else if (grammar && text[i] == '|') {
            out.push_back({"|", false}); ++i;
        } else {
            const auto start = i;
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])) &&
                   !(grammar && (text[i] == '|' || text.compare(i, 2, "->") == 0 || text.compare(i, 3, "→") == 0))) ++i;
            out.push_back({text.substr(start, i-start), false});
        }
    }
    return out;
}

bool epsilon(const Lexeme& t) {
    return t.text.empty() ||
           (!t.quoted &&
            (t.text == "ε" ||
             t.text == "epsilon" ||
             t.text == "eps"));
}

std::string symbol(const std::string& s) {
    if (s.find_first_of(" \t\r\n|\"\\") == std::string::npos && s != "->" && s != "→" &&
        s != "ε" && s != "eps" && s != "epsilon" && (s.empty() || s.front() != '\'')) return s;
    std::string out = "\"";
    for (char c : s) { if (c == '"' || c == '\\') out += '\\'; out += c; }
    return out + '"';
}
std::string production_text(const Production& p) {
    std::string out = p.lhs + " ->";
    if (p.rhs.empty()) return out + " ε";
    for (const auto& s : p.rhs) out += " " + symbol(s);
    return out;
}
std::string quote(const std::string& s) {
    std::ostringstream out; out << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
        else out << c;
    }
    out << '"'; return out.str();
}
template<class Collection, class Render>
std::string array(const Collection& values, Render render) {
    std::string s = "["; bool comma = false;
    for (const auto& v : values) { if (comma) s += ','; comma = true; s += render(v); }
    return s + ']';
}
}

std::string Action::text() const { return kind == 'a' ? "acc" : std::string(1, kind) + std::to_string(value); }
Parser::Parser(const std::string& grammar, std::size_t max_states) {
    if (!max_states || max_states > 5000) throw std::runtime_error("max-states debe estar entre 1 y 5000.");
    read_grammar(grammar); compute_first(); build_states(max_states); build_tables();
}
void Parser::read_grammar(const std::string& grammar) {
    if (grammar.size() > 65536) throw std::runtime_error("La gramatica supera 64 KiB.");
    std::istringstream stream(grammar); std::string line, previous; int line_no = 0;
    std::set<std::string> quoted_symbols, all_symbols;
    std::set<std::pair<std::string, std::vector<std::string>>> seen;
    while (std::getline(stream, line)) {
        ++line_no; line = trim(line);
        if (line.empty() || line.front() == '#') continue;
        try {
            // Accept decorative labels such as 0. S -> G or (0) S -> G.
            std::size_t i = 0;
            if (line[0] == '(') ++i;
            const auto begin = i;
            while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) ++i;
            if (i > begin && i < line.size() && ((begin == 1 && line[i] == ')') ||
                (begin == 0 && (line[i] == '.' || line[i] == ':')))) line = trim(line.substr(i+1));
            auto tokens = lex(line, true);
            if (tokens.empty()) continue;
            std::string lhs; std::size_t rhs_begin;
            if (!tokens[0].quoted && (tokens[0].text == "|" || tokens[0].text == "->")) {
                if (previous.empty()) throw std::runtime_error("Alternativa sin una regla anterior.");
                lhs = previous; rhs_begin = 1;
            } else {
                if (tokens.size() < 2 || tokens[0].quoted || tokens[1].quoted || tokens[1].text != "->")
                    throw std::runtime_error("Usa NoTerminal -> simbolos separados por espacios.");
                lhs = tokens[0].text; rhs_begin = 2;
                if (lhs == "$" || epsilon(tokens[0])) throw std::runtime_error("Nombre de no terminal reservado.");
                previous = lhs;
            }
            if (nonterminal_set.insert(lhs).second) nonterminals.push_back(lhs);
            std::vector<Lexeme> part;
            auto add = [&]() {
                std::vector<std::string> rhs;
                for (const auto& token : part) {
                    if (epsilon(token)) {
                        if (part.size() != 1) throw std::runtime_error("Epsilon debe ocupar toda la alternativa.");
                        continue;
                    }
                    if (token.text == "$" || (!token.quoted && token.text == "->"))
                        throw std::runtime_error("$ esta reservado para EOF; una flecha literal requiere comillas.");
                    if (token.text.size() > 128) throw std::runtime_error("Simbolo de mas de 128 bytes.");
                    if (token.quoted) quoted_symbols.insert(token.text);
                    all_symbols.insert(token.text); rhs.push_back(token.text);
                }
                if (rhs.size() > 128) throw std::runtime_error("Una produccion supera 128 simbolos.");
                if (seen.insert({lhs, rhs}).second) productions.push_back({lhs, rhs});
                if (productions.size() > 512) throw std::runtime_error("Limite de 512 producciones excedido.");
                part.clear();
            };
            for (std::size_t k = rhs_begin; k < tokens.size(); ++k) {
                if (!tokens[k].quoted && tokens[k].text == "|") add(); else part.push_back(tokens[k]);
            }
            add();
        } catch (const std::exception& e) { throw std::runtime_error("Linea " + std::to_string(line_no) + ": " + e.what()); }
    }
    if (productions.empty()) throw std::runtime_error("Escribe al menos una produccion.");
    for (const auto& s : quoted_symbols)
        if (nonterminal_set.count(s)) throw std::runtime_error("El simbolo " + s + " no puede ser terminal y no terminal a la vez.");
    for (const auto& s : all_symbols) if (!nonterminal_set.count(s)) terminals.push_back(s);
    if (all_symbols.size() + nonterminals.size() > 1024) throw std::runtime_error("Demasiados simbolos.");
    const auto start = productions.front().lhs;
    std::string augmented = start + "'";
    while (all_symbols.count(augmented) || nonterminal_set.count(augmented)) augmented += "'";
    productions.insert(productions.begin(), {augmented, {start}});
    nonterminal_set.insert(augmented); nonterminals.insert(nonterminals.begin(), augmented);
    for (std::size_t p = 0; p < productions.size(); ++p) by_lhs[productions[p].lhs].push_back(static_cast<int>(p));
}
void Parser::compute_first() {
    for (const auto& n : nonterminals) first[n];
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& p : productions) {
            bool all_nullable = true;
            for (const auto& x : p.rhs) {
                if (!nonterminal_set.count(x)) {
                    changed |= first[p.lhs].insert(x).second; all_nullable = false; break;
                }
                for (const auto& t : first[x]) changed |= first[p.lhs].insert(t).second;
                if (!nullable.count(x)) { all_nullable = false; break; }
            }
            if (all_nullable) changed |= nullable.insert(p.lhs).second;
        }
    }
}
std::set<std::string> Parser::first_tail(const std::vector<std::string>& rhs, std::size_t begin,
                                         const std::string& lookahead) const {
    std::set<std::string> result;
    for (std::size_t i = begin; i < rhs.size(); ++i) {
        const auto& x = rhs[i];
        if (!nonterminal_set.count(x)) { result.insert(x); return result; }
        const auto& f = first.at(x); result.insert(f.begin(), f.end());
        if (!nullable.count(x)) return result;
    }
    result.insert(lookahead); return result;
}
Items Parser::closure(const Items& kernel) const {
    Items result = kernel; std::vector<Item> work(kernel.begin(), kernel.end());
    for (std::size_t i = 0; i < work.size(); ++i) {
        const auto item = work[i]; const auto& p = productions[item.production];
        if (item.dot == static_cast<int>(p.rhs.size())) continue;
        const auto found = by_lhs.find(p.rhs[item.dot]);
        if (found == by_lhs.end()) continue;
        const auto lookaheads = first_tail(p.rhs, item.dot+1, item.lookahead);
        for (int q : found->second) for (const auto& t : lookaheads) {
            Item next{q, 0, t};
            if (result.insert(next).second) work.push_back(next);
            if (result.size() > 50000) throw std::runtime_error("Cierre demasiado grande (limite: 50000 items).");
        }
    }
    return result;
}
void Parser::build_states(std::size_t max_states) {
    Items initial{{0, 0, "$"}};
    states.push_back({initial, closure(initial), {}});
    std::map<Items, int> ids{{states[0].closure, 0}};
    std::size_t item_count = states[0].closure.size();
    for (std::size_t i = 0; i < states.size(); ++i) {
        std::map<std::string, Items> kernels;
        for (const auto& item : states[i].closure) {
            const auto& p = productions[item.production];
            if (item.dot < static_cast<int>(p.rhs.size())) kernels[p.rhs[item.dot]].insert({item.production, item.dot+1, item.lookahead});
        }
        for (const auto& entry : kernels) {
            const auto complete = closure(entry.second); const auto existing = ids.find(complete);
            int target;
            if (existing != ids.end()) target = existing->second;
            else {
                if (states.size() >= max_states) throw std::runtime_error("Limite de estados excedido. Aumenta --max-states o simplifica la gramatica.");
                item_count += complete.size();
                if (item_count > 250000) throw std::runtime_error("Limite global de 250000 items excedido.");
                target = static_cast<int>(states.size()); ids.emplace(complete, target);
                states.push_back({entry.second, complete, {}});
            }
            states[i].transitions[entry.first] = target;
        }
    }
}
void Parser::build_tables() {
    action.resize(states.size()); go_to.resize(states.size());
    for (std::size_t i = 0; i < states.size(); ++i) {
        for (const auto& edge : states[i].transitions) {
            if (nonterminal_set.count(edge.first)) go_to[i][edge.first] = edge.second;
            else action[i][edge.first].insert({'s', edge.second});
        }
        for (const auto& item : states[i].closure) {
            if (item.dot != static_cast<int>(productions[item.production].rhs.size())) continue;
            action[i][item.lookahead].insert({item.production == 0 ? 'a' : 'r', item.production});
        }
        for (const auto& cell : action[i]) if (cell.second.size() > 1) {
            bool shift = false; int reductions = 0;
            for (const auto& a : cell.second) { shift |= a.kind == 's'; reductions += a.kind == 'r'; }
            std::string type = shift && reductions ? "shift/reduce" : reductions > 1 ? "reduce/reduce" : "accept/conflict";
            conflicts.push_back({static_cast<int>(i), cell.first, type, {cell.second.begin(), cell.second.end()}});
        }
    }
}
ParseResult Parser::parse(const std::string& input, std::size_t max_steps) const {
    if (!max_steps || max_steps > 10000) throw std::runtime_error("max-steps debe estar entre 1 y 10000.");
    if (input.size() > 65536) throw std::runtime_error("Entrada de mas de 64 KiB.");
    ParseResult result;
    for (const auto& t : lex(input, false)) {
        if (t.text.empty()) throw std::runtime_error("Para la entrada vacia deja el campo vacio.");
        result.tokens.push_back(t.text);
    }
    if (!result.tokens.empty() && result.tokens.back() == "$") result.tokens.pop_back();
    if (std::find(result.tokens.begin(), result.tokens.end(), "$") != result.tokens.end())
        throw std::runtime_error("$ solo puede aparecer al final de la entrada.");
    if (result.tokens.size() > 4096) throw std::runtime_error("Limite de 4096 tokens excedido.");
    result.tokens.push_back("$");
    if (!conflicts.empty()) { result.message = "Gramatica no LR(1): resuelve los conflictos antes de analizar."; return result; }
    std::vector<int> stack{0}; std::vector<std::string> symbols;
    std::size_t pos = 0;
    for (std::size_t step = 0; step < max_steps; ++step) {
        const int state = stack.back(); const auto& token = result.tokens[pos];
        const auto found = action[state].find(token);
        if (found == action[state].end()) {
            std::string expected;
            for (const auto& cell : action[state]) { if (!expected.empty()) expected += ", "; expected += symbol(cell.first); }
            result.message = "Error en token " + std::to_string(pos+1) + " (" + symbol(token) + "), estado I" + std::to_string(state) + ". Esperado: " + (expected.empty() ? "ninguno" : expected);
            result.trace.push_back({stack, symbols, pos, result.message}); return result;
        }
        const auto a = *found->second.begin();
        result.trace.push_back({stack, symbols, pos, a.text() + (a.kind == 'r' ? ": " + production_text(productions[a.value]) : "")});
        if (a.kind == 's') {
            stack.push_back(a.value); symbols.push_back(token); ++pos;
        } else if (a.kind == 'r') {
            const auto& p = productions[a.value]; const auto count = p.rhs.size();
            symbols.resize(symbols.size()-count); stack.resize(stack.size()-count);
            const int next = go_to[stack.back()].at(p.lhs);
            symbols.push_back(p.lhs); stack.push_back(next);
        } else {
            result.accepted = true; result.message = "Entrada aceptada."; return result;
        }
    }
    result.message = "Limite de pasos alcanzado; no se ha determinado la aceptacion.";
    return result;
}
std::string Parser::item_text(const Item& item) const {
    const auto& p = productions[item.production]; std::string s = "[" + p.lhs + " ->";
    for (std::size_t i = 0; i <= p.rhs.size(); ++i) {
        if (static_cast<int>(i) == item.dot) s += " •";
        if (i < p.rhs.size()) s += " " + symbol(p.rhs[i]);
    }
    return s + ", " + symbol(item.lookahead) + "]";
}
std::string Parser::json(const ParseResult* result) const {
    const auto qs = [](const std::string& s) { return quote(s); };
    const auto item = [&](const Item& i) { return "{\"production\":" + std::to_string(i.production) + ",\"dot\":" + std::to_string(i.dot) + ",\"lookahead\":" + quote(i.lookahead) + ",\"text\":" + quote(item_text(i)) + "}"; };
    std::string out = "{\"lr1\":" + std::string(conflicts.empty() ? "true" : "false");
    out += ",\"productions\":" + array(productions, [](const Production& p) { return "{\"lhs\":" + quote(p.lhs) + ",\"rhs\":" + array(p.rhs, quote) + ",\"text\":" + quote(production_text(p)) + "}"; });
    out += ",\"terminals\":" + array(terminals, qs) + ",\"nonterminals\":" + array(nonterminals, qs);
    out += ",\"first\":{";
    bool comma = false;
    for (const auto& n : nonterminals) { if (comma) out += ','; comma = true; auto f = first.at(n); if (nullable.count(n)) f.insert("ε"); out += quote(n) + ':' + array(f, qs); }
    out += "},\"nullable\":" + array(nullable, qs) + ",\"states\":[";
    for (std::size_t i = 0; i < states.size(); ++i) {
        if (i) out += ',';
        out += "{\"id\":" + std::to_string(i) + ",\"kernel\":" + array(states[i].kernel, item) + ",\"closure\":" + array(states[i].closure, item) + ",\"transitions\":{";
        comma = false;
        for (const auto& edge : states[i].transitions) { if (comma) out += ','; comma = true; out += quote(edge.first) + ':' + std::to_string(edge.second); }
        out += "}}";
    }
    out += "],\"action\":" + array(action, [](const auto& row) {
        std::string r = "{"; bool c = false;
        for (const auto& cell : row) { if (c) r += ','; c = true; r += quote(cell.first) + ':' + array(cell.second, [](const Action& a) { return quote(a.text()); }); }
        return r + '}';
    });
    out += ",\"goto\":" + array(go_to, [](const auto& row) {
        std::string r = "{"; bool c = false;
        for (const auto& cell : row) { if (c) r += ','; c = true; r += quote(cell.first) + ':' + std::to_string(cell.second); }
        return r + '}';
    });
    out += ",\"conflicts\":" + array(conflicts, [](const Conflict& c) {
        return "{\"state\":" + std::to_string(c.state) + ",\"token\":" + quote(c.token) + ",\"type\":" + quote(c.type) + ",\"actions\":" + array(c.actions, [](const Action& a) { return quote(a.text()); }) + "}";
    });
    if (result) {
        out += ",\"parse\":{\"accepted\":" + std::string(result->accepted ? "true" : "false") + ",\"message\":" + quote(result->message) + ",\"tokens\":" + array(result->tokens, qs);
        out += ",\"trace\":" + array(result->trace, [](const TraceRow& row) {
            return "{\"states\":" + array(row.states, [](int n) { return std::to_string(n); }) + ",\"symbols\":" + array(row.symbols, quote) + ",\"position\":" + std::to_string(row.position) + ",\"action\":" + quote(row.action) + "}";
        });
        out += '}';
    }
    return out + '}';
}
std::string Parser::report(const ParseResult* result) const {
    std::ostringstream out;
    out << "GRAMATICA (r0: aumento automatico)\n";
    for (std::size_t i = 0; i < productions.size(); ++i) out << i << ". " << production_text(productions[i]) << '\n';
    out << "\nFIRST\n";
    for (const auto& n : nonterminals) { out << n << ":"; for (const auto& t : first.at(n)) out << ' ' << symbol(t); if (nullable.count(n)) out << " ε"; out << '\n'; }
    out << "\nESTADOS: KERNEL / CLOSURE / TRANSICIONES\n";
    for (std::size_t i = 0; i < states.size(); ++i) {
        out << "\nI" << i << "\n  Kernel:\n";
        for (const auto& it : states[i].kernel) out << "    " << item_text(it) << '\n';
        out << "  Closure:\n";
        for (const auto& it : states[i].closure) out << "    " << item_text(it) << '\n';
        for (const auto& edge : states[i].transitions) out << "  goto(I" << i << ", " << symbol(edge.first) << ") = I" << edge.second << '\n';
    }
    out << "\nLR TABLE (ACTION | GOTO), - = error\nEstado";
    auto columns = terminals; columns.push_back("$");
    for (const auto& t : columns) out << '\t' << symbol(t);
    for (std::size_t n = 1; n < nonterminals.size(); ++n) out << "\tGOTO:" << nonterminals[n];
    out << '\n';
    for (std::size_t i = 0; i < states.size(); ++i) {
        out << 'I' << i;
        for (const auto& t : columns) {
            out << '\t'; const auto f = action[i].find(t);
            if (f == action[i].end()) out << '-';
            else { bool sep = false; for (const auto& a : f->second) { if (sep) out << '/'; sep = true; out << a.text(); } }
        }
        for (std::size_t n = 1; n < nonterminals.size(); ++n) {
            const auto f = go_to[i].find(nonterminals[n]); out << '\t';
            if (f == go_to[i].end()) out << '-'; else out << f->second;
        }
        out << '\n';
    }
    out << (conflicts.empty() ? "\nGramatica LR(1), sin conflictos.\n" : "\nCONFLICTOS: gramatica no LR(1).\n");
    for (const auto& c : conflicts) { out << "I" << c.state << ", " << symbol(c.token) << ": " << c.type; for (const auto& a : c.actions) out << ' ' << a.text(); out << '\n'; }
    if (result) {
        out << "\nSEGUIMIENTO\nPaso\tPila\tEntrada\tAccion\n"; int step = 0;
        for (const auto& row : result->trace) {
            out << ++step << '\t' << row.states[0];
            for (std::size_t i = 0; i < row.symbols.size(); ++i) out << ' ' << symbol(row.symbols[i]) << ' ' << row.states[i+1];
            out << '\t'; for (std::size_t i = row.position; i < result->tokens.size(); ++i) out << symbol(result->tokens[i]) << ' ';
            out << '\t' << row.action << '\n';
        }
        out << result->message << '\n';
    }
    return out.str();
}
} // namespace lr1
