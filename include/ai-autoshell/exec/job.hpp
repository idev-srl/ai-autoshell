/*
 * Job control - AI-AutoShell (minimal MVP)
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#pragma once
#include <vector>
#include <string>
#include <sys/types.h>
#include <optional>

namespace autoshell {

struct Job {
    int id;            // sequential
    pid_t pgid;        // process group id for pipeline
    std::string command_line;
    bool running;      // true if still has live processes
    bool background;   // launched with &
    bool stopped = false; // at least one process stopped (SIGTSTP); resumed clears
};

class JobTable {
public:
    JobTable() = default;
    int add(pid_t pgid, const std::string& cmdline, bool bg);
    void reap(); // update statuses using waitpid(WNOHANG)
    std::vector<Job> list() const;
    void mark_finished_pgid(pid_t pgid);
private:
    std::vector<Job> m_jobs;
    int m_next_id = 1;
};

} // namespace autoshell
