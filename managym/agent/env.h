#pragma once

#include "managym/agent/observation.h"

#include <map>
#include <string>
#include <tuple>

// The environment class for manabot
class Env {
private:
    std::unique_ptr<Game> game;
    // Skip trivial action spaces (<= 1 action)
    bool skip_trivial;

public:
    Env(bool skip_trivial = false);

    /**
     * Resets the environment with new player configs and returns (observation, info).
     */
    std::pair<Observation*, std::map<std::string, std::string>>
    reset(const std::vector<PlayerConfig>& player_configs);

    // Steps the environment by applying the given action index.
    //
    // Returns a tuple of:
    // Observation* observation: the full game state, including the action space
    // double reward: -1 if the current player lost, 0 if the game is still ongoing, 1 if the
    // current player won bool terminated: true if the game is over bool truncated: true if the game
    // was truncated (e.g. by a timeout) std::map<std::string, std::string> info: additional
    // information about the game state
    std::tuple<Observation*, double, bool, bool, std::map<std::string, std::string>>
    step(int action, bool skip_trivial = false);
};
