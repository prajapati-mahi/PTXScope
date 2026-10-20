#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"
#include "ptxscope/optimizer.hpp"
#include "ptxscope/passes/const_fold.hpp"
#include "ptxscope/passes/algebraic_simp.hpp"
#include "ptxscope/passes/dce.hpp"
#include "ptxscope/passes/redundant_mov.hpp"

using namespace ptxscope;

TEST_CASE(OptimizerTest, ConstantFolding) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_cf() {\n"
        "  .reg .u32 %r1, %r2;\n"
        "  add.u32 %r1, 15, 25;\n"
        "  mul.lo.u32 %r2, 6, 7;\n"
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    ConstantFoldingPass pass;
    auto res = pass.run_on_function(*mod->functions.front());

    EXPECT_TRUE(res.modified);
    EXPECT_EQ(res.num_transformations, 2);

    auto& insts = mod->functions.front()->blocks.front()->instructions;
    EXPECT_EQ(insts[0]->opcode, Opcode::MOV);
    EXPECT_EQ(insts[0]->operands[1].int_val, 40);
    EXPECT_EQ(insts[1]->opcode, Opcode::MOV);
    EXPECT_EQ(insts[1]->operands[1].int_val, 42);
}

TEST_CASE(OptimizerTest, AlgebraicSimplification) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_alg() {\n"
        "  .reg .u32 %r0, %r1, %r2, %r3;\n"
        "  add.u32 %r1, %r0, 0;\n"
        "  mul.lo.u32 %r2, %r0, 1;\n"
        "  mul.lo.u32 %r3, %r0, 0;\n"
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    AlgebraicSimplificationPass pass;
    auto res = pass.run_on_function(*mod->functions.front());

    EXPECT_TRUE(res.modified);
    EXPECT_EQ(res.num_transformations, 3);
}

TEST_CASE(OptimizerTest, RedundantMoveElimination) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_mov() {\n"
        "  .reg .u32 %r1;\n"
        "  mov.u32 %r1, %r1;\n" // self-move
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    RedundantMovePass pass;
    auto res = pass.run_on_function(*mod->functions.front());

    EXPECT_TRUE(res.modified);
    EXPECT_EQ(res.num_transformations, 1);
    EXPECT_EQ(mod->functions.front()->blocks.front()->instructions.size(), 1);
}

TEST_CASE(OptimizerTest, DeadCodePreservesSideEffects) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_dce() {\n"
        "  .reg .u32 %r1, %r2;\n"
        "  .reg .u64 %rd1;\n"
        "  mov.u32 %r1, 100;\n" // dead assignment to %r1
        "  st.global.u32 [%rd1], 42;\n" // side effect store, MUST be preserved!
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    Parser parser(lexer.tokenize(), diag);
    auto mod = parser.parse_module();

    DeadCodeEliminationPass pass;
    auto res = pass.run_on_function(*mod->functions.front());

    EXPECT_TRUE(res.modified);
    EXPECT_EQ(res.num_transformations, 1); // %r1 assignment removed

    bool store_found = false;
    for (const auto& inst : mod->functions.front()->blocks.front()->instructions) {
        if (inst->opcode == Opcode::ST) store_found = true;
    }
    EXPECT_TRUE(store_found);
}
