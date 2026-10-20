#include "ptxscope/passes/dce.hpp"
#include "ptxscope/cfg.hpp"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace ptxscope {

PassResult DeadCodeEliminationPass::run_on_function(Function& fn) {
    PassResult res;
    res.pass_name = name();

    // Pass 1: Eliminate Unreachable Basic Blocks
    ControlFlowGraph cfg(std::make_shared<Function>(fn));
    cfg.build();
    const auto& unreachable = cfg.get_unreachable_blocks();
    std::unordered_set<std::string> unreach_set(unreachable.begin(), unreachable.end());

    if (!unreach_set.empty()) {
        auto orig_blocks = std::move(fn.blocks);
        fn.blocks.clear();
        for (auto& bb : orig_blocks) {
            if (unreach_set.find(bb->label) != unreach_set.end()) {
                res.modified = true;
                res.num_transformations++;
                res.explanations.push_back("Removed unreachable basic block '" + bb->label +
                                           "' (" + std::to_string(bb->instructions.size()) + " dead instructions)");
            } else {
                fn.blocks.push_back(std::move(bb));
            }
        }
    }

    // Pass 2: Eliminate Dead Register Assignments (Iterative)
    bool local_changed = true;
    while (local_changed) {
        local_changed = false;

        // Compute register use counts across all blocks
        std::unordered_map<std::string, int> use_counts;
        for (const auto& bb : fn.blocks) {
            for (const auto& inst : bb->instructions) {
                for (const auto& u : inst->get_use_regs()) {
                    use_counts[u]++;
                }
            }
        }

        for (auto& bb : fn.blocks) {
            std::vector<std::shared_ptr<Instruction>> retained_instructions;
            for (auto& inst : bb->instructions) {
                auto def = inst->get_def_reg();

                // If instruction defines a register
                if (def && !inst->has_side_effects() && !inst->predicate.is_predicated) {
                    int uses = use_counts[*def];
                    if (uses == 0) {
                        res.modified = true;
                        res.num_transformations++;
                        res.explanations.push_back("Dead instruction eliminated: '" + inst->to_ptx() +
                                                   "' (register " + *def + " is never read)");
                        local_changed = true;
                        continue; // skip adding to retained_instructions
                    }
                }

                retained_instructions.push_back(inst);
            }
            bb->instructions = std::move(retained_instructions);
        }
    }

    return res;
}

} // namespace ptxscope
