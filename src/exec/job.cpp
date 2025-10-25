/*
 * Job control implementation - AI-AutoShell (minimal MVP)
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <ai-autoshell/exec/job.hpp>
#include <sys/wait.h>
#include <unistd.h>

namespace autoshell {

int JobTable::add(pid_t pgid, const std::string& cmdline, bool bg) {
    Job j{m_next_id++, pgid, cmdline, true, bg};
    m_jobs.push_back(j);
    return j.id;
}

void JobTable::reap() {
    // Iterate jobs and perform non-blocking waits on their process group.
    // We call waitpid(-pgid, ...) to target any child in the group.
    for (auto &j : m_jobs) {
        if (!j.running) continue;
        while (true) {
            int status = 0;
            pid_t r = waitpid(-j.pgid, &status, WUNTRACED | WNOHANG);
            if (r == 0) break; // Nothing new.
            if (r < 0) {
                if (errno == EINTR) continue; // Retry if interrupted.
                break; // Error or no children.
            }
            if (WIFSTOPPED(status)) {
                j.stopped = true; // Mark job as stopped; still running logically.
                continue; // Check for other events.
            }
            if (WIFSIGNALED(status) || WIFEXITED(status)) {
                // Determine if group fully finished: probe one more time.
                int probe_status = 0;
                pid_t probe = waitpid(-j.pgid, &probe_status, WNOHANG);
                if (probe == 0) {
                    // Some processes still alive -> continue loop.
                    continue;
                }
                if (probe < 0) {
                    // No remaining children.
                    j.running = false; j.stopped = false; break;
                }
                // Another process ended; loop again to drain; final completion will be detected in subsequent iteration when -1.
                continue;
            }
        }
    }
    // TODO: Provide policy to purge completed background jobs from m_jobs
    //       to prevent growth (e.g. after next 'jobs' listing or size threshold).
}

std::vector<Job> JobTable::list() const { return m_jobs; }

void JobTable::mark_finished_pgid(pid_t pgid) {
    for (auto &j : m_jobs) if (j.pgid == pgid) { j.running = false; break; }
}

} // namespace autoshell
