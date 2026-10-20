#pragma once

#include "ptxscope/optimizer.hpp"

namespace ptxscope {

class DeadCodeEliminationPass : public Pass {
public:
    std::string name() const override { return "dce"; }
    PassResult run_on_function(Function& fn) override;
};

} // namespace ptxscope
