#pragma once
#include <memory>
#include <map>

class Game; 
class Player;
class ActionSpace;
class Action;
class PrioritySystem {
public:
    Game* game;
    Player* active_player;

    std::map<Player*, bool> passed;
    bool state_based_actions_complete = false;

    PrioritySystem(Game* game, Player* active_player);

    std::unique_ptr<ActionSpace> tick();

    void passPriority(Player* player);

    bool playersPassed();
    Player* playerWithPriority();
    bool hasPriority(Player* player);

    void reset();
    bool stackEmpty();
    bool isComplete() { return state_based_actions_complete && playersPassed() && stackEmpty(); }

    std::vector<std::unique_ptr<Action>> availablePriorityActions(Player* player);

    std::unique_ptr<ActionSpace> makeActionSpace(Player* player);
    std::unique_ptr<ActionSpace> performStateBasedActions();
};