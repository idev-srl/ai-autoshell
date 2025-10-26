#include <gtest/gtest.h>
#include <ai-autoshell/ai/planner.hpp>

using namespace autoshell::ai;

TEST(Planner, ListFilesRule) {
    PlannerConfig cfg; cfg.enabled=true; Planner p(cfg);
    auto plan = p.plan("please list files");
    ASSERT_FALSE(plan.steps.empty());
    EXPECT_NE(plan.steps[0].command.find("ls"), std::string::npos);
}

TEST(Planner, DangerousFlag) {
    PlannerConfig cfg; cfg.enabled=true; Planner p(cfg);
    auto plan = p.plan("remove build directory and list files");
    bool any_confirm=false; for(auto &s: plan.steps) any_confirm |= s.confirm;
    EXPECT_TRUE(any_confirm);
    EXPECT_TRUE(plan.dangerous);
}

TEST(Planner, FallbackEcho) {
    PlannerConfig cfg; cfg.enabled=true; Planner p(cfg);
    auto plan = p.plan("unrecognized task xyz");
    ASSERT_EQ(plan.steps.size(),1u);
    EXPECT_NE(plan.steps[0].command.find("echo"), std::string::npos);
}
