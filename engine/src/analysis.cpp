#include "ptxscope/analysis.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace ptxscope {

namespace {

std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

bool is_builtin_register(const std::string& name) {
    static const std::unordered_set<std::string> builtins = {
        "%tid.x", "%tid.y", "%tid.z",
        "%ntid.x", "%ntid.y", "%ntid.z",
        "%ctaid.x", "%ctaid.y", "%ctaid.z",
        "%nctaid.x", "%nctaid.y", "%nctaid.z",
        "%laneid", "%warpid", "%clock", "%clock64"
    };
    return builtins.find(name) != builtins.end();
}

} // namespace

StaticAnalyzer::StaticAnalyzer(std::shared_ptr<Function> fn, DiagnosticEngine& diag)
    : fn_(std::move(fn)), diag_(diag) {}

void StaticAnalyzer::run_analysis() {
    if (!fn_) return;

    // Parse declared registers from fn_->reg_declarations
    declared_registers_.clear();
    for (const auto& decl : fn_->reg_declarations) {
        // e.g. .reg .u32 %r<10>; or .reg .u32 %r1, %r2;
        std::regex reg_range(R"(%([a-zA-Z0-9_]+)<(\d+)>)");
        std::smatch match;
        std::string s = decl;
        while (std::regex_search(s, match, reg_range)) {
            std::string prefix = match[1].str();
            int count = std::stoi(match[2].str());
            for (int i = 0; i < count; ++i) {
                declared_registers_.insert("%" + prefix + std::to_string(i));
            }
            s = match.suffix().str();
        }

        std::regex single_reg(R"(%([a-zA-Z0-9_\.]+))");
        s = decl;
        while (std::regex_search(s, match, single_reg)) {
            std::string full_name = "%" + match[1].str();
            if (full_name.find('<') == std::string::npos) {
                declared_registers_.insert(full_name);
            }
            s = match.suffix().str();
        }
    }

    // Also parse kernel parameters into declared symbols
    for (const auto& param : fn_->params) {
        std::stringstream pss(param);
        std::string tok;
        std::string param_name;
        while (pss >> tok) {
            param_name = tok;
        }
        if (!param_name.empty()) {
            declared_registers_.insert(param_name);
            declared_registers_.insert("%" + param_name);
        }
    }

    compute_def_use();
    compute_liveness();
    run_diagnostic_rules();
}

void StaticAnalyzer::compute_def_use() {
    reg_usages_.clear();

    for (const auto& bb : fn_->blocks) {
        for (const auto& inst : bb->instructions) {
            auto def_reg = inst->get_def_reg();
            if (def_reg) {
                auto& u = reg_usages_[*def_reg];
                u.name = *def_reg;
                u.def_count++;
                u.def_locations.push_back(inst->span.start);
            }

            auto use_regs = inst->get_use_regs();
            for (const auto& ur : use_regs) {
                auto& u = reg_usages_[ur];
                u.name = ur;
                u.use_count++;
                u.use_locations.push_back(inst->span.start);
            }
        }
    }
}

void StaticAnalyzer::compute_liveness() {
    liveness_.clear();
    if (fn_->blocks.empty()) return;

    // Step 1: Compute Gen and Kill per block
    for (const auto& bb : fn_->blocks) {
        BlockLiveness bl;
        bl.block_label = bb->label;

        for (const auto& inst : bb->instructions) {
            auto uses = inst->get_use_regs();
            for (const auto& u : uses) {
                // If not defined earlier in this block, it is in gen
                if (bl.kill.find(u) == bl.kill.end()) {
                    bl.gen.insert(u);
                }
            }

            auto def = inst->get_def_reg();
            if (def) {
                bl.kill.insert(*def);
            }
        }
        liveness_[bb->label] = std::move(bl);
    }

    // Step 2: Fixed-point iteration (backward analysis)
    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;

    while (changed && iterations++ < max_iterations) {
        changed = false;

        // Iterate in reverse topological / reverse order
        for (auto it = fn_->blocks.rbegin(); it != fn_->blocks.rend(); ++it) {
            const auto& bb = *it;
            auto& curr_live = liveness_[bb->label];

            // LiveOut[B] = union over succ S of LiveIn[S]
            std::unordered_set<std::string> new_out;
            for (const auto& succ_lbl : bb->successors) {
                auto succ_it = liveness_.find(succ_lbl);
                if (succ_it != liveness_.end()) {
                    new_out.insert(succ_it->second.live_in.begin(), succ_it->second.live_in.end());
                }
            }

            if (new_out != curr_live.live_out) {
                curr_live.live_out = std::move(new_out);
                changed = true;
            }

            // LiveIn[B] = Gen[B] union (LiveOut[B] \ Kill[B])
            std::unordered_set<std::string> new_in = curr_live.gen;
            for (const auto& r : curr_live.live_out) {
                if (curr_live.kill.find(r) == curr_live.kill.end()) {
                    new_in.insert(r);
                }
            }

            if (new_in != curr_live.live_in) {
                curr_live.live_in = std::move(new_in);
                changed = true;
            }
        }
    }
}

