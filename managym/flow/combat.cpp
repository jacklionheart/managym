#include "combat.h"

#include "rules/zones/battlefield.h"
#include "rules/game.h"
#include "agents/action.h"

#include <memory>
#include <spdlog/spdlog.h>

void DeclareAttackersStep::initialize() {
    Player* active_player = game()->activePlayer();
    attackers_to_declare = game()->zones->battlefield->eligibleAttackers(active_player);
}

void DeclareBlockersStep::initialize() {
    Player* defending_player = game()->nonActivePlayer();
    blockers_to_declare = game()->zones->battlefield->eligibleBlockers(defending_player);
}

std::unique_ptr<ActionSpace> DeclareAttackersStep::makeActionSpace(Permanent* attacker) {

    std::vector<std::unique_ptr<Action>> actions;
    Player* active_player = game()->activePlayer();

    actions.emplace_back(new DeclareAttackerAction(attacker, true, active_player, this));
    actions.emplace_back(new DeclareAttackerAction(attacker, false, active_player, this));

    return std::make_unique<ActionSpace>(active_player, ActionType::DeclareAttacker, actions);
}

std::unique_ptr<ActionSpace> DeclareAttackersStep::performTurnBasedActions() {

    if (attackers_to_declare.empty()) {
        turn_based_actions_complete = true;
        return nullptr;
    }

    Permanent* attacker = attackers_to_declare.back();
    attackers_to_declare.pop_back();
    
    return makeActionSpace(attacker);
}

std::unique_ptr<ActionSpace> DeclareBlockersStep::makeActionSpace(Permanent* blocker) {

    std::vector<std::unique_ptr<Action>> actions;
    Player* blocking_player = game()->nonActivePlayer();

    return std::make_unique<ActionSpace>();

    for (Permanent* attacker : combat_phase->attackers) {
        actions.emplace_back(new DeclareBlockerAction(blocker, attacker, blocking_player, this));
    }
        actions.emplace_back(new DeclareBlockerAction(blocker, nullptr, blocking_player, this));

    return std::make_unique<ActionSpace>(blocking_player, ActionType::DeclareBlocker, actions);
}


std::unique_ptr<ActionSpace> DeclareBlockersStep::performTurnBasedActions() {
    if (blockers_to_declare.empty()) {
        turn_based_actions_complete = true;
        return nullptr;
    }

    Permanent* blocker = blockers_to_declare.back();
    blockers_to_declare.pop_back();
    
    return makeActionSpace(blocker);
}

std::unique_ptr<ActionSpace> CombatDamageStep::performTurnBasedActions() {
    Player* active_player = combat_phase->turn->active_player;
    Game* game = combat_phase->turn->turn_system->game;

    for (auto& pair : combat_phase->attacker_to_blockers) {
        Permanent* attacker = pair.first;
        std::vector<Permanent*> blockers = pair.second;
        for (Permanent* blocker : blockers) {

            spdlog::info("{} blocks {}", blocker->card->toString(), attacker->card->toString());
            attacker->takeDamage(blocker->card->power.value_or(0));
            spdlog::info("{} receives {} damage", attacker->card->toString(), blocker->card->power.value_or(0));
            blocker->takeDamage(attacker->card->power.value_or(0));
            spdlog::info("{} receives {} damage", blocker->card->toString(), attacker->card->power.value_or(0));
        }
        if (blockers.empty()) {
            game->nonActivePlayer()->takeDamage(attacker->card->power.value_or(0));
            spdlog::info("{} takes {} damage", game->nonActivePlayer()->name, attacker->card->power.value_or(0));
        }
    }
}
