#include "ptxscope/parser.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace ptxscope {

Parser::Parser(std::vector<Token> tokens, DiagnosticEngine& diag)
    : tokens_(std::move(tokens)), diag_(diag) {}

const Token& Parser::peek() const {
    if (current_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_];
}

const Token& Parser::peek_next() const {
    if (current_ + 1 >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_ + 1];
}

const Token& Parser::previous() const {
    if (current_ == 0) return tokens_[0];
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& err_code, const std::string& err_msg) {
    if (check(type)) return advance();
    const Token& t = peek();
    diag_.report_error(err_code, err_msg + " (found '" + t.text + "')", t.location,
                       "PTX grammar requires a specific token here.",
                       "Ensure statements end with semicolons and delimiters match.");
    return t;
}

void Parser::synchronize_to_statement() {
    while (!is_at_end()) {
        if (peek().type == TokenType::SEMICOLON) {
            advance();
            return;
        }
        if (peek().type == TokenType::RBRACE) {
            return;
        }
        advance();
    }
}

std::shared_ptr<Module> Parser::parse_module() {
    auto mod = std::make_shared<Module>();

    while (!is_at_end()) {
        if (check(TokenType::DIRECTIVE)) {
            const std::string& dir = peek().text;
            if (dir == ".version" || dir == ".target" || dir == ".address_size") {
                parse_header_directive(*mod);
            } else if (dir == ".entry" || dir == ".func") {
                bool is_entry = (dir == ".entry");
                advance(); // consume .entry or .func
                auto fn = parse_function(is_entry);
                if (fn) {
                    mod->functions.push_back(fn);
                }
            } else if (dir == ".visible" || dir == ".weak") {
                // Modifier before .entry or .func
                std::string mod_text = advance().text;
                if (check(TokenType::DIRECTIVE) && (peek().text == ".entry" || peek().text == ".func")) {
                    bool is_entry = (peek().text == ".entry");
                    advance();
                    auto fn = parse_function(is_entry);
                    if (fn) {
                        mod->functions.push_back(fn);
                    }
                } else {
                    diag_.report_error("E002_UNEXPECTED_DIRECTIVE", "Unexpected directive after linkage modifier: " + peek().text, peek().location);
                    synchronize_to_statement();
                }
            } else {
                diag_.report_error("E003_UNSUPPORTED_TOP_LEVEL", "Unsupported top-level directive: " + dir, peek().location,
                                   "PTXScope supports .version, .target, .address_size, .entry, .func.",
                                   "Verify your PTX file header directives.");
                advance();
                synchronize_to_statement();
            }
        } else {
            diag_.report_error("E001_SYNTAX_ERROR", "Unexpected token at top level: '" + peek().text + "'", peek().location);
            advance();
        }
    }

    return mod;
}

void Parser::parse_header_directive(Module& mod) {
    const Token& dir_tok = advance();
    std::string line = dir_tok.text;

    while (!is_at_end() && !check(TokenType::SEMICOLON) && peek().location.line == dir_tok.location.line) {
        line += " " + advance().text;
    }
    match(TokenType::SEMICOLON);
    mod.header_directives.push_back(line);

    if (dir_tok.text == ".target") {
        size_t pos = line.find("sm_");
        if (pos != std::string::npos) {
            mod.target_arch = line.substr(pos, 5);
        }
    } else if (dir_tok.text == ".address_size") {
        if (line.find("64") != std::string::npos) mod.address_size = 64;
        else if (line.find("32") != std::string::npos) mod.address_size = 32;
    }
}

std::shared_ptr<Function> Parser::parse_function(bool is_entry) {
    auto fn = std::make_shared<Function>();
    fn->is_entry = is_entry;

    if (!check(TokenType::IDENTIFIER)) {
        diag_.report_error("E004_EXPECTED_FUNC_NAME", "Expected function or kernel name after " + std::string(is_entry ? ".entry" : ".func"), peek().location);
        return nullptr;
    }
    fn->name = advance().text;

    if (match(TokenType::LPAREN)) {
        parse_parameters(*fn);
        consume(TokenType::RPAREN, "E005_EXPECTED_RPAREN", "Expected ')' after parameter list");
    }

    if (!check(TokenType::LBRACE)) {
        diag_.report_error("E006_EXPECTED_LBRACE", "Expected '{' to begin function body", peek().location);
        return nullptr;
    }
    advance(); // consume '{'

    parse_function_body(*fn);

    consume(TokenType::RBRACE, "E007_EXPECTED_RBRACE", "Expected '}' at end of function body");
    return fn;
}

void Parser::parse_parameters(Function& fn) {
    while (!is_at_end() && !check(TokenType::RPAREN)) {
        std::string param_str;
        while (!is_at_end() && !check(TokenType::COMMA) && !check(TokenType::RPAREN)) {
            if (!param_str.empty()) param_str += " ";
            param_str += advance().text;
        }
        if (!param_str.empty()) {
            fn.params.push_back(param_str);
        }
        if (check(TokenType::COMMA)) {
            advance();
        }
    }
}

