#include "ptxscope/ir.hpp"
#include <sstream>
#include <algorithm>

namespace ptxscope {

Operand Operand::make_reg(std::string name, DataType dt) {
    Operand op;
    op.kind = Kind::REGISTER;
    op.reg_name = std::move(name);
    op.type = dt;
    return op;
}

Operand Operand::make_int(int64_t val, DataType dt) {
    Operand op;
    op.kind = Kind::IMMEDIATE_INT;
    op.int_val = val;
    op.type = dt;
    return op;
}

Operand Operand::make_float(double val, std::string raw, DataType dt) {
    Operand op;
    op.kind = Kind::IMMEDIATE_FLOAT;
    op.float_val = val;
    op.float_raw = std::move(raw);
    op.type = dt;
    return op;
}

Operand Operand::make_label(std::string name) {
    Operand op;
    op.kind = Kind::LABEL;
    op.label_name = std::move(name);
    return op;
}

Operand Operand::make_mem(std::string base_reg, int64_t offset, DataType dt) {
    Operand op;
    op.kind = Kind::MEMORY;
    op.mem_base_reg = std::move(base_reg);
    op.mem_offset = offset;
    op.type = dt;
    return op;
}

std::string Operand::to_ptx() const {
    switch (kind) {
        case Kind::REGISTER:
            return reg_name;
        case Kind::IMMEDIATE_INT:
            return std::to_string(int_val);
        case Kind::IMMEDIATE_FLOAT:
            if (!float_raw.empty()) return float_raw;
            return std::to_string(float_val);
        case Kind::LABEL:
            return label_name;
        case Kind::MEMORY: {
            std::ostringstream ss;
            ss << "[" << mem_base_reg;
            if (mem_offset > 0) ss << "+" << mem_offset;
            else if (mem_offset < 0) ss << mem_offset;
            ss << "]";
            return ss.str();
        }
    }
    return "";
}

bool Operand::operator==(const Operand& other) const {
    if (kind != other.kind) return false;
    switch (kind) {
        case Kind::REGISTER:        return reg_name == other.reg_name;
        case Kind::IMMEDIATE_INT:    return int_val == other.int_val;
        case Kind::IMMEDIATE_FLOAT:  return float_raw == other.float_raw && float_val == other.float_val;
        case Kind::LABEL:            return label_name == other.label_name;
        case Kind::MEMORY:           return mem_base_reg == other.mem_base_reg && mem_offset == other.mem_offset;
    }
    return false;
}

bool Instruction::is_terminator() const {
    return opcode == Opcode::BRA || opcode == Opcode::RET || opcode == Opcode::EXIT;
}

bool Instruction::is_branch() const {
    return opcode == Opcode::BRA;
}

bool Instruction::is_conditional_branch() const {
    return opcode == Opcode::BRA && predicate.is_predicated;
}

std::optional<std::string> Instruction::branch_target() const {
    if (!is_branch() || operands.empty()) return std::nullopt;
    if (operands[0].kind == Operand::Kind::LABEL) {
        return operands[0].label_name;
    }
    if (operands[0].kind == Operand::Kind::REGISTER) {
        return operands[0].reg_name;
    }
    return std::nullopt;
}

bool Instruction::has_side_effects() const {
    if (opcode == Opcode::ST || opcode == Opcode::RET ||
        opcode == Opcode::EXIT || opcode == Opcode::BAR_SYNC) {
        return true;
    }
    return false;
}

std::optional<std::string> Instruction::get_def_reg() const {
    if (opcode == Opcode::ST || opcode == Opcode::BRA ||
        opcode == Opcode::RET || opcode == Opcode::EXIT ||
        opcode == Opcode::BAR_SYNC) {
        return std::nullopt;
    }
    if (operands.empty()) return std::nullopt;
    if (operands[0].kind == Operand::Kind::REGISTER) {
        return operands[0].reg_name;
    }
    return std::nullopt;
}

std::vector<std::string> Instruction::get_use_regs() const {
    std::vector<std::string> uses;
    if (predicate.is_predicated && !predicate.reg_name.empty()) {
        uses.push_back(predicate.reg_name);
    }
    if (opcode == Opcode::ST) {
        // st.[state].[type] [%rd_dest], %r_src
        for (const auto& op : operands) {
            if (op.kind == Operand::Kind::REGISTER) {
                uses.push_back(op.reg_name);
            } else if (op.kind == Operand::Kind::MEMORY && !op.mem_base_reg.empty()) {
                uses.push_back(op.mem_base_reg);
            }
        }
    } else {
        // operands[0] is typically def; operands[1..] are uses
        for (size_t i = 1; i < operands.size(); ++i) {
            const auto& op = operands[i];
            if (op.kind == Operand::Kind::REGISTER) {
                uses.push_back(op.reg_name);
            } else if (op.kind == Operand::Kind::MEMORY && !op.mem_base_reg.empty()) {
                uses.push_back(op.mem_base_reg);
            }
        }
    }
    return uses;
}

std::string Instruction::to_ptx() const {
    std::ostringstream ss;
    if (predicate.is_predicated) {
        ss << predicate.to_string() << "\t";
    } else {
        ss << "\t";
    }

    ss << opcode_str;
    if (!state_space.empty()) {
        ss << "." << state_space;
    }
    if (!comp_op.empty()) {
        ss << "." << comp_op;
    }
    for (const auto& mod : modifiers) {
        ss << "." << mod;
    }
    if (type != DataType::NONE) {
        ss << data_type_to_string(type);
    }

    if (!operands.empty()) {
        ss << "\t";
        for (size_t i = 0; i < operands.size(); ++i) {
            ss << operands[i].to_ptx();
            if (i + 1 < operands.size()) {
                ss << ", ";
            }
        }
    }
    ss << ";";
    return ss.str();
}

std::shared_ptr<Instruction> Instruction::clone() const {
    auto copy = std::make_shared<Instruction>();
    copy->opcode = opcode;
    copy->opcode_str = opcode_str;
    copy->type = type;
    copy->comp_op = comp_op;
    copy->state_space = state_space;
    copy->modifiers = modifiers;
    copy->predicate = predicate;
    copy->operands = operands;
    copy->span = span;
    return copy;
}

std::string BasicBlock::to_ptx() const {
    std::ostringstream ss;
    if (!label.empty() && label != "entry") {
        ss << label << ":\n";
    }
    for (const auto& inst : instructions) {
        ss << inst->to_ptx() << "\n";
    }
    return ss.str();
}

std::string Function::to_ptx() const {
    std::ostringstream ss;
    ss << (is_entry ? ".entry " : ".func ") << name << "(\n";
    for (size_t i = 0; i < params.size(); ++i) {
        ss << "\t" << params[i];
        if (i + 1 < params.size()) ss << ",";
        ss << "\n";
    }
    ss << ") {\n";

    for (const auto& decl : reg_declarations) {
        ss << "\t" << decl << "\n";
    }

    for (const auto& bb : blocks) {
        ss << bb->to_ptx();
    }
    ss << "}\n";
    return ss.str();
}

size_t Function::get_instruction_count() const {
    size_t count = 0;
    for (const auto& b : blocks) {
        count += b->instructions.size();
    }
    return count;
}

std::string Module::to_ptx() const {
    std::ostringstream ss;
    for (const auto& h : header_directives) {
        ss << h << "\n";
    }
    ss << "\n";
    for (const auto& f : functions) {
        ss << f->to_ptx() << "\n";
    }
    return ss.str();
}

size_t Module::get_total_instructions() const {
    size_t count = 0;
    for (const auto& f : functions) {
        count += f->get_instruction_count();
    }
    return count;
}

} // namespace ptxscope
