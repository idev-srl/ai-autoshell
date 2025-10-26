/*
 * Built-in commands - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace autoshell {

struct BuiltinResult {
    int exit_code = 0;
    bool should_exit = false;
};

struct ExecContext; // forward decl
// Returns nullopt if not a builtin.
std::optional<BuiltinResult> run_builtin(const std::vector<std::string>& argv, ExecContext* ctx=nullptr);

bool is_builtin(const std::string& name);

} // namespace autoshell
