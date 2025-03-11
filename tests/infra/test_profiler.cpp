#include "managym/agent/env.h"
#include "managym/infra/log.h"
#include "managym/infra/profiler.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <string>
#include <thread>
#include <unordered_map>

TEST(ProfilerTest, BasicHierarchyAndTiming) {
    // Create an enabled profiler.
    Profiler profiler(true, 50);

    {
        // Begin scope "A"
        Profiler::Scope scopeA = profiler.track("A");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        {
            // Nested scope "B" under "A"
            Profiler::Scope scopeB = profiler.track("B");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        {
            // Another nested scope "C" under "A"
            Profiler::Scope scopeC = profiler.track("C");
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
    std::unordered_map<std::string, Profiler::Stats> stats = profiler.getStats();

    // Expect nodes "A", "A/B", and "A/C" to exist.
    ASSERT_TRUE(stats.find("A") != stats.end());
    ASSERT_TRUE(stats.find("A/B") != stats.end());
    ASSERT_TRUE(stats.find("A/C") != stats.end());

    // Check that total time for A is roughly 600ms (0.6 sec) with some tolerance.
    double totalA = stats["A"].total_time;
    EXPECT_NEAR(totalA, 0.6, 0.1);

    // For A/B, expect ~200ms and for A/C ~300ms.
    double totalB = stats["A/B"].total_time;
    double totalC = stats["A/C"].total_time;
    EXPECT_NEAR(totalB, 0.2, 0.1);
    EXPECT_NEAR(totalC, 0.3, 0.1);

    // For a root node "A", its pct_of_total should be 100%.
    EXPECT_NEAR(stats["A"].pct_of_total, 100.0, 1e-5);
}

TEST(ProfilerTest, LabelEnforcement) {
    Profiler profiler(true, 50);
    EXPECT_THROW({ Profiler::Scope dummy = profiler.track(""); }, std::runtime_error);
}

TEST(ProfilerTest, RepeatedScopesAccumulate) {
    Profiler profiler(true, 50);
    // Run the same scope repeatedly.
    for (int i = 0; i < 5; i++) {
        Profiler::Scope scope = profiler.track("Loop");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::unordered_map<std::string, Profiler::Stats> stats = profiler.getStats();
    ASSERT_TRUE(stats.find("Loop") != stats.end());
    double total = stats["Loop"].total_time;
    // 5 * 50ms = 250ms (0.25 sec) roughly.
    EXPECT_NEAR(total, 0.25, 0.1);
}


TEST(ProfilerTest, SampleOutput) {
    LogScope log_scope(spdlog::level::warn);

    std::map<std::string, int> mixed_deck{
        {"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}};

    std::vector<PlayerConfig> configs{PlayerConfig("gaea", mixed_deck),
                                      PlayerConfig("urza", mixed_deck)};

    // Create environment instance
    Env env(true);

    for (int i = 0; i < 10; ++i) {
        Observation* obs;
        std::map<std::string, std::string> info;
        std::tie(obs, info) = env.reset(configs);

        bool terminated = false;
        // Keep taking the first action until the game terminates.
        while (!terminated) {
            std::tie(obs, std::ignore, terminated, std::ignore, info) = env.step(0);
        }
    }

    Observation* obs;
    std::map<std::string, std::string> info;
    std::tie(obs, info) = env.reset(configs);
    std::tie(obs, std::ignore, std::ignore, std::ignore, info) = env.step(0);

    // Print the profiler info to standard output.
    std::string output = env.profiler->toString();
    std::cout << "Pretty Printed Profiler Info:\n" << output << std::endl;
}