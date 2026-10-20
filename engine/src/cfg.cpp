#include "ptxscope/cfg.hpp"
#include <sstream>
#include <queue>
#include <unordered_map>

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

} // namespace

ControlFlowGraph::ControlFlowGraph(std::shared_ptr<Function> fn) : fn_(std::move(fn)) {}

void ControlFlowGraph::build() {
    edges_.clear();
    if (!fn_) return;

    // Reset predecessor and successor lists in all blocks
    for (auto& bb : fn_->blocks) {
        bb->predecessors.clear();
        bb->successors.clear();
    }

    std::unordered_set<std::string> known_blocks;
    for (const auto& bb : fn_->blocks) {
        known_blocks.insert(bb->label);
    }

    for (size_t i = 0; i < fn_->blocks.size(); ++i) {
        auto& bb = fn_->blocks[i];
        bool has_unconditional_term = false;

        for (const auto& inst : bb->instructions) {
            if (inst->is_branch()) {
                auto target = inst->branch_target();
                if (target && known_blocks.find(*target) != known_blocks.end()) {
                    EdgeType etype = inst->is_conditional_branch()
                        ? EdgeType::CONDITIONAL_TRUE
                        : EdgeType::UNCONDITIONAL_BRANCH;

                    edges_.push_back({bb->label, *target, etype, inst->predicate.to_string()});
                    bb->successors.push_back(*target);

                    auto target_bb = fn_->find_block(*target);
                    if (target_bb) {
                        target_bb->predecessors.push_back(bb->label);
                    }
                }

                if (!inst->is_conditional_branch()) {
                    has_unconditional_term = true;
                    break;
                }
            } else if (inst->opcode == Opcode::RET || inst->opcode == Opcode::EXIT) {
                if (!inst->predicate.is_predicated) {
                    has_unconditional_term = true;
                    break;
                }
            }
        }

        // If the block does not end with an unconditional terminator, it falls through to the next sequential block
        if (!has_unconditional_term && i + 1 < fn_->blocks.size()) {
            const std::string& next_lbl = fn_->blocks[i + 1]->label;
            edges_.push_back({bb->label, next_lbl, EdgeType::FALLTHROUGH, ""});
            bb->successors.push_back(next_lbl);
            fn_->blocks[i + 1]->predecessors.push_back(bb->label);
        }
    }

    analyze_reachability();
}

void ControlFlowGraph::analyze_reachability() {
    reachable_blocks_.clear();
    unreachable_blocks_.clear();
    if (!fn_ || fn_->blocks.empty()) return;

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    std::string entry_label = fn_->blocks.front()->label;
    q.push(entry_label);
    visited.insert(entry_label);

    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();

        auto bb = fn_->find_block(curr);
        if (!bb) continue;

        for (const auto& succ : bb->successors) {
            if (visited.find(succ) == visited.end()) {
                visited.insert(succ);
                q.push(succ);
            }
        }
    }

    for (const auto& bb : fn_->blocks) {
        if (visited.find(bb->label) != visited.end()) {
            reachable_blocks_.push_back(bb->label);
        } else {
            unreachable_blocks_.push_back(bb->label);
        }
    }
}

std::string ControlFlowGraph::to_dot() const {
    std::ostringstream ss;
    ss << "digraph \"" << (fn_ ? fn_->name : "PTX_CFG") << "\" {\n";
    ss << "  node [shape=box, fontname=\"Courier\", style=\"filled,rounded\", fillcolor=\"#1e293b\", fontcolor=\"#f8fafc\", color=\"#475569\"];\n";
    ss << "  edge [fontname=\"Courier\", color=\"#94a3b8\", fontcolor=\"#cbd5e1\"];\n";

    if (!fn_) {
        ss << "}\n";
        return ss.str();
    }

    for (const auto& bb : fn_->blocks) {
        bool is_unreachable = false;
        for (const auto& unreach : unreachable_blocks_) {
            if (unreach == bb->label) {
                is_unreachable = true;
                break;
            }
        }

        std::string fillcolor = is_unreachable ? "#450a0a" : "#1e293b";
        std::string bordercolor = is_unreachable ? "#ef4444" : "#3b82f6";

        ss << "  \"" << bb->label << "\" [label=\"" << bb->label << "\\n";
        for (const auto& inst : bb->instructions) {
            std::string line = inst->to_ptx();
            // clean up tabs
            for (char& c : line) {
                if (c == '\t') c = ' ';
                if (c == '"') c = '\'';
            }
            ss << line << "\\l";
        }
        ss << "\", fillcolor=\"" << fillcolor << "\", color=\"" << bordercolor << "\"];\n";
    }

    for (const auto& edge : edges_) {
        ss << "  \"" << edge.source << "\" -> \"" << edge.target << "\"";
        if (edge.type == EdgeType::CONDITIONAL_TRUE) {
            ss << " [label=\"" << (edge.condition.empty() ? "true" : edge.condition) << "\", color=\"#22c55e\", fontcolor=\"#86efac\"];\n";
        } else if (edge.type == EdgeType::FALLTHROUGH) {
            ss << " [label=\"fallthrough\", style=\"dashed\", color=\"#64748b\"];\n";
        } else {
            ss << " [color=\"#3b82f6\"];\n";
        }
    }

    ss << "}\n";
    return ss.str();
}

