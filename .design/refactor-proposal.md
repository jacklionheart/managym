# Refactor Proposal

## Summary

The turn/phase/step hierarchy accounts for 31% of simulation time despite doing minimal actual work. The root cause is not the Magic rules engine itself, but **the overhead of traversing a deep object hierarchy 10x more often than necessary**. The current design creates and destroys Turn/Phase/Step objects per turn, allocates vectors per tick, and runs profiler scopes at every level of the hierarchy.

The core insight: **flatten the state machine**. Magic's turn structure is fixed and predictable. Instead of a tree of polymorphic objects, represent the game state as a simple enum + integer pair. Skip phases/steps that cannot produce decisions, not just trivial action spaces.

## Proposals

### 1. Flat State Machine — Expected impact: 25-35%

**Current state:**
The turn structure is a 4-level object hierarchy (TurnSystem → Turn → Phase → Step). Each tick traverses this entire tree:
- `TurnSystem::tick()` → `Turn::tick()` → `Phase::tick()` → `Step::tick()` → `PrioritySystem::tick()`
- Each level creates a `Profiler::Scope` object (4 allocations + 4 destructor calls per tick)
- `Turn` constructor allocates 5 Phase objects (`turn.cpp:198-202`)
- Each Phase constructor allocates 1-5 Step objects (`turn.cpp:351-367`)
- `playersStartingWithActive()` allocates a new vector on every call (`turn.cpp:177-186`)

For 9,779 agent steps, this hierarchy processes 98,502 turn/phase/step ticks—a **10.1x amplification**.

**Problem:**
The polymorphic design serves code organization, not performance. Virtual dispatch, object allocation, and deep call chains create overhead for a state machine that's entirely deterministic. Magic's turn structure is the same every turn: Beginning (3 steps) → Main → Combat (5 steps) → Main → Ending (2 steps).

**Proposal:**
Replace the object hierarchy with a flat state machine. The entire turn state becomes two integers:

```cpp
struct TurnState {
    int step;           // 0-11 (StepType enum value)
    int priority_pass;  // 0 = active player, 1 = NAP, 2 = done
};
```

A single `advanceTurn()` function replaces the entire hierarchy:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::advanceTurn() {
    // Skip steps that can't produce decisions
    while (true) {
        if (step == StepType::BEGINNING_UNTAP) {
            untapAndMarkNotSick();
            step++;
            continue;
        }

        if (!hasDecisionAtStep(step)) {
            step++;
            if (step > ENDING_CLEANUP) {
                step = 0;
                advanceActivePlayer();
            }
            continue;
        }

        // This step needs a decision
        return makeActionSpace();
    }
}
```

Key changes:
- Remove Turn, Phase, Step classes entirely
- One profiler scope for the entire tick, not nested scopes
- Pre-computed player order (2-element array, never reallocated)
- No virtual dispatch, no heap allocations in the hot path

**Implementation sketch:**
1. Add `TurnState` struct to `TurnSystem`
2. Implement `advanceTurn()` as a single switch/loop
3. Move step-specific logic (untap, draw, cleanup) into helper methods
4. Remove `turn.cpp` Phase/Step classes
5. Update `turn.h` to keep only TurnSystem + enums

**Risks:**
- Significant refactor touching turn.cpp, turn.h, game.cpp
- Combat phase has complex step skipping logic that needs careful handling
- Tests assume Turn/Phase/Step objects exist

---

### 2. Phase/Step Skipping — Expected impact: 15-25%

**Current state:**
The game ticks through every step even when no decisions are possible. With `skip_trivial=True`, we skip trivial *action spaces* (≤1 action), but we still:
- Enter each step
- Create a profiler scope
- Check if priority is needed
- Create an ActionSpace with just PassPriority
- Then skip it

**Problem:**
Many steps are *statically* known to have no decisions before we enter them:
- Untap step: no priority window
- Draw step with empty hand and no permanents with activated abilities: priority passes immediately
- Combat steps when there are no creatures: no attackers/blockers to declare
- All steps when hand is empty and no battlefield abilities: only PassPriority available

The 10.1x tick amplification could be reduced to ~2-3x if we skipped these phases entirely.

**Proposal:**
Before entering a step, check if it can possibly produce a non-trivial decision:

```cpp
bool TurnSystem::canStepProduceDecision(StepType step) const {
    if (!stepHasPriorityWindow(step)) return false;

    Player* active = activePlayer();
    Player* nap = nonActivePlayer();

    // Check if either player has any possible actions besides pass
    bool active_has_actions =
        (game->zones->size(ZoneType::HAND, active) > 0) ||
        hasBattlefieldAbilities(active);
    bool nap_has_actions =
        (game->zones->size(ZoneType::HAND, nap) > 0) ||
        hasBattlefieldAbilities(nap);

    // For combat steps, also need attackers/blockers
    if (step == COMBAT_DECLARE_ATTACKERS) {
        return game->zones->constBattlefield()->hasEligibleAttackers(active);
    }
    // ... similar for blockers

    return active_has_actions || nap_has_actions;
}
```

**Implementation sketch:**
1. Add `canStepProduceDecision()` to TurnSystem
2. Add `hasBattlefieldAbilities()` helper (checks for untapped permanents with abilities)
3. In `tick()`, skip entire step if `!canStepProduceDecision()`
4. Track stats on how many steps are skipped to validate impact

**Risks:**
- Subtle bugs if the skip logic is wrong (could skip steps that actually need decisions)
- Requires careful handling of triggered abilities (not currently implemented)
- Complexity if cards with "at beginning of X step" triggers are added

---

### 3. Cached Producible Mana — Expected impact: 10-15%

**Current state:**
`canPayManaCost()` is called for every castable card on every priority check. Each call triggers `Battlefield::producibleMana()` which iterates all permanents:

```cpp
Mana Battlefield::producibleMana(Player* player) const {
    Mana total_mana;
    for (const auto& permanent : permanents.at(player)) {
        total_mana.add(permanent->producibleMana());
    }
    return total_mana;
}
```

With ~90k priority ticks and ~10 cards in hand, that's ~900k iterations over permanents. Each permanent checks its mana abilities, each ability checks if it can be activated (not tapped).

**Problem:**
Producible mana only changes when:
- A permanent enters or leaves the battlefield
- A permanent taps or untaps
- A mana ability is activated

Between these events, producible mana is constant. We're recalculating it hundreds of thousands of times when it could be cached.

**Proposal:**
Cache producible mana per player. Invalidate on battlefield/tap state changes.

```cpp
// In TurnSystem or a new ManaCache class
struct ManaCache {
    Mana producible[2];  // Indexed by player_index
    bool valid[2] = {false, false};

