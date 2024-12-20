// turn.cpp
#include "turn.h"
#include "combat.h"
#include "rules/game.h"

#include <spdlog/spdlog.h>

TurnSystem::TurnSystem(Game* game) : game(game) {

    for (std::unique_ptr<Player>& player : game->players) {
        turn_counts[player.get()] = 0;
    }

    global_turn_count = 0;
    active_player_index = 0;
}

std::unique_ptr<ActionSpace> TurnSystem::tick() {

    if (current_turn == nullptr || current_turn->isComplete()) {
        startNextTurn();
    }   

    return current_turn->tick();
}

std::unique_ptr<ActionSpace> TurnSystem::startNextTurn() {
    if (global_turn_count != 0) {
        active_player_index = (active_player_index + 1) % game->players.size();
    }
    current_turn = std::make_unique<Turn>(activePlayer(), *this);
    turn_counts[activePlayer()]++;
    global_turn_count++;
}

std::vector<Player*> TurnSystem::priorityOrder() {
    std::vector<Player*> order;
    int num_players = game->players.size();

    for (int i = 0; i < num_players; i++) {
        order.push_back(game->players[(active_player_index + i) % num_players].get());
    }

    return order;
}


Player* TurnSystem::activePlayer() {
    return game->players.at(active_player_index).get();
}

Player* TurnSystem::nonActivePlayer() {
    return game->players.at((active_player_index + 1) % game->players.size()).get();
}

Turn::Turn(Player* player, TurnSystem* turn_system)
    : active_player(player), turn_system(turn_system) {

    phases.emplace_back(new BeginningPhase(this));
    phases.emplace_back(new PrecombatMainPhase(this));
    phases.emplace_back(new CombatPhase(this));
    phases.emplace_back(new PostcombatMainPhase(this));
    phases.emplace_back(new EndingPhase(this));
}

std::unique_ptr<ActionSpace> Turn::tick() {

    if (current_phase_index >= phases.size()) {
        return nullptr;
    }

    Phase* current_phase = phases[current_phase_index].get();

    if (current_phase->isComplete()) {
        current_phase_index++;
        return nullptr;
    } else {
        return current_phase->tick();
    }
}

bool Step::isComplete() {
    return turn_based_actions_complete && (!has_priority_window || priority_system->isComplete()) && mana_pools_emptied;
}

std::unique_ptr<ActionSpace> Step::tick() {
    
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (isComplete()) {
        throw std::runtime_error("Step is complete");
    }

    if (!turn_based_actions_complete) {
        return performTurnBasedActions();
    }

    if (has_priority_window && !priority_system->isComplete()) {
        return priority_system->tick();
    }

    game()->clearManaPools();
    mana_pools_emptied = true;
    return nullptr;
}

bool Step::isComplete() {
    return turn_based_actions_complete && priority_system->isComplete();
}

// Implementations of specific Steps

std::unique_ptr<ActionSpace> UntapStep::performTurnBasedActions() {
    spdlog::info("Starting {}", std::string(typeid(*this).name()));
    game()->markPermanentsNotSummoningSick(activePlayer());
    game()->untapAllPermanents(activePlayer());
    turn_based_actions_complete = true;
    return nullptr;
}


std::unique_ptr<ActionSpace> DrawStep::performTurnBasedActions() {
    game()->drawCards(activePlayer(), 1);
    turn_based_actions_complete = true;
    return nullptr;
}

std::unique_ptr<ActionSpace> CleanupStep::performTurnBasedActions() {
    game()->clearDamage();
    turn_based_actions_complete = true;
    return nullptr;
;
}