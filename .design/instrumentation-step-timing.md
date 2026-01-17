# Instrumentation: Per-Step-Type Timing

Shows time spent in each step type (UNTAP, UPKEEP, DRAW, MAIN, COMBAT_*, etc.).

## What It Measures

Each step in the turn structure is now tracked with a profiler scope named `step/<STEP_TYPE>`:

- `step/BEGINNING_UNTAP` — Permanents untap
- `step/BEGINNING_UPKEEP` — Upkeep triggers + priority
- `step/BEGINNING_DRAW` — Draw card + priority
- `step/PRECOMBAT_MAIN_STEP` — First main phase with priority
- `step/COMBAT_BEGIN` — Beginning of combat + priority
- `step/COMBAT_DECLARE_ATTACKERS` — Attacker declarations
- `step/COMBAT_DECLARE_BLOCKERS` — Blocker declarations
- `step/COMBAT_DAMAGE` — Damage assignment + priority
- `step/COMBAT_END` — End of combat + priority
- `step/POSTCOMBAT_MAIN_STEP` — Second main phase with priority
- `step/ENDING_END` — End step + priority
- `step/ENDING_CLEANUP` — Cleanup (no priority)

## How to Read the Output

Run the profiler:

```bash
python scripts/profile.py --games 500 --seed 42
```

Look for `step/` entries in the profiler output:

```
env_step/game/tick/turn/step/PRECOMBAT_MAIN_STEP: total=0.028s, count=8681
env_step/game/tick/turn/step/POSTCOMBAT_MAIN_STEP: total=0.013s, count=4621
env_step/game/tick/turn/step/COMBAT_DECLARE_ATTACKERS: total=0.007s, count=8528
```

Key metrics:
- **total**: Wall-clock time spent in that step type
- **count**: How many times the step was entered
- **per-call**: `total / count` — average time per step entry

## What to Do With This Information

**If a main phase dominates:**
- Look at `step/PRECOMBAT_MAIN_STEP/priority` — is priority checking the bottleneck?
- Consider if `canPlayerAct()` fast-path is working (should reduce priority calls)
- Check action construction in `availablePriorityActions()`

**If combat steps dominate:**
- Check `COMBAT_DECLARE_ATTACKERS` — may indicate large creature boards
- Look at attacker/blocker iteration in combat.cpp
- Consider skip-empty-combat optimization from refactor-proposal.md

**If beginning/ending steps are slow:**
- Usually indicates issues with untap/draw operations
- Check `zones->forEachPermanent` in `untapAllPermanents()`

**Step count anomalies:**
- High count for COMBAT_DECLARE_ATTACKERS vs other combat steps indicates many attacker decisions
- Compare counts across runs to understand turn structure overhead

## Implementation Details

Added `stepType()` virtual method to `Step` base class. Each step subclass overrides it to return its `StepType`. The profiler scope is created at the start of `Step::tick()`:

```cpp
std::unique_ptr<ActionSpace> Step::tick() {
    std::string scope_name = std::string("step/") + toString(stepType());
    Profiler::Scope scope = game()->profiler->track(scope_name);
    // ... rest of step logic
}
```

Overhead is minimal because:
1. `stepType()` is a direct return (no computation)
2. `toString(StepType)` is a switch statement (fast)
3. Profiler scopes are already used throughout the hot path

## Files Changed

- `managym/flow/turn.h` — Added `stepType()` pure virtual to `Step`, implemented in each step subclass
- `managym/flow/turn.cpp` — Added profiler scope in `Step::tick()`, updated `MainStep` constructor
- `managym/flow/combat.h` — Added `stepType()` to all combat step subclasses
