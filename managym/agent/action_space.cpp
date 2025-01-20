#include "action_space.h"

#include "managym/state/player.h"

#include <fmt/format.h>

bool ActionSpace::empty() { return actions.empty(); }

std::string ActionSpace::toString() const {
    std::string actions_str = "[\n";
    for (size_t i = 0; i < actions.size(); i++) {
        if (chosen_index == i) {
            actions_str += "  *"; // Mark chosen action with asterisk
        } else {
            actions_str += "   ";
        }
        actions_str += actions[i]->toString();
        if (i < actions.size() - 1) {
            actions_str += ",";
        }
        actions_str += "\n";
    }
    actions_str += "]";

    std::string name = player ? player->name : "None";

    return fmt::format("ActionSpace(type={}, player={}, actions={})", ::toString(action_type), name,
                       actions_str);
}