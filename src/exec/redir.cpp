/*
 * Redirection utilities implementation - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <ai-autoshell/exec/redir.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>

namespace autoshell {

static int dup_to(int fd, int target) {
    if (dup2(fd, target) < 0) { perror("dup2"); return -1; }
    return 0;
}

int apply_redirections(const std::vector<RedirSpec>& specs) {
    for (auto &r : specs) {
        int fd = -1;
        switch (r.type) {
            case RedirType::Out:
                fd = ::open(r.target.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644); break;
            case RedirType::OutAppend:
                fd = ::open(r.target.c_str(), O_CREAT|O_WRONLY|O_APPEND, 0644); break;
            case RedirType::In:
                fd = ::open(r.target.c_str(), O_RDONLY); break;
            case RedirType::Err:
                fd = ::open(r.target.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644); break;
            case RedirType::ErrToOut:
                // handle after loop? Simplify: create a pipe by duplicating stdout later.
                // We'll implement by marking special; continue.
                continue;
        }
        if (fd < 0) { perror("open"); return -1; }
        if (r.type == RedirType::In) {
            if (dup_to(fd, STDIN_FILENO) != 0) { ::close(fd); return -1; }
        } else if (r.type == RedirType::Err) {
            if (dup_to(fd, STDERR_FILENO) != 0) { ::close(fd); return -1; }
        } else { // Out / OutAppend
            if (dup_to(fd, STDOUT_FILENO) != 0) { ::close(fd); return -1; }
        }
        ::close(fd);
    }
    // Second pass for ErrToOut
    for (auto &r : specs) {
        if (r.type == RedirType::ErrToOut) {
            if (dup_to(STDOUT_FILENO, STDERR_FILENO) != 0) return -1;
        }
    }
    return 0;
}

} // namespace autoshell