void Parser::parse_function_body(Function& fn) {
    auto current_bb = std::make_shared<BasicBlock>("entry");
    fn.blocks.push_back(current_bb);

    while (!is_at_end() && !check(TokenType::RBRACE)) {
        parse_statement(fn, current_bb);
    }

    while (fn.blocks.size() > 1 && fn.blocks.back()->instructions.empty()) {
        fn.blocks.pop_back();
    }
}

void Parser::parse_reg_declaration(Function& fn) {
    // We already saw .reg
    const Token& reg_tok = advance();
    std::string decl = reg_tok.text;

    while (!is_at_end() && !check(TokenType::SEMICOLON)) {
        decl += " " + advance().text;
    }
    decl += ";";
    consume(TokenType::SEMICOLON, "E008_EXPECTED_SEMICOLON", "Expected ';' after .reg declaration");
    fn.reg_declarations.push_back(decl);
}

void Parser::parse_statement(Function& fn, std::shared_ptr<BasicBlock>& current_bb) {
    // 1. Label: IDENTIFIER followed by COLON
    if (check(TokenType::IDENTIFIER) && peek_next().type == TokenType::COLON) {
        std::string label_name = advance().text;
        advance(); // consume ':'

        // If current block is empty, reuse it for this label
        if (current_bb->instructions.empty()) {
            current_bb->label = label_name;
        } else {
            current_bb = std::make_shared<BasicBlock>(label_name);
            fn.blocks.push_back(current_bb);
        }
        return;
    }

    // 2. Register declaration
    if (check(TokenType::DIRECTIVE) && peek().text == ".reg") {
        parse_reg_declaration(fn);
        return;
    }

    // 3. Local variable or other supported directive
    if (check(TokenType::DIRECTIVE) && (peek().text == ".local" || peek().text == ".shared" || peek().text == ".param")) {
        std::string decl = advance().text;
        while (!is_at_end() && !check(TokenType::SEMICOLON)) {
            decl += " " + advance().text;
        }
        decl += ";";
        consume(TokenType::SEMICOLON, "E008_EXPECTED_SEMICOLON", "Expected ';' after declaration");
        fn.reg_declarations.push_back(decl);
        return;
    }

    // 4. Predicate prefix @p0 or @!p1
    Predicate pred;
    if (match(TokenType::AT)) {
        pred.is_predicated = true;
        if (match(TokenType::BANG)) {
            pred.is_negated = true;
        }
        if (check(TokenType::REGISTER) || check(TokenType::IDENTIFIER)) {
            pred.reg_name = advance().text;
        } else {
            diag_.report_error("E009_INVALID_PREDICATE", "Expected predicate register after '@'", peek().location);
            synchronize_to_statement();
            return;
        }
    }

    // 5. Instruction
    if (check(TokenType::INSTRUCTION) || check(TokenType::IDENTIFIER)) {
        auto inst = parse_instruction(pred);
        if (inst) {
            current_bb->add_instruction(inst);

            // If this is an unconditional terminator (ret, exit, unconditional bra), next instruction belongs in a new block
            if (inst->is_terminator() && !inst->predicate.is_predicated) {
                // If there are more statements before a label, they will belong to an unreachable block
                std::string new_label = "$block_" + std::to_string(++bb_counter_);
                current_bb = std::make_shared<BasicBlock>(new_label);
                fn.blocks.push_back(current_bb);
            }
        }
        return;
    }

    // Unexpected token
    diag_.report_error("E010_UNEXPECTED_TOKEN", "Unexpected statement token: '" + peek().text + "'", peek().location);
    advance();
    synchronize_to_statement();
}

