/*
 * Parser tests - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <gtest/gtest.h>
#include <ai-autoshell/lex/lexer.hpp>
#include <ai-autoshell/parse/parser.hpp>
#include <ai-autoshell/parse/ast.hpp>

using namespace autoshell;

TEST(ParserBasic, AndOrList) {
    std::string line = "cd /tmp && echo ok || echo fail; pwd";
    Lexer lx(line);
    auto ts = lx.run();
    auto ast = parse_tokens(ts);
    ASSERT_TRUE(ast.list);
    // Expect at least 2 list segments (before ; and after)
    EXPECT_GE(ast.list->segments.size(), 2u);
    auto &firstSeg = ast.list->segments[0];
    ASSERT_TRUE(firstSeg.and_or);
    EXPECT_GE(firstSeg.and_or->segments.size(), 2u); // cd /tmp && echo ok (maybe followed by || echo fail)
}

TEST(ParserPipeline, PipeCount) {
    std::string line = "echo a | grep a | wc -l";
    Lexer lx(line);
    auto ts = lx.run();
    auto ast = parse_tokens(ts);
    ASSERT_TRUE(ast.list);
    ASSERT_FALSE(ast.list->segments.empty());
    auto &seg0 = ast.list->segments[0];
    ASSERT_TRUE(seg0.and_or);
    ASSERT_FALSE(seg0.and_or->segments.empty());
    auto &pipe = seg0.and_or->segments[0].pipeline;
    ASSERT_TRUE(pipe);
    EXPECT_EQ(pipe->elements.size(), 3u);
}
