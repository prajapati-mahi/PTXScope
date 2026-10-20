#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"

using namespace ptxscope;

TEST_CASE(ParserTest, ParseKernelStructure) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".address_size 64\n"
        ".entry vecAdd(\n"
        "  .param .u64 A,\n"
        "  .param .u64 B\n"
        ") {\n"
        "  .reg .u64 %rd<4>;\n"
        "  .reg .u32 %r<4>;\n"
        "  ld.param.u64 %rd1, [A];\n"
        "  add.u32 %r1, %r2, %r3;\n"
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    EXPECT_FALSE(diag.has_errors());
    EXPECT_EQ(mod->functions.size(), 1);
    EXPECT_EQ(mod->functions[0]->name, "vecAdd");
    EXPECT_EQ(mod->functions[0]->is_entry, true);
    EXPECT_TRUE(mod->get_total_instructions() >= 3);
}

TEST_CASE(ParserTest, SyntaxErrorRecovery) {
    std::string bad_src =
        ".version 7.0\n"
        ".entry broken() {\n"
        "  add.u32 %r1, %r2\n" // missing semicolon and 3rd operand
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(bad_src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    EXPECT_TRUE(diag.has_errors());
    EXPECT_TRUE(diag.error_count() > 0);
}
