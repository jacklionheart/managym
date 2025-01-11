#pragma once

#include "managym/flow/turn.h"

#include <map>
#include <vector>

class Permanent;
class ActionSpace;

// Main phase for handling combat sequence
struct CombatPhase : public Phase {
    CombatPhase(Turn* parent_turn);

    // Data
    std::vector<Permanent*> attackers;
    std::map<Permanent*, std::vector<Permanent*>> attacker_to_blockers;
};

// Base step for all combat steps
struct CombatStep : public Step {
    CombatStep(CombatPhase* parent_combat_phase);

    // Data
    CombatPhase* combat_phase;
};

// First step of combat phase
struct BeginningOfCombatStep : public CombatStep {
    BeginningOfCombatStep(CombatPhase* parent_combat_phase) : CombatStep(parent_combat_phase) {}
};

// Step for declaring attacking creatures
struct DeclareAttackersStep : public CombatStep {
    DeclareAttackersStep(CombatPhase* parent_combat_phase) : CombatStep(parent_combat_phase) {}

    // Data
    std::vector<Permanent*> attackers_to_declare;

    // Writes

    // Process turn-based actions for this step
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
    // Initialize step state
    void initialize() override;
    // Create action space for a potential attacker
    std::unique_ptr<ActionSpace> makeActionSpace(Permanent* attacker);
};

// Step for declaring blocking creatures
struct DeclareBlockersStep : public CombatStep {
    DeclareBlockersStep(CombatPhase* parent_combat_phase) : CombatStep(parent_combat_phase) {}

    // Data
    std::vector<Permanent*> blockers_to_declare;

    // Writes

    // Process turn-based actions for this step
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
    // Initialize step state
    void initialize() override;
    // Create action space for a potential blocker
    std::unique_ptr<ActionSpace> makeActionSpace(Permanent* blocker);
};

// Step for dealing and resolving combat damage
struct CombatDamageStep : public CombatStep {
    CombatDamageStep(CombatPhase* parent_combat_phase) : CombatStep(parent_combat_phase) {}

    // Writes

    // Process turn-based actions for this step
    std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

// Final step of combat phase
struct EndOfCombatStep : public CombatStep {
    EndOfCombatStep(CombatPhase* parent_combat_phase) : CombatStep(parent_combat_phase) {}
};