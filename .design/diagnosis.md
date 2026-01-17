# Performance Diagnosis

## Summary

The **primary bottleneck is InfoDict construction on every step**, consuming 54.6% of env_step time. Lines 81-82 of `env.cpp` call `addProfilerInfo()` and `addBehaviorInfo()` which iterate all profiler nodes and format strings on every single step—73,652 times in a 500-game run. The tick loop hierarchy (turn/phase/step) accounts for 26% and observation for 4%. After eliminating InfoDict overhead, the next target is the 9.5x tick amplification where 699,093 internal ticks service only 73,652 agent steps.

## Primary Bottleneck: InfoDict Construction (54.6%)

**Where**: `env.cpp:81-82` and `env.cpp:94-132`
**Time**: 2.13s of 3.9s total (54.6%)
**Root cause**: `addProfilerInfo()` and `addBehaviorInfo()` called on every step

### Evidence from Profile

```
env_step:      3.899s (100%)
env_step/game: 1.614s (41.4%)
env_step/observation: 0.155s (4.0%)
(unaccounted): 2.130s (54.6%)  ← This is InfoDict construction
```

### Why It's Slow

Looking at `env.cpp:42-85`, the `step()` method:
```cpp
std::tuple<Observation*, double, bool, bool, InfoDict> Env::step(int action, bool skip_trivial) {
    Profiler::Scope scope = profiler->track("env_step");  // Line 43

    // ...game logic is inside "game" scope...
    bool done = game->step(action);           // Line 53 - creates "game" scope
    Observation* obs = game->observation();   // Line 54 - creates "observation" scope

    // EVERYTHING BELOW is OUTSIDE all tracked scopes but INSIDE env_step
    double reward = 0.0;
    bool terminated = false;
    // ... lines 56-65 ...

    InfoDict info = create_empty_info_dict(); // Line 65

    // THESE ARE THE PROBLEM - called on EVERY step:
    addProfilerInfo(info);                    // Line 81
    addBehaviorInfo(info);                    // Line 82

    return std::make_tuple(obs, reward, terminated, truncated, info);
}
```

`addProfilerInfo()` at lines 94-107:
```cpp
void Env::addProfilerInfo(InfoDict& info) {
    InfoDict profiler_info = create_empty_info_dict();
    if (profiler && profiler->isEnabled()) {
        std::unordered_map<std::string, Profiler::Stats> stats = profiler->getStats();
        // Iterates ALL timing nodes (~15+), formats strings for each
        for (it = stats.begin(); it != stats.end(); ++it) {
            std::ostringstream oss;
            oss << "total=" << it->second.total_time << "s, count=" << it->second.count;
            insert_info(profiler_info, it->first, oss.str());
        }
    }
    insert_info(info, "profiler", profiler_info);
}
```

`addBehaviorInfo()` at lines 109-133:
```cpp
void Env::addBehaviorInfo(InfoDict& info) {
    InfoDict behavior_info = create_empty_info_dict();
    // Gets stats map, creates nested InfoDict, inserts each stat
    if (hero_tracker && hero_tracker->isEnabled()) {
        std::map<std::string, std::string> hero_stats = hero_tracker->getStats();
        InfoDict hero_info = create_empty_info_dict();
        for (hit = hero_stats.begin(); hit != hero_stats.end(); ++hit) {
            insert_info(hero_info, hit->first, hit->second);
        }
        insert_info(behavior_info, "hero", hero_info);
    }
    // Same for villain...
}
```

With 73,652 steps:
- `profiler->getStats()` called 73,652 times (iterates all nodes, computes percentiles)
- String formatting with `ostringstream` for ~15 stats × 73,652 = ~1.1M string operations
- InfoDict map insertions: ~20 per step × 73,652 = ~1.5M insertions
- `BehaviorTracker::getStats()` called 2× per step × 73,652 = 147,304 times

**This work is completely wasted** because the profiler stats during a game are meaningless—only the final stats matter.

## Secondary Bottleneck: Turn Hierarchy (26%)

**Where**: `turn.cpp:207-226` (Turn::tick → Phase::tick → Step::tick cascade)
**Time**: 1.014s for 699,093 calls (26% of env_step)
**Per-call**: 1.45μs

