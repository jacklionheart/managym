#include "managym_test.h"

#include <spdlog/spdlog.h>

PlayerConfig makePlayerConfig(const std::string& playerName,
                              const std::vector<TestDeckEntry>& deckEntries) {
    std::map<std::string, int> deckMap;
    for (const auto& entry : deckEntries) {
        deckMap[entry.name] += entry.quantity;
    }
    return PlayerConfig(playerName, deckMap);
}

std::unique_ptr<Game> elvesVsOgres(bool headless, int redMountains, int redOgres, int greenForests,
                                   int greenElves) {
    // Build the Green player's deck
    std::vector<TestDeckEntry> greenDeck{{"Forest", greenForests}, {"Llanowar Elves", greenElves}};
    PlayerConfig greenConfig = makePlayerConfig("Green Mage", greenDeck);

    // Build the Red player's deck
    std::vector<TestDeckEntry> redDeck{{"Mountain", redMountains}, {"Grey Ogre", redOgres}};
    PlayerConfig redConfig = makePlayerConfig("Red Mage", redDeck);

    // Create the game
    std::vector<PlayerConfig> configs = {greenConfig, redConfig};
    auto game = std::make_unique<Game>(configs, headless);
    return game;
}

void putPermanentInPlay(Game* game, Player* player, const std::string& cardName) {
    // Create new card instance
    std::unique_ptr<Card> card = CardRegistry::instance().instantiate(cardName);
    if (!card->types.isPermanent()) {
        throw std::invalid_argument("Card '" + cardName + "' is not a permanent type");
    }

    card->owner = player;

    // Add to player's deck
    player->deck->push_back(std::move(card));
    Card* card_ptr = player->deck->back().get();

    // Move directly to battlefield
    game->zones->move(card_ptr, ZoneType::BATTLEFIELD);
    Permanent* permanent = game->zones->constBattlefield()->find(card_ptr);
    permanent->summoning_sick = false;
}

bool advanceToPhaseStep(Game* game, PhaseType targetPhase, std::optional<StepType> targetStep,
                        bool require_action_space, int maxTicks) {
    if (!game)
        throw std::invalid_argument("Game is null");

    // If step is provided, validate it belongs to the target phase
    if (targetStep && TurnSystem::getPhaseForStep(*targetStep) != targetPhase) {
        throw std::invalid_argument("Target step does not belong to target phase");
    }

    int ticks = 0;
    bool got_to_phase_step = false;

    while (ticks < maxTicks) {
        spdlog::debug("Current phase: {}", toString(game->turn_system->currentPhaseType()));
        spdlog::debug("Current step: {}", toString(game->turn_system->currentStepType()));
        if (game->current_action_space) {
            spdlog::debug("Current action space: {}", game->current_action_space->toString());
        }
        if (game->turn_system->currentPhaseType() == targetPhase) {
            if (!targetStep || game->turn_system->currentStepType() == *targetStep) {
                got_to_phase_step = true;
                if (require_action_space) {
                    if (game->current_action_space != nullptr) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
        } else {
            if (got_to_phase_step) {
                // We went past the target step and didn't find an action space.
                return false;
            }
        }

        // Take one step
        if (game->tick()) {
            return false; // Game ended
        }
        ticks++;
    }

    return false; // Couldn't reach target within maxTicks
}

bool advanceToNextTurn(Game* game, int maxTicks) {
    if (!game)
        return false;

    int currentTurnCount = game->turn_system->global_turn_count;

    int ticks = 0;
    while (ticks < maxTicks) {
        if (game->tick()) {
            return false; // Game ended
        }
        ticks++;

        // Check if we've advanced to a new turn
        if (game->turn_system->global_turn_count > currentTurnCount) {
            return true;
        }
    }

    return false;
}
// Convenience overloads
bool advanceToPhase(Game* game, PhaseType phase, bool require_action_space, int maxTicks) {
    return advanceToPhaseStep(game, phase, std::nullopt, require_action_space, maxTicks);
}
