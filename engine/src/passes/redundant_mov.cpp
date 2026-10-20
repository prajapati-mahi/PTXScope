#include "ptxscope/passes/redundant_mov.hpp"
#include <unordered_map>
#include <vector>

namespace ptxscope {

PassResult RedundantMovePass::run_on_function(Function& fn) {
    PassResult res;
    res.pass_name = name();

    for (auto& bb : fn.blocks) {
        std::vector<std::shared_ptr<Instruction>> retained;
        std::unordered_map<std::string, std::string> copy_map; // dst -> src

        for (auto& inst : bb->instructions) {
            // Check self-move: mov %r1, %r1
            if (inst->opcode == Opcode::MOV && inst->operands.size() >= 2 && !inst->predicate.is_predicated) {
                const auto& dst = inst->operands[0];
                const auto& src = inst->operands[1];

                if (dst.kind == Operand::Kind::REGISTER && src.kind == Operand::Kind::REGISTER) {
                    if (dst.reg_name == src.reg_name) {
                        res.modified = true;
                        res.num_transformations++;
                        res.explanations.push_back("Eliminated self-move: '" + inst->to_ptx() + "'");
                        continue; // drop this instruction
                    }

                    // Check if dst is already mapped to src
                    auto it = copy_map.find(dst.reg_name);
                    if (it != copy_map.end() && it->second == src.reg_name) {
                        res.modified = true;
                        res.num_transformations++;
                        res.explanations.push_back("Eliminated redundant duplicate move: '" + inst->to_ptx() + "'");
                        continue;
                    }

                    // Record new copy mapping
                    copy_map[dst.reg_name] = src.reg_name;
                }
            } else {
                // Invalidate any copies if register is redefined
                auto def = inst->get_def_reg();
                if (def) {
                    copy_map.erase(*def);
                    // Also erase any keys mapping to *def
                    for (auto it = copy_map.begin(); it != copy_map.end(); ) {
                        if (it->second == *def) {
                            it = copy_map.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }

            retained.push_back(inst);
        }
        bb->instructions = std::move(retained);
    }

    return res;
}

} // namespace ptxscope
