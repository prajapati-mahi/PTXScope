#pragma once

#include "ptxscope/ir.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ptxscope {

struct PassResult {
    std::string pass_name;
    bool modified = false;
    int num_transformations = 0;
    std::vector<std::string> explanations;
    std::vector<std::string> skipped_reasons;

    std::string to_json() const;
};

class Pass {
public:
    virtual ~Pass() = default;
    virtual std::string name() const = 0;
    virtual PassResult run_on_function(Function& fn) = 0;
};

class PassManager {
public:
    void add_pass(std::shared_ptr<Pass> pass) {
        passes_.push_back(std::move(pass));
    }

    std::vector<PassResult> run(Module& mod);
    std::vector<PassResult> run_on_function(Function& fn);

    static std::shared_ptr<PassManager> create_standard_pipeline(const std::vector<std::string>& pass_names = {});

private:
    std::vector<std::shared_ptr<Pass>> passes_;
};

} // namespace ptxscope
