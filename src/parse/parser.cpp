/*
 * AI-AutoShell Parser Implementation
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 * Description: See header for details.
 */
#include <ai-autoshell/parse/ast.hpp>
#include <ai-autoshell/parse/tokens.hpp>
#include <iostream>
#include <optional>

namespace autoshell {

class Parser {
public:
    Parser(const TokenStream& ts) : m_ts(ts) {}
    AST parse_line() {
        AST ast; ast.list = parse_list();
        return ast;
    }
private:
    const Token& peek() const { return m_ts[m_index]; }
    bool eof() const { return peek().kind == TokenKind::Eof; }
    const Token& get() { return m_ts[m_index++]; }

    std::unique_ptr<ListNode> parse_list() {
        auto list = std::make_unique<ListNode>();
        while (true) {
            auto and_or = parse_and_or();
            if (!and_or) break;
            list->segments.push_back({std::move(and_or)});
            if (peek().kind == TokenKind::Semi) { get(); continue; }
            break;
        }
        return list;
    }

    std::unique_ptr<AndOrNode> parse_and_or() {
        auto node = std::make_unique<AndOrNode>();
        // first pipeline
        auto pipe_first = parse_pipeline();
        if (!pipe_first) return nullptr;
        node->segments.push_back({std::move(pipe_first), ""});
        while (peek().kind == TokenKind::AndIf || peek().kind == TokenKind::OrIf) {
            std::string op = (peek().kind == TokenKind::AndIf) ? "&&" : "||";
            get();
            auto pipe_next = parse_pipeline();
            if (!pipe_next) break; // error tolerant
            node->segments.push_back({std::move(pipe_next), op});
        }
        return node;
    }

    std::unique_ptr<PipelineNode> parse_pipeline() {
        auto pipe = std::make_unique<PipelineNode>();
        auto first = parse_command_or_subshell();
        if (!first) return nullptr;
        pipe->elements.push_back(std::move(*first));
        while (peek().kind == TokenKind::Pipe) {
            get();
            auto next = parse_command_or_subshell();
            if (!next) break; // error tolerant
            pipe->elements.push_back(std::move(*next));
        }
        return pipe;
    }

    std::optional<PipelineNode::Element> parse_command_or_subshell() {
        if (peek().kind == TokenKind::LeftParen) {
            auto subshell = parse_subshell();
            if (!subshell) return std::nullopt;
            return PipelineNode::Element{std::move(subshell)};
        }
        auto cmd = parse_command();
        if (!cmd) return std::nullopt;
        return PipelineNode::Element{std::move(cmd)};
    }

    std::unique_ptr<SubshellNode> parse_subshell() {
        if (peek().kind != TokenKind::LeftParen) return nullptr;
        get(); // '('
        auto inner_list = parse_list();
        if (peek().kind != TokenKind::RightParen) {
            // errore: manca ')'
            return nullptr;
        }
        get(); // ')'
        auto node = std::make_unique<SubshellNode>();
        node->list = std::move(inner_list);
        if (peek().kind == TokenKind::Background) { node->background = true; get(); }
        return node;
    }

    std::unique_ptr<CommandNode> parse_command() {
        auto cmd = std::make_unique<CommandNode>();
        // prefix assigns
        while (peek().kind == TokenKind::Assign) {
            cmd->assigns.push_back(get());
        }
        // words
        while (peek().kind == TokenKind::Word) {
            cmd->argv.push_back(get().lexeme);
        }
        // redirs
        while (true) {
            if (peek().kind == TokenKind::RedirOut || peek().kind == TokenKind::RedirOutAppend ||
                peek().kind == TokenKind::RedirIn || peek().kind == TokenKind::RedirErr ||
                peek().kind == TokenKind::RedirErrToOut) {
                Token op = get();
                if (peek().kind != TokenKind::Word && peek().kind != TokenKind::Assign) {
                    // need a target word (simplified); abort
                    break;
                }
                Token target = get();
                RedirNode::Type type;
                switch (op.kind) {
                    case TokenKind::RedirOut: type = RedirNode::Type::Out; break;
                    case TokenKind::RedirOutAppend: type = RedirNode::Type::OutAppend; break;
                    case TokenKind::RedirIn: type = RedirNode::Type::In; break;
                    case TokenKind::RedirErr: type = RedirNode::Type::Err; break;
                    case TokenKind::RedirErrToOut: type = RedirNode::Type::ErrToOut; break;
                    default: type = RedirNode::Type::Out; break;
                }
                cmd->redirs.push_back({type, target.lexeme});
                continue;
            }
            break;
        }
        if (peek().kind == TokenKind::Background) { cmd->background = true; get(); }
        if (cmd->argv.empty() && cmd->assigns.empty()) return nullptr; // not a valid command
        return cmd;
    }

    const TokenStream& m_ts;
    std::size_t m_index = 0;
};

// Exposed helper
AST parse_tokens(const TokenStream& ts) {
    Parser p(ts);
    return p.parse_line();
}

} // namespace autoshell
