#pragma once

#include "ptxscope/optimizer.hpp"

namespace ptxscope {

class ConstantFoldingPass : public Pass {
public:
    std::string name() const override { return "const-fold"; }
    PassResult run_on_function(Function& fn) override;
};

} // namespace ptxscope
