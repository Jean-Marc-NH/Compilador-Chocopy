#pragma once
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace lr1 {
struct Production { std::string lhs; std::vector<std::string> rhs; };
struct Item {
    int production, dot;
    std::string lookahead;
    bool operator<(const Item& other) const {
        return std::tie(production, dot, lookahead) <
               std::tie(other.production, other.dot, other.lookahead);
    }
    bool operator==(const Item& other) const {
        return production == other.production && dot == other.dot && lookahead == other.lookahead;
    }
};
using Items = std::set<Item>;
struct State { Items kernel, closure; std::map<std::string, int> transitions; };
struct Action {
    char kind; // s: shift, r: reduce, a: accept
    int value;
    bool operator<(const Action& other) const {
        return std::tie(kind, value) < std::tie(other.kind, other.value);
    }
    std::string text() const;
};
struct Conflict { int state; std::string token, type; std::vector<Action> actions; };
struct TraceRow { std::vector<int> states; std::vector<std::string> symbols; std::size_t position; std::string action; };
struct ParseResult {
    bool accepted = false;
    std::string message;
    std::vector<std::string> tokens;
    std::vector<TraceRow> trace;
};
class Parser {
public:
    explicit Parser(const std::string& grammar, std::size_t max_states = 1000);
    ParseResult parse(const std::string& input, std::size_t max_steps = 1000) const;
    std::string json(const ParseResult* result = nullptr) const;
    std::string report(const ParseResult* result = nullptr) const;
    std::vector<Production> productions; // production 0 is generated augmentation
    std::vector<std::string> nonterminals, terminals;
    std::map<std::string, std::set<std::string>> first;
    std::set<std::string> nullable;
    std::vector<State> states;
    std::vector<std::map<std::string, std::set<Action>>> action;
    std::vector<std::map<std::string, int>> go_to;
    std::vector<Conflict> conflicts;
private:
    std::set<std::string> nonterminal_set;
    std::map<std::string, std::vector<int>> by_lhs;
    void read_grammar(const std::string& grammar);
    void compute_first();
    std::set<std::string> first_tail(const std::vector<std::string>& rhs,
                                     std::size_t begin, const std::string& lookahead) const;
    Items closure(const Items& kernel) const;
    void build_states(std::size_t max_states);
    void build_tables();
    std::string item_text(const Item& item) const;
};
} // namespace lr1
