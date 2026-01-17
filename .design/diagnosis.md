# Diagnosis

## Summary

The simulation spends **57% of time in the turn system** and **15% in observation building**.

### Addressed bottlenecks (previous sessions):
1. ✅ **Fused priority check**: `computePlayerActions()` replaces separate `canPlayerAct()` + `availablePriorityActions()` calls
2. ✅ **Cached `playersStartingWithAgent()`**: Returns const reference from cache instead of allocating new vector
3. ✅ **Const-reference in priority.cpp**: All calls use `const std::vector<Player*>&`
4. ✅ **Fixed `populateActionSpace` profiler scope**: Now correctly tracks as `populateActionSpace`

### Addressed bottlenecks (this session):
5. ✅ **Removed string copy from CardData**: `CardData.name` field removed (not used by Python API, only debug output)

### Remaining bottlenecks:
- `std::map<Card*, Zone*>` for card-to-zone lookup (low impact: ~20 zone moves/game)
- `addCard()` called for every permanent (creates full CardData including ManaCost copy and 12 type checks)

## Primary bottleneck

**Where**: `priority.cpp:69` → `priority.cpp:76` (sequential calls to `canPlayerAct()` then `makeActionSpace()`)

**Time**: ~25% of PRECOMBAT_MAIN_STEP time (extrapolated from priority scope taking 50% of main step time)

**Why**: When `skip_trivial` is enabled and a player CAN act:
1. `canPlayerAct()` at line 69 iterates the entire hand, checking `canPlayLand()`, `canCastSorceries()`, and `canPayManaCost()` for each card
2. If returns `true`, `makeActionSpace()` at line 76 calls `availablePriorityActions()` at line 98
3. `availablePriorityActions()` at lines 109-136 iterates the entire hand AGAIN, checking the exact same conditions

This duplication means every card's playability is computed twice when the player can act. With mana-intensive checks via `canPayManaCost()`, this is the dominant cost.

**Profile evidence**: `PRECOMBAT_MAIN_STEP` takes 0.156s total, with `/priority` taking 0.078s (50%). The step has 46464 counts vs 9006 inner priority counts, suggesting ~5 priority passes per step on average. The duplicated iteration happens on every pass where the player has actions.

## Secondary bottlenecks

### 2. `playersStartingWithAgent()` allocates vector on every call

**Where**: `game.cpp:103-121`

**Time**: ~4% of observation time (3 calls × vector allocation overhead)

**Why**: Called in:
- `populatePlayers()` at observation.cpp:99
- `populateCards()` at observation.cpp:133, 141 (two loops)
- `populatePermanents()` at observation.cpp:197

Each call creates a new `std::vector<Player*>`, finds the agent's index with `std::find_if`, and fills the vector. Compare to `playersStartingWithActive()` in turn.cpp:175-188, which caches the result and returns a const reference. The same pattern should apply here.

### 3. `playersStartingWithActive()` copies vector in priority.cpp

**Where**: `priority.cpp:16, 94, 139`

**Time**: Proportional to number of priority passes per step

**Why**: Several methods in `priority.cpp` call `game->playersStartingWithActive()` but store the result in a local `std::vector<Player*>` instead of a `const std::vector<Player*>&`:
- Line 16: `isComplete()` - `std::vector<Player*> players = ...` (should be const ref)
- Line 94: `makeActionSpace()` - `std::vector<Player*> players = ...` (should be const ref)
- Line 139: `performStateBasedActions()` - `std::vector<Player*> players = ...` (should be const ref)

Line 62 correctly uses `const std::vector<Player*>&`.

### 4. `populateActionSpace` uses wrong profiler scope

**Where**: `observation.cpp:65`

**Time**: Shows as part of `populateTurn` in profile (147304 counts = 2x observation count)

**Why**: The profiler scope is `track("populateTurn")` but should be `track("populateActionSpace")`. This causes action space population time to be attributed to turn population, making the profile misleading. The 147304 count (exactly 2× the 73652 observation count) shows both functions are being counted under the same name.

### 5. `std::map<Card*, Zone*>` for card-to-zone lookup

**Where**: `zones.h:92`, used in `zones.cpp:75-88`

**Time**: O(log n) per zone transition vs O(1) with vector

**Why**: Every `move()` and `pushStack()` call does a `std::map::find()` and `std::map::operator[]` insertion. With sequential card IDs, a `std::vector<Zone*>` indexed by card ID would be O(1). Grey Ogre games move ~20 cards per game through zone changes.

### 6. `addCard()` called redundantly for permanents

**Where**: `observation.cpp:221`

**Time**: Proportional to battlefield size

**Why**: `addPermanent()` calls `addCard(perm->card, ZoneType::BATTLEFIELD)` for every permanent. This is intentional design (permanents need card data too), but `CardData` construction at lines 158-186 involves copying strings (`card->name`), creating ManaCost copies, and 12+ type checks. Consider whether PermanentData could embed card_id and registry_key without creating full CardData.

## Unaccounted time

The profile shows 3.9% unaccounted (0.037s). Based on code review, this likely includes:
- Action execution at game.cpp:209-211 (tracked as `action_execute` at 0.082s, within `game` scope)
- Zone operations during tick (not tracked individually)
- std::unique_ptr creations for Phase/Step/Action objects

**Missing instrumentation**:
1. Individual `canPlayerAct()` timing to quantify duplication
2. Time in `playersStartingWithAgent()` vector allocations
3. Breakdown of `addCard()` vs `addPermanent()` costs
4. Mana cache hit/miss ratio (currently just tracks validity)

## Recommendations

Ordered by expected impact:

1. **Merge `canPlayerAct()` and `availablePriorityActions()` into single pass** (priority.cpp:20-48, 109-136)
   - Option A: Return `std::pair<bool, std::vector<Action>>` from a single iteration
   - Option B: Store `can_act` bool from first check, skip iteration in `availablePriorityActions()` if false
   - Expected: ~50% reduction in main step priority time (~4% of total)

2. **Cache `playersStartingWithAgent()` like `playersStartingWithActive()`** (game.cpp:103-121)
   - Add `cached_agent_player` to Game (invalidate when action_space changes)
   - Add `players_agent_first` vector to Game
   - Return const reference instead of new vector
   - Expected: Eliminate 3 vector allocations per observation (~1% of observation time)

3. **Fix const-reference usage in priority.cpp** (lines 16, 94, 139)
   - Change `std::vector<Player*> players = game->playersStartingWithActive()` to `const std::vector<Player*>& players = ...`
   - Simple fix, eliminates unnecessary copies

4. **Fix `populateActionSpace` profiler scope** (observation.cpp:65)
   - Change `track("populateTurn")` to `track("populateActionSpace")`
   - Required for accurate profiling

5. **Convert `card_to_zone` from `std::map` to `std::vector`** (zones.h:92)
   - Index by card ID
   - Requires knowing max card count or dynamic resizing
   - Expected: O(1) instead of O(log n) zone lookups

6. **Consider lighter-weight permanent observation** (observation.cpp:204-222)
   - Store card_id and registry_key directly in PermanentData
   - Skip full CardData construction for battlefield cards
   - Trade-off: API change vs performance
