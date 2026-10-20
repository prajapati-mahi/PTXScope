#pragma once

#include "ptxscope/token.hpp"
#include "ptxscope/ir.hpp"
#include "ptxscope/diagnostics.hpp"
#include <vector>
#include <memory>

namespace ptxscope {

class Parser {
public:
    Parser(std::vector<Token> tokens, DiagnosticEngine& diag);

    std::shared_ptr<Module> parse_module();

private:
    const Token& peek() const;
    const Token& peek_next() const;
    const Token& previous() const;
    bool is_at_end() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& err_code, const std::string& err_msg);

    void synchronize_to_statement();

    void parse_header_directive(Module& mod);
    std::shared_ptr<Function> parse_function(bool is_entry);
    void parse_parameters(Function& fn);
    void parse_function_body(Function& fn);
    void parse_reg_declaration(Function& fn);
    void parse_statement(Function& fn, std::shared_ptr<BasicBlock>& current_bb);
    std::shared_ptr<Instruction> parse_instruction(Predicate pred);
    Operand parse_operand(DataType expected_type);

    std::vector<Token> tokens_;
    size_t current_ = 0;
    DiagnosticEngine& diag_;
    int bb_counter_ = 0;
};

} // namespace ptxscope
