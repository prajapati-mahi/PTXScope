#pragma once

#include "ptxscope/ir.hpp"
#include "ptxscope/cfg.hpp"
#include "ptxscope/diagnostics.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace ptxscope {

struct RegisterUsage {
    std::string name;
    int def_count = 0;
    int use_count = 0;
    std::vector<SourceLocation> def_locations;
    std::vector<SourceLocation> use_locations;
};

struct BlockLiveness {
    std::string block_label;
    std::unordered_set<std::string> gen;
    std::unordered_set<std::string> kill;
    std::unordered_set<std::string> live_in;
    std::unordered_set<std::string> live_out;
};

class StaticAnalyzer {
public:
    explicit StaticAnalyzer(std::shared_ptr<Function> fn, DiagnosticEngine& diag);

    void run_analysis();

    const std::unordered_map<std::string, RegisterUsage>& get_reg_usages() const { return reg_usages_; }
    const std::unordered_map<std::string, BlockLiveness>& get_liveness() const { return liveness_; }

    std::string to_json() const;

private:
    void compute_def_use();
    void compute_liveness();
    void run_diagnostic_rules();

    std::shared_ptr<Function> fn_;
    DiagnosticEngine& diag_;
    std::unordered_map<std::string, RegisterUsage> reg_usages_;
    std::unordered_map<std::string, BlockLiveness> liveness_;
    std::unordered_set<std::string> declared_registers_;
};

} // namespace ptxscope
