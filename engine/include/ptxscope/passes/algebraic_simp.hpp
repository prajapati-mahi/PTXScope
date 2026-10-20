#pragma once

#include "ptxscope/optimizer.hpp"

namespace ptxscope {

class AlgebraicSimplificationPass : public Pass {
public:
    std::string name() const override { return "algebraic-simp"; }
    PassResult run_on_function(Function& fn) override;
};

} // namespace ptxscope
