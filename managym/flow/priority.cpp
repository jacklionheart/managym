#include "priority.h"

#include "managym/agent/action_space.h"
#include "managym/flow/game.h"
#include "managym/state/zones.h"

#include <spdlog/spdlog.h>

#include <memory>

PrioritySystem::PrioritySystem(Game* game, Player* active_player)
    : game(game), active_player(active_player) {
    reset();
}

std::unique_ptr<ActionSpace> PrioritySystem::makeActionSpace(Player* player) {
    return std::make_unique<ActionSpace>(ActionType::Priority,
                                         availablePriorityActions(player), player, game);
}

bool PrioritySystem::stackEmpty() {
    return game->zones->size(ZoneType::STACK, game->activePlayer()) == 0;
}

void PrioritySystem::reset() {
    state_based_actions_complete = false;
    for (Player* player : game->priorityOrder()) {
        passed[player] = false;
    }
}
bool PrioritySystem::hasPriority(Player* player) { return player == playerWithPriority(); }

void PrioritySystem::passPriority(Player* player) {
    if (!hasPriority(player)) {
        throw std::runtime_error("Player does not have priority");
    }
    passed[player] = true;
}

Player* PrioritySystem::playerWithPriority() {
    std::vector<Player*> players = game->priorityOrder();
    for (Player* player : players) {
        if (!passed[player]) {
            return player;
        }
    }
    throw std::runtime_error("No player has priority");
}

bool PrioritySystem::playersPassed() {
    for (Player* player : game->priorityOrder()) {
        if (!passed[player]) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<ActionSpace> PrioritySystem::tick() {
    spdlog::debug("Ticking {}", std::string(typeid(*this).name()));

    if (isComplete()) {
        throw std::runtime_error("Priority system is complete");
    }

    if (!state_based_actions_complete) {
        state_based_actions_complete = true;
        return performStateBasedActions();
    }

    if (!passed[game->activePlayer()]) {
        return makeActionSpace(game->activePlayer());
    } else if (!passed[game->nonActivePlayer()]) {
        return makeActionSpace(game->nonActivePlayer());
    }

    std::unique_ptr<ActionSpace> result = resolveTopOfStack();
    reset();
    return result;
}

std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    // Get cards in hand
    const std::vector<Card*> hand_cards = game->zones->constHand()->cards.at(player);

    if (game->canCastSorceries(player)) {
        spdlog::debug("Producible mana: {}",
                      game->zones->constBattlefield()->producibleMana(player).toString());
    }

    for (Card* card : hand_cards) {
        if (card == NULL) {
            throw std::logic_error("Card should never be null");
        }
        if (card->types.isLand() && game->canPlayLand(player)) {
            actions.push_back(std::make_unique<PlayLand>(card, player, game));
        } else if (card->types.isCastable() && game->canCastSorceries(player)) {
            Mana producible_mana = game->zones->constBattlefield()->producibleMana(player);

            if (game->canPayManaCost(player, card->mana_cost.value())) {
                actions.push_back(std::make_unique<CastSpell>(card, player, game));
            }
        }
    }

    // Always allow passing priority
    actions.push_back(std::make_unique<PassPriority>(player, this));

    return actions;
}

std::unique_ptr<ActionSpace> PrioritySystem::performStateBasedActions() {
    std::vector<Player*> players = game->priorityOrder();

    // MR04.5a If a player has 0 or less life, that player loses the game->
    for (Player* player : players) {
        if (player->life <= 0) {
            game->loseGame(player);
        }
    }

    // MR704.5b 704.5b If a player attempted to draw a card from a library with no
    // cards in it since the last time state-based actions were checked, that
    // player loses the game->
    //  TODO: Move from drawCards

    // 704.5g If a creature has toughness greater than 0, it has damage marked on
    // it, and the total damage marked on it is greater than or equal to its
    // toughness, that creature has been dealt lethal damage and is destroyed.
    // Regeneration can replace this event Collect permanents to be destroyed
    std::vector<Permanent*> permanents_to_destroy;

    for (Player* player : players) {
        game->zones->forEachPermanent(
            [&](Permanent* permanent) {
                if (permanent->hasLethalDamage()) {
                    permanents_to_destroy.push_back(permanent);
                }
            },
            player);
    }

    // Destroy permanents after iteration
    for (Permanent* permanent : permanents_to_destroy) {
        spdlog::info("{} has lethal damage and is destroyed", permanent->card->toString());
        game->zones->destroy(permanent);
    }

    return nullptr;
}

std::unique_ptr<ActionSpace> PrioritySystem::resolveTopOfStack() {
    if (game->zones->size(ZoneType::STACK, game->activePlayer()) > 0) {
        Card* card = game->zones->popStack();
        spdlog::info("Casting {}", card->toString());
        if (card->types.isPermanent()) {
            game->zones->move(card, ZoneType::BATTLEFIELD);
        } else {
            // TODO
        }
    }
    return nullptr;
}
