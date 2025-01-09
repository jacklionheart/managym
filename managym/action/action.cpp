#include "action.h"

#include <spdlog/spdlog.h>

#include <cassert>

#include "managym/rules/turns/combat.h"
#include "managym/rules/turns/turn.h"
#include "managym/state/battlefield.h"

Action *Agent::selectAction(ActionSpace *action_space) {
  if (action_space->empty()) {
    throw std::logic_error("No actions in action space");
  }
  // For now, just select the first action
  return action_space->actions[0].get();
}

bool ActionSpace::empty() { return actions.empty(); }

void DeclareAttackerAction::execute() {
  if (attack) {
    if (!attacker->canAttack()) {
      throw std::logic_error("attacker cannot attack");
    }

    step->combat_phase->attackers.push_back(attacker);
    step->combat_phase->attacker_to_blockers[attacker] =
        std::vector<Permanent *>();
    attacker->attack();
  }
}

void DeclareBlockerAction::execute() {
  step->combat_phase->attacker_to_blockers[attacker].push_back(blocker);
}

void PlayLand::execute() {
  spdlog::info("Player {} PlayLand: {}", player->id, card->toString());
  game->playLand(player->id, card);
}

CastSpell::CastSpell(Card *card, Player *player, Game *game)
    : Action(player), game(game), card(card) {
  if (!card->types.isCastable()) {
    throw std::invalid_argument("Cannot cast a land card.");
  }
}

void CastSpell::execute() {
  spdlog::info("Player {} Casting spell: {}", player->id, card->toString());
  game->zones->battlefield.produceMana(card->mana_cost.value(), player->id);
  game->castSpell(player->id, card);
  game->spendMana(player->id, card->mana_cost.value());
}

PassPriority::PassPriority(Player *player, PrioritySystem *priority_system)
    : Action(player), priority_system(priority_system) {}

void PassPriority::execute() { priority_system->passPriority(player); }
