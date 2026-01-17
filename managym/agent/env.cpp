#include "env.h"

#include "managym/flow/game.h"
#include "managym/infra/info_dict.h"

#include <random>
#include <sstream>
#include <stdexcept>

// Thread-local random number generator
thread_local std::mt19937 rng;


Env::Env(int seed, bool skip_trivial, bool enable_profiler, bool enable_behavior_tracking)
    : game(nullptr), skip_trivial(skip_trivial), seed(seed), enable_profiler(enable_profiler),
      enable_behavior_tracking(enable_behavior_tracking) {
    rng.seed(seed);
    // Seed the random number generator.
    profiler = std::make_unique<Profiler>(enable_profiler, 50);
    hero_tracker = std::make_unique<BehaviorTracker>(enable_behavior_tracking);
    villain_tracker = std::make_unique<BehaviorTracker>(enable_behavior_tracking);
}

std::tuple<Observation*, InfoDict> Env::reset(const std::vector<PlayerConfig>& player_configs) {
    Profiler::Scope scope = profiler->track("env_reset");

    std::vector<BehaviorTracker*> trackers;
    trackers.push_back(hero_tracker.get());
    trackers.push_back(villain_tracker.get());

    // Destroy any old game and create a new one.
    game.reset(new Game(player_configs, &rng, skip_trivial, profiler.get(), trackers));

    // Build initial observation.
    Observation* obs = game->observation();

    // Create and return an empty InfoDict.
    InfoDict info = create_empty_info_dict();
    return std::make_tuple(obs, info);
}

std::tuple<Observation*, double, bool, bool, InfoDict> Env::step(int action, bool skip_trivial) {
    Profiler::Scope scope = profiler->track("env_step");

    if (!game) {
        throw std::runtime_error("Env::step called before reset/game init.");
    }
    if (game->isGameOver()) {
        throw AgentError("env.step called after game is over.");
    }

    Player* agent = game->actionSpace()->player;
    bool done = game->step(action); // returns true if game is over
    Observation* obs = game->observation();

    // Default reward logic:
    //  -  0.0 if the game is still ongoing
    //  - +1.0 if the *active* player is the winner
    //  - -1.0 if the *other* player is the winner
    double reward = 0.0;
    bool terminated = false;
    bool truncated = false;

    // Build a new nested InfoDict.
    InfoDict info = create_empty_info_dict();

    if (done) {
        terminated = true;
        // Identify who won.
        int widx = game->winnerIndex(); // -1 if no winner identified.
        if (widx >= 0) {
            Player* winner = game->players[widx].get();
            reward = (winner == agent) ? 1.0 : -1.0;
            insert_info(info, "winner_name", winner->name);
        } else {
            reward = 0.0;
            insert_info(info, "winner_name", "draw");
        }
    }

    // Only populate profiler/behavior stats at episode end to avoid per-step overhead.
    // Caller can use env.info() explicitly if needed during game.
    if (done) {
        addProfilerInfo(info);
        addBehaviorInfo(info);
    }

    return std::make_tuple(obs, reward, terminated, truncated, info);
}

InfoDict Env::info() {
    InfoDict info = create_empty_info_dict();
    addProfilerInfo(info);
    addBehaviorInfo(info);
    return info;
}

void Env::addProfilerInfo(InfoDict& info) {
    // Build nested profiler info.
    InfoDict profiler_info = create_empty_info_dict();
    if (profiler && profiler->isEnabled()) {
        std::unordered_map<std::string, Profiler::Stats> stats = profiler->getStats();
        std::unordered_map<std::string, Profiler::Stats>::iterator it = stats.begin();
        for (; it != stats.end(); ++it) {
            std::ostringstream oss;
            oss << "total=" << it->second.total_time << "s, count=" << it->second.count;
            insert_info(profiler_info, it->first, oss.str());
        }
    }
    insert_info(info, "profiler", profiler_info);
}

void Env::addBehaviorInfo(InfoDict& info) {

    // Build nested behavior info.
    InfoDict behavior_info = create_empty_info_dict();
    if (hero_tracker && hero_tracker->isEnabled()) {
        std::map<std::string, std::string> hero_stats =
            hero_tracker->getStats(); // new API: no prefix
        InfoDict hero_info = create_empty_info_dict();
        std::map<std::string, std::string>::iterator hit = hero_stats.begin();
        for (; hit != hero_stats.end(); ++hit) {
            insert_info(hero_info, hit->first, hit->second);
        }
        insert_info(behavior_info, "hero", hero_info);
    }
    if (villain_tracker && villain_tracker->isEnabled()) {
        std::map<std::string, std::string> villain_stats = villain_tracker->getStats();
        InfoDict villain_info = create_empty_info_dict();
        std::map<std::string, std::string>::iterator vit = villain_stats.begin();
        for (; vit != villain_stats.end(); ++vit) {
            insert_info(villain_info, vit->first, vit->second);
        }
        insert_info(behavior_info, "villain", villain_info);
    }
    insert_info(info, "behavior", behavior_info);
}