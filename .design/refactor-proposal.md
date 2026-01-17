# Refactor Proposals

## Summary
The core architectural problem is **work proportional to micro‑ticks, not to decisions**. The turn
loop repeatedly advances tiny step/priority states that usually yield no choices, while the
observation path rebuilds full state snapshots each step. A 10x path requires a **decision‑driven
scheduler** that jumps directly between decision points and **incremental state materialization**
inside the observation boundary (not deferral at the API).

## Proposals

### 1. Decision Scheduler + Fast‑Forwarded Steps — Impact: ~30–50%
**Current**: `Game::tick()` iterates `TurnSystem::tick()` through phases/steps; each step may run
priority checks even when no choices exist (`managym/flow/game.cpp:205-241`, `managym/flow/turn.cpp:147-160`).
**Problem**: ~4.2 turn ticks per env step (profile), so fixed per‑tick overhead dominates even when
no player decisions are produced.
**Proposal**: Replace the micro‑tick loop with a **decision scheduler** that advances directly to
next decision points (priority or combat declarations) and batch‑applies deterministic step effects.
**Sketch**:
1. Introduce `DecisionPoint` enum (`PRIORITY`, `DECLARE_ATTACKER`, `DECLARE_BLOCKER`, `GAME_OVER`).
2. Add `TurnSystem::nextDecision()` that fast‑forwards over steps with no choices, calling
   `applyStepEffects()` for each skipped step.
3. Move step logic from `Step::tick()` into `applyStepEffects()` + `decisionRequired()`.
4. `Game::tick()` becomes a loop over `nextDecision()` until an ActionSpace is produced.
**Risk**: Must preserve MTG rule order and state‑based action timing; requires careful audit of
step boundaries and when priority windows open/close.

### 2. Priority Window Snapshotting — Impact: ~15–30%
**Current**: Each priority pass recomputes playable actions, rescanning hand and rechecking mana
(`managym/flow/priority.cpp:20-57`).
**Problem**: The same pass can recompute identical action sets multiple times across a single
priority window; O(hand * battlefield) repeats 4–5x per step.
**Proposal**: Treat each priority window as a **stable snapshot** with invalidation on relevant
mutations. Reuse playable actions across passes until state changes.
**Sketch**:
1. Add `priority_epoch` on `Game`, incremented on hand/stack/battlefield or tap/untap changes.
2. Cache `PriorityWindow` per player: `{epoch, actions, can_act}`.
3. In `PrioritySystem::tick()`, reuse cached actions if `epoch` unchanged.
4. Centralize invalidation in `Zones::move`, `Battlefield::tap/untap`, stack push/pop.
**Risk**: Incorrect invalidation permits illegal actions; requires comprehensive mutation hooks.

### 3. Incremental Observation State (Persistent Snapshot) — Impact: ~15–25%
**Current**: `Observation` fully rebuilt each step; zones and permanents are re‑walked (`managym/agent/observation.cpp:22-202`).
**Problem**: Most fields are unchanged between steps, but all costs are paid every step.
**Proposal**: Maintain a **persistent ObservationState** inside `Game` with dirty‑flagged slices
for players/zones/permanents, and serialize into the public Observation struct each step.
**Sketch**:
1. Add `ObservationState` in `Game` with cached vectors for agent/opponent cards/permanents.
2. Add dirty flags per zone/player and per‑battlefield list.
3. On mutation (move, destroy, draw, tap/untap), update caches and mark dirty.
4. `Observation::Observation(Game*)` becomes a light copy from cached state.
**Risk**: Stale data if any mutation path misses a dirty flag; increases complexity.

### 4. ActionSpace Templates + Pools — Impact: ~10–20%
**Current**: New Action objects and vectors are allocated every decision point, even for common
cases like `PassPriority` (`managym/agent/action.h`, `managym/flow/priority.cpp`).
**Problem**: Allocation churn in the hottest loop, short lifetimes, and repeated vector growth.
**Proposal**: Pre‑allocate Action templates and use object pools per action type; reuse immutable
actions (e.g., pass) and only patch focus for variable actions.
**Sketch**:
1. Add `ActionPool<T>` per action type; add `reset()` to actions with mutable fields.
2. Cache singleton `PassPriority` actions per player.
3. Build `ActionSpace` using pooled actions and reserved capacity.
4. Return actions to pools when ActionSpace is destroyed.
**Risk**: Lifetime errors if actions escape ActionSpace scope; requires strict ownership discipline.

## Sequence
1. Decision Scheduler + Fast‑Forwarded Steps
2. Priority Window Snapshotting (pairs with decision scheduler)
3. Incremental Observation State
4. ActionSpace Templates + Pools

## Open questions
1. Which step transitions are guaranteed to have no decisions for the current card sets?
2. What is the minimal mutation set required to invalidate a priority window?
3. Can ObservationState be added without changing the Python API, or do we need a versioned struct?
4. Are Action objects referenced outside ActionSpace lifetime (logging, trackers, debug tools)?