### Tick Amplification

| Level | Calls | Ratio to Agent Steps |
|-------|-------|---------------------|
| Agent steps | 73,652 | 1.0x |
| Game ticks | 473,780 | 6.4x |
| Turn/Phase/Step ticks | 699,093 | 9.5x |
| Priority ticks | 615,234 | 8.4x |
| Inner priority (decisions) | 12,839 | 0.17x |

Only 12,839 of 615,234 priority ticks (2.1%) produce actual player decisions.

### Why It's Slow

1. **Profiler scope creation/destruction**: 4 Profiler::Scope objects × 699,093 calls = 2,796,372 scope operations

2. **Virtual dispatch and pointer chasing**: Each tick follows:
   ```
   TurnSystem::tick() → current_turn->tick() → phases[i]->tick() → steps[j]->tick() → priority->tick()
   ```

3. **Turn/Phase/Step object allocation**: Each Turn allocates 5 Phases, each Phase 1-5 Steps.
   With 19.4 turns/game × 500 games = 9,705 Turns = ~48,525 Phase allocations + ~97,050 Step allocations

### Evidence

The profiler shows cascading overhead:
- `turn`: 1.014s for 699,093 calls
- `phase`: 0.901s for 699,093 calls (0.113s = 11% lost between turn→phase)
- `step`: 0.735s for 699,093 calls (0.166s = 16% lost between phase→step)
- `priority`: 0.282s for 615,234 calls

## Tertiary Bottleneck: Priority System (7.2%)

**Where**: `priority.cpp:20-49`
**Time**: 0.282s for 615,234 calls (7.2% of env_step)
**Per-call**: 0.46μs

### Why It's Slow

1. **PassPriority allocation on every priority check** (`priority.cpp:90`)
   - `actions.emplace_back(new PassPriority(player, game));` runs 615,234 times
   - 97.9% of these are the only action (trivial action space)

2. **playersStartingWithActive() already cached** (`turn.cpp:175-188`)
   - Fixed in previous optimization: only rebuilds when active_player_index changes

## Observation: Efficient (4%)

**Where**: `observation.cpp:22-39`
**Time**: 0.155s for 73,652 calls (4% of env_step)
**Per-call**: 2.1μs

Observation is efficient after previous optimizations. Breakdown:
- populatePermanents: 0.058s (1.5%)
- populateCards: 0.039s (1.0%)
- populateTurn: 0.011s (0.3%)
- populatePlayers: 0.005s (0.1%)

## Time Breakdown (Profile 20260116-1800)

| Component | Time | % | Calls | Per-call | Notes |
|-----------|------|---|-------|----------|-------|
| env_step | 3.899s | 100% | 73,652 | 52.9μs | Total step time |
| → (unaccounted) | 2.130s | **54.6%** | - | - | **InfoDict construction** |
| → game | 1.614s | 41.4% | 73,652 | 21.9μs | Tick loop wrapper |
| → → action_execute | 0.148s | 3.8% | 473,780 | 0.3μs | Action execution |
| → → tick | 1.126s | 28.9% | 473,780 | 2.4μs | Game ticks (6.4x) |
| → → → turn | 1.014s | 26.0% | 699,093 | 1.5μs | Turn hierarchy |
| → → → → phase | 0.901s | 23.1% | 699,093 | 1.3μs | Phase traversal |
| → → → → → step | 0.735s | 18.9% | 699,093 | 1.1μs | Step logic |
| → → → → → → priority | 0.282s | 7.2% | 615,234 | 0.5μs | Priority system |
| → observation | 0.155s | 4.0% | 73,652 | 2.1μs | Observation building |

## Recommendations

### 1. Don't build InfoDict on every step (CRITICAL)
**Impact**: ~50% improvement (eliminates 54.6% overhead)
**Location**: `env.cpp:81-82`

Only populate profiler/behavior stats when explicitly requested or at episode end:

```cpp
std::tuple<Observation*, double, bool, bool, InfoDict> Env::step(int action, bool skip_trivial) {
    Profiler::Scope scope = profiler->track("env_step");
    // ...
    InfoDict info = create_empty_info_dict();

    if (done) {
        // Only add stats at end of episode
        addProfilerInfo(info);
        addBehaviorInfo(info);
    }

    return std::make_tuple(obs, reward, terminated, truncated, info);
}
```

