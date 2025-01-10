#include "managym_test.h"

#include "managym/cardsets/alpha/alpha.h"
#include "managym/cardsets/basics/basic_lands.h"
#include "managym/state/card_registry.h"

PlayerConfig makePlayerConfig(const std::string& playerName,
                              const std::vector<TestDeckEntry>& deckEntries) {
  std::map<std::string, int> deckMap;
  for (const auto& entry : deckEntries) {
    deckMap[entry.name] += entry.quantity;
  }
  return PlayerConfig(playerName, deckMap);
}

std::unique_ptr<Game> elvesVsOgres(bool headless, int redMountains,
                                   int redOgres, int greenForests,
                                   int greenElves) {
  // Build the Red player's deck
  std::vector<TestDeckEntry> redDeck{{"Mountain", redMountains},
                                     {"Grey Ogre", redOgres}};
  PlayerConfig redConfig = makePlayerConfig("Red Mage", redDeck);

  // Build the Green player's deck
  std::vector<TestDeckEntry> greenDeck{{"Forest", greenForests},
                                       {"Llanowar Elves", greenElves}};
  PlayerConfig greenConfig = makePlayerConfig("Green Mage", greenDeck);

  std::vector<PlayerConfig> configs = {redConfig, greenConfig};
  auto game = std::make_unique<Game>(configs, headless);
  return game;
}

bool advanceToNextTurn(Game* game, int maxTicks) {
  if (!game) return false;

  int currentTurnCount = game->turn_system->global_turn_count;

  int ticks = 0;
  while (ticks < maxTicks) {
    if (!game->tick()) return false;
    ticks++;

    // Check if we've advanced to a new turn
    if (game->turn_system->global_turn_count > currentTurnCount) {
      return true;
    }
  }

  return false;
}

// And advanceToPhaseStep no longer needs to handle turn start
bool advanceToPhaseStep(Game* game, PhaseType targetPhase,
                        std::optional<StepType> targetStep, int maxTicks) {
  if (!game) return false;

  // If step is provided, validate it belongs to the target phase
  if (targetStep && TurnSystem::getPhaseForStep(*targetStep) != targetPhase) {
    throw std::invalid_argument("Target step does not belong to target phase");
  }

  int ticks = 0;
  while (ticks < maxTicks) {
    // Check if we've reached target
    if (game->turn_system->currentPhaseType() == targetPhase) {
      if (!targetStep) {
        return true;  // No specific step required
      }

      // Check if we're at the right step
      if (game->turn_system->currentStepType() == *targetStep) {
        return true;
      }
    }

    // Take one step
    if (!game->tick()) return false;
    ticks++;
  }

  return false;  // Couldn't reach target within maxTicks
}
// Convenience overloads
bool advanceToPhase(Game* game, PhaseType phase, int maxTicks) {
  return advanceToPhaseStep(game, phase, std::nullopt, maxTicks);
}

bool advanceToStep(Game* game, StepType step, int maxTicks) {
  return advanceToPhaseStep(game, TurnSystem::getPhaseForStep(step), step,
                            maxTicks);
}
