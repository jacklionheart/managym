# Performance Diagnosis

## Summary

After significant optimizations (+164% throughput since baseline), the **primary bottleneck is now the skip_trivial loop consuming 69.2% of env_step time**. Within that loop, the Turn hierarchy traversal accounts for 42.3% of total time—625,441 turn ticks service only 12,839 actual player decisions (2.1%). The architecture is fundamentally inefficient: it builds complete ActionSpace objects 556,702 times, yet 97.9% are trivial (single PassPriority action). The remaining opportunities are architectural: integrating skip-trivial into the tick loop and flattening the Turn/Phase/Step hierarchy.

## Current Performance State

**Profile: 2026-01-16-2200** (latest after all optimizations)

| Metric | Value |
|--------|-------|
| Steps/sec | 42,513 |
| Per-step time | 21.5μs |
| Games/sec | 288.60 |
| Avg steps/game | 147.3 |

**Cumulative improvements from baseline:**
- Baseline: 16,122 steps/sec
- Current: 42,513 steps/sec
- **Total improvement: +164%**

## Primary Bottleneck: Skip-Trivial Loop (69.2%)

**Where**: `game.cpp:224-227`
```cpp
while (!game_over && skip_trivial && actionSpaceTrivial()) {
    Profiler::Scope skip_scope = profiler->track("skip_trivial");
    game_over = _step(0);
}
```

**Time**: 1.098s of 1.586s env_step (69.2%)
**Iterations**: 400,128 for 73,652 agent steps (5.4x amplification)

### Why It's Slow

The skip_trivial loop does the same work as a normal step—it just auto-selects action 0:

1. **Builds complete ActionSpace** (`priority.cpp:51-58`) — creates vector of Action unique_ptrs
2. **Allocates PassPriority every time** (`priority.cpp:90`) — 400k+ allocations
3. **Checks `actionSpaceTrivial()`** (`game.cpp:76-78`) — after building the whole ActionSpace
4. **Traverses Turn→Phase→Step→Priority hierarchy** — 625,441 times

### Evidence from Profile

```
env_step/game/skip_trivial: total=1.098s, count=400,128 (69.2%)
├── env_step/game/skip_trivial/tick: total=0.777s, count=400,128 (49.0%)
│   └── env_step/game/skip_trivial/tick/turn: total=0.671s, count=625,441 (42.3%)
│       └── env_step/game/skip_trivial/tick/turn/priority: total=0.234s, count=556,702 (14.8%)
│           └── ...priority/priority: total=0.019s, count=12,839 (1.2%) ← actual decisions
└── env_step/game/skip_trivial/action_execute: total=0.055s, count=400,128 (3.5%)
```

**Key insight**: 556,702 priority ticks produce only 12,839 actual decisions (2.3%). The other 97.7% are trivial (PassPriority only).

## Secondary Bottleneck: Turn Hierarchy Inside Skip-Trivial (42.3%)

**Where**: `turn.cpp:207-227` (Turn::tick) → `turn.cpp:231-250` (Phase::tick) → `turn.cpp:255-304` (Step::tick)
**Time**: 0.671s for 625,441 calls (42.3% of env_step)
**Per-call**: 1.07μs

### Call Flow Analysis

For every game tick inside skip_trivial:
```
Game::_step(0)                          // game.cpp:180
  └── tick()                            // game.cpp:240
      └── turn_system->tick()           // turn.cpp:151
          └── current_turn->tick()      // turn.cpp:207 (Turn::tick)
              └── phase->tick()         // turn.cpp:231 (Phase::tick)
                  └── step->tick()      // turn.cpp:255 (Step::tick)
                      └── priority_system->tick()  // priority.cpp:20
                          └── makeActionSpace()    // priority.cpp:51
                              └── availablePriorityActions() // priority.cpp:67
```

Each Turn creates 5 Phase objects, each Phase creates 1-5 Step objects (`turn.cpp:198-205`, `351-368`).

### Overhead Sources

1. **Object allocation per turn**: `turn.cpp:169` creates new Turn with 5 Phases, ~12 Steps
   - 9,705 turns × (5 Phase + ~12 Step) = ~165,000 object allocations per 500 games

2. **Profiler scope in Turn::tick**: `turn.cpp:208` creates scope 625,441 times
   - 0.671s for turn scope (vs 0.234s for priority = 0.437s turn overhead)

3. **Virtual dispatch chain**: 4 levels of `->tick()` calls with pointer chasing

4. **Step state machine**: Each Step tracks `initialized`, `turn_based_actions_complete`, `has_priority_window`, `completed` flags checked on every tick

## Tertiary Bottleneck: Priority ActionSpace Construction (14.8%)

**Where**: `priority.cpp:67-93`
**Time**: 0.234s for 556,702 calls (14.8% of env_step)
**Per-call**: 0.42μs

### Why It's Slow

Every priority tick:
1. **Iterates hand cards** (`priority.cpp:72-87`): Checks each card for playability
2. **Allocates Action objects** (`priority.cpp:79, 83, 90`): `new PlayLand`, `new CastSpell`, `new PassPriority`
3. **Creates ActionSpace** (`priority.cpp:57`): Wraps actions in unique_ptr vector

**PassPriority allocation is the worst offender**: 556,702 allocations, 97.7% are the only action returned.

### Redundant Work

```cpp
// priority.cpp:32 - creates new vector every call
std::vector<Player*> players = game->playersStartingWithActive();
```
This calls `playersStartingWithActive()` which now returns a cached reference, but the assignment copies it into a new local vector.

## Observation: Now Efficient (9.4%)

**Where**: `observation.cpp:22-39`
**Time**: 0.149s for 73,652 calls (9.4% of env_step)
**Per-call**: 2.0μs

