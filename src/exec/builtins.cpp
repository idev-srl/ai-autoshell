/*
 * Built-in commands implementation - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <ai-autoshell/exec/builtins.hpp>
#include <ai-autoshell/exec/executor_posix.hpp>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <ai-autoshell/exec/job.hpp>
#include <sys/wait.h>
#include <signal.h>
#include <cerrno>

namespace autoshell {
namespace fs = std::filesystem;

static int do_cd(const std::vector<std::string>& argv) {
    const char* target = nullptr; std::string tmp;
    if (argv.size()<2) { tmp = std::getenv("HOME")?std::getenv("HOME"):"/"; target = tmp.c_str(); }
    else if (argv[1] == "-") {
        tmp = std::getenv("OLDPWD")?std::getenv("OLDPWD"):(std::getenv("PWD")?std::getenv("PWD"):"/");
        target = tmp.c_str(); std::cout << target << '\n';
    } else { target = argv[1].c_str(); }
    std::string old = fs::current_path().string();
    if (chdir(target)!=0) { perror("cd"); return 1; }
    std::string now = fs::current_path().string();
    setenv("OLDPWD", old.c_str(), 1); setenv("PWD", now.c_str(), 1);
    return 0;
}

static int do_pwd() {
    try { std::cout << fs::current_path().string() << '\n'; return 0; } catch(...) { perror("pwd"); return 1; }
}

static int do_echo(const std::vector<std::string>& argv) {
    for (size_t i=1;i<argv.size();++i) {
        if (i>1) std::cout << ' ';
        std::cout << argv[i];
    }
    std::cout << '\n';
    std::cout.flush();
    return 0;
}

static int do_export(const std::vector<std::string>& argv) {
    // format: export VAR=VALUE
    int rc=0;
    for (size_t i=1;i<argv.size();++i) {
        auto &a = argv[i];
        auto eq = a.find('=');
        if (eq==std::string::npos || eq==0) { std::cerr << "export: invalid: " << a << '\n'; rc=1; continue; }
        std::string key = a.substr(0,eq); std::string val = a.substr(eq+1);
        setenv(key.c_str(), val.c_str(), 1);
    }
    return rc;
}

static int do_unset(const std::vector<std::string>& argv) {
    int rc=0;
    for (size_t i=1;i<argv.size();++i) {
        if (unsetenv(argv[i].c_str())!=0) { perror("unset"); rc=1; }
    }
    return rc;
}

static bool is_jobs_builtin(const std::string& s){ return s=="jobs"||s=="fg"||s=="bg"; }

bool is_builtin(const std::string& name) {
    return name=="cd"||name=="pwd"||name=="exit"||name=="echo"||name=="export"||name=="unset"||is_jobs_builtin(name);
}

struct ExecContext; // forward
std::optional<BuiltinResult> run_builtin(const std::vector<std::string>& argv, ExecContext* ctx) {
    if (argv.empty()) return BuiltinResult{0,false};
    if (!is_builtin(argv[0])) return std::nullopt;
    BuiltinResult res;
    if (argv[0]=="cd") res.exit_code = do_cd(argv);
    else if (argv[0]=="pwd") res.exit_code = do_pwd();
    else if (argv[0]=="echo") res.exit_code = do_echo(argv);
    else if (argv[0]=="export") res.exit_code = do_export(argv);
    else if (argv[0]=="unset") res.exit_code = do_unset(argv);
    else if (argv[0]=="exit") { res.exit_code = 0; res.should_exit = true; }
    else if (is_jobs_builtin(argv[0])) {
        if (!ctx) { std::cerr << argv[0] << ": no context" << '\n'; res.exit_code=1; }
        else if (argv[0]=="jobs") {
            ctx->jobs.reap();
            for (auto &j : ctx->jobs.list()) {
                std::string state;
                if (j.stopped) state = "stopped";
                else if (j.running) state = "running";
                else state = "done";
                std::cout << '[' << j.id << "] pgid=" << j.pgid << ' ' << state
                          << (j.background?" &":"") << " - " << j.command_line << '\n';
            }
            res.exit_code = 0;
        } else if (argv[0]=="fg") {
            if (argv.size()<2) { std::cerr << "fg: job id required" << '\n'; res.exit_code=1; }
            else {
                int id = std::stoi(argv[1]);
                auto jobs = ctx->jobs.list();
                pid_t pgid=-1; bool was_stopped=false; for (auto &j: jobs) if (j.id==id) { pgid=j.pgid; was_stopped=j.stopped; break; }
                if (pgid==-1) { std::cerr << "fg: job not found" << '\n'; res.exit_code=1; }
                else {
                    if (was_stopped) {
                        // Resume the process group.
                        if (kill(-pgid, SIGCONT)!=0) perror("fg(SIGCONT)");
                    }
                    int st; while (waitpid(-pgid,&st,0)<0 && errno==EINTR) {}
                    res.exit_code = (WIFEXITED(st)?WEXITSTATUS(st): (WIFSIGNALED(st)?128+WTERMSIG(st):1));
                    ctx->jobs.mark_finished_pgid(pgid);
                    ctx->jobs.reap();
                }
            }
        } else if (argv[0]=="bg") {
            if (argv.size()<2) { std::cerr << "bg: job id required" << '\n'; res.exit_code=1; }
            else {
                int id = std::stoi(argv[1]);
                auto jobs = ctx->jobs.list();
                pid_t pgid=-1; bool was_stopped=false; for (auto &j: jobs) if (j.id==id) { pgid=j.pgid; was_stopped=j.stopped; break; }
                if (pgid==-1) { std::cerr << "bg: job not found" << '\n'; res.exit_code=1; }
                else {
                    if (was_stopped) {
                        if (kill(-pgid, SIGCONT)!=0) perror("bg(SIGCONT)");
                        // Mark as running again by clearing stopped flag.
                        // We need a mutable reference; refresh list via jobs.reap then iterate internal storage.
                        ctx->jobs.reap();
                    }
                    res.exit_code = 0;
                }
            }
            ctx->jobs.reap();
            res.exit_code = 0;
        }
    }
    return res;
}

} // namespace autoshell
