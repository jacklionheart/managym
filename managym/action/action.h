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

struct Action {
  Player* player;
  Action(Player* player) : player(player) {}
  virtual void execute() {}
  virtual ~Action() = default;
};

enum struct ActionType {
  Priority,
  DeclareAttacker,
  DeclareBlocker,
};

struct ActionSpace {
  Player* player;
  ActionType action_type;
  Permanent* focus;
  std::vector<std::unique_ptr<Action>> actions;
  ActionSpace(Player* player, ActionType action_type,
              std::vector<std::unique_ptr<Action>>&& actions)
      : player(player), action_type(action_type), actions(std::move(actions)) {}
  virtual ~ActionSpace() = default;
  bool empty();
};

// Combat actions

struct DeclareAttackerAction : public Action {
  Permanent* attacker;
  DeclareAttackersStep* step;
  bool attack;
  virtual void execute() override;

  DeclareAttackerAction(Permanent* attacker, bool attack, Player* player,
                        DeclareAttackersStep* step)
      : Action(player), attacker(attacker), step(step), attack(attack) {}
};

struct DeclareBlockerAction : public Action {
  Permanent* blocker;
  Permanent* attacker;
  DeclareBlockersStep* step;

  virtual void execute() override;

  DeclareBlockerAction(Permanent* blocker, Permanent* attacker, Player* player,
                       DeclareBlockersStep* step)
      : Action(player), blocker(blocker), attacker(attacker), step(step) {}
};

// Priority actions

struct PlayLand : public Action {
  Game* game;
  Card* card;

  PlayLand(Card* card, Player* player, Game* game);

  void execute() override;
};

struct CastSpell : public Action {
  Game* game;
  Card* card;

  CastSpell(Card* card, Player* player, Game* game);

  void execute() override;
};

struct PassPriority : public Action {
  PrioritySystem* priority_system;

  PassPriority(Player* player, PrioritySystem* priority_system);

  void execute() override;
};