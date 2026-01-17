# Diagnosis

## Summary

The simulation's hot paths suffer from three main performance issues: (1) **`playersStartingWithAgent()` allocates a new vector on every call**, invoked ~6x per observation build; (2) **observation building iterates zones multiple times** and calls `addCard()` per permanent which duplicates card data creation; (3) the **tick loop's skip_trivial handling calls `canPlayerAct()` which recomputes producible mana** despite mana cache validity during trivial passes. The tick loop structure is sound—the real cost is hidden in repetitive allocations and redundant iterations inside each tick.

## Primary bottleneck

**Where**: `game.cpp:103-121` (`Game::playersStartingWithAgent()`)

**Time**: Estimated 5-10% of observation time, invoked 6+ times per observation

**Why**: This method creates a new `std::vector<Player*>` on every call. It's invoked in:
- `populatePlayers()` at observation.cpp:99 (to find opponent)
- `populateCards()` at observation.cpp:133, 141 (graveyard, exile iteration)
- `populatePermanents()` at observation.cpp:197 (battlefield iteration)

With only 2 players, this creates 6+ unnecessary vector allocations per observation. Compare to `playersStartingWithActive()` at turn.cpp:175-188, which caches its result and returns a const reference. `playersStartingWithAgent()` should follow the same pattern.

## Secondary bottlenecks

### 2. Duplicate card creation in `addPermanent()`

**Where**: `observation.cpp:204-222` (`Observation::addPermanent()`)

**Time**: Proportional to battlefield size

**Why**: `addPermanent()` calls `addCard(perm->card, ZoneType::BATTLEFIELD)` at line 221, creating a CardData entry. But `populatePermanents()` at line 189-202 also calls `populateCards()` before it (line 37), and `populateCards()` does NOT handle battlefield cards. This design is intentional (permanents have both Permanent and Card representations), but `addCard()` is called once per permanent AFTER `populateCards()` has already run, so the card data could potentially be deduplicated.

### 3. `canPlayerAct()` recomputes producible mana during trivial passes

**Where**: `priority.cpp:20-48` (`PrioritySystem::canPlayerAct()`)

**Time**: Proportional to hand size × battlefield size during skip_trivial loop iterations

**Why**: In the fast-path at priority.cpp:69, `canPlayerAct()` is called to check if a player can only pass. This calls `game->canPayManaCost()` at line 41 for each castable card, which invokes `cachedProducibleMana()`. The mana cache IS used correctly here. However, `canPlayerAct()` iterates the full hand even after finding a playable card—it should return `true` immediately upon finding the first playable action. Currently it does (line 37-44 has early returns), so this is actually fine.

**Revised issue**: The actual problem is that `canPlayerAct()` and `availablePriorityActions()` at priority.cpp:109-136 have duplicated logic. When skip_trivial is enabled and a player CAN act, we:
1. Call `canPlayerAct()` → iterates hand, checks mana
2. Call `makeActionSpace()` → calls `availablePriorityActions()` → iterates hand AGAIN, checks mana AGAIN

### 4. `std::map<Card*, Zone*>` for card-to-zone lookup

**Where**: `zones.h:92` (`card_to_zone`)

**Time**: O(log n) per zone transition, called frequently

**Why**: Every card move in `Zones::move()` at zones.cpp:68-89 performs a map lookup and insert. With sequential card IDs, a vector lookup would be O(1). This is already noted in performance.md but appears unimplemented.

### 5. Turn/Phase/Step allocation per turn

**Where**: `turn.cpp:198-205` (`Turn` constructor)

**Time**: Once per turn, but adds up

**Why**: Each new turn creates 5 Phase objects and ~12 Step objects via `emplace_back(new ...)`. These could be pooled or reused since the turn structure is fixed.

## Unaccounted time

Current profiler tracks:
- `game` (wraps step)
- `tick` (wraps turn system tick loop)
- `turn` (wraps Turn::tick)
- `step/<step_type>` (wraps Step::tick)
- `priority` (wraps PrioritySystem::tick)
- `action_execute` (wraps action execution)
- `observation` (wraps Observation constructor)
- `populateTurn`, `populatePlayers`, `populateCards`, `populatePermanents` (note: `populateActionSpace` uses wrong scope name "populateTurn")

**Missing instrumentation**:
1. Time spent in `canPlayerAct()` vs `availablePriorityActions()` to quantify duplication
2. Time in `playersStartingWithAgent()` vector allocations
3. Time in `addCard()` / `addPermanent()` individually
4. Breakdown of `Zones::move()`, `Zones::size()`, other zone operations
5. Mana cache hit/miss ratio

## Recommendations

Ordered by expected impact:

1. **Cache `playersStartingWithAgent()` like `playersStartingWithActive()`** (game.cpp:103-121)
   - Add `cached_agent_index` and `players_agent_first` vector to Game
   - Return const reference instead of new vector
   - Expected: Eliminate 6+ vector allocations per observation

2. **Deduplicate `canPlayerAct()` and `availablePriorityActions()` logic** (priority.cpp:20-48, 109-136)
   - Option A: Have `availablePriorityActions()` use `canPlayerAct()` result to short-circuit
   - Option B: Merge into single pass that returns both bool and actions
   - Expected: ~2x faster priority checks when player can act

3. **Fix `populateActionSpace` profiler scope name** (observation.cpp:65)
   - Change `track("populateTurn")` to `track("populateActionSpace")`
   - Required for accurate profiling

4. **Add fine-grained profiling**
   - Wrap `canPlayerAct()` calls
   - Wrap individual `addCard()` calls
   - Track zone operation counts
   - Required to validate other bottlenecks

5. **Convert `card_to_zone` from `std::map` to `std::vector`** (zones.h:92)
   - Use card ID as index
   - Already designed in performance.md, needs implementation
   - Expected: O(1) instead of O(log n) zone lookups

6. **Pool Turn/Phase/Step objects** (turn.cpp:198-205)
   - Reuse turn structure across turns
   - Lower priority unless profiling shows significant allocation time
