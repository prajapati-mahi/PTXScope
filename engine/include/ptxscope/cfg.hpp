#pragma once

#include "ptxscope/ir.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

namespace ptxscope {

enum class EdgeType {
    FALLTHROUGH,
    UNCONDITIONAL_BRANCH,
    CONDITIONAL_TRUE,
    CONDITIONAL_FALSE
};

inline std::string edge_type_to_string(EdgeType et) {
    switch (et) {
        case EdgeType::FALLTHROUGH:          return "fallthrough";
        case EdgeType::UNCONDITIONAL_BRANCH: return "unconditional";
        case EdgeType::CONDITIONAL_TRUE:     return "branch_true";
        case EdgeType::CONDITIONAL_FALSE:    return "branch_false";
    }
    return "unknown";
}

struct CFGEdge {
    std::string source;
    std::string target;
    EdgeType type;
    std::string condition; // e.g. "@p0" or "@!p1"
};

class ControlFlowGraph {
public:
    explicit ControlFlowGraph(std::shared_ptr<Function> fn);

    void build();
    void analyze_reachability();

    const std::vector<std::string>& get_reachable_blocks() const { return reachable_blocks_; }
    const std::vector<std::string>& get_unreachable_blocks() const { return unreachable_blocks_; }
    const std::vector<CFGEdge>& get_edges() const { return edges_; }

    std::string to_dot() const;
    std::string to_json() const;

private:
    std::shared_ptr<Function> fn_;
    std::vector<CFGEdge> edges_;
    std::vector<std::string> reachable_blocks_;
    std::vector<std::string> unreachable_blocks_;
};

} // namespace ptxscope
