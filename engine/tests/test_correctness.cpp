#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"
#include "ptxscope/optimizer.hpp"
#include "ptxscope/passes/const_fold.hpp"

using namespace ptxscope;

TEST_CASE(CorrectnessTest, FloatPreservation) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_fp() {\n"
        "  .reg .f32 %f1, %f2, %f3;\n"
        "  add.f32 %f1, 1.0, 2.0;\n" // FP add must NOT be folded by integer rules
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    ConstantFoldingPass pass;
    auto res = pass.run_on_function(*mod->functions.front());

    EXPECT_FALSE(res.modified); // Not modified!
    EXPECT_FALSE(res.skipped_reasons.empty()); // Reason recorded
}

TEST_CASE(CorrectnessTest, PipelineOptimizationChain) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_chain() {\n"
        "  .reg .u32 %r0, %r1, %r2, %r3, %r4;\n"
        "  .reg .u64 %rd1;\n"
        "  add.u32 %r1, 10, 20;\n"      // const fold -> mov %r1, 30
        "  add.u32 %r2, %r1, 0;\n"       // algebraic simp -> mov %r2, %r1
        "  mov.u32 %r3, %r3;\n"          // redundant mov -> eliminated
        "  mov.u32 %r4, 999;\n"          // dead instruction -> eliminated
        "  st.global.u32 [%rd1], %r2;\n" // store output
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    auto pm = PassManager::create_standard_pipeline();
    auto results = pm->run(*mod);

    int total_transformations = 0;
    for (const auto& r : results) {
        total_transformations += r.num_transformations;
    }
    EXPECT_TRUE(total_transformations >= 4);

    // Verify side effect store still exists and ret still exists
    auto& insts = mod->functions.front()->blocks.front()->instructions;
    bool store_found = false;
    bool ret_found = false;
    for (const auto& i : insts) {
        if (i->opcode == Opcode::ST) store_found = true;
        if (i->opcode == Opcode::RET) ret_found = true;
    }
    EXPECT_TRUE(store_found);
    EXPECT_TRUE(ret_found);
}
