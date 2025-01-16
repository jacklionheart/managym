#pragma once

#include "managym/agent/action.h"
#include "managym/state/card.h"
#include "managym/state/zones.h"

#include <optional>
#include <vector>

// Forward declarations
class Game;
class ActionSpace;

// Represents a complete game state observation for ML agents
struct Observation {
    Observation(const Game* game, const ActionSpace* action_space = nullptr);

    // Data

    // Gameflow state
    int turn_number;
    int active_player_id;
    bool is_game_over;
    ActionType action_type; // Current type of decision being made
    int winner_id;

    struct CardState {
        int card_id;
        std::optional<int> instance_id; // For permanents
        std::string name;
        bool tapped;
        bool summoning_sick;
        int damage;
        bool attacking;
        bool blocking;

        std::string toJSON() const;
    };

    struct PlayerState {
        int life_total;
        int library_size;
        Mana mana_pool;
        std::map<ZoneType, std::vector<CardState>> zones; // Changed this - organize cards by zone

        std::string toJSON() const;
    };

    std::vector<PlayerState> player_states;

    // Reads

    // Export to JSON
    std::string toJSON() const;
};
