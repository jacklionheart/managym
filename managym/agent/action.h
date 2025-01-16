#pragma once

#include <string>
class Game;
class Player;
class PrioritySystem;
class Card;
class DeclareBlockersStep;
class DeclareAttackersStep;
class Permanent;

// Base struct for all game actions performed by agents
struct Action {
    // Data
    Player* player;

    // Constructor
    Action(Player* player) : player(player) {}
    virtual ~Action() = default;

    // Reads
    virtual std::string toString() const { return "Action"; }

    // Writes
    // Execute this action's game effects
    virtual void execute() {}
};

// Types of actions available to players
enum struct ActionType {
    Invalid,
    Priority,
    DeclareAttacker,
    DeclareBlocker,
};

inline std::string toString(ActionType type) {
    switch (type) {
    case ActionType::Invalid:
        return "Invalid";
    case ActionType::Priority:
        return "Priority";
    case ActionType::DeclareAttacker:
        return "DeclareAttacker";
    case ActionType::DeclareBlocker:
        return "DeclareBlocker";
    default:
        return "Unknown";
    }
}

// Action for declaring an attacking creature
struct DeclareAttackerAction : public Action {
    DeclareAttackerAction(Permanent* attacker, bool attack, Player* player,
                          DeclareAttackersStep* step)
        : Action(player), attacker(attacker), step(step), attack(attack) {}

    // Data
    Permanent* attacker;
    DeclareAttackersStep* step;
    bool attack;

    // Reads
    std::string toString() const override;

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

    // Reads
    std::string toString() const override;

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

    // Reads
    std::string toString() const override;

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

    // Reads
    std::string toString() const override;

    // Writes

    // Execute the spell cast
    void execute() override;
};

// Action for passing priority
struct PassPriority : public Action {
    PassPriority(Player* player, PrioritySystem* priority_system);

    // Data
    PrioritySystem* priority_system;

    // Reads
    std::string toString() const override;

    // Writes

    // Execute the priority pass
    void execute() override;
};
