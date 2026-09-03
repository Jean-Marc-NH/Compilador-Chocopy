#include "tokens.hpp"
#include <sstream>

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KW_CLASS:     return "KW_CLASS";
        case TokenType::KW_DEF:       return "KW_DEF";
        case TokenType::KW_IF:        return "KW_IF";
        case TokenType::KW_ELIF:      return "KW_ELIF";
        case TokenType::KW_ELSE:      return "KW_ELSE";
        case TokenType::KW_WHILE:     return "KW_WHILE";
        case TokenType::KW_RETURN:    return "KW_RETURN";
        case TokenType::KW_FOR:       return "KW_FOR";
        case TokenType::KW_IN:        return "KW_IN";
        case TokenType::KW_IS:        return "KW_IS";
        case TokenType::KW_AND:       return "KW_AND";
        case TokenType::KW_OR:        return "KW_OR";
        case TokenType::KW_NOT:       return "KW_NOT";
        case TokenType::KW_NONE:      return "KW_NONE";
        case TokenType::KW_TRUE:      return "KW_TRUE";
        case TokenType::KW_FALSE:     return "KW_FALSE";
        case TokenType::KW_PASS:      return "KW_PASS";
        case TokenType::KW_RESERVED:  return "KW_RESERVED";
        case TokenType::IDENTIFIER:   return "IDENTIFIER";
        case TokenType::INTEGER:      return "INTEGER";
        case TokenType::STRING:       return "STRING";
        case TokenType::IDSTRING:     return "IDSTRING";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::STAR:         return "STAR";
        case TokenType::SLASH_SLASH:  return "SLASH_SLASH";
        case TokenType::PERCENT:      return "PERCENT";
        case TokenType::EQ_EQ:        return "EQ_EQ";
        case TokenType::BANG_EQ:      return "BANG_EQ";
        case TokenType::LESS_EQ:      return "LESS_EQ";
        case TokenType::GREATER_EQ:   return "GREATER_EQ";
        case TokenType::LESS:         return "LESS";
        case TokenType::GREATER:      return "GREATER";
        case TokenType::EQUAL:        return "EQUAL";
        case TokenType::COLON:        return "COLON";
        case TokenType::DOT:          return "DOT";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::LBRACKET:     return "LBRACKET";
        case TokenType::RBRACKET:     return "RBRACKET";
        case TokenType::ARROW:        return "ARROW";
        case TokenType::NEWLINE:      return "NEWLINE";
        case TokenType::INDENT:       return "INDENT";
        case TokenType::DEDENT:       return "DEDENT";
        case TokenType::END_OF_FILE:  return "END_OF_FILE";
        case TokenType::UNKNOWN_ERROR:return "UNKNOWN_ERROR";
    }
    return "UNKNOWN";
}

std::string tokenToString(const Token& token) {
    std::ostringstream ss;
    ss << "Token(" << tokenTypeToString(token.type)
       << ", \"" << token.lexeme << "\", L" 
       << token.location.line << ":C" << token.location.column << ")";
    return ss.str();
}