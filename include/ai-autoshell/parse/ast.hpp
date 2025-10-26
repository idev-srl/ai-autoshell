/*
 * AI-AutoShell AST Definitions
 *
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 *
 * Description:
 *   Defines Abstract Syntax Tree node structures representing shell command
 *   constructs: Commands with assignments and redirections, Pipelines, logical
 *   AND/OR chains, and Lists separated by semicolons. This AST is produced by
 *   the parser and later consumed by the executor module.
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
#include <memory>
#include <optional>
#include <variant>
#include "ai-autoshell/parse/tokens.hpp"

namespace autoshell {

struct RedirNode {
    enum class Type { Out, OutAppend, In, Err, ErrToOut } type; 
    std::string target; // file path
};

struct CommandNode {
    std::vector<Token> assigns;
    std::vector<std::string> argv;
    std::vector<RedirNode> redirs;
    bool background = false;
};

struct PipelineNode {
    // Una pipeline ora può contenere sia comandi che subshell.
    using Element = std::variant<std::unique_ptr<CommandNode>, std::unique_ptr<struct SubshellNode>>;
    std::vector<Element> elements;
};

struct AndOrSegment {
    std::unique_ptr<PipelineNode> pipeline;
    std::string op; // "", "&&" or "||"
};

struct AndOrNode {
    std::vector<AndOrSegment> segments;
};

struct ListSegment {
    std::unique_ptr<AndOrNode> and_or;
};

struct ListNode {
    std::vector<ListSegment> segments;
};

struct SubshellNode {
    std::unique_ptr<ListNode> list; // contenuto fra parentesi
    bool background = false;        // '(cmd) &' 
};

struct AST {
    std::unique_ptr<ListNode> list;
};

} // namespace autoshell
