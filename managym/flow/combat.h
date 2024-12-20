#pragma once

#include <map>

#include "rules/turns/turn.h"

class Permanent;
class ActionSpace;

class CombatPhase : public Phase {
public:
    std::vector<Permanent*> attackers;
    // blocked_by[blocker] = attacker
    std::map<Permanent*, std::vector<Permanent*>> attacker_to_blockers;
    CombatPhase(Turn* parent_turn): Phase(parent_turn) { 
        steps.emplace_back(new BeginningOfCombatStep(*this));
        steps.emplace_back(new DeclareAttackersStep(*this));
        steps.emplace_back(new DeclareBlockersStep(*this));
        steps.emplace_back(new CombatDamageStep(*this));
        steps.emplace_back(new EndOfCombatStep(*this));
    }
};

class CombatStep : public Step {
public:
    CombatPhase* combat_phase;
    CombatStep(CombatPhase& parent_combat_phase);
};

class BeginningOfCombatStep : public CombatStep {
public:
    BeginningOfCombatStep(CombatPhase& parent_combat_phase): CombatStep(parent_combat_phase) {}
};

class DeclareAttackersStep : public CombatStep {
public:
    std::vector<Permanent*> attackers_to_declare;
    
    DeclareAttackersStep(CombatPhase& parent_combat_phase): CombatStep(parent_combat_phase) {}
    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
    virtual void initialize() override;

private:
    std::unique_ptr<ActionSpace> makeActionSpace(Permanent* attacker);
};

class DeclareBlockersStep : public CombatStep {
public:
    std::vector<Permanent*> blockers_to_declare;

    DeclareBlockersStep(CombatPhase& parent_combat_phase): CombatStep(parent_combat_phase) {}
    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
    virtual void initialize() override;

private:
    std::unique_ptr<ActionSpace> makeActionSpace(Permanent* blocker);
};

class CombatDamageStep : public CombatStep {
public:
    CombatDamageStep(CombatPhase& parent_combat_phase): CombatStep(parent_combat_phase) {}
    virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

class EndOfCombatStep : public CombatStep {
public:
    EndOfCombatStep(CombatPhase& parent_combat_phase): CombatStep(parent_combat_phase) {}
};
