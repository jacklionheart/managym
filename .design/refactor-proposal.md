# Refactor Proposals

## Summary
The core architectural problem is **work proportional to micro-ticks, not to decisions**. The
turn loop advances through fine-grained step/priority states that rarely produce choices, while
observation building reconstructs full snapshots every step. The fastest path is a **decision-
driven scheduler** that jumps directly between decision points and a **persistent observation
snapshot** that updates incrementally inside the C++ core (not deferred at the API).

## Proposals

### 1. Decision-Point Scheduler (Replace Micro-Ticks) — Impact: ~30–50%
**Current**: `Game::tick()` repeatedly calls `TurnSystem::tick()` which advances phases/steps one
micro-tick at a time (`managym/flow/game.cpp:205-241`, `managym/flow/turn.cpp:147-160`). Each step
runs a priority window even when no choices exist, causing ~4.2 turn ticks per env step.
**Problem**: Per-tick overhead dominates when most ticks do not produce decisions.
**Proposal**: Replace micro-ticks with a **decision-point scheduler** that fast-forwards to the next
choice point (priority/declare attackers/declare blockers/game over) and batch-applies deterministic
step effects.
**Sketch**:
1. Add `DecisionPoint` enum (`PRIORITY`, `DECLARE_ATTACKER`, `DECLARE_BLOCKER`, `GAME_OVER`).
2. Move per-step deterministic effects into `applyStepEffects()` and gate choice with
   `decisionRequired()`.
3. Add `TurnSystem::nextDecision()` that loops steps internally until a decision is required,
   returning the resulting `ActionSpace`.
4. `Game::tick()` becomes a loop over `nextDecision()` instead of calling `TurnSystem::tick()`.
**Risk**: Requires careful audit of MTG rule timing (state-based actions, priority windows, step
boundaries). Misordered effects can cause subtle rule violations.

### 2. Priority Window Snapshotting + Invalidation — Impact: ~15–30%
**Current**: Every priority pass recomputes playable actions and checks mana from scratch
(`managym/flow/priority.cpp:20-57`). This repeats 4–5x per step across the same window.
**Problem**: O(hand * battlefield) work repeats without state change.
**Proposal**: Treat each priority window as a stable snapshot and reuse computed actions until a
relevant mutation invalidates it.
**Sketch**:
1. Add `priority_epoch` to `Game`, bump on hand/stack/battlefield/tap changes.
2. Cache `PriorityWindow` per player: `{epoch, actions, can_act}`.
3. `PrioritySystem::tick()` reuses cached actions if `epoch` unchanged.
4. Centralize invalidation in `Zones::move`, `Zones::pushStack/popStack`,
   `Battlefield::tap/untap`, and other state mutations.
**Risk**: Invalidation gaps allow illegal actions; requires complete mutation coverage.

### 3. Persistent Observation Snapshot (Dirty-Flagged) — Impact: ~15–25%
**Current**: `Observation` rebuilds all vectors and `CardData` every step
(`managym/agent/observation.cpp:22-202`).
**Problem**: Most fields are unchanged between steps, but full rebuild cost is paid every time.
**Proposal**: Maintain a persistent `ObservationState` inside `Game` with dirty flags per zone,
player, and battlefield list, then serialize into the public `Observation` struct each step.
**Sketch**:
1. Add `ObservationState` with cached `agent/opponent` vectors and zone counts.
2. Track dirty flags in zones and battlefield (e.g., on move, draw, destroy, tap/untap).
3. Update cached slices eagerly on mutation; `Observation::Observation(Game*)` becomes a light
   copy from caches.
4. Keep the public Observation struct unchanged to preserve Python bindings.
**Risk**: Stale data if any mutation path misses a dirty flag; increases complexity.

### 4. ActionSpace Templates + Object Pools — Impact: ~10–20%
**Current**: Every decision allocates new `Action` objects and vectors (`managym/agent/action.h`,
`managym/flow/priority.cpp`).
**Problem**: Allocation churn in the hottest loop, especially for common cases like PassPriority.
**Proposal**: Introduce action pools and templates for common actions, reuse immutable actions
across ticks, and reserve vector capacity up front.
**Sketch**:
1. Add per-type `ActionPool<T>` with `reset()` or `init()` methods.
2. Cache singleton PassPriority actions per player.
3. Build ActionSpace with pre-reserved capacity and pooled actions.
4. Return actions to pools on ActionSpace destruction.
**Risk**: Lifetime and ownership bugs if actions escape the ActionSpace scope.

## Sequence
1. Decision-Point Scheduler (structural shift; unlocks other wins).
2. Priority Window Snapshotting (pairs with scheduler for big tick-loop savings).
3. Persistent Observation Snapshot (largest observation-side savings).
4. ActionSpace Templates + Pools (allocation cleanup once structure stabilizes).

## Open questions
1. Which steps are guaranteed to have no decisions for the current card sets (untap, cleanup, etc.)?
2. What is the minimal mutation set required to invalidate priority windows safely?
3. Can ObservationState remain internal without changing the Python Observation API?
4. Are Action objects ever referenced outside the ActionSpace lifetime (logs, trackers, debug)?
