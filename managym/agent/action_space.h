#pragma once

#include "managym/agent/action.h"

#include <memory>
#include <vector>

// Collection of possible actions for a game decision point
struct ActionSpace {
    ActionSpace(ActionType action_type, std::vector<std::unique_ptr<Action>>&& actions,
                Player* player, Game* game)
        : player(player), action_type(action_type), actions(std::move(actions)), game(game) {}
    virtual ~ActionSpace() = default;

    // Data
    Player* player;
    ActionType action_type;
    Permanent* focus;
    int chosen_index = -1;
    std::vector<std::unique_ptr<Action>> actions;
    Game* game;

    // Writes
    // Select an action from the available choices
    void selectAction(int index) { chosen_index = index; }

    // Reads
    // Check if an action has been selected
    bool actionSelected() { return chosen_index != -1; }
    // Check if there are no available actions
    bool empty();
    std::string toString() const;

    // Create an action space with ActionType::Invalid and no actions.
    // Returned when the game is over.
    static std::unique_ptr<ActionSpace> createEmpty() {
        return std::make_unique<ActionSpace>(ActionType::Invalid, std::vector<std::unique_ptr<Action>>(),
                                             nullptr, // No player
                                             nullptr  // No game
        );
    }
};
