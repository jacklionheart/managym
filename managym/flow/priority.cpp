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
    const std::vector<Player*>& players = game->playersStartingWithActive();
    return pass_count >= players.size() && game->zones->constStack()->objects.empty();
}

bool PrioritySystem::canPlayerAct(Player* player) {
    const std::vector<Card*>& hand = game->zones->constHand()->cards[player->index];
    if (hand.empty()) {
        return false;
    }

    bool can_play_land = game->canPlayLand(player);
    bool can_cast = game->canCastSorceries(player);
    const Mana* producible = nullptr;
    int total_mana = 0;

    for (Card* card : hand) {
        if (card == nullptr) {
            throw std::logic_error("Card should never be null");
        }
        if (card->types.isLand()) {
            if (can_play_land) {
                return true;
            }
            continue;
        }

        if (!card->types.isCastable() || !can_cast) {
            continue;
        }

        if (!card->mana_cost.has_value()) {
            return true;
        }

        if (producible == nullptr) {
            const Mana& cached = game->cachedProducibleMana(player);
            producible = &cached;
            total_mana = cached.total();
        }

        const ManaCost& mana_cost = card->mana_cost.value();
        if (mana_cost.mana_value <= total_mana && producible->canPay(mana_cost)) {
            return true;
        }
    }

    return false;
}

std::pair<bool, std::vector<std::unique_ptr<Action>>>
PrioritySystem::computePlayerActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    const std::vector<Card*>& hand = game->zones->constHand()->cards[player->index];
    actions.reserve(hand.size() + 1);

    bool can_play_land = game->canPlayLand(player);
    bool can_cast = game->canCastSorceries(player);
    const Mana* producible = nullptr;
    int total_mana = 0;

    for (Card* card : hand) {
        if (card == nullptr) {
            throw std::logic_error("Card should never be null");
        }
        if (card->types.isLand() && can_play_land) {
            actions.emplace_back(new PlayLand(card, player, game));
            log_debug(LogCat::PRIORITY, "Added PlayLand action for {}", card->toString());
        } else if (card->types.isCastable() && can_cast) {
            if (producible == nullptr) {
                const Mana& cached = game->cachedProducibleMana(player);
                producible = &cached;
                total_mana = cached.total();
            }

            if (!card->mana_cost.has_value()) {
                actions.emplace_back(new CastSpell(card, player, game));
                log_debug(LogCat::PRIORITY, "Added CastSpell action for {}", card->toString());
                continue;
            }

            const ManaCost& mana_cost = card->mana_cost.value();
            if (mana_cost.mana_value <= total_mana && producible->canPay(mana_cost)) {
                actions.emplace_back(new CastSpell(card, player, game));
                log_debug(LogCat::PRIORITY, "Added CastSpell action for {}", card->toString());
            }
        }
    }

    bool can_act = !actions.empty();
    actions.emplace_back(new PassPriority(player, game));
    log_debug(LogCat::PRIORITY, "Added PassPriority action");

    return {can_act, std::move(actions)};
}

std::unique_ptr<ActionSpace> PrioritySystem::tick() {
    Profiler::Scope scope = game->profiler->track("priority");
    log_debug(LogCat::PRIORITY, "Ticking PrioritySystem (pass_count={})", pass_count);

    if (!sba_done) {
        performStateBasedActions();
        sba_done = true;
        if (game->isGameOver()) {
            return nullptr;
        }
    }

    const std::vector<Player*>& players = game->playersStartingWithActive();

    while (pass_count < players.size()) {
        Player* player = players[pass_count];

        if (game->skip_trivial) {
            if (!canPlayerAct(player)) {
                log_debug(LogCat::PRIORITY, "Fast-path: {} auto-passes (no actions)", player->name);
                pass_count++;
                continue;
            }
            std::pair<bool, std::vector<std::unique_ptr<Action>>> result =
                computePlayerActions(player);
            log_debug(LogCat::PRIORITY, "Generating actions for {}", player->name);
            return std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY, std::move(result.second),
                                                 player);
        }

        // Non-skip_trivial path: always build action space
        log_debug(LogCat::PRIORITY, "Generating actions for {}", player->name);
        std::pair<bool, std::vector<std::unique_ptr<Action>>> result = computePlayerActions(player);
        return std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY, std::move(result.second),
                                             player);
    }

    log_debug(LogCat::PRIORITY, "All players have passed");
    reset();

    if (!game->zones->constStack()->objects.empty()) {
        log_debug(LogCat::PRIORITY, "Resolving stack object");
        resolveTopOfStack();
        return tick();
    }

    return nullptr;
}

void PrioritySystem::passPriority() {
    log_debug(LogCat::PRIORITY, "Passing priority to next player {} --> {}", pass_count,
              pass_count + 1);
    pass_count++;
}

void PrioritySystem::performStateBasedActions() {
    const std::vector<Player*>& players = game->playersStartingWithActive();

    // MR704.5a If a player has 0 or less life, that player loses the game.
    for (Player* player : players) {
        if (player->life <= 0) {
            game->loseGame(player);
        }
    }

    // MR704.5b If a player attempted to draw a card from a library with no cards in it since
    // the last time state-based actions were checked, that player loses the game.
    for (Player* player : players) {
        if (player->drew_when_empty) {
            game->loseGame(player);
        }
    }

    if (game->isGameOver()) {
        return;
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
        log_info(LogCat::PRIORITY, "{} has lethal damage and is destroyed",
                 permanent->card->toString());
        game->invalidateManaCache(permanent->controller);
        game->zones->destroy(permanent);
    }
}

void PrioritySystem::resolveTopOfStack() {
    if (game->zones->constStack()->objects.empty()) {
        return;
    }

    Card* card = game->zones->popStack();
    log_info(LogCat::PRIORITY, "Resolving {}", card->toString());
    if (card->types.isPermanent()) {
        game->zones->move(card, ZoneType::BATTLEFIELD);
        game->invalidateManaCache(card->owner);
    } else {
        // TODO: Handle non-permanent spells
    }
}
