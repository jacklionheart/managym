// turn.h
#pragma once

#include "managym/flow/priority.h"

#include <map>
#include <memory>
#include <vector>

struct Step;
struct Phase;
struct Turn;
class Player;
class Game;

// Represents different phases of a turn
enum struct PhaseType {
    BEGINNING = 0,
    PRECOMBAT_MAIN = 1,
    COMBAT = 2,
    POSTCOMBAT_MAIN = 3,
    ENDING = 4,
};

inline std::string toString(PhaseType phase) {
    switch (phase) {
    case PhaseType::BEGINNING:
        return "BEGINNING";
    case PhaseType::PRECOMBAT_MAIN:
        return "PRECOMBAT_MAIN";
    case PhaseType::COMBAT:
        return "COMBAT";
    case PhaseType::POSTCOMBAT_MAIN:
        return "POSTCOMBAT_MAIN";
    case PhaseType::ENDING:
        return "ENDING";
    default:
        return "UNKNOWN";
    }
}
// Represents specific steps within each phase
enum struct StepType {
    BEGINNING_UNTAP = 0,
    BEGINNING_UPKEEP = 1,
    BEGINNING_DRAW = 2,

    PRECOMBAT_MAIN_STEP = 3,

    COMBAT_BEGIN = 4,
    COMBAT_DECLARE_ATTACKERS = 5,
    COMBAT_DECLARE_BLOCKERS = 6,
    COMBAT_DAMAGE = 7,
    COMBAT_END = 8,

    POSTCOMBAT_MAIN_STEP = 9,

    ENDING_END = 10,
    ENDING_CLEANUP = 11
};

inline std::string toString(StepType step) {
    switch (step) {
    case StepType::BEGINNING_UNTAP:
        return "BEGINNING_UNTAP";
    case StepType::BEGINNING_UPKEEP:
        return "BEGINNING_UPKEEP";
    case StepType::BEGINNING_DRAW:
        return "BEGINNING_DRAW";
    case StepType::PRECOMBAT_MAIN_STEP:
        return "PRECOMBAT_MAIN_STEP";
    case StepType::COMBAT_BEGIN:
        return "COMBAT_BEGIN";
    case StepType::COMBAT_DECLARE_ATTACKERS:
        return "COMBAT_DECLARE_ATTACKERS";
    case StepType::COMBAT_DECLARE_BLOCKERS:
        return "COMBAT_DECLARE_BLOCKERS";
    case StepType::COMBAT_DAMAGE:
        return "COMBAT_DAMAGE";
    case StepType::COMBAT_END:
        return "COMBAT_END";
    case StepType::POSTCOMBAT_MAIN_STEP:
        return "POSTCOMBAT_MAIN_STEP";
    case StepType::ENDING_END:
        return "ENDING_END";
    case StepType::ENDING_CLEANUP:
        return "ENDING_CLEANUP";
    default:
        return "UNKNOWN";
    }
}
// Manages turn sequence and transitions between players
class TurnSystem {
public:
    // Data
    std::unique_ptr<Turn> current_turn;
    int active_player_index;
    int global_turn_count;
    std::map<Player*, int> turn_counts;
    Game* game;

    // Pre-allocated player order vectors (reused to avoid allocations)
    std::vector<Player*> players_active_first;
    // Cached index to avoid rebuilding players_active_first on every call
    int cached_active_index = -1;

    TurnSystem(Game* game);

    // Reads

    // Get the current phase
    const Phase* currentPhase() const;
    // Get the current phase type
    PhaseType currentPhaseType() const;
    // Get the current step type
    StepType currentStepType() const;
    // Check if game is in a specific phase
    bool isInPhase(PhaseType phase) const;
    // Check if game is in a specific step
    bool isInStep(StepType step) const;
    // Get controlling player's order for priority (returns reference to internal buffer)
    const std::vector<Player*>& playersStartingWithActive();
    // Get current active player
    Player* activePlayer();
    // Get non-active player
    Player* nonActivePlayer();

    // Writes

    // Advance game state by one action
    std::unique_ptr<ActionSpace> tick();

