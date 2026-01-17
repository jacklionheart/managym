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

### 1. Turn/Phase/Step Hierarchy
**Location**: `managym/flow/turn.cpp`
**Time share**: ~24% of simulation time
**Issue**: Deep call hierarchy with 10x tick amplification from agent steps

### 2. Priority System
**Location**: `managym/flow/priority.cpp`
**Time share**: ~8% of simulation time
**Issue**: Action generation iterates hand cards and checks mana affordability per castable card

## Quick Wins for Next Pass

1. **Cache producible mana**: Mana production only changes on battlefield/tap state changes
2. **Pre-allocate action vectors**: Reuse action storage instead of allocating new vectors
3. **Batch-skip empty phases**: Pre-compute which phases have no decisions based on board state
