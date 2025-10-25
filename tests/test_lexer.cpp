/*
 * Lexer tests - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <gtest/gtest.h>
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/tokens.hpp>

using namespace autoshell;

TEST(LexerBasic, QuotedAndOperators) {
    std::string line = "echo \"ciao mondo\" && ls -l | grep cpp";
    Lexer lx(line);
    auto ts = lx.run();
    // Expect sequence: echo WORD, ciao mondo WORD, &&, ls WORD, -l WORD, |, grep WORD, cpp WORD, EOF
    std::vector<TokenKind> kinds;
    for (auto &t : ts) kinds.push_back(t.kind);
    ASSERT_GE(kinds.size(), 9u);
    EXPECT_EQ(kinds[0], TokenKind::Word);
    EXPECT_EQ(kinds[1], TokenKind::Word);
    EXPECT_EQ(kinds[2], TokenKind::AndIf);
    EXPECT_EQ(kinds[3], TokenKind::Word);
    EXPECT_EQ(kinds[4], TokenKind::Word);
    EXPECT_EQ(kinds[5], TokenKind::Pipe);
    EXPECT_EQ(kinds[6], TokenKind::Word);
    EXPECT_EQ(kinds[7], TokenKind::Word);
    EXPECT_EQ(kinds.back(), TokenKind::Eof);
}

TEST(LexerAssign, AssignmentDetection) {
    std::string line = "VAR=123 echo test";
    Lexer lx(line);
    auto ts = lx.run();
    ASSERT_GE(ts.size(), 4u);
    EXPECT_EQ(ts[0].kind, TokenKind::Assign);
    EXPECT_EQ(ts[1].kind, TokenKind::Word);
}

TEST(LexerRedir, Redirections) {
    std::string line = "echo hi > out.txt 2>&1";
    Lexer lx(line);
    auto ts = lx.run();
    bool foundOut=false, foundErrToOut=false;
    for (auto &t : ts) {
        if (t.kind == TokenKind::RedirOut) foundOut=true;
        if (t.kind == TokenKind::RedirErrToOut) foundErrToOut=true;
    }
    EXPECT_TRUE(foundOut);
    EXPECT_TRUE(foundErrToOut);
}
