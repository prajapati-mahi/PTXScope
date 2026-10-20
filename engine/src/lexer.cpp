#include "ptxscope/lexer.hpp"
#include <cctype>
#include <unordered_set>

namespace ptxscope {

namespace {

const std::unordered_set<std::string_view> kKnownInstructions = {
    "mov", "add", "sub", "mul", "mad", "setp", "ld", "st", "bra", "ret", "exit", "bar"
};

} // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    column_++;
    return c;
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (source_[current_] != expected) return false;
    current_++;
    column_++;
    return true;
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            line_++;
            column_ = 1;
            current_++;
        } else if (c == '/' && peek_next() == '/') {
            // Line comment
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek_next() == '*') {
            // Block comment
            advance(); // '/'
            advance(); // '*'
            while (!is_at_end()) {
                if (peek() == '*' && peek_next() == '/') {
                    advance();
                    advance();
                    break;
                }
                if (peek() == '\n') {
                    line_++;
                    column_ = 1;
                    current_++;
                } else {
                    advance();
                }
            }
        } else {
            break;
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skip_whitespace_and_comments();
        if (is_at_end()) {
            Token eof_tok;
            eof_tok.type = TokenType::END_OF_FILE;
            eof_tok.text = "";
            eof_tok.location = {line_, column_};
            eof_tok.length = 0;
            tokens.push_back(eof_tok);
            break;
        }
        tokens.push_back(scan_token());
    }
    return tokens;
}

Token Lexer::scan_token() {
    start_ = current_;
    token_col_ = column_;
    char c = advance();

    Token tok;
    tok.location = {line_, token_col_};

    switch (c) {
        case '@': tok.type = TokenType::AT; tok.text = "@"; tok.length = 1; return tok;
        case '!': tok.type = TokenType::BANG; tok.text = "!"; tok.length = 1; return tok;
        case ',': tok.type = TokenType::COMMA; tok.text = ","; tok.length = 1; return tok;
        case ':': tok.type = TokenType::COLON; tok.text = ":"; tok.length = 1; return tok;
        case ';': tok.type = TokenType::SEMICOLON; tok.text = ";"; tok.length = 1; return tok;
        case '(': tok.type = TokenType::LPAREN; tok.text = "("; tok.length = 1; return tok;
        case ')': tok.type = TokenType::RPAREN; tok.text = ")"; tok.length = 1; return tok;
        case '{': tok.type = TokenType::LBRACE; tok.text = "{"; tok.length = 1; return tok;
        case '}': tok.type = TokenType::RBRACE; tok.text = "}"; tok.length = 1; return tok;
        case '[': tok.type = TokenType::LBRACKET; tok.text = "["; tok.length = 1; return tok;
        case ']': tok.type = TokenType::RBRACKET; tok.text = "]"; tok.length = 1; return tok;
        case '+': tok.type = TokenType::PLUS; tok.text = "+"; tok.length = 1; return tok;
        case '-':
            if (std::isdigit(static_cast<unsigned char>(peek()))) {
                return scan_number();
            }
            tok.type = TokenType::MINUS;
            tok.text = "-";
            tok.length = 1;
            return tok;
        case '"':
            return scan_string();
        case '.':
            return scan_directive_or_dot();
        case '%':
            return scan_register();
        default:
            if (std::isdigit(static_cast<unsigned char>(c))) {
                return scan_number();
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
                return scan_identifier();
            }
            tok.type = TokenType::UNKNOWN;
            tok.text = std::string(1, c);
            tok.length = 1;
            errors_.push_back("Unexpected character '" + tok.text + "' at line " +
                              std::to_string(line_) + ", col " + std::to_string(token_col_));
            return tok;
    }
}

Token Lexer::scan_directive_or_dot() {
    // start_ is at '.'
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    std::string text = source_.substr(start_, current_ - start_);
    Token tok;
    tok.location = {line_, token_col_};
    tok.length = static_cast<uint32_t>(text.length());
    tok.text = text;

    if (text.length() == 1) {
        tok.type = TokenType::DOT;
    } else {
        tok.type = TokenType::DIRECTIVE;
    }
    return tok;
}

Token Lexer::scan_register() {
    // start_ is at '%'
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                           peek() == '_' || peek() == '.' || peek() == '$')) {
        advance();
    }
    std::string text = source_.substr(start_, current_ - start_);
    Token tok;
    tok.type = TokenType::REGISTER;
    tok.text = text;
    tok.location = {line_, token_col_};
    tok.length = static_cast<uint32_t>(text.length());
    return tok;
}

Token Lexer::scan_identifier() {
    while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                           peek() == '_' || peek() == '$')) {
        advance();
    }
    std::string text = source_.substr(start_, current_ - start_);
    Token tok;
    tok.location = {line_, token_col_};
    tok.length = static_cast<uint32_t>(text.length());
    tok.text = text;

    if (kKnownInstructions.find(text) != kKnownInstructions.end()) {
        tok.type = TokenType::INSTRUCTION;
    } else if (text.length() >= 2 && text[0] == 'p' && std::isdigit(static_cast<unsigned char>(text[1]))) {
        bool all_digits = true;
        for (size_t i = 1; i < text.length(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
                all_digits = false;
                break;
            }
        }
        tok.type = all_digits ? TokenType::REGISTER : TokenType::IDENTIFIER;
    } else {
        tok.type = TokenType::IDENTIFIER;
    }
    return tok;
}

Token Lexer::scan_number() {
    // Check if hex: 0x... or float hex: 0f..., 0d...
    if (source_[start_] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // 'x'
        while (!is_at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        std::string text = source_.substr(start_, current_ - start_);
        Token tok;
        tok.type = TokenType::INT_LITERAL;
        tok.text = text;
        tok.location = {line_, token_col_};
        tok.length = static_cast<uint32_t>(text.length());
        return tok;
    }

    if (source_[start_] == '0' && (peek() == 'f' || peek() == 'F' || peek() == 'd' || peek() == 'D')) {
        advance(); // 'f' or 'd'
        while (!is_at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        std::string text = source_.substr(start_, current_ - start_);
        Token tok;
        tok.type = TokenType::FLOAT_LITERAL;
        tok.text = text;
        tok.location = {line_, token_col_};
        tok.length = static_cast<uint32_t>(text.length());
        return tok;
    }

    bool has_dot = false;
    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        has_dot = true;
        advance(); // '.'
        while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        if (peek() == 'f' || peek() == 'F') {
            advance();
        }
    }

    std::string text = source_.substr(start_, current_ - start_);
    Token tok;
    tok.type = has_dot ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL;
    tok.text = text;
    tok.location = {line_, token_col_};
    tok.length = static_cast<uint32_t>(text.length());
    return tok;
}

Token Lexer::scan_string() {
    // start_ is at '"'
    while (!is_at_end() && peek() != '"') {
        if (peek() == '\n') {
            line_++;
            column_ = 1;
        }
        advance();
    }

    if (is_at_end()) {
        errors_.push_back("Unterminated string literal at line " + std::to_string(line_));
        Token tok;
        tok.type = TokenType::UNKNOWN;
        tok.text = source_.substr(start_);
        tok.location = {line_, token_col_};
        tok.length = static_cast<uint32_t>(tok.text.length());
        return tok;
    }

    advance(); // closing '"'
    std::string text = source_.substr(start_, current_ - start_);
    Token tok;
    tok.type = TokenType::STRING_LITERAL;
    tok.text = text;
    tok.location = {line_, token_col_};
    tok.length = static_cast<uint32_t>(text.length());
    return tok;
}

} // namespace ptxscope
