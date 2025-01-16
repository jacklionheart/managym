#include "agent.h"

void Agent::selectAction(ActionSpace* action_space) {
    if (action_space->empty()) {
        throw std::logic_error("No actions in action space");
    }
    action_space->selectAction(0);
}
