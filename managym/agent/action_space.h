#pragma once

#include "managym/agent/action.h"

#include <memory>
#include <vector>

enum struct ActionSpaceType {
    GAME_OVER,
    PRIORITY,
    DECLARE_ATTACKER,
    DECLARE_BLOCKER,
};

inline std::string toString(ActionSpaceType type) {
    switch (type) {
    case ActionSpaceType::GAME_OVER:
        return "game_over";
    case ActionSpaceType::PRIORITY:
        return "priority";
    case ActionSpaceType::DECLARE_ATTACKER:
        return "declare_attacker";
    case ActionSpaceType::DECLARE_BLOCKER:
        return "declare_blocker";
    default:
        return "unknown";
    }
}

// Collection of possible actions for a game decision point
struct ActionSpace {
    ActionSpace(ActionSpaceType type, std::vector<std::unique_ptr<Action>>&& actions,
                Player* player)
        : player(player), type(type), actions(std::move(actions)) {}
    virtual ~ActionSpace() = default;

    // Data
    Player* player;
    ActionSpaceType type;
    std::vector<std::unique_ptr<Action>> actions;

    // Check if there are no available actions
    bool empty();
    std::string toString() const;

    // Create an action space with ActionType::Invalid and no actions.
    // Returned when the game is over.
    static std::unique_ptr<ActionSpace> createEmpty() {
        return std::make_unique<ActionSpace>(ActionSpaceType::GAME_OVER,
                                             std::vector<std::unique_ptr<Action>>(), nullptr);
    }
};
