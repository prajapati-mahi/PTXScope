#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"
#include "ptxscope/analysis.hpp"

using namespace ptxscope;

TEST_CASE(AnalysisTest, UnusedRegisterAndDiagnostics) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_diag() {\n"
        "  .reg .u32 %r0, %r1, %r2;\n"
        "  mov.u32 %r0, 10;\n"
        "  mov.u32 %r1, %r0;\n"
        "  mov.u32 %r2, 999;\n" // %r2 defined but never read
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    StaticAnalyzer analyzer(mod->functions.front(), diag);
    analyzer.run_analysis();

    const auto& usages = analyzer.get_reg_usages();
    EXPECT_TRUE(usages.find("%r2") != usages.end());
    EXPECT_EQ(usages.at("%r2").def_count, 1);
    EXPECT_EQ(usages.at("%r2").use_count, 0);

    bool found_unused_warning = false;
    for (const auto& d : diag.get_diagnostics()) {
        if (d.code == "W001_UNUSED_REGISTER") {
            found_unused_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_unused_warning);
}

TEST_CASE(AnalysisTest, LivenessSets) {
    std::string src =
        ".version 7.0\n"
        ".target sm_70\n"
        ".entry test_liveness() {\n"
        "  .reg .u32 %r1, %r2;\n"
        "  mov.u32 %r1, 5;\n"
        "  add.u32 %r2, %r1, 10;\n"
        "  ret;\n"
        "}\n";

    DiagnosticEngine diag;
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    StaticAnalyzer analyzer(mod->functions.front(), diag);
    analyzer.run_analysis();

    const auto& liveness = analyzer.get_liveness();
    EXPECT_FALSE(liveness.empty());
}