std::shared_ptr<Instruction> Parser::parse_instruction(Predicate pred) {
    Token op_tok = advance();
    auto inst = std::make_shared<Instruction>();
    inst->span.start = op_tok.location;
    inst->predicate = pred;

    std::string base_op = op_tok.text;
    inst->opcode_str = base_op;

    if (base_op == "mov")      inst->opcode = Opcode::MOV;
    else if (base_op == "add") inst->opcode = Opcode::ADD;
    else if (base_op == "sub") inst->opcode = Opcode::SUB;
    else if (base_op == "mul") inst->opcode = Opcode::MUL;
    else if (base_op == "mad") inst->opcode = Opcode::MAD;
    else if (base_op == "setp") inst->opcode = Opcode::SETP;
    else if (base_op == "ld")  inst->opcode = Opcode::LD;
    else if (base_op == "st")  inst->opcode = Opcode::ST;
    else if (base_op == "bra") inst->opcode = Opcode::BRA;
    else if (base_op == "ret") inst->opcode = Opcode::RET;
    else if (base_op == "exit") inst->opcode = Opcode::EXIT;
    else if (base_op == "bar") inst->opcode = Opcode::BAR_SYNC;
    else {
        inst->opcode = Opcode::UNKNOWN;
        diag_.report_warning("W010_UNSUPPORTED_INSTRUCTION",
                             "Instruction '" + base_op + "' is outside the primary analyzed PTX subset.",
                             op_tok.location,
                             "PTXScope supports static analysis for mov, add, sub, mul, mad, setp, ld, st, bra, ret, exit, bar.",
                             "Verify that this instruction is supported by target hardware.");
    }

    // Parse modifiers (e.g. .global, .u32, .lo, .wide, .sync, .uni, .eq, etc.)
    while (check(TokenType::DIRECTIVE) || check(TokenType::DOT)) {
        Token mod_tok = advance();
        std::string mod = mod_tok.text;
        if (mod.rfind(".", 0) == 0) {
            mod = mod.substr(1);
        }

        DataType dt = parse_data_type("." + mod);
        if (dt != DataType::NONE) {
            inst->type = dt;
        } else if (mod == "global" || mod == "shared" || mod == "param" || mod == "const" || mod == "local") {
            inst->state_space = mod;
        } else if (mod == "eq" || mod == "ne" || mod == "lt" || mod == "le" || mod == "gt" || mod == "ge") {
            inst->comp_op = mod;
        } else {
            inst->modifiers.push_back(mod);
        }
    }

    // Parse operands separated by commas until semicolon
    if (!check(TokenType::SEMICOLON)) {
        while (!is_at_end() && !check(TokenType::SEMICOLON)) {
            Operand op = parse_operand(inst->type);
            inst->operands.push_back(op);

            if (check(TokenType::COMMA)) {
                advance(); // consume ','
            } else {
                break;
            }
        }
    }

    consume(TokenType::SEMICOLON, "E011_EXPECTED_SEMICOLON", "Expected ';' at the end of instruction");
    inst->span.end = previous().location;
    return inst;
}

Operand Parser::parse_operand(DataType expected_type) {
    // 1. Memory reference: [reg] or [reg+offset] or [label]
    if (match(TokenType::LBRACKET)) {
        std::string base_reg;
        int64_t offset = 0;

        if (check(TokenType::REGISTER) || check(TokenType::IDENTIFIER)) {
            base_reg = advance().text;
        } else {
            diag_.report_error("E012_INVALID_MEMORY_OPERAND", "Expected register inside memory operand '[...]' ", peek().location);
        }

        if (match(TokenType::PLUS)) {
            if (check(TokenType::INT_LITERAL)) {
                Token off_tok = advance();
                offset = std::stoll(off_tok.text, nullptr, 0);
            }
        } else if (match(TokenType::MINUS)) {
            if (check(TokenType::INT_LITERAL)) {
                Token off_tok = advance();
                offset = -std::stoll(off_tok.text, nullptr, 0);
            }
        }

        consume(TokenType::RBRACKET, "E013_EXPECTED_RBRACKET", "Expected ']' at end of memory operand");
        return Operand::make_mem(base_reg, offset, expected_type);
    }

    // 2. Register: %r1, %rd2, %p0, etc.
    if (check(TokenType::REGISTER)) {
        return Operand::make_reg(advance().text, expected_type);
    }

    // 3. Float literal
    if (check(TokenType::FLOAT_LITERAL)) {
        Token t = advance();
        double val = 0.0;
        if (t.text.rfind("0f", 0) == 0 || t.text.rfind("0d", 0) == 0) {
            // Hex encoded float representation in PTX
            return Operand::make_float(0.0, t.text, expected_type);
        }
        try {
            val = std::stod(t.text);
        } catch (...) {}
        return Operand::make_float(val, t.text, expected_type);
    }

    // 4. Integer literal (decimal or hex)
    if (check(TokenType::INT_LITERAL)) {
        Token t = advance();
        int64_t val = 0;
        try {
            val = std::stoll(t.text, nullptr, 0);
        } catch (...) {}
        return Operand::make_int(val, expected_type);
    }

    // 5. Negative number
    if (match(TokenType::MINUS)) {
        if (check(TokenType::INT_LITERAL)) {
            Token t = advance();
            int64_t val = -std::stoll(t.text, nullptr, 0);
            return Operand::make_int(val, expected_type);
        }
    }

    // 6. Identifier / Label
    if (check(TokenType::IDENTIFIER)) {
        return Operand::make_label(advance().text);
    }

    // Fallback unknown operand
    Token t = advance();
    diag_.report_error("E014_UNEXPECTED_OPERAND", "Unexpected operand token: '" + t.text + "'", t.location);
    return Operand::make_label(t.text);
}

} // namespace ptxscope
