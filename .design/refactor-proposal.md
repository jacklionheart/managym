# Refactor Proposals

## Summary

The core architectural problem is **turn-system work that is proportional to ticks, not decisions**.
We advance through many micro-steps and priority passes that rarely produce a decision, yet we
rebuild action spaces and re-scan state each time. The observation pipeline then re-walks the same
state from scratch every step. To get order‑of‑magnitude gains, the engine needs a **decision‑driven
loop** and **incremental observation state**, not just faster versions of the current loops.

## Proposals

### 1. Decision-Driven Turn Loop — Impact: ~25–40%
**Current**: `Game::tick()` loops `TurnSystem::tick()` through phases/steps; most ticks produce no
ActionSpace but still run priority and bookkeeping. The action loop is time‑based, not decision‑based.
**Problem**: 311k turn ticks for 73k env steps (profile) means we burn CPU on non‑decision ticks.
**Proposal**: Replace the per‑tick loop with a *decision scheduler* that advances directly to the
next decision point (priority or combat declarations) and executes deterministic steps in batches.
**Sketch**:
1. Add `DecisionPoint` enum (`PRIORITY`, `DECLARE_ATTACKER`, `DECLARE_BLOCKER`, `GAME_OVER`).
2. Give `TurnSystem` a `nextDecision()` method that fast‑forwards over steps with no choices.
3. Move deterministic step logic into `applyStepEffects()` called during fast‑forward.
4. `Game::tick()` becomes: `while (!decision) { nextDecision(); } return ActionSpace`.
**Risk**: Must preserve rule order and ensure no skipped state changes; requires careful auditing
of step boundaries and state-based actions.

### 2. Priority Window Consolidation — Impact: ~15–25%
**Current**: Priority is reevaluated per pass with repeated hand scans and mana checks, even when
nothing changed since the last pass.
**Problem**: Same playable set and pass-only cases are recomputed every pass; the pass loop is
O(hand * battlefield) each time.
**Proposal**: Treat a priority window as a *stable snapshot* and invalidate only on state changes.
Compute playable actions once per priority window, reuse across passes until a state mutation occurs.
**Sketch**:
1. Add `priority_epoch` on `Game` that increments on any state mutation affecting playability
   (hand changes, battlefield tap/untap, stack changes).
2. Cache `PriorityWindow` per player: playable actions + `epoch` seen.
3. In `PrioritySystem::tick()`, if `epoch` unchanged for the player, reuse cached actions.
4. When an action executes, update `priority_epoch` if it mutates relevant state.
**Risk**: Incorrect invalidation could allow illegal actions; requires a clear set of mutation hooks.

### 3. Incremental Observation State (Dirty Flags) — Impact: ~10–20%
**Current**: `Observation` is rebuilt from scratch each step; all zones and permanents are re-walked.
**Problem**: Most fields are unchanged between steps, but we pay full construction cost.
**Proposal**: Maintain a persistent `ObservationState` inside `Game` that is *updated incrementally*
using dirty flags per zone/player and only serialized into the public `Observation` struct each step.
**Sketch**:
1. Add `ObservationState` caches for player counts, zone card lists, permanent lists.
2. Add dirty flags on zones and per‑player caches (e.g., `hand_dirty[player]`).
3. On mutation (move, destroy, draw, tap/untap), mark dirty and update cached slices.
4. `Observation::Observation(Game*)` becomes a cheap copy from cached state.
**Risk**: Stale data if dirty flags are missed; higher code complexity.

### 4. ActionSpace Templates + Object Pools — Impact: ~8–15%
**Current**: Each decision allocates new `Action` objects and vectors, even for common cases like
`PassPriority` or `DeclareAttacker`.
**Problem**: High allocation churn in the hottest path; Action lifetimes are extremely short.
**Proposal**: Pre-build action templates and reuse pooled Action objects for each step type, only
patching focus targets and player pointers.
**Sketch**:
1. Introduce `ActionPool<T>` per action type; add `reset()` to Action classes.
2. For fixed patterns (e.g., pass priority), reuse a singleton action per player.
3. Build `ActionSpace` from pooled actions; return to pool on space destruction.
4. `Observation::populateActionSpace()` reads pooled focus data without extra copies.
**Risk**: Pool misuse can lead to use‑after‑free if actions outlive the action space; needs strict
ownership rules.

## Sequence

1. **Decision-Driven Turn Loop** — biggest win; unlocks fewer priority passes.
2. **Priority Window Consolidation** — pairs with decision loop to cut repeated scanning.
3. **Incremental Observation State** — isolates observation cost after tick loop gains.
4. **ActionSpace Templates + Pools** — allocation cut once behavior is stable.

## Open questions

1. Which step transitions are guaranteed to have no choices (e.g., cleanup, untap) for *all*
current cardsets, and can we codify them for fast‑forward?
2. What are the minimal mutation hooks needed to safely invalidate cached priority windows?
3. Can we keep the public Observation API intact while adding a cached internal representation,
or do we need a versioned Observation struct?
4. Are Action objects ever referenced outside the action space lifetime (e.g., in logs or trackers)?
