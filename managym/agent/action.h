#pragma once

#include "managym/state/game_object.h"

#include <string>
#include <vector>

class Game;
class Player;
class PrioritySystem;
class Card;
class DeclareBlockersStep;
class DeclareAttackersStep;
class Permanent;

// Types of actions available to players
enum struct ActionType {
    PRIORITY_PLAY_LAND,
    PRIORITY_CAST_SPELL,
    PRIORITY_PASS_PRIORITY,
    DECLARE_ATTACKER,
    DECLARE_BLOCKER,
};

inline std::string toString(ActionType type) {
    switch (type) {
    case ActionType::PRIORITY_PLAY_LAND:
        return "priority_play_land";
    case ActionType::PRIORITY_CAST_SPELL:
        return "priority_cast_spell";
    case ActionType::PRIORITY_PASS_PRIORITY:
        return "priority_pass_priority";
    case ActionType::DECLARE_ATTACKER:
        return "declare_attacker";
    case ActionType::DECLARE_BLOCKER:
        return "declare_blocker";
    default:
        return "unknown";
    }
}

// Base struct for all game actions performed by agents
struct Action {
    Action(Player* player, ActionType type) : player(player), type(type) {}
    virtual ~Action() = default;

    // Data
    Player* player;
    ActionType type;

    // Reads
    virtual std::string toString() const = 0;
    virtual std::vector<ObjectId> focus() const = 0;

    // Writes
    // Execute this action's game effects
    virtual void execute() = 0;
};

// Action for declaring an attacking creature
struct DeclareAttackerAction : public Action {
    DeclareAttackerAction(Permanent* attacker, bool attack, Player* player,
                          DeclareAttackersStep* step)
        : Action(player, ActionType::DECLARE_ATTACKER), attacker(attacker), step(step),
          attack(attack) {}

    // Data
    Permanent* attacker;
    DeclareAttackersStep* step;
    bool attack;

    // Reads
    std::string toString() const override;
    std::vector<ObjectId> focus() const override;

    // Writes
    // Execute the attack declaration
    void execute() override;
};

// Action for declaring a blocking creature
struct DeclareBlockerAction : public Action {
    DeclareBlockerAction(Permanent* blocker, Permanent* attacker, Player* player,
                         DeclareBlockersStep* step)
        : Action(player, ActionType::DECLARE_BLOCKER), blocker(blocker), attacker(attacker),
          step(step) {}

    // Data
    Permanent* blocker;
    Permanent* attacker;
    DeclareBlockersStep* step;

    // Reads
    std::string toString() const override;
    std::vector<ObjectId> focus() const override;
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
    std::vector<ObjectId> focus() const override;
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
    std::vector<ObjectId> focus() const override;
    // Writes

    // Execute the spell cast
    void execute() override;
};

// Action for passing priority
struct PassPriority : public Action {
    PassPriority(Player* player, Game* game);

    // Data
    Game* game;

    // Reads
    std::string toString() const override;
    std::vector<ObjectId> focus() const override;
    // Writes

    // Execute the priority pass
    void execute() override;
};

// Thrown when an invalid action is taken.
class AgentError : public std::runtime_error {
public:
    explicit AgentError(const std::string& message) : std::runtime_error(message) {}
};
