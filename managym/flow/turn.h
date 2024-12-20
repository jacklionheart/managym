#pragma once

#include <vector>
#include <memory>
#include <map>

#include "rules/turns/priority.h"

class ActionSpace;
class Step;
class Phase;
class Turn;
class Player;
class Game;

class TurnSystem {
public:
    std::unique_ptr<Turn> current_turn;
    int active_player_index;
    int global_turn_count;
    std::map<Player*, int> turn_counts;
    Game* game;

    Player* activePlayer();
    Player* nonActivePlayer();

    TurnSystem(Game* game);
    std::unique_ptr<ActionSpace> tick();
    std::unique_ptr<ActionSpace> startNextTurn();
    std::vector<Player*> priorityOrder();

};  

class Turn {
public:
    TurnSystem* turn_system;
    Player* active_player;

    std::vector<std::unique_ptr<Phase>> phases;
    int current_phase_index = 0;
    int lands_played = 0;

    Turn(Player* active_player, TurnSystem* turn_system);
    std::unique_ptr<ActionSpace> tick();
    bool isComplete() {
        return current_phase_index >= phases.size();
    }
};

class Phase {
public:
    Turn* turn;
    
    std::vector<std::unique_ptr<Step>> steps;
    int current_step_index = 0;

    Phase(Turn* turn) : turn(turn) {}
    
    bool isComplete() {
        return current_step_index >= steps.size();
    }

    std::unique_ptr<ActionSpace> tick();
    virtual ~Phase() = default;
};

class Step {
public:
    Phase* phase;
    bool initialized = false;
    bool has_priority_window = true;
    bool turn_based_actions_complete = false;
    bool mana_pools_emptied = false;

    std::unique_ptr<PrioritySystem> priority_system;

    bool isComplete();

    virtual void initialize() {}

    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() {
        return nullptr;
    };

    Step(Phase* phase): phase(phase), priority_system(std::make_unique<PrioritySystem>(phase->turn->turn_system->game, phase->turn->active_player)) {}

    // Inline reference accesors
    Game* game() {
        return phase->turn->turn_system->game;
    }
    TurnSystem* turn_system() {
        return phase->turn->turn_system;
    }
    Turn* turn() {
        return phase->turn;
    }
    Player* activePlayer() {
        return phase->turn->active_player;
    }   

    std::unique_ptr<ActionSpace> tick();
    virtual ~Step() = default;
};

// Define specific Steps and Phases


// BeginningSteps

class UntapStep : public Step {
public:
    UntapStep(Phase* parent_phase) : Step(parent_phase) { has_priority_window = false; }

    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

class UpkeepStep : public Step {
public:
    UpkeepStep(Phase* parent_phase) : Step(parent_phase) {
        has_priority_window = true;
    }
};

class DrawStep : public Step {
public:
    DrawStep(Phase* parent_phase) : Step(parent_phase) {
        has_priority_window = true;
    }
    
    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Main Step

class MainStep : public Step {
public:
    bool can_cast_sorceries = true;
    MainStep(Phase* parent_phase) : Step(parent_phase) { 
        has_priority_window = true;
        can_cast_sorceries = true; 
    }
};


// Ending Steps

class EndStep : public Step {
public:
    EndStep(Phase* parent_phase) : Step(parent_phase) {
        has_priority_window = true;
    }
};

class CleanupStep : public Step {
public:
    CleanupStep(Phase* parent_phase) : Step(parent_phase) {
        has_priority_window = false;
    }

    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};


// Phases
class BeginningPhase : public Phase {
public:
    BeginningPhase(Turn* parent_turn) : Phase(parent_turn) {
        steps.emplace_back(new UntapStep(this));
        steps.emplace_back(new UpkeepStep(this));
        steps.emplace_back(new DrawStep(this));
    }
};

class PrecombatMainPhase : public Phase {
public:
PrecombatMainPhase(Turn* parent_turn): Phase(parent_turn) {
        steps.emplace_back(new MainStep(this));
    }
};

class PostcombatMainPhase : public Phase {
public:
    PostcombatMainPhase(Turn* parent_turn): Phase(parent_turn) {
        steps.emplace_back(new MainStep(this));
    }
};

class EndingPhase : public Phase {
public:
    EndingPhase(Turn* parent_turn): Phase(parent_turn) {
        steps.emplace_back(new EndStep(this));
        steps.emplace_back(new CleanupStep(this));
    }
};


