#pragma once

#include <memory>
#include <vector>

class Player;
class PrioritySystem;
class Card;
class Game;
class DeclareBlockersStep;
class DeclareAttackersStep;
class Permanent;

// Base struct for all game actions
struct Action {
    // Data
    Player* player;

    // Constructor
    Action(Player* player) : player(player) {}
    virtual ~Action() = default;

    // Writes
    // Execute this action's game effects
    virtual void execute() {}
};

// Types of actions available to players
enum struct ActionType {
    Priority,
    DeclareAttacker,
    DeclareBlocker,
};

// Collection of possible actions for a game decision point
struct ActionSpace {
    ActionSpace(Player* player, ActionType action_type,
                std::vector<std::unique_ptr<Action>>&& actions)
        : player(player), action_type(action_type), actions(std::move(actions)) {}
    virtual ~ActionSpace() = default;

    // Data
    Player* player;
    ActionType action_type;
    Permanent* focus;
    int chosen_index = -1;
    std::vector<std::unique_ptr<Action>> actions;

    // Writes
    // Select an action from the available choices
    void selectAction(int index) { chosen_index = index; }

    // Reads
    // Check if an action has been selected
    bool actionSelected() { return chosen_index != -1; }
    // Check if there are no available actions
    bool empty();
};

// Action for declaring an attacking creature
struct DeclareAttackerAction : public Action {
    DeclareAttackerAction(Permanent* attacker, bool attack, Player* player,
                          DeclareAttackersStep* step)
        : Action(player), attacker(attacker), step(step), attack(attack) {}

    // Data
    Permanent* attacker;
    DeclareAttackersStep* step;
    bool attack;

    // Writes
    // Execute the attack declaration
    void execute() override;
};

// Action for declaring a blocking creature
struct DeclareBlockerAction : public Action {
    DeclareBlockerAction(Permanent* blocker, Permanent* attacker, Player* player,
                         DeclareBlockersStep* step)
        : Action(player), blocker(blocker), attacker(attacker), step(step) {}

    // Data
    Permanent* blocker;
    Permanent* attacker;
    DeclareBlockersStep* step;

    // Writes
    // Execute the block declaration
    void execute() override;
};

// Action for playing a land
struct PlayLand : public Action {
    PlayLand(Card* card, Player* player, Game* game);

    // Data
    Game* game;
    Card* card;

    // Writes
    // Execute the land play
    void execute() override;
};

// Action for casting a spell
struct CastSpell : public Action {
    CastSpell(Card* card, Player* player, Game* game);

    // Data
    Game* game;
    Card* card;

    // Writes

    // Execute the spell cast
    void execute() override;
};

// Action for passing priority
struct PassPriority : public Action {
    PassPriority(Player* player, PrioritySystem* priority_system);

    // Data
    PrioritySystem* priority_system;

    // Writes

    // Execute the priority pass
    void execute() override;
};

// Interface for AI agents making game decisions
struct Agent {
    // Data
    Action* chosen_action;

    // Writes
    // Choose an action from available options
    void selectAction(ActionSpace* action_space);

    // Reads
    // Get the currently selected action
    Action* chosenAction() { return chosen_action; }
};