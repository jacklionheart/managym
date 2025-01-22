#include "managym/flow/priority.h"

#include "managym/agent/action_space.h"
#include "managym/flow/game.h"
#include "managym/infra/log.h"
#include "managym/state/zones.h"

PrioritySystem::PrioritySystem(Game* game) : game(game) {}

void PrioritySystem::reset() {
    pass_count = 0;
    sba_done = false;
}

bool PrioritySystem::isComplete() const {
    std::vector<Player*> players = game->priorityOrder();
    return pass_count >= players.size() && game->zones->constStack()->objects.empty();
}

std::unique_ptr<ActionSpace> PrioritySystem::tick() {
    managym::log::debug(Category::PRIORITY, "Ticking PrioritySystem (pass_count={})", pass_count);

    if (!sba_done) {
        performStateBasedActions();
        sba_done = true;
    }

    std::vector<Player*> players = game->priorityOrder();
    if (pass_count < players.size()) {
        return makeActionSpace(players[pass_count]);
    }

    managym::log::debug(Category::PRIORITY, "All players have passed");
    reset();

    if (!game->zones->constStack()->objects.empty()) {
        managym::log::debug(Category::PRIORITY, "Resolving stack object");
        resolveTopOfStack();
        // Restart priority system now with stack -1
        return tick();
    }

    // All players have passed and stack is empty, so we're done.
    return nullptr;
}

std::unique_ptr<ActionSpace> PrioritySystem::makeActionSpace(Player* player) {
    std::vector<Player*> players = game->priorityOrder();
    Player* current_player = players[pass_count];

    managym::log::debug(Category::PRIORITY, "Generating actions for {}", current_player->name);
    std::vector<std::unique_ptr<Action>> actions = availablePriorityActions(current_player);
    return std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY, std::move(actions),
                                         current_player, game);
}

void PrioritySystem::passPriority() {
    managym::log::debug(Category::PRIORITY, "Passing priority to next player {} --> {}", pass_count,
                        pass_count + 1);
    pass_count++;
}

std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;
    managym::log::debug(Category::PRIORITY, "Generating priority actions for player {}",
                        player->name);

    // Get cards in hand
    const std::vector<Card*> hand_cards = game->zones->constHand()->cards.at(player);

    for (Card* card : hand_cards) {
        if (card == NULL) {
            throw std::logic_error("Card should never be null");
        }
        if (card->types.isLand() && game->canPlayLand(player)) {
            actions.emplace_back(new PlayLand(card, player, game));
            managym::log::debug(Category::PRIORITY, "Added PlayLand action for {}",
                                card->toString());
        } else if (card->types.isCastable() && game->canCastSorceries(player)) {
            if (game->canPayManaCost(player, card->mana_cost.value())) {
                actions.emplace_back(new CastSpell(card, player, game));
                managym::log::debug(Category::PRIORITY, "Added CastSpell action for {}",
                                    card->toString());
            }
        }
    }

    // Always allow passing priority
    actions.emplace_back(new PassPriority(player, game));
    managym::log::debug(Category::PRIORITY, "Added PassPriority action");

    return actions;
}

void PrioritySystem::performStateBasedActions() {
    std::vector<Player*> players = game->priorityOrder();

    // MR04.5a If a player has 0 or less life, that player loses the game.
    for (Player* player : players) {
        if (player->life <= 0) {
            game->loseGame(player);
        }
    }

    // 704.5g If a creature has toughness greater than 0, it has damage marked on it,
    // and that damage is greater than or equal to its toughness, destroy it
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

    for (Permanent* permanent : permanents_to_destroy) {
        managym::log::info(Category::PRIORITY, "{} has lethal damage and is destroyed",
                           permanent->card->toString());
        game->zones->destroy(permanent);
    }
}

void PrioritySystem::resolveTopOfStack() {
    if (game->zones->constStack()->objects.empty()) {
        return;
    }

    Card* card = game->zones->popStack();
    managym::log::info(Category::PRIORITY, "Resolving {}", card->toString());
    if (card->types.isPermanent()) {
        game->zones->move(card, ZoneType::BATTLEFIELD);
    } else {
        // TODO: Handle non-permanent spells
    }
}
