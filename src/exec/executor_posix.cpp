/*
 * POSIX Executor implementation - AI-AutoShell (MVP)
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <ai-autoshell/exec/executor_posix.hpp>
#include <ai-autoshell/expand/expand.hpp>
#include <ai-autoshell/exec/builtins.hpp>
#include <ai-autoshell/exec/path.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>
#include <iostream>
#include <optional>
#include <variant>

std::optional<pid_t> g_foreground_pgid; // definizione globale

namespace autoshell {

int ExecutorPOSIX::run(const AST& ast) {
    if (!ast.list) return 0;
    return run_list(*ast.list);
}

int ExecutorPOSIX::run_list(const ListNode& list) {
    int status = 0;
    for (size_t i=0;i<list.segments.size();++i) {
        status = run_andor(*list.segments[i].and_or);
    }
    m_ctx.last_status = status;
    return status;
}

int ExecutorPOSIX::run_andor(const AndOrNode& node) {
    int status = 0;
    for (size_t i=0;i<node.segments.size();++i) {
        auto &seg = node.segments[i];
        if (i>0) {
            if (seg.op == "&&" && status != 0) return status;
            if (seg.op == "||" && status == 0) return status;
        }
        status = run_pipeline(*seg.pipeline);
    }
    return status;
}

int ExecutorPOSIX::run_pipeline(const PipelineNode& pipeline) {
    size_t n = pipeline.elements.size();
    if (n==0) return 0;
    // Caso singolo elemento (command o subshell)
    if (n==1) {
        return std::visit([&](auto &ptr)->int {
            using T = std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<CommandNode>>) {
                if (ptr->background) {
                    // Background single command (come prima)
                    auto &cmd = *ptr;
                    auto argv_expanded = expand_words(cmd.argv);
                    if (argv_expanded.empty()) return 0;
                    if (is_builtin(argv_expanded[0])) {
                        auto r = run_builtin(argv_expanded);
                        return r ? r->exit_code : 0;
                    }
                    auto exe = resolve_executable(argv_expanded[0]);
                    if (!exe) { std::cerr << argv_expanded[0] << ": command not found" << '\n'; return 127; }
                    pid_t pid = fork();
                    if (pid < 0) { perror("fork"); return 1; }
                    if (pid == 0) {
                        std::signal(SIGINT, SIG_IGN);
                        auto specs = build_redirs(cmd);
                        if (apply_redirections(specs)!=0) _exit(1);
                        std::vector<char*> cargv; cargv.reserve(argv_expanded.size()+1);
                        for (auto &s : argv_expanded) cargv.push_back(const_cast<char*>(s.c_str()));
                        cargv.push_back(nullptr);
                        setpgid(0,0);
                        execvp(cargv[0], cargv.data());
                        perror("execvp"); _exit(127);
                    }
                    setpgid(pid,pid);
                    m_ctx.jobs.add(pid, argv_expanded[0], true);
                    std::cout << "[" << pid << "] running in background" << '\n';
                    return 0;
                }
                return run_command(*ptr);
            } else if constexpr (std::is_same_v<T, std::unique_ptr<SubshellNode>>) {
                // Subshell singola
                if (ptr->background) return run_subshell(*ptr, true);
                return run_subshell(*ptr, false);
            }
            return 0;
        }, pipeline.elements[0]);
    }

    std::vector<int> fds(2*(n-1), -1);
    for (size_t i=0;i<n-1;++i) {
        int p[2];
        if (pipe(p) != 0) { perror("pipe"); return 1; }
        fds[2*i] = p[0];
        fds[2*i+1] = p[1];
    }

    std::vector<pid_t> pids;
    for (size_t i=0;i<n;++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            std::signal(SIGINT, SIG_DFL);
            if (i>0) {
                int in_fd = fds[2*(i-1)];
                dup2(in_fd, STDIN_FILENO);
            }
            if (i < n-1) {
                int out_fd = fds[2*i+1];
                dup2(out_fd, STDOUT_FILENO);
            }
            for (int fd : fds) if (fd!=-1) close(fd);
            // Esegue elemento (command o subshell) nel processo figlio
            int rc = std::visit([&](auto &ptr)->int {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, std::unique_ptr<CommandNode>>) {
                    return run_command(*ptr);
                } else if constexpr (std::is_same_v<T, std::unique_ptr<SubshellNode>>) {
                    // Esecuzione subshell inline: fork extra per isolare? Qui già siamo in child
                    return run_subshell(*ptr, false);
                }
                return 0;
            }, pipeline.elements[i]);
            _exit(rc);
        }
        pids.push_back(pid);
    }
    // Close all pipe fds in parent
    for (int fd : fds) if (fd!=-1) close(fd);

    bool background = false;
    for (auto &elem : pipeline.elements) {
        std::visit([&](auto &ptr){
            using T = std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<CommandNode>>) {
                if (ptr->background) background=true;
            } else if constexpr (std::is_same_v<T, std::unique_ptr<SubshellNode>>) {
                if (ptr->background) background=true;
            }
        }, elem);
        if (background) break;
    }
    int status = 0;
    if (background) {
        // Put all into same process group
        for (size_t i=0;i<pids.size();++i) setpgid(pids[i], pids[0]);
        m_ctx.jobs.add(pids[0], "pipeline", true);
        std::cout << "[" << pids[0] << "] pipeline running in background" << '\n';
        // Do not wait
        return 0;
    }
    // Imposta pgid comune
    for (size_t i=0;i<pids.size();++i) setpgid(pids[i], pids[0]);
    if (!background) g_foreground_pgid = pids[0];
    for (pid_t pid : pids) {
        int st=0; while (waitpid(pid, &st, 0)<0 && errno==EINTR) {}
        if (WIFEXITED(st)) status = WEXITSTATUS(st); else if (WIFSIGNALED(st)) status = 128+WTERMSIG(st);
    }
    if (!background) g_foreground_pgid.reset();
    return status;
}

int ExecutorPOSIX::run_subshell(const SubshellNode& node, bool background) {
    // Subshell: nuovo processo che esegue la lista internamente
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        // Child subshell
        std::signal(SIGINT, background ? SIG_IGN : SIG_DFL);
        setpgid(0,0);
        int st = 0;
        if (node.list) {
            st = run_list(*node.list); // usa stesso executor; attenzione a m_ctx (condiviso). Per isolamento variabili servirebbe copia.
        }
        _exit(st);
    }
    setpgid(pid,pid);
    if (background || node.background) {
        m_ctx.jobs.add(pid, "subshell", true);
        std::cout << "[" << pid << "] subshell running in background" << '\n';
        return 0;
    }
    g_foreground_pgid = pid;
    int st=0; while (waitpid(pid,&st,0)<0 && errno==EINTR) {}
    g_foreground_pgid.reset();
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128+WTERMSIG(st);
    return 0;
}

std::vector<RedirSpec> ExecutorPOSIX::build_redirs(const CommandNode& cmd) {
    std::vector<RedirSpec> specs;
    for (auto &r : cmd.redirs) {
        RedirSpec s; s.target = r.target;
        switch (r.type) {
            case RedirNode::Type::Out: s.type = RedirType::Out; break;
            case RedirNode::Type::OutAppend: s.type = RedirType::OutAppend; break;
            case RedirNode::Type::In: s.type = RedirType::In; break;
            case RedirNode::Type::Err: s.type = RedirType::Err; break;
            case RedirNode::Type::ErrToOut: s.type = RedirType::ErrToOut; break;
        }
        specs.push_back(s);
    }
    return specs;
}

int ExecutorPOSIX::run_command(const CommandNode& cmd) {
    // Expand argv words
    auto argv_expanded = expand_words(cmd.argv);
    if (argv_expanded.empty()) return 0;
    // Built-in?
    if (is_builtin(argv_expanded[0])) {
        // Apply redirections in subscope (dup fds) then restore
        int saved_stdin=-1, saved_stdout=-1, saved_stderr=-1;
        auto specs = build_redirs(cmd);
        if (!specs.empty()) {
            saved_stdin = dup(STDIN_FILENO);
            saved_stdout = dup(STDOUT_FILENO);
            saved_stderr = dup(STDERR_FILENO);
            if (apply_redirections(specs)!=0) {
                if (saved_stdin!=-1) { dup2(saved_stdin, STDIN_FILENO); close(saved_stdin);} 
                if (saved_stdout!=-1) { dup2(saved_stdout, STDOUT_FILENO); close(saved_stdout);} 
                if (saved_stderr!=-1) { dup2(saved_stderr, STDERR_FILENO); close(saved_stderr);} 
                return 1;
            }
        }
    auto r = run_builtin(argv_expanded, &m_ctx);
        if (saved_stdin!=-1) { dup2(saved_stdin, STDIN_FILENO); close(saved_stdin);} 
        if (saved_stdout!=-1) { dup2(saved_stdout, STDOUT_FILENO); close(saved_stdout);} 
        if (saved_stderr!=-1) { dup2(saved_stderr, STDERR_FILENO); close(saved_stderr);} 
        if (r && r->should_exit) std::exit(r->exit_code);
        return r ? r->exit_code : 0;
    }
    auto exe = resolve_executable(argv_expanded[0]);
    if (!exe) { std::cerr << argv_expanded[0] << ": command not found" << '\n'; return 127; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        std::signal(SIGINT, SIG_DFL);
        auto specs = build_redirs(cmd);
        if (apply_redirections(specs)!=0) _exit(1);
        // Build cargv
        std::vector<char*> cargv; cargv.reserve(argv_expanded.size()+1);
        for (auto &s : argv_expanded) cargv.push_back(const_cast<char*>(s.c_str()));
        cargv.push_back(nullptr);
        execvp(cargv[0], cargv.data());
        perror("execvp");
        _exit(127);
    }
    int st=0; while (waitpid(pid,&st,0)<0 && errno==EINTR) {}
    int status=0; if (WIFEXITED(st)) status=WEXITSTATUS(st); else if (WIFSIGNALED(st)) status=128+WTERMSIG(st);
    return status;
}

} // namespace autoshell
