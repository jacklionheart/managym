#include "managym_test.h"

#include "managym/infra/log.h"

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
    std::unique_ptr<Card> card = game->card_registry->instantiate(cardName);
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
                        int maxTicks) {
    managym::log::debug(Category::TURN, "Advancing to phase step: {} {}, maxTicks={}",
                        toString(targetPhase), targetStep ? toString(*targetStep) : "none",
                        maxTicks);
    if (!game)
        throw std::invalid_argument("Game is null");

    // If step is provided, validate it belongs to the target phase
    if (targetStep && TurnSystem::getPhaseForStep(*targetStep) != targetPhase) {
        throw std::invalid_argument("Target step does not belong to target phase");
    }

    int ticks = 0;

    while (ticks < maxTicks) {
        managym::log::debug(Category::TURN, "Current phase: {}",
                            toString(game->turn_system->currentPhaseType()));
        managym::log::debug(Category::TURN, "Current step: {}",
                            toString(game->turn_system->currentStepType()));

        if (game->current_action_space) {
            managym::log::debug(Category::TURN, "Current action space: {}",
                                game->current_action_space->toString());
        }
        if (game->turn_system->currentPhaseType() == targetPhase) {
            if (!targetStep || game->turn_system->currentStepType() == *targetStep) {
                return true;
            }
        }

        // Take one step
        bool game_over = game->step(0);
        if (game_over) {
            managym::log::debug(Category::TURN, "Game over in advanceToPhaseStep");
            return false;
        }
        ticks++;
    }

    managym::log::debug(Category::TURN, "Couldn't advance to target phase step within maxTicks");
    return false;
}

bool advanceToNextTurn(Game* game, int maxTicks) {
    if (!game)
        return false;

    int currentTurnCount = game->turn_system->global_turn_count;

    int ticks = 0;
    while (ticks < maxTicks) {
        bool game_over = game->step(0);
        if (game_over) {
            managym::log::debug(Category::TURN, "Game over in advanceToNextTurn");
            return false;
        }

        if (game->turn_system->global_turn_count > currentTurnCount) {
            return true;
        }

        ticks++;
    }

    managym::log::debug(Category::TURN, "Couldn't advance to next turn within maxTicks");
    return false;
}
// Convenience overloads
bool advanceToPhase(Game* game, PhaseType phase, int maxTicks) {
    return advanceToPhaseStep(game, phase, std::nullopt, maxTicks);
}
