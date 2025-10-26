/*
 * Redirection utilities - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace autoshell {

enum class RedirType { Out, OutAppend, In, Err, ErrToOut };

struct RedirSpec {
    RedirType type;
    std::string target; // path
};

// Open and apply redirections for child process; returns non-zero on error.
int apply_redirections(const std::vector<RedirSpec>& specs);

} // namespace autoshell
