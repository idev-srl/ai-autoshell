/*
 * Expansion tests - AI-AutoShell
 * Copyright (c) 2025 iDev srl - Luigi De Astis <l.deastis@idev-srl.com>
 * MIT License.
 */
#include <gtest/gtest.h>
#include <ai-autoshell/expand/expand.hpp>
#include <cstdlib>
#include <string>

using namespace autoshell;

TEST(ExpandBasic, HomeAndEnv) {
    std::string home = std::getenv("HOME") ? std::getenv("HOME") : std::string();
    ASSERT_FALSE(home.empty());
    auto words = expand_words({"~", "$HOME", "~/Documents"});
    EXPECT_EQ(words[0], home);
    EXPECT_EQ(words[1], home);
    if (home.size()>0) EXPECT_EQ(words[2].substr(0, home.size()), home);
}

TEST(ExpandBraces, BracedVar) {
    setenv("TESTVAR", "XYZ", 1);
    auto words = expand_words({"${TESTVAR}"});
    ASSERT_EQ(words.size(), 1u);
    EXPECT_EQ(words[0], "XYZ");
}
