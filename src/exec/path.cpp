/*
 * PATH resolution implementation - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <ai-autoshell/exec/path.hpp>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace autoshell {

static bool is_executable(const std::string& p) {
    struct stat st{};
    if (stat(p.c_str(), &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;
    if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) return true;
    return false;
}

std::optional<std::string> resolve_executable(const std::string& cmd) {
    if (cmd.empty()) return std::nullopt;
    if (cmd.find('/') != std::string::npos) {
        if (is_executable(cmd)) return cmd; else return std::nullopt;
    }
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return std::nullopt;
    std::string paths = pathEnv;
    std::vector<std::string> parts;
    size_t start=0;
    while (true) {
        size_t colon = paths.find(':', start);
        if (colon == std::string::npos) { parts.push_back(paths.substr(start)); break; }
        parts.push_back(paths.substr(start, colon-start));
        start = colon+1;
    }
    for (auto &d : parts) {
        if (d.empty()) continue;
        std::string full = d + '/' + cmd;
        if (is_executable(full)) return full;
    }
    return std::nullopt;
}

} // namespace autoshell
