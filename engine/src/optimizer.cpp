#include "ptxscope/optimizer.hpp"
#include "ptxscope/passes/const_fold.hpp"
#include "ptxscope/passes/algebraic_simp.hpp"
#include "ptxscope/passes/dce.hpp"
#include "ptxscope/passes/redundant_mov.hpp"
#include <sstream>

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

std::string PassResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"pass_name\": \"" << escape_json(pass_name) << "\",\n"
       << "  \"modified\": " << (modified ? "true" : "false") << ",\n"
       << "  \"transformations\": " << num_transformations << ",\n"
       << "  \"explanations\": [\n";
    for (size_t i = 0; i < explanations.size(); ++i) {
        ss << "    \"" << escape_json(explanations[i]) << "\"";
        if (i + 1 < explanations.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n"
       << "  \"skipped_reasons\": [\n";
    for (size_t i = 0; i < skipped_reasons.size(); ++i) {
        ss << "    \"" << escape_json(skipped_reasons[i]) << "\"";
        if (i + 1 < skipped_reasons.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n"
       << "}";
    return ss.str();
}

std::vector<PassResult> PassManager::run_on_function(Function& fn) {
    std::vector<PassResult> results;
    for (const auto& pass : passes_) {
        results.push_back(pass->run_on_function(fn));
    }
    return results;
}

std::vector<PassResult> PassManager::run(Module& mod) {
    std::vector<PassResult> combined;
    for (const auto& pass : passes_) {
        PassResult pr;
        pr.pass_name = pass->name();

        for (auto& fn : mod.functions) {
            PassResult fn_res = pass->run_on_function(*fn);
            if (fn_res.modified) pr.modified = true;
            pr.num_transformations += fn_res.num_transformations;
            pr.explanations.insert(pr.explanations.end(), fn_res.explanations.begin(), fn_res.explanations.end());
            pr.skipped_reasons.insert(pr.skipped_reasons.end(), fn_res.skipped_reasons.begin(), fn_res.skipped_reasons.end());
        }

        combined.push_back(std::move(pr));
    }
    return combined;
}

std::shared_ptr<PassManager> PassManager::create_standard_pipeline(const std::vector<std::string>& pass_names) {
    auto pm = std::make_shared<PassManager>();

    if (pass_names.empty()) {
        pm->add_pass(std::make_shared<ConstantFoldingPass>());
        pm->add_pass(std::make_shared<AlgebraicSimplificationPass>());
        pm->add_pass(std::make_shared<RedundantMovePass>());
        pm->add_pass(std::make_shared<DeadCodeEliminationPass>());
        return pm;
    }

    for (const auto& p : pass_names) {
        if (p == "const-fold" || p == "constant_folding") {
            pm->add_pass(std::make_shared<ConstantFoldingPass>());
        } else if (p == "algebraic-simp" || p == "algebraic_simplification") {
            pm->add_pass(std::make_shared<AlgebraicSimplificationPass>());
        } else if (p == "redundant-mov" || p == "redundant_move") {
            pm->add_pass(std::make_shared<RedundantMovePass>());
        } else if (p == "dce" || p == "dead_code_elimination") {
            pm->add_pass(std::make_shared<DeadCodeEliminationPass>());
        }
    }

    return pm;
}

} // namespace ptxscope
