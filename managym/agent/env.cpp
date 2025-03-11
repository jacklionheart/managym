#include "env.h"

#include "managym/flow/game.h"
#include "managym/infra/log.h"

#include <stdexcept>

Env::Env(int seed, bool skip_trivial) : game(nullptr), skip_trivial(skip_trivial), seed(seed) {
    // Seed the random number generator
    std::srand(seed);
    profiler = std::make_unique<Profiler>(true, 50);
}

std::pair<Observation*, std::map<std::string, std::string>>
Env::reset(const std::vector<PlayerConfig>& player_configs) {
    Profiler::Scope scope = profiler->track("env_reset");

    // Destroy any old game
    game.reset(new Game(player_configs, skip_trivial, profiler.get()));

    // Build initial observation
    Observation* obs = game->observation(); // game creates a fresh observation

    // For now, we’ll leave info empty
    std::map<std::string, std::string> info;
    return {obs, info};
}

std::tuple<Observation*, double, bool, bool, std::map<std::string, std::string>>
Env::step(int action, bool skip_trivial) {
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

    std::map<std::string, std::string> info;

    if (done) {
        terminated = true;
        // Identify who won
        int widx = game->winnerIndex(); // -1 if no winner identified
        if (widx >= 0) {
            Player* winner = game->players[widx].get();
            if (winner == agent) {
                reward = 1.0;
            } else {
                reward = -1.0;
            }
            info["winner_name"] = winner->name;
        } else {
            // e.g. a draw
            reward = 0.0;
            info["winner_name"] = "draw";
        }
    }

    if (profiler && profiler->isEnabled()) {
        auto stats = profiler->getStats();
        std::string profStr;
        for (const auto& [label, stat] : stats) {
            profStr += label + ": total=" + std::to_string(stat.total_time) +
                       "s, count=" + std::to_string(stat.count) + "; ";
        }
        info["profiler"] = profStr;
    }
    return std::make_tuple(obs, reward, terminated, truncated, info);
}
