#pragma once

#include "ptxscope/token.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace ptxscope {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();
    const std::vector<std::string>& get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    char peek() const;
    char peek_next() const;
    char advance();
    bool is_at_end() const;
    bool match(char expected);

    void skip_whitespace_and_comments();
    Token scan_token();

    Token scan_directive_or_dot();
    Token scan_register();
    Token scan_identifier();
    Token scan_number();
    Token scan_string();

    std::string source_;
    size_t start_ = 0;
    size_t current_ = 0;
    uint32_t line_ = 1;
    uint32_t column_ = 1;
    uint32_t token_col_ = 1;

    std::vector<std::string> errors_;
};

} // namespace ptxscope