void StaticAnalyzer::run_diagnostic_rules() {
    // Rule 1: Unused register definitions
    for (const auto& [reg, usage] : reg_usages_) {
        if (usage.def_count > 0 && usage.use_count == 0 && !is_builtin_register(reg)) {
            SourceLocation loc = usage.def_locations.empty() ? SourceLocation{1, 1} : usage.def_locations.front();
            diag_.report_warning(
                "W001_UNUSED_REGISTER",
                "Register " + reg + " is assigned a value but never read in function '" + fn_->name + "'.",
                loc,
                "Dead assignments consume register space without contributing to program outputs.",
                "Consider removing the dead computation or enabling Dead Code Elimination (DCE)."
            );
        }
    }

    // Rule 2: Unreachable basic blocks
    ControlFlowGraph cfg(fn_);
    cfg.build();
    for (const auto& unreach_label : cfg.get_unreachable_blocks()) {
        auto bb = fn_->find_block(unreach_label);
        SourceLocation loc = (bb && !bb->instructions.empty())
            ? bb->instructions.front()->span.start
            : SourceLocation{1, 1};

        diag_.report_warning(
            "W002_UNREACHABLE_CODE",
            "Basic block '" + unreach_label + "' is unreachable from entry point.",
            loc,
            "Instructions in unreachable blocks will never execute at runtime and waste instruction cache space.",
            "Remove unreachable code or verify branch targets."
        );
    }

    // Rule 3: Redundant moves (mov %r1, %r1)
    for (const auto& bb : fn_->blocks) {
        for (const auto& inst : bb->instructions) {
            if (inst->opcode == Opcode::MOV && inst->operands.size() >= 2) {
                if (inst->operands[0].kind == Operand::Kind::REGISTER &&
                    inst->operands[1].kind == Operand::Kind::REGISTER &&
                    inst->operands[0].reg_name == inst->operands[1].reg_name) {
                    diag_.report_suggestion(
                        "S001_REDUNDANT_MOV",
                        "Redundant self-move '" + inst->to_ptx() + "' has no functional effect.",
                        inst->span.start,
                        "Moving a register to itself is a no-op.",
                        "Eliminate this move instruction."
                    );
                }
            }
        }
    }

    // Rule 4: Invalid branch targets
    std::unordered_set<std::string> existing_blocks;
    for (const auto& bb : fn_->blocks) {
        existing_blocks.insert(bb->label);
    }

    for (const auto& bb : fn_->blocks) {
        for (const auto& inst : bb->instructions) {
            if (inst->is_branch()) {
                auto target = inst->branch_target();
                if (target && existing_blocks.find(*target) == existing_blocks.end()) {
                    diag_.report_error(
                        "W004_BRANCH_INVALID_LABEL",
                        "Branch targets non-existent label '" + *target + "'.",
                        inst->span.start,
                        "Branching to an undefined label will fail during compilation/assembly.",
                        "Define the missing label or update the branch instruction."
                    );
                }
            }
        }
    }

    // Rule 5: Undeclared registers
    if (!declared_registers_.empty()) {
        for (const auto& [reg, usage] : reg_usages_) {
            if (!is_builtin_register(reg) && declared_registers_.find(reg) == declared_registers_.end()) {
                SourceLocation loc = usage.use_locations.empty()
                    ? (usage.def_locations.empty() ? SourceLocation{1, 1} : usage.def_locations.front())
                    : usage.use_locations.front();

                diag_.report_warning(
                    "W005_UNDECLARED_REG",
                    "Register " + reg + " used without an explicit .reg declaration.",
                    loc,
                    "PTX requires registers to be declared within the function body.",
                    "Add '.reg <type> " + reg + ";' to function header."
                );
            }
        }
    }

    // Rule 6: Long dependency chains (heuristic ILP warning)
    for (const auto& bb : fn_->blocks) {
        int chain_depth = 0;
        std::string last_def;
        for (const auto& inst : bb->instructions) {
            auto uses = inst->get_use_regs();
            bool uses_last = false;
            for (const auto& u : uses) {
                if (!last_def.empty() && u == last_def) {
                    uses_last = true;
                    break;
                }
            }
            if (uses_last) {
                chain_depth++;
            } else {
                chain_depth = 1;
            }

            auto def = inst->get_def_reg();
            if (def) {
                last_def = *def;
            } else {
                last_def.clear();
            }

            if (chain_depth >= 8) {
                diag_.report_suggestion(
                    "W003_LONG_DEP_CHAIN",
                    "Consecutive RAW dependency chain of depth " + std::to_string(chain_depth) +
                    " detected in block '" + bb->label + "'.",
                    inst->span.start,
                    "Long back-to-back dependency chains can increase pipeline latency stalls on GPU Streaming Multiprocessors (SMs).",
                    "Consider unrolling or interleaving independent operations to improve instruction-level parallelism (ILP)."
                );
                chain_depth = 0; // reset to avoid duplicate alerts on the same block
            }
        }
    }
}

