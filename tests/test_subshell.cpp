#include <gtest/gtest.h>
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <fstream>

using namespace autoshell;

TEST(Subshell, SimpleEcho) {
    Lexer lx("(echo hi)");
    auto ts = lx.run();
    AST ast = parse_tokens(ts);
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    testing::internal::CaptureStdout();
    int st = ex.run(ast);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(st,0);
    EXPECT_NE(out.find("hi"), std::string::npos);
}

TEST(Subshell, PipelineWithSubshell) {
    Lexer lx("(echo hi)|cat");
    auto ts = lx.run();
    AST ast = parse_tokens(ts);
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    testing::internal::CaptureStdout();
    int st = ex.run(ast);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(st,0);
    EXPECT_NE(out.find("hi"), std::string::npos);
}
