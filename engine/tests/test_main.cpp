#include "test_framework.hpp"

int main() {
    return ptxscope::testing::TestRegistry::instance().run_all();
}
