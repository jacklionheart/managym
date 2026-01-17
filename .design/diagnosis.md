# Performance Diagnosis

## Summary

Observation construction dominated the simulation loop. The original implementation used `std::map<int, CardData>` for storing cards and permanents, causing significant allocation overhead from red-black tree node insertions. Replacing maps with vectors yielded a 33% throughput improvement.

## Addressed

### Observation std::map → std::vector conversion (2026-01-16)

**Change**: Replaced `std::map<int, CardData>` and `std::map<int, PermanentData>` with `std::vector<CardData>` and `std::vector<PermanentData>` in observation.h/cpp.

**Files modified**:
- `managym/agent/observation.h` - Changed data structure declarations
- `managym/agent/observation.cpp` - Updated populate methods and validation
- `managym/agent/pybind.cpp` - Updated docstrings (dict → list)
- `tests/agent/test_observation.cpp` - Updated tests for vector iteration

**Before** (50 games, seed=42, skip_trivial=True):
- 307 games/sec
- 14,268 steps/sec
- observation time: 29.2ms (for 2320 steps)
- populatePermanents: 8.9ms
- populateCards: 5.1ms

**After**:
- 410 games/sec (+33%)
- 19,001 steps/sec (+33%)
- observation time: 4.4ms (for 2320 steps, **85% reduction**)
- populatePermanents: 1.5ms (83% reduction)
- populateCards: 0.8ms (84% reduction)

**Impact**: Observation creation went from ~19% of env_step time to ~4%, freeing up significant budget for other work. The change was purely internal - the Python API now returns lists instead of dicts for cards/permanents, but this matches how the data is typically consumed (iteration rather than lookup).

### Lazy observation creation (2026-01-16)

**Change**: Made observation creation lazy - only create when `Game::observation()` is called, not on every internal tick.

**Files modified**:
- `managym/flow/game.cpp` - Made `observation()` non-const, added lazy initialization
- `managym/flow/game.h` - Updated method signature

**Before** (500 games, Grey Ogre vs Grey Ogre, skip_trivial=True):
- 135 games/sec
- 12,717 steps/sec
- observation calls: 315,091 (called on every tick, ~6.7x per agent step)
- observation time: 0.62s

**After**:
- 166 games/sec (+23%)
- 17,198 steps/sec (+35%)
- observation calls: 47,679 (called only on agent steps, 1:1 ratio)
- observation time: 0.09s (85% reduction)

**Impact**: Observation overhead dropped from ~18% to ~4% of env_step time. The 6.7x reduction in observation calls directly translates to throughput gains.

## Remaining Bottlenecks

### 1. Turn/Phase/Step Hierarchy (Primary)
**Location**: `managym/flow/turn.cpp`
**Time share**: ~31% of simulation time (0.130-0.169s)
**Calls**: 98,502 turn/phase/step ticks for 9,779 agent steps (10.1x amplification)

**Root cause**: Deep object hierarchy with per-tick allocations

Each turn creates:
- 1 Turn object with 5 Phase objects (`turn.cpp:196-203`)
- Each Phase creates 1-5 Step objects (`turn.cpp:351-368`)
- Every tick traverses Turn→Phase→Step→Priority with profiler scopes at each level

The structure is correct for Magic rules but introduces overhead:
1. **Profiler overhead**: 4 nested `Profiler::Scope` objects per tick chain (`turn.cpp:206`, `229`, `255`, `priority.cpp:21`)
2. **Virtual dispatch**: Each step type has virtual methods (`performTurnBasedActions`, `onStepStart`, `onStepEnd`)
3. **Repeated player vector creation**: `playersStartingWithActive()` allocates a new vector every call (`turn.cpp:177-186`)

**Evidence**: Profile shows 98,502 calls at turn/phase/step levels but only 9,779 agent steps. The 10x amplification comes from Magic's turn structure (12 steps per turn × ~7 turns avg = ~84 internal ticks per game, plus priority passes).

### 2. Priority System (Secondary)
**Location**: `managym/flow/priority.cpp`
**Time share**: ~11.5% of simulation time (0.063s)
**Calls**: 87,590 priority ticks for 9,779 agent steps

**Root cause**: Repeated mana affordability checks

For each priority pass, `availablePriorityActions()` (`priority.cpp:67-94`):
1. Creates new `std::vector<std::unique_ptr<Action>>` (allocation)
2. Iterates all hand cards
3. For each castable card, calls `game->canPayManaCost()` which calls `producibleMana()` (`battlefield.cpp:133-142`)
4. `producibleMana()` iterates all permanents and their mana abilities

With ~90k priority checks and ~10 cards in hand average, that's ~900k mana affordability calculations.

**Evidence**: The inner priority tracking (`env_step/game/tick/turn/phase/step/priority/priority`) only shows 2,669 calls (actual agent decisions) vs 87,590 total priority ticks. Most ticks are pass-through.

### 3. Action Allocation Churn
**Location**: `managym/flow/priority.cpp:67-94`, `managym/agent/action_space.h`
**Time share**: Part of priority system overhead

**Root cause**: New allocations per priority pass

Every `availablePriorityActions()` call:
- Allocates `std::vector<std::unique_ptr<Action>>`
- Creates new Action objects via `new PlayLand`, `new CastSpell`, `new PassPriority`
- Creates new `ActionSpace` object

With 87,590 priority ticks, this is significant allocation churn even if most action spaces are trivial (just PassPriority).

### 4. Zone Map Lookups
**Location**: `managym/state/zone.cpp`, `managym/state/battlefield.cpp`
**Time share**: Distributed across turn/priority

**Root cause**: Using `std::map<const Player*, ...>` for 2-player lookups

The zone structures use maps:
- `Zone::cards` is `std::map<const Player*, std::vector<Card*>>` (`zone.cpp:11-14`)
- `Battlefield::permanents` is `std::map<const Player*, std::vector<std::unique_ptr<Permanent>>>` (`battlefield.h:61`)

For 2 players, map lookups (O(log n)) are slower than direct index access. The pattern `cards.at(player)` appears throughout hot paths.

## Quick Wins for Next Pass

1. **Cache producible mana**: Invalidate only on battlefield enter/exit/tap/untap. Could eliminate ~90% of mana calculations.

2. **Pre-allocate player order vector**: `playersStartingWithActive()` returns a new vector every call. Store and reuse in TurnSystem.

3. **Replace zone maps with vectors**: Use `player->index` for direct indexing into `std::vector<std::vector<Card*>>` instead of map lookups.

4. **Pool Action objects**: Reuse PassPriority/PlayLand/CastSpell objects instead of allocating new ones per priority pass.

5. **Batch profiler scopes**: Consider a single profiler scope for the full tick chain instead of nested scopes at each level.

## Architectural Issues

1. **Turn structure amplification**: The 10x tick ratio is inherent to Magic's turn structure. Could potentially skip entire phases when no decisions are possible (empty hand + no permanents with activated abilities = skip main phase priority).

2. **Eager ActionSpace creation**: ActionSpaces are created even when the game will immediately skip them (trivial actions). Consider lazy ActionSpace creation.
