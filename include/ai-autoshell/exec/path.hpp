/*
 * PATH resolution utilities - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#pragma once
#include <string>
#include <optional>

namespace autoshell {

// Resolve command name to absolute path using PATH env if necessary.
// If cmd contains '/' return as-is (no existence check here).
std::optional<std::string> resolve_executable(const std::string& cmd);

} // namespace autoshell
