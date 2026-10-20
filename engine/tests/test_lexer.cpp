#include "test_framework.hpp"
#include "ptxscope/lexer.hpp"

using namespace ptxscope;

TEST_CASE(LexerTest, DirectivesAndRegisters) {
    std::string src = ".version 7.0\n.target sm_70\n.entry kernel() {\n.reg .u32 %r1;\n}";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();

    EXPECT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].type, TokenType::DIRECTIVE);
    EXPECT_EQ(tokens[0].text, ".version");
    EXPECT_EQ(tokens[1].text, "7.0");
}

TEST_CASE(LexerTest, CommentsAndLocations) {
    std::string src = "// Line comment\n/* block */ mov.u32 %r1, 42;\n";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();

    EXPECT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].type, TokenType::INSTRUCTION);
    EXPECT_EQ(tokens[0].text, "mov");
    EXPECT_EQ(tokens[0].location.line, 2);
}

TEST_CASE(LexerTest, PredicatesAndOperators) {
    std::string src = "@p0 add.u32 %r1, %r2, 10;\n@!p1 bra label;\n";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, TokenType::AT);
    EXPECT_EQ(tokens[1].type, TokenType::REGISTER);
    EXPECT_EQ(tokens[1].text, "p0");

    size_t idx = 0;
    while (idx < tokens.size() && tokens[idx].type != TokenType::BANG) idx++;
    EXPECT_TRUE(idx < tokens.size());
}
