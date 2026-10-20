#include "ptxscope/passes/const_fold.hpp"
#include <sstream>

namespace ptxscope {

namespace {

int64_t truncate_to_type(int64_t val, DataType dt) {
    switch (dt) {
        case DataType::U8:  return static_cast<int64_t>(static_cast<uint8_t>(val));
        case DataType::S8:  return static_cast<int64_t>(static_cast<int8_t>(val));
        case DataType::U16: return static_cast<int64_t>(static_cast<uint16_t>(val));
        case DataType::S16: return static_cast<int64_t>(static_cast<int16_t>(val));
        case DataType::U32:
        case DataType::B32: return static_cast<int64_t>(static_cast<uint32_t>(val));
        case DataType::S32: return static_cast<int64_t>(static_cast<int32_t>(val));
        case DataType::U64:
        case DataType::B64: return static_cast<int64_t>(static_cast<uint64_t>(val));
        case DataType::S64: return val;
        default:            return val;
    }
}

} // namespace

PassResult ConstantFoldingPass::run_on_function(Function& fn) {
    PassResult res;
    res.pass_name = name();

    for (auto& bb : fn.blocks) {
        for (auto& inst : bb->instructions) {
            // Check if instruction is a binary integer operation with two immediate constants
            if (inst->opcode != Opcode::ADD && inst->opcode != Opcode::SUB && inst->opcode != Opcode::MUL) {
                continue;
            }

            // Must have 3 operands: dest, src1, src2
            if (inst->operands.size() != 3) continue;

            const auto& dest = inst->operands[0];
            const auto& src1 = inst->operands[1];
            const auto& src2 = inst->operands[2];

            if (dest.kind != Operand::Kind::REGISTER) continue;

            // Check if floating point: do not fold under integer arithmetic rules
            if (is_floating_point_type(inst->type)) {
                res.skipped_reasons.push_back("Skipped floating-point instruction '" + inst->to_ptx() + "' (FP rounding/flush rules must be preserved)");
                continue;
            }

            // Both sources must be immediate integer constants
            if (src1.kind != Operand::Kind::IMMEDIATE_INT || src2.kind != Operand::Kind::IMMEDIATE_INT) {
                continue;
            }

            int64_t c1 = src1.int_val;
            int64_t c2 = src2.int_val;
            int64_t computed = 0;
            std::string op_symbol;

            if (inst->opcode == Opcode::ADD) {
                computed = c1 + c2;
                op_symbol = "+";
            } else if (inst->opcode == Opcode::SUB) {
                computed = c1 - c2;
                op_symbol = "-";
            } else if (inst->opcode == Opcode::MUL) {
                computed = c1 * c2;
                op_symbol = "*";
            }

            computed = truncate_to_type(computed, inst->type);

            std::string old_ptx = inst->to_ptx();

            // Transform into mov.[type] dest, computed
            inst->opcode = Opcode::MOV;
            inst->opcode_str = "mov";
            inst->modifiers.clear();
            inst->operands = { dest, Operand::make_int(computed, inst->type) };

            res.modified = true;
            res.num_transformations++;
            std::ostringstream exp;
            exp << "Constant fold: '" << old_ptx << "' -> '" << inst->to_ptx()
                << "' (" << c1 << " " << op_symbol << " " << c2 << " = " << computed << ")";
            res.explanations.push_back(exp.str());
        }
    }

    return res;
}

} // namespace ptxscope