Breakdown:
- populatePermanents: 0.056s (3.5%)
- populateCards: 0.037s (2.3%)
- populateTurn: 0.010s (0.6%) — called 2x per step
- populatePlayers: 0.005s (0.3%)

Observation is well-optimized. Further gains require architectural changes (incremental updates, direct memory access from Python).

## Unaccounted Time: Minimal (2.2%)

**Gap**: 0.035s (2.2% of env_step)

This is likely:
- ActionSpace creation/destruction overhead
- Vector operations for action storage
- Python binding overhead (not measured in C++ profiler)

The profiler now captures 97.8% of step time.

## Time Breakdown (Profile 2026-01-16-2200)

| Component | Time | % | Calls | Per-call |
|-----------|------|---|-------|----------|
| env_step | 1.586s | 100% | 73,652 | 21.5μs |
| → game | 1.402s | 88.4% | 73,652 | 19.0μs |
| → → skip_trivial | **1.098s** | **69.2%** | 400,128 | 2.7μs |
| → → → tick | 0.777s | 49.0% | 400,128 | 1.9μs |
| → → → → turn | **0.671s** | **42.3%** | 625,441 | 1.07μs |
| → → → → → priority | 0.234s | 14.8% | 556,702 | 0.42μs |
| → → → → → → inner priority | 0.019s | 1.2% | 12,839 | 1.5μs |
| → → → action_execute | 0.055s | 3.5% | 400,128 | 0.14μs |
| → → tick (non-trivial) | 0.087s | 5.5% | 73,652 | 1.2μs |
| → → action_execute (non-trivial) | 0.085s | 5.4% | 73,652 | 1.2μs |
| → observation | 0.149s | 9.4% | 73,652 | 2.0μs |
| → (unaccounted) | 0.035s | 2.2% | - | - |

## Tick Amplification Analysis

| Level | Calls | Ratio to Agent Steps | % Trivial |
|-------|-------|---------------------|-----------|
| Agent steps | 73,652 | 1.0x | 0% (by definition) |
| Skip trivial iterations | 400,128 | 5.4x | 100% |
| Turn ticks (total) | 699,093 | 9.5x | ~90% |
| Turn ticks (in skip_trivial) | 625,441 | 8.5x | ~100% |
| Priority ticks (total) | 615,234 | 8.4x | 97.9% |
| Actual decisions | 12,839 | 0.17x | 0% |

**Only 1.8% of priority ticks produce non-trivial ActionSpaces**.

## Recommendations

### 1. Integrate skip_trivial into tick loop (HIGH IMPACT)
**Impact**: ~40-50% improvement
**Location**: `game.cpp:224-227`, `priority.cpp:20-49`

Don't build ActionSpace for trivial actions. Check feasibility before allocating:

```cpp
// In PrioritySystem::tick() or a new fast-path
bool canPlayerAct(Player* player) {
    // Quick check before building full ActionSpace
    if (game->zones->constHand()->cards[player->index].empty()) {
        return false;  // Can only pass priority
    }
    // Check for playable lands/spells...
}
```

Skip the ActionSpace construction entirely when only PassPriority is available.

### 2. Pool or flyweight PassPriority objects
**Impact**: ~5-10%
**Location**: `priority.cpp:90`

556,702 PassPriority allocations. Use one per player, reuse:

```cpp
// In PrioritySystem or TurnSystem
PassPriority pass_actions[2];  // One per player

// In availablePriorityActions:
actions.push_back(&pass_actions[player->index]);  // Raw pointer, not unique_ptr
```

### 3. Avoid copying player vector in priority.cpp
**Impact**: ~1-2%
**Location**: `priority.cpp:32, 52, 97`

```cpp
// Current (copies vector):
std::vector<Player*> players = game->playersStartingWithActive();

// Better (use reference):
const std::vector<Player*>& players = game->playersStartingWithActive();
```

### 4. Remove Turn::tick profiler scope for non-debug builds
**Impact**: ~5-8%
**Location**: `turn.cpp:208`

625,441 profiler scope creations. Either:
- Use `#ifdef DEBUG` around profiler calls
- Make profiler scopes completely zero-overhead when disabled

### 5. Consider flat state machine (ARCHITECTURAL)
**Impact**: ~30-50%
**Location**: `turn.cpp:198-368`

Replace Turn/Phase/Step object hierarchy with switch-based state machine. See `.design/refactor-proposal.md` for full proposal.

## Addressed Optimizations

| Optimization | Profile | Impact |
|--------------|---------|--------|
| Lazy observation (skip during internal ticks) | 20260116-1614 | 17% → 4% observation |
| Map → vector in Observation | 20260116-1614 | Part of above |
| Cached producible mana | 20260116-1614 | Removed mana iteration |
| Cache playersStartingWithActive() | 20260116-1631 | +3.0% steps/sec |
| InfoDict only at episode end | post-infodict | **+129% steps/sec** |
| Remove phase/step profiler scopes | 20260116-2200 | **+8.7% steps/sec** |

## Open Questions

1. **Profiler overhead when disabled?** `Profiler::Scope` objects are created even with `enable_profiler=false`. Constructor checks `enabled` but still runs. Verify this is negligible.

2. **Python binding overhead?** pybind11 marshalling not measured. Could explain some unaccounted time in full pipeline.

3. **Target throughput?** Current 42k steps/sec. Is 50k? 100k? Determines whether architectural refactors are worth the risk.

## Related Documents

- `.design/refactor-proposal.md` — Bold architectural changes (flat state machine, integrated skip-trivial)
- `.design/profile-20260116-2200.md` — Latest profile data
- `.design/questions.md` — Open questions about performance targets
