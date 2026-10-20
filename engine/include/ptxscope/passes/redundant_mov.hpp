#pragma once

#include "ptxscope/optimizer.hpp"

namespace ptxscope {

class RedundantMovePass : public Pass {
public:
    std::string name() const override { return "redundant-mov"; }
    PassResult run_on_function(Function& fn) override;
};

} // namespace ptxscope
