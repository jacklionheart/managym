# Performance Diagnosis

## Summary

The simulation loop spends 91.5% of wall time in `env.step()`, with **44.6% unaccounted** by current profiler instrumentation. The unaccounted time is spread across action execution, state-based action checks, available action generation, and combat setup—all of which iterate over permanents multiple times per tick. Observation encoding (18.4%) and turn logic (21.7%) are secondary concerns. The primary opportunity is reducing redundant permanent iteration and adding profiler coverage to the mystery 44.6%.

## Primary Bottleneck: Unaccounted "Other" Time

**Component**: Multiple untracked operations within `Game::_step()`
**Location**: `managym/flow/game.cpp:164-198`
**Time share**: 44.6% (~1.51s of 3.39s)
**Root cause**: Five distinct operations happen per tick without profiler tracking, each involving permanent iteration.

### Breakdown of "Other" Time

| Operation | Location | Estimated Share | Issue |
|-----------|----------|-----------------|-------|
| Action execution | `action.cpp:69-81` (CastSpell) | ~15-20% | `produceMana()` iterates all permanents |
| State-based actions | `priority.cpp:96-137` | ~10-15% | `forEachPermanent()` for lethal damage check |
| Available actions | `priority.cpp:67-94` | ~5-10% | `canPayManaCost()` calls `producibleMana()` per card |
| Combat setup | `combat.cpp:22-32` | ~5-10% | `eligibleAttackers/Blockers()` iterate permanents |
| Skip-trivial loop | `game.cpp:205-207` | Variable | Multiple `_step(0)` calls per `step()` |

### Evidence

From profile data:
- `env_step/game/tick` count: **315,091** (6.7 ticks per step)
- `env_step/game` count: **47,088** (matches step count)
- Gap: `env_step/game` (1.88s) vs `env_step/game/tick` (1.48s) = **0.40s** untracked at game level

The profiler tracks `game` around `_step()` but action execution at line 192 happens *before* `tick()` is called:

```cpp
// game.cpp:192
current_action_space->actions[action]->execute();  // NOT TRACKED
// ...
return tick();  // tick() IS tracked
```

## Secondary Bottleneck: Observation Encoding

**Component**: Observation construction
**Location**: `managym/agent/observation.cpp:22-39`
**Time share**: 18.4% (0.62s)
**Root cause**: Observation is rebuilt from scratch on every tick (315,091 times), not just on every step (47,088 times).

### Sub-component breakdown

| Method | Time | % of Observation | Issue |
|--------|------|------------------|-------|
| `populatePermanents()` | 0.22s | 35% | Iterates all permanents, creates `PermanentData` |
| `populateCards()` | 0.12s | 19% | Iterates hand/graveyard/exile/stack |
| `populatePlayers()` | 0.03s | 5% | 7 zone size lookups per player |
| `populateTurn()` | 0.02s | 3% | Minimal |
| `populateActionSpace()` | ~0.02s | 3% | Tracked under wrong label ("populateTurn") |

**Bug found**: `observation.cpp:65` uses profiler label `"populateTurn"` for `populateActionSpace()`, causing data to be double-counted.

### Inefficiency

Observation is built in `Game::tick()` (line 237):
```cpp
current_observation = std::make_unique<Observation>(this);
```

This runs inside the `tick` scope, which is called **315,091 times** (once per game tick), not 47,088 times (once per agent step). When `skip_trivial=true`, most ticks don't require agent input, but observations are still built.

## Tertiary Bottleneck: Turn/Phase/Step Logic

**Component**: Turn system tick cascade
**Location**: `managym/flow/turn.cpp:205-225`, `priority.cpp:20-49`
**Time share**: 21.7% (0.74s)
**Root cause**: Deep call hierarchy with profiler scope overhead; priority system does expensive work each tick.

Call chain per tick:
```
Turn::tick() → Phase::tick() → Step::tick() → PrioritySystem::tick()
```

Each level creates a `Profiler::Scope` object. With 315,091 ticks, this is significant overhead even at microseconds per scope.

`PrioritySystem::tick()` (priority.cpp:20-49):
- Calls `performStateBasedActions()` (not tracked separately)
- Calls `playersStartingWithActive()` which constructs a vector each time
- Calls `availablePriorityActions()` which iterates hand cards and checks `canPayManaCost()`

## Quick Wins

### 1. Add profiler instrumentation to action execution
**Location**: `game.cpp:192`
**Change**: Wrap `execute()` in profiler scope
**Impact**: Visibility into 15-20% of runtime

### 2. Fix duplicate profiler label
**Location**: `observation.cpp:65`
**Change**: `"populateTurn"` → `"populateActionSpace"`
**Impact**: Accurate profiler data

### 3. Only build observation when needed
**Location**: `game.cpp:237`
**Change**: Move observation construction outside `tick()`, only build when step returns to caller
**Impact**: Reduce observation builds from 315,091 to 47,088 (~6.7x reduction)

### 4. Cache `playersStartingWithActive()` result
**Location**: `turn.cpp:177-186`
**Change**: Store result in TurnSystem instead of rebuilding vector each call
**Impact**: Eliminates ~1M vector constructions per 500 games

## Architectural Issues

### 1. Redundant permanent iteration
Every mana cost check (`canPayManaCost`) iterates all permanents to compute producible mana. During action generation, this happens once per castable card in hand. With 7 cards in hand and 5 lands on battlefield:
- 7 cards × `producibleMana()` = 7 × 5 permanent iterations = 35 iterations per action generation

**Solution**: Cache producible mana at start of priority check, invalidate only when permanents tap/untap.

### 2. Observation built too frequently
315,091 observations for 47,088 steps. The observation is valid until an action executes, but we rebuild it after every internal tick.

**Solution**: Build observation lazily or only on step boundaries, not tick boundaries.

### 3. State-based actions iterate permanents even when nothing changed
`performStateBasedActions()` iterates all permanents checking lethal damage after every action, even when no damage was dealt.

**Solution**: Track dirty flag for damage, skip iteration if no damage occurred since last check.

### 4. `std::vector` allocations in hot paths
Multiple functions return `std::vector` by value:
- `eligibleAttackers()` / `eligibleBlockers()` - battlefield.cpp:100-120
- `playersStartingWithActive()` - turn.cpp:177-186
- `availablePriorityActions()` - priority.cpp:67-94

**Solution**: Reuse pre-allocated vectors or return by const reference where possible.

## Questions

1. **Why 6.7 ticks per step?** With skip_trivial=true, each step may call `_step(0)` multiple times (game.cpp:205-207). Is this expected? Could trivial skipping happen without full tick/observation overhead?

2. **Profiler overhead**: With 315,091 scope creations across multiple hierarchy levels, what's the profiler's own overhead? May need to benchmark profiler vs no-profiler.

3. **Mana production strategy**: `Battlefield::produceMana()` greedily taps permanents. Is this optimal? Could a smarter algorithm reduce mana ability activations?

4. **Combat frequency**: Profile shows 3,557 attacks declared, 1,752 blocks. What percentage of steps involve combat action spaces? If low, combat-related permanent iterations could be conditional.
