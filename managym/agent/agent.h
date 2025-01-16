#pragma once

#include "managym/agent/action_space.h"

// Interface for agents making game decisions
struct Agent {
    virtual ~Agent() = default;

    // Data
    Action* chosen_action;

    // Writes
    // Choose an action from available options
    void selectAction(ActionSpace* action_space);

    // Reads
    // Get the currently selected action
    Action* chosenAction() { return chosen_action; }
};