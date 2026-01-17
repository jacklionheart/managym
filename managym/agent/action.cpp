#include "action.h"

#include "managym/agent/observation.h"
#include "managym/flow/combat.h"
#include "managym/flow/game.h"
#include "managym/flow/turn.h"
#include "managym/infra/log.h"
#include "managym/state/battlefield.h"
#include "managym/state/zones.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <memory>

void DeclareAttackerAction::execute() {
    log_info(LogCat::AGENT, "Player {} Declaring attacker", player->name);
    if (attack) {
        if (!attacker->canAttack()) {
            throw std::logic_error("attacker cannot attack");
        }

        step->combat_phase->attackers.push_back(attacker);
        step->combat_phase->attacker_to_blockers[attacker] = std::vector<Permanent*>();
        attacker->attack();
        player->behavior_tracker->onAttackerDeclared(attacker);
    }
}

std::vector<ObjectId> DeclareAttackerAction::focus() const { return {attacker->id}; }

void DeclareBlockerAction::execute() {
    log_info(LogCat::AGENT, "Player {} Declaring blocker", player->name);
    if (attacker != nullptr) {
        step->combat_phase->attacker_to_blockers[attacker].push_back(blocker);
        player->behavior_tracker->onBlockerDeclared(blocker, attacker);
    }
}

std::vector<ObjectId> DeclareBlockerAction::focus() const {
    std::vector<ObjectId> focus;
    focus.push_back(blocker->id);
    if (attacker != nullptr) {
        focus.push_back(attacker->id);
    }
    return focus;
}

PlayLand::PlayLand(Card* card, Player* player, Game* game)
    : Action(player, ActionType::PRIORITY_PLAY_LAND), game(game), card(card) {}

void PlayLand::execute() {
    log_info(LogCat::AGENT, "Player {} PlayLand: {}", player->name, card->toString());
    game->playLand(player, card);
    player->behavior_tracker->onLandPlayed(card);
}

std::vector<ObjectId> PlayLand::focus() const { return {card->id}; }

CastSpell::CastSpell(Card* card, Player* player, Game* game)
    : Action(player, ActionType::PRIORITY_CAST_SPELL), game(game), card(card) {
    if (!card->types.isCastable()) {
        throw std::invalid_argument("Cannot cast a land card.");
    }
    player->behavior_tracker->onSpellCast(card, card->mana_cost.value().mana_value);
}

void CastSpell::execute() {
    log_info(LogCat::AGENT, "Player {} Casting spell: {}", player->name, card->toString());
    log_debug(LogCat::AGENT, "Player's mana pool before: {}", player->mana_pool.toString());
    game->zones->produceMana(card->mana_cost.value(), player);
    game->invalidateManaCache(player);
    log_debug(LogCat::AGENT, "Player's mana pool after producing mana: {}",
              player->mana_pool.toString());
    game->castSpell(player, card);
    log_debug(LogCat::AGENT, "Player's mana pool after casting spell: {}",
              player->mana_pool.toString());
    game->spendMana(player, card->mana_cost.value());
    log_debug(LogCat::AGENT, "Player's mana pool after spending mana: {}",
              player->mana_pool.toString());
}

std::vector<ObjectId> CastSpell::focus() const { return {card->id}; }

PassPriority::PassPriority(Player* player, Game* game)
    : Action(player, ActionType::PRIORITY_PASS_PRIORITY), game(game) {}

void PassPriority::execute() {
    log_debug(LogCat::AGENT, "Player {} Passing priority", player->name);
    game->priority_system->passPriority();
}

std::vector<ObjectId> PassPriority::focus() const { return {}; }

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
