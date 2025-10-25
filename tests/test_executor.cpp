#include <gtest/gtest.h>
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <fstream>
#include <unistd.h>

using namespace autoshell;

static std::unique_ptr<CommandNode> make_cmd(std::initializer_list<const char*> argv) {
    auto cmd = std::make_unique<CommandNode>();
    for (auto s : argv) cmd->argv.emplace_back(s);
    return cmd;
}

TEST(ExecutorPipeline, EchoCatPipeline) {
    // Build AST: echo hello | cat
    AST ast; ast.list = std::make_unique<ListNode>();
    ListSegment ls; ls.and_or = std::make_unique<AndOrNode>();
    AndOrSegment seg; seg.pipeline = std::make_unique<PipelineNode>();
    seg.pipeline->elements.push_back(PipelineNode::Element{make_cmd({"echo","hello"})});
    seg.pipeline->elements.push_back(PipelineNode::Element{make_cmd({"cat"})});
    ls.and_or->segments.push_back(std::move(seg));
    ast.list->segments.push_back(std::move(ls));

    ExecContext ctx; ExecutorPOSIX ex(ctx);
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
}

TEST(ExecutorAndOr, TrueFalseLogic) {
    // true && false || true  -> final status of last executed pipeline
    AST ast; ast.list = std::make_unique<ListNode>();
    ListSegment ls; ls.and_or = std::make_unique<AndOrNode>();

    // segment1: true
    {
        AndOrSegment seg; seg.op = ""; seg.pipeline = std::make_unique<PipelineNode>();
    seg.pipeline->elements.push_back(PipelineNode::Element{make_cmd({"/usr/bin/true"})});
        ls.and_or->segments.push_back(std::move(seg));
    }
    // segment2: && false
    {
        AndOrSegment seg; seg.op = "&&"; seg.pipeline = std::make_unique<PipelineNode>();
    seg.pipeline->elements.push_back(PipelineNode::Element{make_cmd({"/usr/bin/false"})});
        ls.and_or->segments.push_back(std::move(seg));
    }
    // segment3: || true  (should execute because previous failed)
    {
        AndOrSegment seg; seg.op = "||"; seg.pipeline = std::make_unique<PipelineNode>();
    seg.pipeline->elements.push_back(PipelineNode::Element{make_cmd({"/usr/bin/true"})});
        ls.and_or->segments.push_back(std::move(seg));
    }
    ast.list->segments.push_back(std::move(ls));

    ExecContext ctx; ExecutorPOSIX ex(ctx);
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
}

TEST(ExecutorRedir, RedirectOut) {
    // echo test > /tmp/ai_autoshell_test_out
    const char* outfile = "/tmp/ai_autoshell_test_out";
    unlink(outfile);

    AST ast; ast.list = std::make_unique<ListNode>();
    ListSegment ls; ls.and_or = std::make_unique<AndOrNode>();
    AndOrSegment seg; seg.pipeline = std::make_unique<PipelineNode>();
    auto cmd = make_cmd({"echo","test"});
    RedirNode r; r.type = RedirNode::Type::Out; r.target = outfile;
    cmd->redirs.push_back(r);
    seg.pipeline->elements.push_back(PipelineNode::Element{std::move(cmd)});
    ls.and_or->segments.push_back(std::move(seg));
    ast.list->segments.push_back(std::move(ls));

    ExecContext ctx; ExecutorPOSIX ex(ctx);
    int status = ex.run(ast);
    EXPECT_EQ(status, 0);
    std::ifstream in(outfile);
    ASSERT_TRUE(in.good());
    std::string content; std::getline(in, content);
    EXPECT_EQ(content, "test");
    in.close();
    unlink(outfile);
}
