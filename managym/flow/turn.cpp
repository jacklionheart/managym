#include "turn.h"

#include <spdlog/spdlog.h>

#include "managym/flow/combat.h"
#include "managym/flow/game.h"

TurnSystem::TurnSystem(Game* game) : game(game) {
  for (std::unique_ptr<Player>& player : game->players) {
    turn_counts[player.get()] = 0;
  }

  global_turn_count = 0;
  active_player_index = 0;
}

const Phase* TurnSystem::currentPhase() const {
  if (!current_turn) return nullptr;
  return current_turn->currentPhase();
}

PhaseType TurnSystem::currentPhaseType() const {
  if (!current_turn) return PhaseType::BEGINNING;  // or throw?
  return static_cast<PhaseType>(current_turn->current_phase_index);
}

StepType TurnSystem::currentStepType() const {
  const Phase* phase = currentPhase();
  if (!phase) return StepType::BEGINNING_UNTAP;  // or throw?

  return stepTypeFromIndex(currentPhaseType(), phase->current_step_index);
}

bool TurnSystem::isInPhase(PhaseType phase) const {
  return currentPhaseType() == phase;
}

bool TurnSystem::isInStep(StepType step) const {
  return currentStepType() == step;
}

PhaseType TurnSystem::getPhaseForStep(StepType step) {
  switch (step) {
    case StepType::BEGINNING_UNTAP:
    case StepType::BEGINNING_UPKEEP:
    case StepType::BEGINNING_DRAW:
      return PhaseType::BEGINNING;

    case StepType::MAIN_STEP:
      // Could be either main phase, context needed
      throw std::runtime_error("Main step requires phase context");

    case StepType::COMBAT_BEGIN:
    case StepType::COMBAT_DECLARE_ATTACKERS:
    case StepType::COMBAT_DECLARE_BLOCKERS:
    case StepType::COMBAT_DAMAGE:
    case StepType::COMBAT_END:
      return PhaseType::COMBAT;

    case StepType::ENDING_END:
    case StepType::ENDING_CLEANUP:
      return PhaseType::ENDING;
  }
  throw std::runtime_error("Unknown step type");
}

StepType TurnSystem::stepTypeFromIndex(PhaseType phase, int stepIndex) {
  switch (phase) {
    case PhaseType::BEGINNING:
      switch (stepIndex) {
        case 0:
          return StepType::BEGINNING_UNTAP;
        case 1:
          return StepType::BEGINNING_UPKEEP;
        case 2:
          return StepType::BEGINNING_DRAW;
        default:
          throw std::out_of_range("Invalid beginning phase step index");
      }

    case PhaseType::PRECOMBAT_MAIN:
    case PhaseType::POSTCOMBAT_MAIN:
      if (stepIndex == 0) return StepType::MAIN_STEP;
      throw std::out_of_range("Invalid main phase step index");

    case PhaseType::COMBAT:
      switch (stepIndex) {
        case 0:
          return StepType::COMBAT_BEGIN;
        case 1:
          return StepType::COMBAT_DECLARE_ATTACKERS;
        case 2:
          return StepType::COMBAT_DECLARE_BLOCKERS;
        case 3:
          return StepType::COMBAT_DAMAGE;
        case 4:
          return StepType::COMBAT_END;
        default:
          throw std::out_of_range("Invalid combat phase step index");
      }

    case PhaseType::ENDING:
      switch (stepIndex) {
        case 0:
          return StepType::ENDING_END;
        case 1:
          return StepType::ENDING_CLEANUP;
        default:
          throw std::out_of_range("Invalid ending phase step index");
      }
  }
  throw std::runtime_error("Unknown phase type");
}

int TurnSystem::stepIndexFromType(StepType step) {
  switch (step) {
    case StepType::BEGINNING_UNTAP:
      return 0;
    case StepType::BEGINNING_UPKEEP:
      return 1;
    case StepType::BEGINNING_DRAW:
      return 2;

    case StepType::MAIN_STEP:
      return 0;

    case StepType::COMBAT_BEGIN:
      return 0;
    case StepType::COMBAT_DECLARE_ATTACKERS:
      return 1;
    case StepType::COMBAT_DECLARE_BLOCKERS:
      return 2;
    case StepType::COMBAT_DAMAGE:
      return 3;
    case StepType::COMBAT_END:
      return 4;

    case StepType::ENDING_END:
      return 0;
    case StepType::ENDING_CLEANUP:
      return 1;
  }
  throw std::runtime_error("Unknown step type");
}

