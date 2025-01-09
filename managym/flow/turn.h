#pragma once

#include <map>
#include <memory>
#include <vector>

#include "managym/flow/priority.h"

class ActionSpace;
class Step;
class Phase;
class Turn;
class Player;
class Game;

struct TurnSystem {
  std::unique_ptr<Turn> current_turn;
  int active_player_index;
  int global_turn_count;
  std::map<Player*, int> turn_counts;
  Game* game;

  Player* activePlayer();
  Player* nonActivePlayer();

  TurnSystem(Game* game);
  std::unique_ptr<ActionSpace> tick();
  std::unique_ptr<ActionSpace> startNextTurn();
  std::vector<Player*> priorityOrder();
};

struct Turn {
  TurnSystem* turn_system;
  Player* active_player;

  std::vector<std::unique_ptr<Phase>> phases;
  int current_phase_index = 0;
  int lands_played = 0;

  Turn(Player* active_player, TurnSystem* turn_system);
  std::unique_ptr<ActionSpace> tick();
  bool isComplete() { return current_phase_index >= phases.size(); }
};

struct Phase {
  Turn* turn;

  std::vector<std::unique_ptr<Step>> steps;
  int current_step_index = 0;

  Phase(Turn* turn) : turn(turn) {}

  bool isComplete() { return current_step_index >= steps.size(); }

  std::unique_ptr<ActionSpace> tick();
  virtual ~Phase() = default;
};

struct Step {
  Phase* phase;
  bool initialized = false;
  bool has_priority_window = true;
  bool turn_based_actions_complete = false;
  bool mana_pools_emptied = false;

  std::unique_ptr<PrioritySystem> priority_system;

  bool isComplete();

  virtual void initialize() {}

  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() {
    return nullptr;
  };

  Step(Phase* phase)
      : phase(phase),
        priority_system(std::make_unique<PrioritySystem>(
            phase->turn->turn_system->game, phase->turn->active_player)) {}

  // Inline reference accesors
  Game* game() { return phase->turn->turn_system->game; }
  TurnSystem* turn_system() { return phase->turn->turn_system; }
  Turn* turn() { return phase->turn; }
  Player* activePlayer() { return phase->turn->active_player; }

  std::unique_ptr<ActionSpace> tick();
  virtual ~Step() = default;
};

// Define specific Steps and Phases

// BeginningSteps

struct UntapStep : public Step {
  UntapStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = false;
  }

  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

struct UpkeepStep : public Step {
  UpkeepStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = true;
  }
};

struct DrawStep : public Step {
  DrawStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = true;
  }

  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Main Step

struct MainStep : public Step {
  bool can_cast_sorceries = true;
  MainStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = true;
    can_cast_sorceries = true;
  }
};

// Ending Steps

struct EndStep : public Step {
  EndStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = true;
  }
};

struct CleanupStep : public Step {
  CleanupStep(Phase* parent_phase) : Step(parent_phase) {
    has_priority_window = false;
  }

  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Phases
struct BeginningPhase : public Phase {
  BeginningPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new UntapStep(this));
    steps.emplace_back(new UpkeepStep(this));
    steps.emplace_back(new DrawStep(this));
  }
};

struct PrecombatMainPhase : public Phase {
  PrecombatMainPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new MainStep(this));
  }
};

struct PostcombatMainPhase : public Phase {
  PostcombatMainPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new MainStep(this));
  }
};

struct EndingPhase : public Phase {
  EndingPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new EndStep(this));
    steps.emplace_back(new CleanupStep(this));
  }
};