std::string StaticAnalyzer::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"function\": \"" << (fn_ ? escape_json(fn_->name) : "") << "\",\n";
    ss << "  \"registers\": [\n";

    std::vector<std::string> sorted_regs;
    for (const auto& [reg, _] : reg_usages_) {
        sorted_regs.push_back(reg);
    }
    std::sort(sorted_regs.begin(), sorted_regs.end());

    for (size_t i = 0; i < sorted_regs.size(); ++i) {
        const auto& r = sorted_regs[i];
        const auto& u = reg_usages_.at(r);
        ss << "    {\n";
        ss << "      \"name\": \"" << escape_json(u.name) << "\",\n";
        ss << "      \"def_count\": " << u.def_count << ",\n";
        ss << "      \"use_count\": " << u.use_count << ",\n";
        ss << "      \"is_builtin\": " << (is_builtin_register(u.name) ? "true" : "false") << ",\n";
        ss << "      \"def_lines\": [";
        for (size_t j = 0; j < u.def_locations.size(); ++j) {
            ss << u.def_locations[j].line;
            if (j + 1 < u.def_locations.size()) ss << ", ";
        }
        ss << "],\n";
        ss << "      \"use_lines\": [";
        for (size_t j = 0; j < u.use_locations.size(); ++j) {
            ss << u.use_locations[j].line;
            if (j + 1 < u.use_locations.size()) ss << ", ";
        }
        ss << "]\n";
        ss << "    }";
        if (i + 1 < sorted_regs.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"liveness\": [\n";
    size_t bb_idx = 0;
    for (const auto& [lbl, bl] : liveness_) {
        ss << "    {\n";
        ss << "      \"block\": \"" << escape_json(lbl) << "\",\n";
        ss << "      \"live_in\": [";
        size_t k = 0;
        for (const auto& reg : bl.live_in) {
            ss << "\"" << escape_json(reg) << "\"";
            if (++k < bl.live_in.size()) ss << ", ";
        }
        ss << "],\n";
        ss << "      \"live_out\": [";
        k = 0;
        for (const auto& reg : bl.live_out) {
            ss << "\"" << escape_json(reg) << "\"";
            if (++k < bl.live_out.size()) ss << ", ";
        }
        ss << "]\n";
        ss << "    }";
        if (++bb_idx < liveness_.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

} // namespace ptxscope
