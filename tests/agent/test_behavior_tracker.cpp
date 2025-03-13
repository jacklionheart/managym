#include "managym/agent/behavior_tracker.h"
#include "managym/agent/env.h"
#include "managym/infra/info_dict.h" // for our helper functions
#include "managym/infra/log.h"

#include <gtest/gtest.h>

#include <chrono>
#include <random>
#include <string>
#include <tuple>

TEST(BehaviorTrackerTest, RandomActionsOutput) {
    // Create standard mixed deck configuration.
    std::map<std::string, int> mixed_deck{
        {"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}};

    std::vector<PlayerConfig> configs{PlayerConfig("gaea", mixed_deck),
                                      PlayerConfig("urza", mixed_deck)};

    // Create environment with behavior tracking enabled.
    // Args: seed=0, skip_trivial=true, enable_behavior_tracking=true.
    Env env(0, true, true);

    // Initialize random number generator with a time-based seed.
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

    // Run multiple games with random action selection.
    const int num_games = 10;
    for (int game_num = 0; game_num < num_games; ++game_num) {
        log_debug(LogCat::TEST, "\n========== GAME {} ==========", game_num + 1);

        // Reset environment.
        std::tuple<Observation*, InfoDict> result = env.reset(configs);
        Observation* obs = std::get<0>(result);
        InfoDict info = std::get<1>(result);

        bool terminated = false;
        bool truncated = false;
        int step_count = 0;
        const int max_steps = 2000; // Limit steps to prevent infinite games

        // Play game using random action selection.
        while (!terminated && !truncated && step_count < max_steps) {
            int action = 0;
            if (obs->action_space.actions.size() > 1) {
                std::uniform_int_distribution<int> dist(0, obs->action_space.actions.size() - 1);
                action = dist(rng);
            }

            // Take a step.
            double reward;
            std::tie(obs, reward, terminated, truncated, info) = env.step(action);
            ++step_count;

            // Every 50 steps, log the current behavior stats.
            if (step_count % 50 == 0) {
                std::string stats = fmt::format("Step {} behavior stats:", step_count);

                // Instead of grouping keys by prefix, get stats directly from the trackers.
                std::string heroStats = "Hero behavior stats:";
                auto hstats = env.hero_tracker->getStats();
                for (const auto& stat : hstats) {
                    heroStats += fmt::format("\n  {}: {}", stat.first, stat.second);
                }
                std::string villainStats = "Villain behavior stats:";
                auto vstats = env.villain_tracker->getStats();
                for (const auto& stat : vstats) {
                    villainStats += fmt::format("\n  {}: {}", stat.first, stat.second);
                }
                stats += "\n" + heroStats + "\n" + villainStats;
                log_debug(LogCat::TEST, "{}", stats);
            }
        }

        // End-of-game logging.
        log_debug(LogCat::TEST, "\nGame {} completed in {} steps.", game_num + 1, step_count);
        std::string winnerName = "unknown";
        if (dict_contains(info, "winner_name")) {
            // Retrieve the winner name using our helper function.
            winnerName = std::get<std::string>(dict_get(info, "winner_name").value);
        }
        log_debug(LogCat::TEST, "Winner: {}", winnerName);
        log_debug(LogCat::TEST, "\nFinal behavior stats:");

        {
            std::string heroStats = "Final Hero behavior stats:";
            auto hstats = env.hero_tracker->getStats();
            for (const auto& stat : hstats) {
                heroStats += fmt::format("\n  {}: {}", stat.first, stat.second);
            }
            log_debug(LogCat::TEST, "{}", heroStats);
        }
        {
            std::string villainStats = "Final Villain behavior stats:";
            auto vstats = env.villain_tracker->getStats();
            for (const auto& stat : vstats) {
                villainStats += fmt::format("\n  {}: {}", stat.first, stat.second);
            }
            log_debug(LogCat::TEST, "{}", villainStats);
        }
    }

    // Verify that behavior tracking is working as expected.
    ASSERT_TRUE(env.hero_tracker->isEnabled());
    ASSERT_TRUE(env.villain_tracker->isEnabled());

    // Check that the hero tracker has populated data.
    auto hero_stats = env.hero_tracker->getStats();
    ASSERT_FALSE(hero_stats.empty());
    ASSERT_TRUE(hero_stats.count("land_play_rate") > 0);
    ASSERT_TRUE(hero_stats.count("attack_rate") > 0);

    // Check that the villain tracker has populated data.
    auto villain_stats = env.villain_tracker->getStats();
    ASSERT_FALSE(villain_stats.empty());
    ASSERT_TRUE(villain_stats.count("land_play_rate") > 0);
    ASSERT_TRUE(villain_stats.count("attack_rate") > 0);

    log_debug(LogCat::TEST, "\nBehavior Tracker Test Complete");
}
