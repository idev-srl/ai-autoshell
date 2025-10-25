/*
 * AI-AutoShell Lexer Implementation
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 * Description: See header for overview.
 */
/*
 * AI-AutoShell Lexer Implementation
 * Copyright (c) 2025 iDev srl
 * Author: Luigi De Astis <l.deastis@idev-srl.com>
 * Description: Converts an input line into TokenStream (operators, words,
 *              assignments, redirections). See header for details.
 */
#include <cctype>
#include <ai-autoshell/lex/lexer.hpp>

namespace autoshell {

Lexer::Lexer(std::string input, LexerOptions opts) : m_input(std::move(input)), m_opts(opts) {}

char Lexer::peek() const { return eof() ? '\0' : m_input[m_pos]; }
char Lexer::get() { return eof() ? '\0' : m_input[m_pos++]; }
bool Lexer::eof() const { return m_pos >= m_input.size(); }

void Lexer::skip_space() { while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) get(); }

bool Lexer::is_name_start(char c) const { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool Lexer::is_name_char(char c) const { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

Token Lexer::lex_operator() {
    std::size_t start = m_pos;
    char c = get();
    switch (c) {
        case '|': if (peek() == '|') { get(); return {TokenKind::OrIf, "||", start}; } return {TokenKind::Pipe, "|", start};
        case '&': if (peek() == '&') { get(); return {TokenKind::AndIf, "&&", start}; } return {TokenKind::Background, "&", start};
    case ';': return {TokenKind::Semi, ";", start};
    case '(': return {TokenKind::LeftParen, "(", start};
    case ')': return {TokenKind::RightParen, ")", start};
        case '>': if (peek() == '>') { get(); return {TokenKind::RedirOutAppend, ">>", start}; } return {TokenKind::RedirOut, ">", start};
        case '<': return {TokenKind::RedirIn, "<", start};
        case '2':
            if (peek() == '>') {
                get();
                if (peek() == '&') {
                    std::size_t save = m_pos; get();
                    if (peek() == '1') { get(); return {TokenKind::RedirErrToOut, "2>&1", start}; }
                    m_pos = save; return {TokenKind::RedirErr, "2>", start};
                }
                return {TokenKind::RedirErr, "2>", start};
            }
            m_pos = start; return lex_word();
        default: return {TokenKind::Invalid, std::string(1,c), start};
    }
}

Token Lexer::lex_word() {
    std::size_t start = m_pos; std::string out; bool in_single=false, in_double=false;
    while (!eof()) {
        char c = peek();
        if (!in_single && !in_double) {
            if (std::isspace(static_cast<unsigned char>(c))) break;
            if (c=='|'||c=='&'||c==';'||c=='>'||c=='<'|| (c=='2' && m_pos+1 < m_input.size() && m_input[m_pos+1]=='>')) break;
            if (c=='\'') { in_single=true; get(); continue; }
            if (c=='"') { in_double=true; get(); continue; }
            if (c=='\\') { get(); if(!eof()) out.push_back(get()); continue; }
            out.push_back(get());
        } else if (in_single) {
            get(); if (c=='\'') { in_single=false; continue; } out.push_back(c);
        } else if (in_double) {
            get(); if (c=='"') { in_double=false; continue; }
            if (c=='\\' && !eof()) { char n=peek(); if (n=='"'||n=='\\'||n=='$') { out.push_back(n); get(); continue; }}
            out.push_back(c);
        }
    }
    Token t{TokenKind::Word, out, start};
    if (m_opts.enable_assign_detection) t = try_assign(t);
    return t;
}

Token Lexer::try_assign(const Token& word) {
    auto &lex = word.lexeme; auto eq = lex.find('=');
    if (eq==std::string::npos || eq==0) return word;
    for (std::size_t i=0;i<eq;++i) if (!is_name_char(lex[i]) || (i==0 && !is_name_start(lex[i]))) return word;
    Token assign = word; assign.kind = TokenKind::Assign; return assign;
}

Token Lexer::next() {
    skip_space(); if (eof()) return {TokenKind::Eof, "", m_pos};
    char c = peek(); if (c=='|'||c=='&'||c==';'||c=='>'||c=='<'||c=='('||c==')'||c=='2') return lex_operator();
    return lex_word();
}

TokenStream Lexer::run() {
    TokenStream ts; while (true) { Token t = next(); ts.push_back(t); if (t.kind==TokenKind::Eof) break; }
    return ts;
}

} // namespace autoshell

