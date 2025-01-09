#include "action.h"

#include <spdlog/spdlog.h>

#include <cassert>

#include "managym/flow/combat.h"
#include "managym/flow/game.h"
#include "managym/flow/turn.h"
#include "managym/state/battlefield.h"
#include "managym/state/zones.h"

void Agent::selectAction(ActionSpace *action_space) {
  if (action_space->empty()) {
    throw std::logic_error("No actions in action space");
  }
  action_space->selectAction(0);
}

bool ActionSpace::empty() { return actions.empty(); }

void DeclareAttackerAction::execute() {
  spdlog::info("Player {} Declaring attacker", player->id);
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
  spdlog::info("Player {} Declaring blocker", player->id);
  step->combat_phase->attacker_to_blockers[attacker].push_back(blocker);
}

PlayLand::PlayLand(Card *card, Player *player, Game *game)
    : Action(player), game(game), card(card) {}

void PlayLand::execute() {
  spdlog::info("Player {} PlayLand: {}", player->id, card->toString());
  game->playLand(player, card);
}

CastSpell::CastSpell(Card *card, Player *player, Game *game)
    : Action(player), game(game), card(card) {
  if (!card->types.isCastable()) {
    throw std::invalid_argument("Cannot cast a land card.");
  }
}

void CastSpell::execute() {
  spdlog::info("Player {} Casting spell: {}", player->id, card->toString());
  game->zones->battlefield->produceMana(card->mana_cost.value(), player);
  game->castSpell(player, card);
  game->spendMana(player, card->mana_cost.value());
}

PassPriority::PassPriority(Player *player, PrioritySystem *priority_system)
    : Action(player), priority_system(priority_system) {}

void PassPriority::execute() {
  spdlog::info("Player {} Passing priority", player->id);
  priority_system->passPriority(player);
}
