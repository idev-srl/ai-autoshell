/*
 * AI-AutoShell Expansion Interface
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 *
 * Description:
 *   Provides word expansion utilities for tilde (~), environment variables
 *   ($VAR and ${VAR}). No command substitution or globbing at MVP stage.
 */
#pragma once
#include <string>
#include <vector>

namespace autoshell {

// Expand a single word: tilde, env vars, globbing (simple) if pattern present.
std::string expand_word(const std::string& in);

// Expand a list of words (appends glob matches; if no match keep literal).
std::vector<std::string> expand_words(const std::vector<std::string>& words);

// Detect if word contains glob meta characters.
bool has_glob_chars(const std::string& s);

} // namespace autoshell
