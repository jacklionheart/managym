# perf

Performance optimization branch for managym. Adds profiling infrastructure and implements targeted optimizations achieving +339% throughput improvement.

## Review

**Verdict:** Ready to ship

The branch delivers substantial, measurable performance gains through systematic profiling and targeted fixes. Three major optimizations dominate the improvement:

1. **InfoDict deferred to episode end** (+129%): Eliminates 54.6% of per-step overhead by moving profiler/behavior stat collection from every step to episode end only.

2. **Profiler scope reduction** (+8.7%): Removes `phase` and `step` profiler scopes from the turn hierarchy, eliminating ~700k unnecessary scope operations per 500-game run.

3. **Early-out priority check** (+66%): Adds `canPlayerAct()` fast-path to skip ActionSpace construction when a player has no actions besides PassPriority. Reduces priority ticks by 59%.

Additional infrastructure changes support ongoing optimization work:
- Profiling command framework for repeatable analysis
- Baseline comparison API for measuring impact
- Observation data structures optimized (maps → vectors)
- Mana and player order caching

The code is well-tested, maintains API compatibility, and the performance gains are verified with reproducible benchmarks.

## Performance Summary

| Profile | Steps/sec | vs Baseline | Key Change |
|---------|-----------|-------------|------------|
| Baseline (20260116-1614) | 16,122 | — | Lazy obs, vectors |
| 20260116-1631 | 16,607 | +3.0% | Cache playersStartingWithActive |
| post-infodict | 39,097 | +142% | InfoDict at episode end only |
| 20260116-2200 | 42,513 | +164% | Remove phase/step profiler scopes |
| post-early-out | 70,827 | **+339%** | Early-out priority check |

From 16k to 71k steps/sec is a 4.4x improvement.

## Code Changes

### env.cpp:81-86 — InfoDict Deferred
```cpp
// Only populate profiler/behavior stats at episode end
if (done) {
    addProfilerInfo(info);
    addBehaviorInfo(info);
}
```
Previously called on every step, now only when the game ends. Added `env.info()` for explicit access during game if needed.

### turn.cpp:231, 256 — Profiler Scopes Removed
`Phase::tick()` and `Step::tick()` no longer create profiler scopes. Only `Turn::tick()` and `PrioritySystem::tick()` retain scopes. Reduces scope operations from ~1.9M to ~1.2M per 500 games.

### turn.cpp:175-188 — Player Order Caching
```cpp
if (cached_active_index != active_player_index) {
    // rebuild players_active_first vector
    cached_active_index = active_player_index;
}
return players_active_first;
```
Avoids rebuilding the player order vector on every call.

### observation.h — Maps to Vectors
```cpp
// Before: std::map<int, CardData> agent_cards;
// After:
std::vector<CardData> agent_cards;
std::vector<PermanentData> agent_permanents;
```
Better cache locality, fewer allocations. Tests updated with helper functions for lookups.

### game.cpp — Mana Cache
```cpp
struct ManaCache {
    Mana producible[2];
    bool valid[2] = {false, false};
};
```
Avoids recalculating producible mana on every priority check.

### profiler.cpp — Baseline Comparison API
```cpp
std::string exportBaseline() const;
std::string compareToBaseline(const std::string& baseline) const;
```
Enables reproducible before/after measurements.

### priority.cpp:20-48 — Early-Out Priority Check
```cpp
bool PrioritySystem::canPlayerAct(Player* player) const {
    const std::vector<Card*>& hand_cards = game->zones->constHand()->cards[player->index];
    if (hand_cards.empty()) return false;

    bool can_play_land = game->canPlayLand(player);
    bool can_cast = game->canCastSorceries(player);

    for (Card* card : hand_cards) {
        if (card->types.isLand() && can_play_land) return true;
        if (card->types.isCastable() && can_cast) {
            if (game->canPayManaCost(player, card->mana_cost.value())) return true;
        }
    }
    return false;
}
```
Fast predicate that checks if a player can do anything besides pass. Used in `tick()` to auto-pass players who can only pass, without allocating ActionSpace objects.

### priority.cpp:64-77 — Fast-Path in tick()
```cpp
while (pass_count < players.size()) {
    Player* player = players[pass_count];
    if (game->skip_trivial && !canPlayerAct(player)) {
        pass_count++;  // Auto-pass without allocating
        continue;
    }
    return makeActionSpace(player);  // Slow path: full ActionSpace
}
```
When skip_trivial is enabled and a player has no actions, skip the expensive ActionSpace construction entirely. Reduces priority ticks from 557k to 228k (59% reduction).

## Remaining Bottlenecks

Current profile (70,827 steps/sec) shows:
- **Turn hierarchy**: 43.4% of env_step time (0.392s / 255k calls)
- **Priority system**: 17.6% (0.159s / 184k calls) — reduced from 557k
- **Observation**: 16.7% (0.151s / 74k calls)
- **Skip-trivial loop**: 12.2% (0.111s / 12k iterations) — reduced from 400k

The optimization significantly reduced tick amplification. Further gains require the remaining refactors documented in `refactor-proposal.md`:
- Collapse skip_trivial into tick loop (Proposal 2)
- Flat state machine for turn structure (Proposal 3)
- Skip empty combat phases (Proposal 4)

## Files Changed

### New Infrastructure
- `.claude/commands/{diagnose,instrument,optimize,profile,refactor}.md` — LLM-executable performance commands
- `scripts/profile.py` — Reproducible benchmarking script
- `.design/profile-*.md` — Performance baselines and history
- `.design/diagnosis.md` — Bottleneck analysis
- `.design/refactor-proposal.md` — Architectural proposals

### Core Optimizations
- `managym/agent/env.cpp` — InfoDict deferral
- `managym/flow/turn.cpp` — Profiler scope reduction, player order cache
- `managym/flow/priority.{h,cpp}` — Early-out priority check (canPlayerAct fast-path)
- `managym/agent/observation.{h,cpp}` — Map → vector conversion
- `managym/flow/game.{h,cpp}` — Mana cache, lazy observation
- `managym/infra/profiler.{h,cpp}` — Baseline comparison API

### Test Updates
- `tests/agent/test_observation.cpp` — Updated for vector-based API
- `tests/infra/test_profiler.cpp` — Baseline comparison tests
- `tests/flow/test_flow.cpp` — canPlayerAct/availablePriorityActions consistency test

## Binary Artifacts

The branch includes built binaries that should not be committed:
- `managym/_managym.cpython-312-darwin.so`
- `managym/libmanagym_lib.dylib`

Consider adding these to `.gitignore` if not already present.

## Design Notes

### Manabot Constraint
Observations are consumed immediately after every `step()`. "Lazy" field population provides no benefit—all fields are used. The optimization that works is avoiding observation creation during internal ticks (skip_trivial loop), which is already implemented.

### Open Questions
See `.design/questions.md`. Key ones:
1. Target throughput? Current 42k steps/sec—is more needed?
2. Profiler overhead when disabled?
3. Python binding overhead (not measured in C++ profiler)?
