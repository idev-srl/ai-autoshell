#include <gtest/gtest.h>
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/tokens.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <chrono>
#include <thread>

using namespace autoshell;

static AST parse_line(const std::string& line) {
    Lexer lx(line, {true});
    auto ts = lx.run();
    return parse_tokens(ts);
}

TEST(JobsBackground, SingleSleepBackground) {
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    auto ast = parse_line("/bin/sleep 1 &");
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
    // Immediately job should be recorded
    auto jobs = ctx.jobs.list();
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_TRUE(jobs[0].background);
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    ctx.jobs.reap();
    jobs = ctx.jobs.list();
    EXPECT_FALSE(jobs[0].running);
}

TEST(JobsPipelineBackground, PipelineBackground) {
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    auto ast = parse_line("/bin/echo ok | /bin/cat &");
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
    auto jobs = ctx.jobs.list();
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_TRUE(jobs[0].background);
    ctx.jobs.reap();
}

TEST(JobsFG, ForegroundWait) {
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    auto ast = parse_line("/bin/sleep 1 &");
    ex.run(ast);
    auto jobs = ctx.jobs.list();
    ASSERT_EQ(jobs.size(),1u);
    // Invoke fg via builtin
    auto fg_ast = parse_line("fg " + std::to_string(jobs[0].id));
    int status = ex.run(fg_ast);
    EXPECT_EQ(status, 0);
    jobs = ctx.jobs.list();
    EXPECT_FALSE(jobs[0].running);
}

TEST(PipelineLong, TenEchos) {
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    std::string line;
    for (int i=0;i<10;++i) {
        if (i>0) line += " | ";
        line += "/bin/echo x";
    }
    auto ast = parse_line(line);
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
}

TEST(RedirectError, MissingFileRead) {
    ExecContext ctx; ExecutorPOSIX ex(ctx);
    auto ast = parse_line("/bin/cat < /nonexistent_xyz_file_should_fail");
    int status = ex.run(ast);
    // cat will fail to open file giving exit code 1 or >0 error; accept non-zero
    EXPECT_NE(status, 0);
}
