#include "tokens.hpp"
#include <sstream>

std::string tipoTokenATexto(TipoToken tipo) {
    switch (tipo) {
        case TipoToken::KW_CLASS:     return "KW_CLASS";
        case TipoToken::KW_DEF:       return "KW_DEF";
        case TipoToken::KW_IF:        return "KW_IF";
        case TipoToken::KW_ELIF:      return "KW_ELIF";
        case TipoToken::KW_ELSE:      return "KW_ELSE";
        case TipoToken::KW_WHILE:     return "KW_WHILE";
        case TipoToken::KW_RETURN:    return "KW_RETURN";
        case TipoToken::KW_FOR:       return "KW_FOR";
        case TipoToken::KW_IN:        return "KW_IN";
        case TipoToken::KW_IS:        return "KW_IS";
        case TipoToken::KW_AND:       return "KW_AND";
        case TipoToken::KW_OR:        return "KW_OR";
        case TipoToken::KW_NOT:       return "KW_NOT";
        case TipoToken::KW_NONE:      return "KW_NONE";
        case TipoToken::KW_TRUE:      return "KW_TRUE";
        case TipoToken::KW_FALSE:     return "KW_FALSE";
        case TipoToken::KW_PASS:      return "KW_PASS";
        case TipoToken::KW_GLOBAL:    return "KW_GLOBAL";
        case TipoToken::KW_NONLOCAL:  return "KW_NONLOCAL";
        case TipoToken::KW_RESERVED:  return "KW_RESERVED";
        case TipoToken::IDENTIFIER:   return "IDENTIFIER";
        case TipoToken::INTEGER:      return "INTEGER";
        case TipoToken::STRING:       return "STRING";
        case TipoToken::IDSTRING:     return "IDSTRING";
        case TipoToken::PLUS:         return "PLUS";
        case TipoToken::MINUS:        return "MINUS";
        case TipoToken::STAR:         return "STAR";
        case TipoToken::SLASH_SLASH:  return "SLASH_SLASH";
        case TipoToken::PERCENT:      return "PERCENT";
        case TipoToken::EQ_EQ:        return "EQ_EQ";
        case TipoToken::BANG_EQ:      return "BANG_EQ";
        case TipoToken::LESS_EQ:      return "LESS_EQ";
        case TipoToken::GREATER_EQ:   return "GREATER_EQ";
        case TipoToken::LESS:         return "LESS";
        case TipoToken::GREATER:      return "GREATER";
        case TipoToken::EQUAL:        return "EQUAL";
        case TipoToken::COLON:        return "COLON";
        case TipoToken::DOT:          return "DOT";
        case TipoToken::COMMA:        return "COMMA";
        case TipoToken::LPAREN:       return "LPAREN";
        case TipoToken::RPAREN:       return "RPAREN";
        case TipoToken::LBRACKET:     return "LBRACKET";
        case TipoToken::RBRACKET:     return "RBRACKET";
        case TipoToken::ARROW:        return "ARROW";
        case TipoToken::NEWLINE:      return "NEWLINE";
        case TipoToken::INDENT:       return "INDENT";
        case TipoToken::DEDENT:       return "DEDENT";
        case TipoToken::END_OF_FILE:  return "END_OF_FILE";
        case TipoToken::UNKNOWN_ERROR:return "UNKNOWN_ERROR";
    }
    return "UNKNOWN";
}

std::string tokenATexto(const Token& token) {
    std::ostringstream ss;
    ss << "Token(" << tipoTokenATexto(token.tipo)
       << ", \"" << token.lexema << "\", L"
       << token.posicion.linea << ":C" << token.posicion.columna << ")";
    return ss.str();
}
