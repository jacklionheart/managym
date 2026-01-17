# Performance Diagnosis

## Summary

The tick loop dominates performance after observation optimizations reduced that overhead from 17% to 3.4%. The primary bottleneck is now the **turn/phase/step hierarchy** which consumes 31% of total time despite minimal actual work. The 10.1x tick amplification (98,502 internal ticks for 9,779 agent steps) means the overhead of traversing the object hierarchy and creating profiler scopes compounds dramatically. Secondary bottleneck is the priority system at 11.5%, where repeated `playersStartingWithActive()` calls write to vectors on every invocation.

## Primary Bottleneck: Turn Hierarchy Overhead

**Where**: `turn.cpp:205-248` (Turn::tick, Phase::tick, Step::tick cascade)
**Time**: 30.8% of env_step (0.169s for 98,502 calls)
**Per-call**: 1.7μs

### Why It's Slow

The profiler shows a revealing pattern in the nested timing:
- `turn`: 0.169s for 98,502 calls
- `phase`: 0.153s for 98,502 calls (0.016s = 9.5% lost between turn→phase)
- `step`: 0.130s for 98,502 calls (0.023s = 13.5% lost between phase→step)
- `priority`: 0.063s for 87,590 calls

The gaps between these levels represent pure overhead:
1. **Profiler scope creation**: Each level creates a `Profiler::Scope` object (`turn.cpp:206`, `230`, `255`, `priority.cpp:21`). Four scope objects × 98,502 calls = 394,008 scope creations/destructions.

2. **Object hierarchy traversal**: Each tick follows `TurnSystem::tick()` → `Turn::tick()` → `Phase::tick()` → `Step::tick()`. Each method:
   - Creates a profiler scope
   - Gets the current child object
   - Calls tick on it
   - Checks completion state

3. **Turn construction allocations**: `turn.cpp:196-203` allocates 5 Phase objects per turn, and each Phase allocates 1-5 Step objects (`turn.cpp:351-368`). With ~7.2 turns per game × 200 games = 1,444 turns, this is ~7,220 Phase allocations and ~14,440 Step allocations.

### Evidence

