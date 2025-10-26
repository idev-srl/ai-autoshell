/*
 * AI-AutoShell Token Definitions
 *
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 *
 * Description:
 *   Defines token kinds and Token structure used by the lexer and parser. This
 *   classification supports shell operators, redirections, assignments, words,
 *   and end-of-input markers required for syntactic analysis.
 *
 * License (MIT): (see full text in lexer.hpp header or duplicate below)
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
#include <cstddef>
#include <vector>

namespace autoshell {

enum class TokenKind {
    Word,
    AndIf,
    OrIf,
    Pipe,
    Semi,
    LeftParen,
    RightParen,
    RedirOut,
    RedirOutAppend,
    RedirIn,
    RedirErr,
    RedirErrToOut,
    Assign,
    Background,
    Eof,
    Invalid
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    std::size_t pos;
};

using TokenStream = std::vector<Token>;

} // namespace autoshell
