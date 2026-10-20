#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"
#include "ptxscope/cfg.hpp"

using namespace ptxscope;

TEST_CASE(CFGTest, BranchEdgesAndReachability) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_cfg() {\n"
        "  .reg .pred %p0;\n"
        "  .reg .u32 %r1;\n"
        "entry_block:\n"
        "  @p0 bra target_block;\n"
        "  mov.u32 %r1, 1;\n"
        "  bra exit_block;\n"
        "target_block:\n"
        "  mov.u32 %r1, 2;\n"
        "exit_block:\n"
        "  ret;\n"
        "dead_block:\n"
        "  mov.u32 %r1, 99;\n"
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    EXPECT_FALSE(mod->functions.empty());
    auto fn = mod->functions.front();

    ControlFlowGraph cfg(fn);
    cfg.build();

    EXPECT_FALSE(cfg.get_edges().empty());
    const auto& reachable = cfg.get_reachable_blocks();
    const auto& unreachable = cfg.get_unreachable_blocks();

    EXPECT_TRUE(reachable.size() >= 3);
    EXPECT_EQ(unreachable.size(), 1);
    EXPECT_EQ(unreachable[0], "dead_block");

    std::string dot = cfg.to_dot();
    EXPECT_TRUE(dot.find("digraph") != std::string::npos);

    std::string json = cfg.to_json();
    EXPECT_TRUE(json.find("\"nodes\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"edges\"") != std::string::npos);
}
