#pragma once
#ifdef _WIN32
#include <ai-autoshell/parse/ast.hpp>
#include <string>

namespace autoshell {
struct ExecContext; // forward (riuso struttura se definita altrove)

class ExecutorWindows {
public:
    ExecutorWindows(ExecContext& ctx) : m_ctx(ctx) {}
    int run(const AST& ast);
private:
    int run_list(const ListNode& list);
    int run_andor(const AndOrNode& node);
    int run_pipeline(const PipelineNode& pipeline);
    int run_command(const CommandNode& cmd);
    ExecContext& m_ctx;
};
}
#endif