std::unique_ptr<ActionSpace> TurnSystem::tick() {
  if (current_turn == nullptr || current_turn->isComplete()) {
    startNextTurn();
  }

  return current_turn->tick();
}

std::unique_ptr<ActionSpace> TurnSystem::startNextTurn() {
  if (global_turn_count != 0) {
    active_player_index = (active_player_index + 1) % game->players.size();
  }
  current_turn = std::make_unique<Turn>(activePlayer(), this);
  turn_counts[activePlayer()]++;
  global_turn_count++;

  return nullptr;
}

std::vector<Player*> TurnSystem::priorityOrder() {
  std::vector<Player*> order;
  int num_players = game->players.size();

  for (int i = 0; i < num_players; i++) {
    order.push_back(
        game->players[(active_player_index + i) % num_players].get());
  }

  return order;
}

Player* TurnSystem::activePlayer() {
  return game->players.at(active_player_index).get();
}

Player* TurnSystem::nonActivePlayer() {
  return game->players.at((active_player_index + 1) % game->players.size())
      .get();
}

Turn::Turn(Player* player, TurnSystem* turn_system)
    : active_player(player), turn_system(turn_system) {
  phases.emplace_back(new BeginningPhase(this));
  phases.emplace_back(new PrecombatMainPhase(this));
  phases.emplace_back(new CombatPhase(this));
  phases.emplace_back(new PostcombatMainPhase(this));
  phases.emplace_back(new EndingPhase(this));
}

std::unique_ptr<ActionSpace> Turn::tick() {
  if (current_phase_index >= phases.size()) {
    return nullptr;
  }

  Phase* current_phase = phases[current_phase_index].get();

  if (current_phase->isComplete()) {
    current_phase_index++;
    return nullptr;
  } else {
    return current_phase->tick();
  }
}

std::unique_ptr<ActionSpace> Phase::tick() {
  spdlog::debug("Ticking {}", std::string(typeid(*this).name()));

  if (current_step_index >= steps.size()) {
    return nullptr;
  }

  Step* current_step = steps[current_step_index].get();
  if (current_step->isComplete()) {
    current_step_index++;
    return nullptr;
  } else {
    return current_step->tick();
  }
}

bool Step::isComplete() {
  return turn_based_actions_complete &&
         (!has_priority_window || priority_system->isComplete()) &&
         mana_pools_emptied;
}

std::unique_ptr<ActionSpace> Step::tick() {
  spdlog::debug("Ticking {}", std::string(typeid(*this).name()));

  if (!initialized) {
    initialize();
    initialized = true;
  }

  if (isComplete()) {
    throw std::runtime_error("Step is complete");
  }

  if (!turn_based_actions_complete) {
    return performTurnBasedActions();
  }

  if (has_priority_window && !priority_system->isComplete()) {
    return priority_system->tick();
  }

  game()->clearManaPools();
  mana_pools_emptied = true;
  return nullptr;
}

// Implementations of specific Steps

std::unique_ptr<ActionSpace> UntapStep::performTurnBasedActions() {
  spdlog::debug("Starting {} for {}", std::string(typeid(*this).name()),
               activePlayer()->name);
  game()->markPermanentsNotSummoningSick(activePlayer());
  game()->untapAllPermanents(activePlayer());
  turn_based_actions_complete = true;
  return nullptr;
}

std::unique_ptr<ActionSpace> DrawStep::performTurnBasedActions() {
  game()->drawCards(activePlayer(), 1);
  turn_based_actions_complete = true;
  return nullptr;
}

std::unique_ptr<ActionSpace> CleanupStep::performTurnBasedActions() {
  game()->clearDamage();
  turn_based_actions_complete = true;
  return nullptr;
  ;
}