The call count ratio tells the story:
- 9,779 agent steps (decisions that matter)
- 67,822 game ticks (6.9x amplification from skip_trivial loop)
- 98,502 turn/phase/step ticks (10.1x amplification from Magic's turn structure)

Most of these ticks do nothing useful - they just traverse the hierarchy to find the next decision point.

## Secondary Bottleneck: Priority System

**Where**: `priority.cpp:20-49` (tick method)
**Time**: 11.5% of env_step (0.063s for 87,590 calls)
**Per-call**: 0.7μs

### Why It's Slow

1. **playersStartingWithActive() writes every call**: `turn.cpp:175-186` rebuilds the player order vector on every call:
   ```cpp
   const std::vector<Player*>& TurnSystem::playersStartingWithActive() {
       if (players_active_first.size() != num_players) {
           players_active_first.resize(num_players);
       }
       for (int i = 0; i < num_players; i++) {  // WRITES on every call
           players_active_first[i] = game->players[(active_player_index + i) % num_players].get();
       }
       return players_active_first;
   }
   ```
   Called at `priority.cpp:16`, `32`, `52`, `97` - that's 4× per priority tick × 87,590 = ~350,000 vector writes.

2. **availablePriorityActions allocations**: `priority.cpp:67-94` creates new Action objects for every priority check:
   - `new PlayLand()` per playable land
   - `new CastSpell()` per castable spell
   - `new PassPriority()` always

   With 87,590 priority ticks and most being trivial (only PassPriority), this is ~87,590 PassPriority allocations.

3. **Mana affordability already cached**: The `game->canPayManaCost()` now uses cached producible mana (`game.cpp:129-135`), so this is no longer a significant cost. The cache invalidation on battlefield/tap changes works correctly.

### Evidence

The innermost priority scope (`env_step/game/tick/turn/phase/step/priority/priority`) shows only 2,669 calls vs 87,590 outer priority calls. This means 84,921 priority ticks (97%) are pass-throughs that don't result in actual player decisions.

## Unaccounted Time

**Gap**: 0.548s (env_step) - 0.253s (game) - 0.019s (observation) = 0.276s (50.4% unaccounted)

### What's In The Gap

1. **skip_trivial loop** (`game.cpp:210-212`): After each `_step()`, the loop checks `actionSpaceTrivial()` and potentially calls `_step(0)` again. This is where the 67,822 game ticks come from vs 9,779 agent steps.

2. **Action execution** (`game.cpp:197`): Each action's `execute()` method runs outside profiler tracking. For 9,779 steps, this includes:
   - PlayLand: zone move + mana cache invalidation
   - CastSpell: zone move to stack
   - PassPriority: increment pass_count
   - DeclareAttacker/Blocker: modify combat state

3. **Game state queries**: `isGameOver()`, `actionSpaceTrivial()`, `winnerIndex()` called multiple times per step.

### Instrumentation Needed

Add profiler scopes for:
```cpp
// game.cpp:197
Profiler::Scope scope = profiler->track("action_execute");
current_action_space->actions[action]->execute();
```

```cpp
// game.cpp:210-212 - track skip count
Profiler::Scope scope = profiler->track("skip_trivial_loop");
while (!game_over && skip_trivial && actionSpaceTrivial()) { ... }
```

## Addressed (Previous Optimizations)

### Observation std::map → std::vector (2026-01-16)

**Change**: Replaced `std::map<int, CardData>` and `std::map<int, PermanentData>` with `std::vector<CardData>` and `std::vector<PermanentData>` in observation.h/cpp.

**Impact**: Observation creation went from ~19% of env_step time to ~4%.

### Lazy observation creation (2026-01-16)

**Change**: Made observation creation lazy - only create when `Game::observation()` is called, not on every internal tick.

**Impact**: Observation overhead dropped from ~18% to ~4% of env_step time.

### Cached producible mana (2026-01-16)

**Change**: `ManaCache` added to Game (`game.h:36-44`). Invalidated on battlefield enter/exit and tap/untap.

**Impact**: `canPayManaCost()` now uses cached values instead of iterating permanents.

### Zone map → vector conversion (Already done)

**Note**: The diagnosis previously listed this as a remaining bottleneck, but the code shows:
- `Zone::cards` is already `std::vector<std::vector<Card*>>` (`zone.h:24`)
- `Battlefield::permanents` is already `std::vector<std::vector<std::unique_ptr<Permanent>>>` (`battlefield.cpp:82`)

This optimization was already implemented.

## Recommendations

### 1. Cache playersStartingWithActive() (Quick win)
**Impact**: ~5-10%
**Location**: `turn.cpp:175-186`

Only rebuild when `active_player_index` changes:
```cpp
const std::vector<Player*>& TurnSystem::playersStartingWithActive() {
    if (cached_active_index != active_player_index) {
        for (int i = 0; i < num_players; i++) {
            players_active_first[i] = game->players[(active_player_index + i) % num_players].get();
        }
        cached_active_index = active_player_index;
    }
    return players_active_first;
}
```

### 2. Reduce profiler scope nesting (Medium effort)
**Impact**: ~10-15%
**Location**: `turn.cpp:206, 230, 255`, `priority.cpp:21`

Replace nested scopes with a single scope at the step level:
```cpp
std::unique_ptr<ActionSpace> Step::tick() {
    Profiler::Scope scope = game()->profiler->track("step_tick");
    // ... all logic here, no scopes in Turn::tick or Phase::tick
}
```

### 3. Pre-allocate PassPriority action
**Impact**: ~3-5%
**Location**: `priority.cpp:90`

Pool or reuse PassPriority since it's created 87,590 times with identical behavior.

### 4. Skip steps without possible decisions (Phase/Step Skipping)
**Impact**: ~15-25%
**Location**: `turn.cpp:151-163`

Before entering a step, check if it can produce non-trivial actions:
- Untap step: no priority window, always skip priority
- Empty hand + no battlefield abilities = skip main phase priority
- No creatures = skip combat steps entirely

This could reduce the 10.1x amplification to ~2-3x.

### 5. Flatten state machine (Major refactor)
**Impact**: ~25-35%
**Location**: `turn.cpp` entire file

Replace Turn/Phase/Step object hierarchy with flat state machine. See `.design/refactor-proposal.md` for details.

## Time Breakdown

| Component | Time | % | Calls | Per-call | Notes |
|-----------|------|---|-------|----------|-------|
| env_step | 0.548s | 100% | 9,779 | 56.1μs | Total step time |
| → game | 0.253s | 46.2% | 9,779 | 25.9μs | Tick loop wrapper |
| → → tick | 0.185s | 33.7% | 67,822 | 2.7μs | Game ticks (6.9x agent steps) |
| → → → turn | 0.169s | 30.8% | 98,502 | 1.7μs | Turn hierarchy entry |
| → → → → phase | 0.153s | 27.9% | 98,502 | 1.6μs | Phase traversal |
| → → → → → step | 0.130s | 23.7% | 98,502 | 1.3μs | Step logic |
| → → → → → → priority | 0.063s | 11.5% | 87,590 | 0.7μs | Priority system |
| → observation | 0.019s | 3.4% | 9,779 | 1.9μs | Observation building |
| → (unaccounted) | 0.276s | 50.4% | - | - | Action execution, skip loop |

## Instrumentation

### Profile Comparison Tool (2026-01-16)

Added comparative profiling infrastructure to measure before/after optimization effects.

**Files modified**:
- `managym/infra/profiler.h` - Added `exportBaseline()`, `compareToBaseline()`, `parseBaseline()`
- `managym/infra/profiler.cpp` - Implemented comparison logic
- `managym/agent/pybind.cpp` - Exposed to Python: `env.export_profile_baseline()`, `env.compare_profile(baseline)`

**Usage**:
```python
baseline = env.export_profile_baseline()
# ... make changes, run again ...
print(env2.compare_profile(baseline))
```

## Open Questions

1. **Profiler overhead**: The nested profiler scopes themselves add overhead. Consider benchmarking with profiler disabled to measure true baseline.

2. **Action execution breakdown**: What percentage of unaccounted time is action execution vs game state queries?

3. **Skip_trivial effectiveness**: With 6.9x tick amplification, is skip_trivial actually skipping enough? Could it be more aggressive?