    // Helper functions
    // Get the phase that contains a step
    static PhaseType getPhaseForStep(StepType step);

protected:
    // Start a new turn; called by tick().
    void startNextTurn();

    // Convert step index to step type
    static StepType stepTypeFromIndex(PhaseType phase, int stepIndex);
    // Convert step type to index
    static int stepIndexFromType(StepType step);
};

// Represents a single step within a phase
struct Step {

    Step(Phase* phase);
    virtual ~Step() = default;

    // Data
    Phase* phase;
    bool initialized = false;
    bool has_priority_window = true;
    bool turn_based_actions_complete = false;
    bool completed = false;

    // Reads
    Game* game();
    // Get turn system
    TurnSystem* turn_system();
    // Get current turn
    Turn* turn();
    // Get active player
    Player* activePlayer();
    // Get the step type for profiler tracking
    virtual StepType stepType() const = 0;

    // Writes
    // Perform game actions for this step
    virtual std::unique_ptr<ActionSpace> performTurnBasedActions();

    virtual void onStepStart();
    virtual void onStepEnd();
    std::unique_ptr<ActionSpace> tick();
};

// Base struct for all phases in a turn
struct Phase {
    Phase(Turn* turn) : turn(turn) {}
    virtual ~Phase() = default;

    // Data
    Turn* turn;
    std::vector<std::unique_ptr<Step>> steps;
    int current_step_index = 0;
    bool completed = false;

    // Reads
    Game* game();

    // Check if sorcery-speed spells can be cast
    virtual bool canCastSorceries() { return false; }

    // Writes

    // Move forward one unit of time. returning the next ActionSpace (possibly nullptr) for a player
    // to take.
    std::unique_ptr<ActionSpace> tick();
};

// Represents a complete turn in the game
struct Turn {
    Turn(Player* active_player, TurnSystem* turn_system);

    // Data
    TurnSystem* turn_system;
    Player* active_player;
    std::vector<std::unique_ptr<Phase>> phases;
    int current_phase_index = 0;
    int lands_played = 0;
    bool completed = false;

    // Reads

    // Get current phase
    Phase* currentPhase() { return phases[current_phase_index].get(); }

    // Writes

    // Advance turn state
    std::unique_ptr<ActionSpace> tick();
};

// Beginning phase steps

// Step where permanents become untapped
struct UntapStep : public Step {
    UntapStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = false; }
    StepType stepType() const override { return StepType::BEGINNING_UNTAP; }
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Step for upkeep triggers
struct UpkeepStep : public Step {
    UpkeepStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = true; }
    StepType stepType() const override { return StepType::BEGINNING_UPKEEP; }
};

// Step where active player draws a card
struct DrawStep : public Step {
    DrawStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = true; }
    StepType stepType() const override { return StepType::BEGINNING_DRAW; }
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Main phase step
struct MainStep : public Step {
    StepType step_type;
    MainStep(Phase* parent_phase, StepType type) : Step(parent_phase), step_type(type) { has_priority_window = true; }
    StepType stepType() const override { return step_type; }
    void onStepStart() override;
    void onStepEnd() override;
};

// End phase steps

// Step for end of turn triggers
struct EndStep : public Step {
    EndStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = true; }
    StepType stepType() const override { return StepType::ENDING_END; }
};

// Step for cleanup actions
struct CleanupStep : public Step {
    CleanupStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = false; }
    StepType stepType() const override { return StepType::ENDING_CLEANUP; }
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Phase implementations

// Initial phase of the turn
struct BeginningPhase : public Phase {
    BeginningPhase(Turn* parent_turn);
};

// First main phase
struct PrecombatMainPhase : public Phase {
    bool canCastSorceries() override { return true; }
    PrecombatMainPhase(Turn* parent_turn);
};

// Second main phase
struct PostcombatMainPhase : public Phase {
    bool canCastSorceries() override { return true; }
    PostcombatMainPhase(Turn* parent_turn);
};

// Final phase of the turn
struct EndingPhase : public Phase {
    EndingPhase(Turn* parent_turn);
};