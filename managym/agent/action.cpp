#include "action.h"

#include "managym/agent/observation.h"
#include "managym/flow/combat.h"
#include "managym/flow/game.h"
#include "managym/flow/turn.h"
#include "managym/state/battlefield.h"
#include "managym/state/zones.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <memory>

void DeclareAttackerAction::execute() {
    spdlog::info("Player {} Declaring attacker", player->id);
    if (attack) {
        if (!attacker->canAttack()) {
            throw std::logic_error("attacker cannot attack");
        }

        step->combat_phase->attackers.push_back(attacker);
        step->combat_phase->attacker_to_blockers[attacker] = std::vector<Permanent*>();
        attacker->attack();
    }
}

void DeclareBlockerAction::execute() {
    spdlog::info("Player {} Declaring blocker", player->id);
    if (attacker != nullptr) {
        step->combat_phase->attacker_to_blockers[attacker].push_back(blocker);
    }
}

PlayLand::PlayLand(Card* card, Player* player, Game* game)
    : Action(player), game(game), card(card) {}

void PlayLand::execute() {
    spdlog::info("Player {} PlayLand: {}", player->id, card->toString());
    game->playLand(player, card);
}

CastSpell::CastSpell(Card* card, Player* player, Game* game)
    : Action(player), game(game), card(card) {
    if (!card->types.isCastable()) {
        throw std::invalid_argument("Cannot cast a land card.");
    }
}

void CastSpell::execute() {
    spdlog::info("Player {} Casting spell: {}", player->id, card->toString());
    spdlog::debug("Player's mana pool before: {}", player->mana_pool.toString());
    game->zones->produceMana(card->mana_cost.value(), player);
    spdlog::debug("Player's mana pool after producing mana: {}", player->mana_pool.toString());
    game->castSpell(player, card);
    spdlog::debug("Player's mana pool after casting spell: {}", player->mana_pool.toString());
    game->spendMana(player, card->mana_cost.value());
    spdlog::debug("Player's mana pool after spending mana: {}", player->mana_pool.toString());
}

PassPriority::PassPriority(Player* player, PrioritySystem* priority_system)
    : Action(player), priority_system(priority_system) {}

void PassPriority::execute() {
    spdlog::debug("Player {} Passing priority", player->id);
    priority_system->passPriority(player);
}

std::string DeclareAttackerAction::toString() const {
    return fmt::format("DeclareAttackerAction(attacker={}, attack={}, player={})",
                       attacker->card->toString(), attack, player->name);
}

std::string DeclareBlockerAction::toString() const {
    return fmt::format("DeclareBlockerAction(blocker={}, attacker={}, player={})",
                       blocker->card->toString(), attacker ? attacker->card->toString() : "nullptr",
                       player->name);
}

std::string PlayLand::toString() const {
    return fmt::format("PlayLand(card={}, player={})", card->toString(), player->name);
}

std::string CastSpell::toString() const {
    return fmt::format("CastSpell(card={}, player={})", card->toString(), player->name);
}

std::string PassPriority::toString() const {
    return fmt::format("PassPriority(player={})", player->name);
}
