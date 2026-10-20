#pragma once

#include "ptxscope/common.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ptxscope {

class Operand {
public:
    enum class Kind {
        REGISTER,
        IMMEDIATE_INT,
        IMMEDIATE_FLOAT,
        MEMORY,
        LABEL
    };

    Kind kind = Kind::REGISTER;
    std::string reg_name;
    int64_t int_val = 0;
    double float_val = 0.0;
    std::string float_raw; // e.g. "0f3f800000" or "1.5"
    std::string label_name;
    std::string mem_base_reg;
    int64_t mem_offset = 0;
    DataType type = DataType::NONE;

    static Operand make_reg(std::string name, DataType dt = DataType::NONE);
    static Operand make_int(int64_t val, DataType dt = DataType::NONE);
    static Operand make_float(double val, std::string raw = "", DataType dt = DataType::NONE);
    static Operand make_label(std::string name);
    static Operand make_mem(std::string base_reg, int64_t offset = 0, DataType dt = DataType::NONE);

    std::string to_ptx() const;
    bool operator==(const Operand& other) const;
};

class Instruction {
public:
    Opcode opcode = Opcode::UNKNOWN;
    std::string opcode_str;
    DataType type = DataType::NONE;
    std::string comp_op;     // e.g. "eq", "ne", "lt", "gt" for setp
    std::string state_space; // e.g. "param", "global", "shared", "const"
    std::vector<std::string> modifiers; // additional modifiers e.g. "uni", "approx"
    Predicate predicate;
    std::vector<Operand> operands;
    SourceSpan span;

    bool is_terminator() const;
    bool is_branch() const;
    bool is_conditional_branch() const;
    std::optional<std::string> branch_target() const;
    bool has_side_effects() const;

    std::optional<std::string> get_def_reg() const;
    std::vector<std::string> get_use_regs() const;

    std::string to_ptx() const;
    std::shared_ptr<Instruction> clone() const;
};

class BasicBlock {
public:
    std::string label;
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;

    explicit BasicBlock(std::string lbl) : label(std::move(lbl)) {}

    void add_instruction(std::shared_ptr<Instruction> inst) {
        instructions.push_back(std::move(inst));
    }

    std::string to_ptx() const;
};

class Function {
public:
    std::string name;
    bool is_entry = false;
    std::vector<std::string> params;
    std::vector<std::string> reg_declarations;
    std::vector<std::shared_ptr<BasicBlock>> blocks;

    std::shared_ptr<BasicBlock> get_entry_block() const {
        return blocks.empty() ? nullptr : blocks.front();
    }

    std::shared_ptr<BasicBlock> find_block(const std::string& lbl) const {
        for (const auto& b : blocks) {
            if (b->label == lbl) return b;
        }
        return nullptr;
    }

    std::string to_ptx() const;
    size_t get_instruction_count() const;
};

class Module {
public:
    int version_major = 7;
    int version_minor = 0;
    std::string target_arch = "sm_70";
    int address_size = 64;
    std::vector<std::string> header_directives;
    std::vector<std::shared_ptr<Function>> functions;

    std::string to_ptx() const;
    size_t get_total_instructions() const;
};

} // namespace ptxscope
