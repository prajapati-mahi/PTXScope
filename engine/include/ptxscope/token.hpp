#pragma once

#include "ptxscope/common.hpp"
#include <string>
#include <string_view>

namespace ptxscope {

enum class TokenType {
    END_OF_FILE,
    DIRECTIVE,        // .version, .target, .entry, .reg, etc.
    INSTRUCTION,      // mov, add, mul, etc.
    REGISTER,         // %r1, %rd0, %p0, %tid.x
    IDENTIFIER,       // label names, function names
    INT_LITERAL,      // 42, 0x1F, -5
    FLOAT_LITERAL,    // 1.5, 0f3f800000
    STRING_LITERAL,   // "sm_70"
    AT,               // @
    BANG,             // !
    COMMA,            // ,
    COLON,            // :
    SEMICOLON,        // ;
    LPAREN,           // (
    RPAREN,           // )
    LBRACE,           // {
    RBRACE,           // }
    LBRACKET,         // [
    RBRACKET,         // ]
    DOT,              // .
    PLUS,             // +
    MINUS,            // -
    UNKNOWN
};

inline std::string token_type_to_string(TokenType tt) {
    switch (tt) {
        case TokenType::END_OF_FILE:    return "EOF";
        case TokenType::DIRECTIVE:      return "DIRECTIVE";
        case TokenType::INSTRUCTION:    return "INSTRUCTION";
        case TokenType::REGISTER:       return "REGISTER";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::INT_LITERAL:    return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::AT:             return "@";
        case TokenType::BANG:           return "!";
        case TokenType::COMMA:          return ",";
        case TokenType::COLON:          return ":";
        case TokenType::SEMICOLON:      return ";";
        case TokenType::LPAREN:         return "(";
        case TokenType::RPAREN:         return ")";
        case TokenType::LBRACE:         return "{";
        case TokenType::RBRACE:         return "}";
        case TokenType::LBRACKET:       return "[";
        case TokenType::RBRACKET:       return "]";
        case TokenType::DOT:            return ".";
        case TokenType::PLUS:           return "+";
        case TokenType::MINUS:          return "-";
        default:                        return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, TokenType tt) {
    return os << token_type_to_string(tt);
}

struct Token {
    TokenType type = TokenType::UNKNOWN;
    std::string text;
    SourceLocation location;
    uint32_t length = 0;

    SourceSpan span() const {
        return {location, {location.line, location.column + length}};
    }
};

} // namespace ptxscope