Or simpler: return empty InfoDict always, let caller use `env.info()` explicitly.

### 2. ~~Reduce profiler scope nesting~~ **DONE**
**Impact**: **+18% steps/sec** (36k → 42k)
**Location**: `turn.cpp:231, 256`

Removed `phase` and `step` profiler scopes. Now only `turn` and `priority` scopes remain. Reduces scope creations from ~1.9M to ~1.2M per 500 games.

### 3. Pre-allocate PassPriority action
**Impact**: ~3-5%
**Location**: `priority.cpp:90`

Use flyweight pattern - single PassPriority per player, reuse instead of allocating.

### 4. Skip steps without possible decisions
**Impact**: ~15-25%
**Location**: `turn.cpp:151-163`

Before entering a step, check if it can produce non-trivial actions:
- Untap/cleanup: no priority window
- No hand + no abilities = skip main phase priority
- No creatures = skip combat steps entirely

### 5. Cache playersStartingWithAgent()
**Impact**: ~1-2%
**Location**: `game.cpp:103-121`

Like playersStartingWithActive(), cache and only rebuild when agent changes.

## Manabot Usage Context

**Critical insight**: manabot consumes observations IMMEDIATELY after every step() and reset() call. The training loop uses ALL observation fields right away for action selection, validation, and buffer storage. There is zero slack—observations cannot be deferred or lazy-loaded at the API boundary.

This means:
- "Lazy observation" at the API level provides NO benefit
- "Deferred field population" doesn't help—ALL fields are used
- Focus should be on making observation building FAST, not on when it happens
- The only valid "lazy" optimization was avoiding observation creation during internal ticks (skip_trivial loop)

## Addressed (Previous Optimizations)

| Optimization | When | Impact |
|--------------|------|--------|
| Skip observation during internal ticks* | 2026-01-16 | 17% → 4% |
| Map → vector in Observation | 2026-01-16 | Part of above |
| Cached producible mana | 2026-01-16 | Removed mana iteration from hot path |
| Zone vector storage | Already done | N/A |
| Cache playersStartingWithActive() | 2026-01-16 | +3.8% throughput |
| action_execute profiler scope | 2026-01-16 | Diagnostic (3.8% of game time) |
| **InfoDict only at episode end** | 2026-01-16 | **+129% steps/sec** (17k → 39k) |
| **Remove phase/step profiler scopes** | 2026-01-16 | **+18% steps/sec** (36k → 42k) |

*Note: "lazy observation creation" is misleading. The optimization avoids building observations during the skip_trivial internal tick loop where observations are never returned to Python. This is NOT about deferring observation creation at the API boundary.

**Profiler scope reduction**: Removed nested `phase` and `step` profiler scopes from `turn.cpp:231` and `turn.cpp:256`. Each tick was creating 4 scopes (turn/phase/step/priority). With 625k+ ticks, this was ~1.9M scope creations. Now only 2 scopes (turn/priority), reducing overhead by ~0.27s (14% of tick loop time).

## Open Questions

1. ~~**Why is InfoDict built every step?**~~ **RESOLVED**: Changed to only build at episode end. Caller can use `env.info()` explicitly if needed during game. Impact: +129% steps/sec.

2. **Profiler overhead when disabled?** Even with `enable_profiler=false`, Profiler::Scope objects are created. Is this overhead negligible?

3. **Python binding overhead?** How much time is pybind11 marshalling between C++ and Python? This could explain some unaccounted time.

## Related Documents

See `.design/refactor-proposal.md` for bold architectural changes targeting the turn hierarchy (26%) and priority system (7.2%) bottlenecks:

1. **Flat State Machine** - Replace Turn/Phase/Step object hierarchy with switch-based state machine (40-60% impact)
2. **Integrated Skip-Trivial** - Eliminate 6.9x tick amplification by integrating skip logic into tick loop (25-35% impact)
3. **ActionSpace Object Pool** - Pool Action objects to eliminate 90k+ allocations per 200 games (10-15% impact)
4. **Combat Phase Skip** - Short-circuit entire combat phase when no creatures (5-10% impact)
