/*
 * POSIX Executor - AI-AutoShell (MVP)
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#pragma once
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/exec/redir.hpp>
#include <ai-autoshell/exec/builtins.hpp>
#include <ai-autoshell/exec/path.hpp>
#include <ai-autoshell/exec/job.hpp>
#include <vector>
#include <optional>
#include <variant>

extern std::optional<pid_t> g_foreground_pgid;

namespace autoshell {

struct ExecContext {
    JobTable jobs;
    int last_status = 0;
};

class ExecutorPOSIX {
public:
    ExecutorPOSIX(ExecContext& ctx) : m_ctx(ctx) {}
    int run(const AST& ast);
private:
    int run_list(const ListNode& list);
    int run_andor(const AndOrNode& node);
    int run_pipeline(const PipelineNode& pipe);
    int run_command(const CommandNode& cmd);
    int run_subshell(const SubshellNode& node, bool background);
    std::vector<RedirSpec> build_redirs(const CommandNode& cmd);
    ExecContext& m_ctx;
};

} // namespace autoshell
