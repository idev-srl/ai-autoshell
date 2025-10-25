#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <ai-autoshell/expand/expand.hpp>

using namespace autoshell;
namespace fs = std::filesystem;

TEST(GlobBasic, MatchStar) {
    std::string a = "__glob_alpha.txt";
    std::string b = "__glob_beta.txt";
    std::string c = "__glob_gamma.log";
    std::ofstream(a).put('\n');
    std::ofstream(b).put('\n');
    std::ofstream(c).put('\n');
    auto words = expand_words({"__glob_*.txt"});
    ASSERT_GE(words.size(), 2);
    EXPECT_EQ(words[0], a);
    EXPECT_EQ(words[1], b);
    fs::remove(a); fs::remove(b); fs::remove(c);
}

TEST(GlobBasic, NoMatchKeepsLiteral) {
    auto words = expand_words({"doesnotexist.*"});
    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0], "doesnotexist.*");
}

TEST(GlobBasic, QuestionMark) {
    std::string f1="__glob_z1.tmp"; std::string f2="__glob_z2.tmp";
    std::ofstream(f1).put('\n');
    std::ofstream(f2).put('\n');
    auto words = expand_words({"__glob_z?.tmp"});
    ASSERT_EQ(words.size(), 2);
    fs::remove(f1); fs::remove(f2);
}
