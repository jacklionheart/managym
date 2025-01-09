#pragma once

#include <map>
#include <vector>

#include "managym/flow/combat.h"
#include "managym/flow/turn.h"

class Permanent;
class ActionSpace;

struct CombatPhase : public Phase {
  CombatPhase(Turn* parent_turn);

  std::vector<Permanent*> attackers;
  std::map<Permanent*, std::vector<Permanent*>> attacker_to_blockers;
};

struct CombatStep : public Step {
  CombatPhase* combat_phase;
  CombatStep(CombatPhase* parent_combat_phase);
};

struct BeginningOfCombatStep : public CombatStep {
  BeginningOfCombatStep(CombatPhase* parent_combat_phase)
      : CombatStep(parent_combat_phase) {}
};

struct DeclareAttackersStep : public CombatStep {
  std::vector<Permanent*> attackers_to_declare;

  DeclareAttackersStep(CombatPhase* parent_combat_phase)
      : CombatStep(parent_combat_phase) {}
  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
  virtual void initialize() override;
  std::unique_ptr<ActionSpace> makeActionSpace(Permanent* attacker);
};

struct DeclareBlockersStep : public CombatStep {
  std::vector<Permanent*> blockers_to_declare;

  DeclareBlockersStep(CombatPhase* parent_combat_phase)
      : CombatStep(parent_combat_phase) {}
  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
  virtual void initialize() override;
  std::unique_ptr<ActionSpace> makeActionSpace(Permanent* blocker);
};

struct CombatDamageStep : public CombatStep {
  CombatDamageStep(CombatPhase* parent_combat_phase)
      : CombatStep(parent_combat_phase) {}
  virtual std::unique_ptr<ActionSpace> performTurnBasedActions() override;
};

struct EndOfCombatStep : public CombatStep {
  EndOfCombatStep(CombatPhase* parent_combat_phase)
      : CombatStep(parent_combat_phase) {}
};
