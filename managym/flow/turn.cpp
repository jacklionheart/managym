#include "turn.h"

#include "managym/flow/combat.h"
#include "managym/flow/game.h"
#include "managym/infra/log.h"

#include <cstdarg>
// TurnSystem implementation

TurnSystem::TurnSystem(Game* game) : game(game) {
    global_turn_count = 0;
    active_player_index = 0;
    // Note: players_active_first and players_nap_first are lazily initialized
    // in playersStartingWithActive() because game->players is empty at this point
}

const Phase* TurnSystem::currentPhase() const {
    if (!current_turn)
        return nullptr;
    return current_turn->currentPhase();
}

PhaseType TurnSystem::currentPhaseType() const {
    if (!current_turn)
        return PhaseType::BEGINNING;
    return static_cast<PhaseType>(current_turn->current_phase_index);
}

StepType TurnSystem::currentStepType() const {
    const Phase* phase = currentPhase();
    if (!phase)
        return StepType::BEGINNING_UNTAP;

    return stepTypeFromIndex(currentPhaseType(), phase->current_step_index);
}

bool TurnSystem::isInPhase(PhaseType phase) const { return currentPhaseType() == phase; }

bool TurnSystem::isInStep(StepType step) const { return currentStepType() == step; }

PhaseType TurnSystem::getPhaseForStep(StepType step) {
    switch (step) {
    case StepType::BEGINNING_UNTAP:
    case StepType::BEGINNING_UPKEEP:
    case StepType::BEGINNING_DRAW:
        return PhaseType::BEGINNING;

    case StepType::PRECOMBAT_MAIN_STEP:
        return PhaseType::PRECOMBAT_MAIN;

    case StepType::COMBAT_BEGIN:
    case StepType::COMBAT_DECLARE_ATTACKERS:
    case StepType::COMBAT_DECLARE_BLOCKERS:
    case StepType::COMBAT_DAMAGE:
    case StepType::COMBAT_END:
        return PhaseType::COMBAT;

    case StepType::POSTCOMBAT_MAIN_STEP:
        return PhaseType::POSTCOMBAT_MAIN;

    case StepType::ENDING_END:
    case StepType::ENDING_CLEANUP:
        return PhaseType::ENDING;
    }
    throw std::runtime_error("Unknown step type");
}

StepType TurnSystem::stepTypeFromIndex(PhaseType phase, int stepIndex) {
    switch (phase) {
    case PhaseType::BEGINNING:
        switch (stepIndex) {
        case 0:
            return StepType::BEGINNING_UNTAP;
        case 1:
            return StepType::BEGINNING_UPKEEP;
        case 2:
            return StepType::BEGINNING_DRAW;
        default:
            throw std::out_of_range("Invalid beginning phase step index: " +
                                    std::to_string(stepIndex));
        }

    case PhaseType::PRECOMBAT_MAIN:
        if (stepIndex == 0)
            return StepType::PRECOMBAT_MAIN_STEP;
        throw std::out_of_range("Invalid main phase step index");
    case PhaseType::POSTCOMBAT_MAIN:
        if (stepIndex == 0)
            return StepType::POSTCOMBAT_MAIN_STEP;
        throw std::out_of_range("Invalid main phase step index");

    case PhaseType::COMBAT:
        switch (stepIndex) {
        case 0:
            return StepType::COMBAT_BEGIN;
        case 1:
            return StepType::COMBAT_DECLARE_ATTACKERS;
        case 2:
            return StepType::COMBAT_DECLARE_BLOCKERS;
        case 3:
            return StepType::COMBAT_DAMAGE;
        case 4:
            return StepType::COMBAT_END;
        default:
            throw std::out_of_range("Invalid combat phase step index");
        }

    case PhaseType::ENDING:
        switch (stepIndex) {
        case 0:
            return StepType::ENDING_END;
        case 1:
            return StepType::ENDING_CLEANUP;
        default:
            throw std::out_of_range("Invalid ending phase step index");
        }
    }
    throw std::runtime_error("Unknown phase type");
}

int TurnSystem::stepIndexFromType(StepType step) {
    switch (step) {
    case StepType::BEGINNING_UNTAP:
        return 0;
    case StepType::BEGINNING_UPKEEP:
        return 1;
    case StepType::BEGINNING_DRAW:
        return 2;
    case StepType::PRECOMBAT_MAIN_STEP:
        return 0;
    case StepType::COMBAT_BEGIN:
        return 0;
    case StepType::COMBAT_DECLARE_ATTACKERS:
        return 1;
    case StepType::COMBAT_DECLARE_BLOCKERS:
        return 2;
    case StepType::COMBAT_DAMAGE:
        return 3;
    case StepType::COMBAT_END:
        return 4;
    case StepType::POSTCOMBAT_MAIN_STEP:
        return 0;
    case StepType::ENDING_END:
        return 0;
    case StepType::ENDING_CLEANUP:
        return 1;
    }
    throw std::runtime_error("Unknown step type");
}

std::unique_ptr<ActionSpace> TurnSystem::tick() {
    if (current_turn == nullptr) {
        startNextTurn();
    }

    std::unique_ptr<ActionSpace> result = current_turn->tick();

    if (current_turn->completed) {
        startNextTurn();
    }

    return result;
}

