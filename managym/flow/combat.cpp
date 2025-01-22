#include "combat.h"

#include "managym/agent/action.h"
#include "managym/flow/game.h"
#include "managym/infra/log.h"
#include "managym/state/battlefield.h"
#include "managym/state/zones.h"

#include <memory>

CombatPhase::CombatPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new BeginningOfCombatStep(this));
    steps.emplace_back(new DeclareAttackersStep(this));
    steps.emplace_back(new DeclareBlockersStep(this));
    steps.emplace_back(new CombatDamageStep(this));
    steps.emplace_back(new EndOfCombatStep(this));
}

CombatStep::CombatStep(CombatPhase* parent_combat_phase)
    : Step(parent_combat_phase), combat_phase(parent_combat_phase) {}

void DeclareAttackersStep::initialize() {
    Player* active_player = game()->activePlayer();
    managym::log::debug(Category::COMBAT, "DeclareAttackersStep::initialize");
    attackers_to_declare = game()->zones->constBattlefield()->eligibleAttackers(active_player);
}

void DeclareBlockersStep::initialize() {
    Player* defending_player = game()->nonActivePlayer();
    blockers_to_declare = game()->zones->constBattlefield()->eligibleBlockers(defending_player);
}

std::unique_ptr<ActionSpace> DeclareAttackersStep::makeActionSpace(Permanent* attacker) {
    std::vector<std::unique_ptr<Action>> actions;
    Player* active_player = game()->activePlayer();

    actions.emplace_back(new DeclareAttackerAction(attacker, true, active_player, this));
    actions.emplace_back(new DeclareAttackerAction(attacker, false, active_player, this));

    return std::make_unique<ActionSpace>(ActionSpaceType::DECLARE_ATTACKER, std::move(actions),
                                         active_player, game());
}

std::unique_ptr<ActionSpace> DeclareAttackersStep::performTurnBasedActions() {
    if (attackers_to_declare.empty()) {
        managym::log::debug(
            Category::COMBAT,
            "DeclareAttackersStep::performTurnBasedActions -- no more attackers to declare");
        turn_based_actions_complete = true;
        return nullptr;
    }

    Permanent* attacker = attackers_to_declare.back();
    attackers_to_declare.pop_back();

    managym::log::debug(Category::COMBAT,
                        "DeclareAttackersStep::performTurnBasedActions -- making actionSpace for "
                        "declaring an attacker");
    return makeActionSpace(attacker);
}

std::unique_ptr<ActionSpace> DeclareBlockersStep::makeActionSpace(Permanent* blocker) {
    std::vector<std::unique_ptr<Action>> actions;
    Player* blocking_player = game()->nonActivePlayer();

    for (Permanent* attacker : combat_phase->attackers) {
        actions.emplace_back(new DeclareBlockerAction(blocker, attacker, blocking_player, this));
    }
    actions.emplace_back(new DeclareBlockerAction(blocker, nullptr, blocking_player, this));

    return std::make_unique<ActionSpace>(ActionSpaceType::DECLARE_BLOCKER, std::move(actions),
                                         blocking_player, game());
}

std::unique_ptr<ActionSpace> DeclareBlockersStep::performTurnBasedActions() {
    if (blockers_to_declare.empty()) {
        managym::log::debug(Category::COMBAT, "No blockers to declare");
        turn_based_actions_complete = true;
        return nullptr;
    }

    managym::log::debug(Category::COMBAT, "Blockers to declare: {}", blockers_to_declare.size());
    Permanent* blocker = blockers_to_declare.back();
    blockers_to_declare.pop_back();

    return makeActionSpace(blocker);
}

std::unique_ptr<ActionSpace> CombatDamageStep::performTurnBasedActions() {
    managym::log::debug(Category::COMBAT, "CombatDamageStep.performTurnBasedActions");
    Player* active_player = combat_phase->turn->active_player;
    Game* game = combat_phase->turn->turn_system->game;

    managym::log::debug(Category::COMBAT, "hi");
    for (auto pair = combat_phase->attacker_to_blockers.begin();
         pair != combat_phase->attacker_to_blockers.end(); pair++) {
        assert(pair->first != nullptr);

        managym::log::debug(Category::COMBAT, "Attacker: {}", pair->first->card->toString());
        Permanent* attacker = pair->first;
        std::vector<Permanent*> blockers = pair->second;
        for (Permanent* blocker : blockers) {
            managym::log::info(Category::COMBAT, "{} blocks {}", blocker->card->toString(),
                               attacker->card->toString());
            attacker->takeDamage(blocker->card->power.value_or(0));
            managym::log::info(Category::COMBAT, "{} receives {} damage",
                               attacker->card->toString(), blocker->card->power.value_or(0));
            blocker->takeDamage(attacker->card->power.value_or(0));
            managym::log::info(Category::COMBAT, "{} receives {} damage", blocker->card->toString(),
                               attacker->card->power.value_or(0));
        }
        if (blockers.empty()) {
            game->nonActivePlayer()->takeDamage(attacker->card->power.value_or(0));
            managym::log::info(Category::COMBAT, "{} takes {} damage",
                               game->nonActivePlayer()->name, attacker->card->power.value_or(0));
        }
    }
    turn_based_actions_complete = true;

    return nullptr;
};