    void invalidate(int player_index) { valid[player_index] = false; }
    void invalidateAll() { valid[0] = valid[1] = false; }

    const Mana& get(int player_index, const Battlefield* bf, const Player* player) {
        if (!valid[player_index]) {
            producible[player_index] = bf->producibleMana(player);
            valid[player_index] = true;
        }
        return producible[player_index];
    }
};
```

Invalidation points:
- `Battlefield::enter()` → invalidate controller
- `Battlefield::exit()` → invalidate controller
- `Permanent::tap()` → invalidate controller
- `Permanent::untap()` → invalidate controller
- Step transition (untap step) → invalidate active player

**Implementation sketch:**
1. Add `ManaCache` to `Game` or `TurnSystem`
2. Add `Game::cachedProducibleMana(Player*)` that uses the cache
3. Replace `canPayManaCost()` to use cached version
4. Add invalidation calls to battlefield enter/exit and tap/untap

**Risks:**
- Cache invalidation bugs could cause incorrect mana calculations
- Need to ensure all mutation points are covered
- Adds complexity for modest gain

---

### 4. Zone Map → Vector Conversion — Expected impact: 5-10%

**Current state:**
Zones use `std::map<const Player*, std::vector<Card*>>` for storing cards per player. Every zone access does a red-black tree lookup:

```cpp
const std::vector<Card*> hand_cards = game->zones->constHand()->cards.at(player);
```

For 2 players, this is O(log 2) = 1 comparison, but with map overhead (pointer chasing, node allocation).

Similarly, `Zones::card_to_zone` is `std::map<Card*, Zone*>` requiring O(log n) lookups for every zone query.

**Problem:**
With only 2 players, we're paying map overhead for no benefit. A vector indexed by `player->index` would be:
- O(1) direct array access
- Better cache locality
- No allocator overhead

**Proposal:**
Replace zone maps with vectors indexed by player index:

```cpp
// Zone base class
class Zone {
    std::vector<std::vector<Card*>> cards;  // cards[player_index]

    void enter(Card* card) {
        cards[card->owner->index].push_back(card);
    }
};

// Zones card lookup
class Zones {
    std::vector<Zone*> card_to_zone;  // card_to_zone[card->id]

    Zone* getCardZone(Card* card) {
        if (card->id >= card_to_zone.size()) return nullptr;
        return card_to_zone[card->id];
    }
};
```

**Implementation sketch:**
1. Change `Zone::cards` from `map<Player*, vector>` to `vector<vector<Card*>>`
2. Update all zone accessors to use `player->index`
3. Change `Battlefield::permanents` similarly
4. Change `Zones::card_to_zone` to vector, resize as needed
5. Update tests that iterate over map entries

**Risks:**
- Assumes `player->index` is stable and small (0 or 1)
- Card IDs must be reasonably bounded for the vector approach
- Breaking change to zone iteration patterns

---

## Recommended Sequence

1. **Phase/Step Skipping** (Proposal 2) — Lowest risk, highest confidence. Can be done without structural changes. Measure impact before proceeding.

2. **Cached Producible Mana** (Proposal 3) — Isolated change, easy to verify. Good warmup for more invasive changes.

3. **Zone Map → Vector** (Proposal 4) — Moderate refactor, clear benefits. Foundation for other optimizations.

4. **Flat State Machine** (Proposal 1) — Highest impact but largest refactor. Do this last when the smaller changes have proven the methodology.

Each proposal can be implemented and measured independently. If the simpler changes achieve sufficient throughput, the state machine refactor may not be necessary.

## Questions

1. **Are triggered abilities planned?** Proposals 1 and 2 assume the turn structure is static. If "at beginning of upkeep" triggers are coming, the skip logic becomes much more complex.

2. **What's the target throughput?** Current ~330 games/sec may be sufficient. Understanding the goal helps prioritize effort.

3. **How stable are Card IDs?** Proposal 4's vector-based `card_to_zone` assumes IDs are sequential and bounded. If IDs can be sparse or very large, a different approach is needed.

4. **Is combat phase skipping acceptable?** Proposal 2 would skip the entire combat phase when there are no creatures. This matches current behavior but may need flags for future features.
