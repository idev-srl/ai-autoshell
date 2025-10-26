/*
 * AI-AutoShell Lexer Module
 *
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 *
 * Description:
 *   Provides lexical analysis for the AI-AutoShell. Converts an input line into a
 *   stream of Token objects handling quoting, escaping, operators (|, &&, ||, ;, >, >>,<, 2>, 2>&1),
 *   and assignment detection (NAME=VALUE). This is the first stage of the shell
 *   pipeline prior to parsing into an AST.
 *
 * License (MIT):
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of this
 *   software and associated documentation files (the "Software"), to deal in the Software
 *   without restriction, including without limitation the rights to use, copy, modify, merge,
 *   publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 *   to whom the Software is furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in all copies or
 *   substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 *   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include "ai-autoshell/parse/tokens.hpp"

namespace autoshell {

struct LexerOptions {
    bool enable_assign_detection = true; // treat NAME=VALUE as a single token
};

class Lexer {
public:
    Lexer(std::string input, LexerOptions opts = {});
    TokenStream run();
private:
    Token next();
    char peek() const;
    char get();
    bool eof() const;
    void skip_space();
    Token lex_word();
    Token lex_operator();
    bool is_name_start(char c) const;
    bool is_name_char(char c) const;
    Token try_assign(const Token& word);

    std::string m_input;
    LexerOptions m_opts;
    std::size_t m_pos = 0; // current index
};

} // namespace autoshell
