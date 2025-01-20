#pragma once

#include "managym/agent/observation.h"
#include "managym/state/player.h"

#include <memory>
#include <vector>

class Game;

class Env {
private:
    std::unique_ptr<Game> game;

public:
    // Default constructor creates an uninitialized env
    Env();

    // Constructor with initial player configs
    explicit Env(const std::vector<PlayerConfig>& player_configs);

    // Reset the environment with new player configs
    // A new game is created.
    std::pair<Observation*, ActionSpace*> reset(const std::vector<PlayerConfig>& player_configs);

    // Step through the game
    // Throws an error if game is not initialized or action space is invalid
    std::tuple<Observation*, ActionSpace*, bool> step(int action);
};