std::string ControlFlowGraph::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"function\": \"" << (fn_ ? escape_json(fn_->name) : "") << "\",\n";
    ss << "  \"nodes\": [\n";

    if (fn_) {
        for (size_t i = 0; i < fn_->blocks.size(); ++i) {
            const auto& bb = fn_->blocks[i];
            bool is_unreachable = false;
            for (const auto& unreach : unreachable_blocks_) {
                if (unreach == bb->label) {
                    is_unreachable = true;
                    break;
                }
            }

            ss << "    {\n";
            ss << "      \"id\": \"" << escape_json(bb->label) << "\",\n";
            ss << "      \"label\": \"" << escape_json(bb->label) << "\",\n";
            ss << "      \"is_entry\": " << (i == 0 ? "true" : "false") << ",\n";
            ss << "      \"is_unreachable\": " << (is_unreachable ? "true" : "false") << ",\n";
            ss << "      \"instruction_count\": " << bb->instructions.size() << ",\n";
            ss << "      \"instructions\": [\n";
            for (size_t j = 0; j < bb->instructions.size(); ++j) {
                ss << "        \"" << escape_json(bb->instructions[j]->to_ptx()) << "\"";
                if (j + 1 < bb->instructions.size()) ss << ",";
                ss << "\n";
            }
            ss << "      ],\n";
            ss << "      \"predecessors\": [";
            for (size_t j = 0; j < bb->predecessors.size(); ++j) {
                ss << "\"" << escape_json(bb->predecessors[j]) << "\"";
                if (j + 1 < bb->predecessors.size()) ss << ", ";
            }
            ss << "],\n";
            ss << "      \"successors\": [";
            for (size_t j = 0; j < bb->successors.size(); ++j) {
                ss << "\"" << escape_json(bb->successors[j]) << "\"";
                if (j + 1 < bb->successors.size()) ss << ", ";
            }
            ss << "]\n";
            ss << "    }";
            if (i + 1 < fn_->blocks.size()) ss << ",";
            ss << "\n";
        }
    }

    ss << "  ],\n";
    ss << "  \"edges\": [\n";
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& e = edges_[i];
        ss << "    {\n";
        ss << "      \"id\": \"e" << i << "\",\n";
        ss << "      \"source\": \"" << escape_json(e.source) << "\",\n";
        ss << "      \"target\": \"" << escape_json(e.target) << "\",\n";
        ss << "      \"type\": \"" << edge_type_to_string(e.type) << "\",\n";
        ss << "      \"condition\": \"" << escape_json(e.condition) << "\"\n";
        ss << "    }";
        if (i + 1 < edges_.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"reachable_blocks\": [";
    for (size_t i = 0; i < reachable_blocks_.size(); ++i) {
        ss << "\"" << escape_json(reachable_blocks_[i]) << "\"";
        if (i + 1 < reachable_blocks_.size()) ss << ", ";
    }
    ss << "],\n";
    ss << "  \"unreachable_blocks\": [";
    for (size_t i = 0; i < unreachable_blocks_.size(); ++i) {
        ss << "\"" << escape_json(unreachable_blocks_[i]) << "\"";
        if (i + 1 < unreachable_blocks_.size()) ss << ", ";
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

} // namespace ptxscope
