#include "ptxscope/passes/algebraic_simp.hpp"
#include <sstream>

namespace ptxscope {

PassResult AlgebraicSimplificationPass::run_on_function(Function& fn) {
    PassResult res;
    res.pass_name = name();

    for (auto& bb : fn.blocks) {
        for (auto& inst : bb->instructions) {
            if (inst->opcode != Opcode::ADD && inst->opcode != Opcode::SUB && inst->opcode != Opcode::MUL) {
                continue;
            }

            if (inst->operands.size() != 3) continue;

            const auto& dest = inst->operands[0];
            const auto& src1 = inst->operands[1];
            const auto& src2 = inst->operands[2];

            if (dest.kind != Operand::Kind::REGISTER) continue;

            // Reject floating point: IEEE-754 identities differ for +0.0, -0.0, NaN
            if (is_floating_point_type(inst->type)) {
                res.skipped_reasons.push_back("Skipped FP algebraic simplification for '" + inst->to_ptx() + "'");
                continue;
            }

            std::string old_ptx = inst->to_ptx();
            bool applied = false;
            std::string rule_desc;

            // Rule 1: x + 0 -> x
            if (inst->opcode == Opcode::ADD) {
                if (src2.kind == Operand::Kind::IMMEDIATE_INT && src2.int_val == 0) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, src1 };
                    applied = true;
                    rule_desc = "Identity: x + 0 -> x";
                } else if (src1.kind == Operand::Kind::IMMEDIATE_INT && src1.int_val == 0) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, src2 };
                    applied = true;
                    rule_desc = "Identity: 0 + x -> x";
                }
            }
            // Rule 2: x - 0 -> x
            else if (inst->opcode == Opcode::SUB) {
                if (src2.kind == Operand::Kind::IMMEDIATE_INT && src2.int_val == 0) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, src1 };
                    applied = true;
                    rule_desc = "Identity: x - 0 -> x";
                }
            }
            // Rule 3: x * 1 -> x, and x * 0 -> 0
            else if (inst->opcode == Opcode::MUL) {
                if (src2.kind == Operand::Kind::IMMEDIATE_INT && src2.int_val == 1) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, src1 };
                    applied = true;
                    rule_desc = "Identity: x * 1 -> x";
                } else if (src1.kind == Operand::Kind::IMMEDIATE_INT && src1.int_val == 1) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, src2 };
                    applied = true;
                    rule_desc = "Identity: 1 * x -> x";
                } else if ((src1.kind == Operand::Kind::IMMEDIATE_INT && src1.int_val == 0) ||
                           (src2.kind == Operand::Kind::IMMEDIATE_INT && src2.int_val == 0)) {
                    inst->opcode = Opcode::MOV;
                    inst->opcode_str = "mov";
                    inst->modifiers.clear();
                    inst->operands = { dest, Operand::make_int(0, inst->type) };
                    applied = true;
                    rule_desc = "Zero multiplication: x * 0 -> 0";
                }
            }

            if (applied) {
                res.modified = true;
                res.num_transformations++;
                res.explanations.push_back("Algebraic simplification: '" + old_ptx + "' -> '" + inst->to_ptx() + "' (" + rule_desc + ")");
            }
        }
    }

    return res;
}

} // namespace ptxscope