void TurnSystem::startNextTurn() {
    if (global_turn_count != 0) {
        active_player_index = (active_player_index + 1) % game->players.size();
    }
    current_turn = std::make_unique<Turn>(activePlayer(), this);
    turn_counts[activePlayer()]++;
    activePlayer()->behavior_tracker->onTurnStart();
    global_turn_count++;
}

const std::vector<Player*>& TurnSystem::playersStartingWithActive() {
    // Only rebuild when active_player_index changes
    if (cached_active_index != active_player_index) {
        int num_players = game->players.size();
        if (players_active_first.size() != num_players) {
            players_active_first.resize(num_players);
        }
        for (int i = 0; i < num_players; i++) {
            players_active_first[i] = game->players[(active_player_index + i) % num_players].get();
        }
        cached_active_index = active_player_index;
    }
    return players_active_first;
}

Player* TurnSystem::activePlayer() { return game->players.at(active_player_index).get(); }

Player* TurnSystem::nonActivePlayer() {
    return game->players.at((active_player_index + 1) % game->players.size()).get();
}

// Turn implementation

Turn::Turn(Player* active_player, TurnSystem* turn_system)
    : active_player(active_player), turn_system(turn_system) {
    phases.emplace_back(new BeginningPhase(this));
    phases.emplace_back(new PrecombatMainPhase(this));
    phases.emplace_back(new CombatPhase(this));
    phases.emplace_back(new PostcombatMainPhase(this));
    phases.emplace_back(new EndingPhase(this));
}

std::unique_ptr<ActionSpace> Turn::tick() {
    Profiler::Scope scope = turn_system->game->profiler->track("turn");

    if (completed || current_phase_index >= phases.size()) {
        throw std::runtime_error("Turn is complete");
    }

    Phase* current_phase = phases[current_phase_index].get();
    std::unique_ptr<ActionSpace> result = current_phase->tick();

    if (current_phase->completed) {
        if (current_phase_index < phases.size() - 1) {
            current_phase_index++;
        } else {
            completed = true;
            active_player->behavior_tracker->onTurnEnd();
        }
    }

    return result;
}

// Phase implementation

std::unique_ptr<ActionSpace> Phase::tick() {
    log_debug(LogCat::TURN, "Ticking {}", std::string(typeid(*this).name()));

    if (completed || current_step_index >= steps.size()) {
        throw std::runtime_error("Phase is complete");
    }

    Step* current_step = steps[current_step_index].get();
    auto result = current_step->tick();

    if (current_step->completed) {
        if (current_step_index < steps.size() - 1) {
            current_step_index++;
        } else {
            completed = true;
        }
    }

    return result;
}

// Step implementations

Step::Step(Phase* phase) : phase(phase) {}
std::unique_ptr<ActionSpace> Step::tick() {
    log_debug(LogCat::TURN, "Ticking {}", std::string(typeid(*this).name()));

    if (!initialized) {
        onStepStart();
        initialized = true;
        log_debug(LogCat::TURN, "Step initialized");
    }

    if (completed) {
        throw std::runtime_error("Step is complete");
    }

    std::unique_ptr<ActionSpace> result = nullptr;

    // Debug the state machine
    log_debug(LogCat::TURN, "Step state: turn_based_actions_complete={}, has_priority_window={}",
              turn_based_actions_complete, has_priority_window);

    if (!turn_based_actions_complete) {
        result = performTurnBasedActions();
        if (!result) {
            turn_based_actions_complete = true;
            log_debug(LogCat::TURN, "Turn based actions completed with no result");
        } else {
            log_debug(LogCat::TURN, "Turn based actions produced an action space");
            return result;
        }
    }

    if (has_priority_window) {
        PrioritySystem* priority_system = game()->priority_system.get();
        log_debug(LogCat::TURN, "Ticking priority system");
        result = priority_system->tick();
        if (result) {
            return result;
        }
        log_debug(LogCat::TURN, "Priority system completed");
    }

    log_debug(LogCat::TURN, "Emptying mana pools");
    game()->clearManaPools();

    log_debug(LogCat::TURN, "Step completing");

    onStepEnd();
    completed = true;

    return nullptr;
}

void Step::onStepStart() {}
void Step::onStepEnd() {}

std::unique_ptr<ActionSpace> Step::performTurnBasedActions() {
    turn_based_actions_complete = true;
    return nullptr;
}

Game* Step::game() { return phase->turn->turn_system->game; }
Game* Phase::game() { return turn->turn_system->game; }

TurnSystem* Step::turn_system() { return phase->turn->turn_system; }

Turn* Step::turn() { return phase->turn; }

Player* Step::activePlayer() { return phase->turn->active_player; }

void MainStep::onStepStart() {
    activePlayer()->behavior_tracker->onMainPhaseStart(game(), activePlayer());
}
void MainStep::onStepEnd() { activePlayer()->behavior_tracker->onMainPhaseEnd(); }

std::unique_ptr<ActionSpace> UntapStep::performTurnBasedActions() {
    log_debug(LogCat::TURN, "Starting {} for {}", std::string(typeid(*this).name()),
              activePlayer()->name);
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
}

// Phase constructor implementations

BeginningPhase::BeginningPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new UntapStep(this));
    steps.emplace_back(new UpkeepStep(this));
    steps.emplace_back(new DrawStep(this));
}

PrecombatMainPhase::PrecombatMainPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new MainStep(this));
}

PostcombatMainPhase::PostcombatMainPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new MainStep(this));
}

EndingPhase::EndingPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new EndStep(this));
    steps.emplace_back(new CleanupStep(this));
}