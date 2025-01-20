#pragma once
#include <memory>

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

protected:
    // Helper methods
    void performStateBasedActions();
    std::unique_ptr<ActionSpace> makeActionSpace(Player* player);
    void resolveTopOfStack();
    std::vector<std::unique_ptr<Action>> availablePriorityActions(Player* player);
};