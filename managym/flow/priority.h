#pragma once
#include <memory>
#include <utility>
#include <vector>

class Game;
class Player;
class ActionSpace;
class Action;

// System for managing priority passing between players
struct PrioritySystem {
    // Data
    Game* game;
    int pass_count = 0;
    bool sba_done = false;

    // Constructor
    explicit PrioritySystem(Game* game);

    // Writes

    // Reset priority system so that state-based actions will be performed again and all players
    // will receive priority.
    void reset();
    // Pass priority to the next player.
    void passPriority();

    // Returns the priority action space for the current player or nullptr if all players have
    // passed and
    std::unique_ptr<ActionSpace> tick();

    // Reads

    // Return true if priority round is complete - all players passed and stack is empty
    bool isComplete() const;
    // Return true if player has any action beyond passing priority.
    bool canPlayerAct(Player* player);

protected:
    // Helper methods
    void performStateBasedActions();
    void resolveTopOfStack();

    // Computes available actions for a player in a single pass.
    // Returns pair: first = true if player has actions beyond PassPriority, second = action list.
    std::pair<bool, std::vector<std::unique_ptr<Action>>> computePlayerActions(Player* player);
};
