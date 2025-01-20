#include "env.h"

#include "managym/flow/game.h"

Env::Env() : game(nullptr) {}

std::pair<Observation*, ActionSpace*> Env::reset(const std::vector<PlayerConfig>& player_configs) {
    // If game hasn't been initialized, create a new game
    if (!game) {
        game = std::make_unique<Game>(player_configs, true);
    } else {
        game.reset(new Game(player_configs, true));
    }

    return {game->observation(), game->actionSpace()};
}

std::tuple<Observation*, ActionSpace*, bool> Env::step(int action) {
    // Ensure game is initialized
    if (!game) {
        throw std::runtime_error("Game must be initialized before stepping");
    }

    // Validate action space exists
    ActionSpace* action_space = game->current_action_space.get();
    if (!action_space) {
        throw std::runtime_error("No active action space");
    }

    // Execute action
    bool done = game->step(action);

    // Return observation, action space, and done flag
    return {game->observation(), game->actionSpace(), done};
}