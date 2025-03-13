#pragma once

#include "managym/agent/behavior_tracker.h"
#include "managym/agent/observation.h"
#include "managym/flow/game.h"
#include "managym/infra/info_dict.h"
#include "managym/infra/profiler.h"

#include <tuple>
#include <utility>
#include <vector>

// The environment class for manabot
class Env {
private:
    std::unique_ptr<Game> game;
    // Skip trivial action spaces (<= 1 action)
    bool skip_trivial;
    // Random seed for environment
    int seed;

public:
    std::unique_ptr<Profiler> profiler;
    std::unique_ptr<BehaviorTracker> hero_tracker;
    std::unique_ptr<BehaviorTracker> villain_tracker;

    Env(int seed = 0, bool skip_trivial = false, bool enable_behavior_tracking = false);

    /**
     * Resets the environment with new player configs and returns (observation, info).
     */
    std::tuple<Observation*, InfoDict> reset(const std::vector<PlayerConfig>& player_configs);

    // Steps the environment by applying the given action index.
    //
    // Returns a tuple of:
    //   Observation*  -- the full game state, including the action space
    //   double        -- reward (+1 if active player wins, -1 if loses, 0 otherwise)
    //   bool          -- terminated (game over)
    //   bool          -- truncated (e.g. timeout)
    //   py::dict      -- info dictionary with nested sub-dicts:
    //                      "profiler"  : dict of profiler stats,
    //                      "behavior"  : dict with keys "hero" and "villain" mapping to their
    //                      stats.
    std::tuple<Observation*, double, bool, bool, InfoDict> step(int action,
                                                                bool skip_trivial = false);
};
