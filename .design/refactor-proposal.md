# Refactor Proposals

## Summary

The simulation bottleneck is **architectural, not algorithmic**. After optimizations reduced observation overhead to 3.4%, the turn/phase/step hierarchy now dominates at 31% of total time—despite doing minimal actual work. The 10.1x tick amplification (98,502 internal ticks for 9,779 agent decisions) reveals the core problem: **the game engine is organized for code clarity, not for fast simulation**.

The fundamental insight: **Magic's turn structure is deterministic and sparse**. Most steps produce no decisions. Most priority passes are automatic. The engine traverses a 4-level object hierarchy and creates 4 profiler scopes to ultimately return "PassPriority" 97% of the time.

Bold refactoring means restructuring the engine around the actual workload: ~10,000 decisions per 200 games, not ~100,000 hierarchy traversals.

---

## Proposals

### 1. Flat State Machine — Expected impact: 40-60%

**Current state:**
The tick loop traverses a 4-level object hierarchy on every tick:

```
Game::tick() → TurnSystem::tick() → Turn::tick() → Phase::tick() → Step::tick() → PrioritySystem::tick()
```

Each level:
- Creates a profiler scope (`turn.cpp:208`, `232`, `257`)
- Gets the current child via virtual dispatch
- Checks completion state
- Returns nullptr or ActionSpace upward

For 9,779 agent decisions, this hierarchy executes:
- 67,822 game ticks (skip_trivial loop iterations)
- 98,502 turn/phase/step ticks (hierarchy traversals)
- 394,008 profiler scope creations/destructions

**Problem:**
The engine represents Magic's static turn structure with dynamic objects. But the turn structure never changes at runtime:

```
UNTAP → UPKEEP → DRAW → MAIN1 → COMBAT_BEGIN → ATTACKERS → BLOCKERS → DAMAGE → COMBAT_END → MAIN2 → END → CLEANUP
```

A switch statement is faster than 4 levels of virtual dispatch.

**Proposal:**
Replace Turn/Phase/Step object hierarchy with a flat state machine. The entire turn state becomes:

```cpp
// In TurnSystem
struct TurnState {
    StepType step = StepType::BEGINNING_UNTAP;
    int priority_pass = 0;       // Which player's priority (0=active, 1=NAP)
    bool sba_done = false;       // State-based actions performed this round
    bool step_initialized = false;
    int lands_played = 0;        // Per-turn counter
};

TurnState state;
```

A single function advances to the next decision point:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::tick() {
    while (true) {
        // 1. Initialize step if needed
        if (!state.step_initialized) {
            executeStepStart(state.step);
            state.step_initialized = true;
        }

        // 2. Check for turn-based actions that produce decisions
        std::unique_ptr<ActionSpace> tba = executeTurnBasedActions(state.step);
        if (tba) return tba;

        // 3. Priority window (if this step has one)
        if (stepHasPriority(state.step)) {
            std::unique_ptr<ActionSpace> priority = tickPriority();
            if (priority) return priority;
        }

        // 4. Step complete - execute end actions and advance
        executeStepEnd(state.step);
        if (!advanceStep()) {
            // Turn complete
            advanceTurn();
        }
    }
}
```

Where `stepHasPriority()` is a simple lookup table:

```cpp
bool TurnSystem::stepHasPriority(StepType step) {
    // No priority in untap or cleanup
    return step != StepType::BEGINNING_UNTAP && step != StepType::ENDING_CLEANUP;
}
```

And `advanceStep()` is a switch:

```cpp
bool TurnSystem::advanceStep() {
    state.step_initialized = false;
    game->clearManaPools();

    switch (state.step) {
    case StepType::BEGINNING_UNTAP:    state.step = StepType::BEGINNING_UPKEEP; break;
    case StepType::BEGINNING_UPKEEP:   state.step = StepType::BEGINNING_DRAW; break;
    case StepType::BEGINNING_DRAW:     state.step = StepType::PRECOMBAT_MAIN_STEP; break;
    case StepType::PRECOMBAT_MAIN_STEP: state.step = StepType::COMBAT_BEGIN; break;
    case StepType::COMBAT_BEGIN:       state.step = StepType::COMBAT_DECLARE_ATTACKERS; break;
    case StepType::COMBAT_DECLARE_ATTACKERS: state.step = StepType::COMBAT_DECLARE_BLOCKERS; break;
    case StepType::COMBAT_DECLARE_BLOCKERS:  state.step = StepType::COMBAT_DAMAGE; break;
    case StepType::COMBAT_DAMAGE:      state.step = StepType::COMBAT_END; break;
    case StepType::COMBAT_END:         state.step = StepType::POSTCOMBAT_MAIN_STEP; break;
    case StepType::POSTCOMBAT_MAIN_STEP: state.step = StepType::ENDING_END; break;
    case StepType::ENDING_END:         state.step = StepType::ENDING_CLEANUP; break;
    case StepType::ENDING_CLEANUP:     return false;  // Turn over
    }
    return true;
}
```

**Implementation sketch:**
1. Add `TurnState` struct to TurnSystem
2. Add `executeStepStart(StepType)` - switch for untap/draw/etc
3. Add `executeTurnBasedActions(StepType)` - switch for attacker/blocker declarations
4. Add `executeStepEnd(StepType)` - cleanup actions
5. Move priority logic into `tickPriority()` without nested profiler scopes
6. Remove Turn, Phase, Step classes (keep types for observation/API)
7. Move combat state (attackers, blockers) into TurnSystem or separate CombatState

**Impact breakdown:**
- Eliminates 4-level object hierarchy: ~15% savings
- Eliminates Turn/Phase/Step allocations per turn: ~5% savings
- Eliminates 3 of 4 profiler scopes: ~10% savings
- Direct switch dispatch vs virtual calls: ~5% savings
- Better instruction cache locality: ~5% savings

**Risks:**
- Large refactor touching turn.cpp, turn.h, game.cpp, combat.cpp
- Tests assume Turn/Phase/Step objects exist for assertions
- Combat state management needs careful migration
- BehaviorTracker callbacks need to fire at correct points

---

### 2. Integrated Skip-Trivial — Expected impact: 25-35%

**Current state:**
The skip_trivial logic runs at two levels, neither integrated with the tick loop:

```cpp
// game.cpp:224-226 - outer level
while (!game_over && skip_trivial && actionSpaceTrivial()) {
    game_over = _step(0);
}

// game.cpp:239-252 - inner tick loop
while (!current_action_space) {
    current_action_space = turn_system->tick();
    // ...
}
```

This creates the 6.9x tick amplification: 67,822 game ticks for 9,779 agent steps.

**Problem:**
The engine builds a complete ActionSpace, returns it, checks if trivial, executes action 0, clears the ActionSpace, then repeats. For 97% of priority passes (PassPriority only), this is pure waste.

**Proposal:**
Integrate skip_trivial into the tick loop itself. Never return ActionSpaces with only 1 action:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::tickPriority() {
    while (state.priority_pass < 2) {  // 2 players
        Player* player = game->players[(active_player_index + state.priority_pass) % 2].get();

        // State-based actions
        if (!state.sba_done) {
            performStateBasedActions();
            state.sba_done = true;
            if (game->isGameOver()) return nullptr;
        }

        // Quick feasibility check - can this player do anything besides pass?
        if (skip_trivial && !canDoAnything(player)) {
            // Auto-pass without building ActionSpace
            state.priority_pass++;
            continue;
        }

        // Build ActionSpace
        std::vector<std::unique_ptr<Action>> actions = buildPriorityActions(player);

        // If only PassPriority and skip_trivial, auto-execute
        if (skip_trivial && actions.size() == 1) {
            // It's PassPriority - just increment pass count
            state.priority_pass++;
            continue;
        }

        // Real decision point
        return std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY, std::move(actions), player);
    }

    // Both players passed - resolve stack or end priority
    state.priority_pass = 0;
    state.sba_done = false;

    if (!game->zones->constStack()->objects.empty()) {
        resolveTopOfStack();
        return tickPriority();  // Restart priority
    }

    return nullptr;  // Priority complete
}
```

**Key insight:** The `canDoAnything()` check is much faster than building a full ActionSpace:

```cpp
bool TurnSystem::canDoAnything(Player* player) {
    // Empty hand = definitely only PassPriority
    if (game->zones->size(ZoneType::HAND, player) == 0) {
        return false;  // TODO: check battlefield abilities when added
    }

    // Check if can play any card
    const std::vector<Card*>& hand = game->zones->constHand()->cards[player->index];
    const Mana& available = game->cachedProducibleMana(player);

    for (Card* card : hand) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            return true;
        }
        if (card->types.isCastable() && game->canCastSorceries(player) &&
            available.canPay(card->mana_cost.value())) {
            return true;
        }
    }

    return false;
}
```

**Implementation sketch:**
1. Add `canDoAnything()` fast-path check to TurnSystem
2. Integrate skip_trivial logic into tickPriority()
3. Remove outer skip_trivial loop from Game::step()
4. Remove inner while loop from Game::tick()
5. Game::step() becomes single call to TurnSystem::tick()

**Impact breakdown:**
- Eliminates 6.9x game tick amplification: ~20% savings
- Avoids building 84,921 trivial ActionSpaces: ~10% savings
- Avoids 84,921 PassPriority allocations: ~5% savings

**Risks:**
- Must be 100% correct - skipping a real decision breaks the game
- Need to maintain the non-skip_trivial code path for debugging
- Future cards with abilities complicate `canDoAnything()`

---

### 3. ActionSpace Object Pool — Expected impact: 10-15%

**Current state:**
Every priority check allocates new Action objects:

```cpp
// priority.cpp:67-94
std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    for (Card* card : hand_cards) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            actions.emplace_back(new PlayLand(card, player, game));  // ALLOCATION
        } else if (card->types.isCastable() && ...) {
            actions.emplace_back(new CastSpell(card, player, game));  // ALLOCATION
        }
    }

    actions.emplace_back(new PassPriority(player, game));  // ALLOCATION (87,590 times!)
    return actions;
}
```

For 200 games: ~87,590 PassPriority allocations, ~5,000 PlayLand allocations, ~5,000 CastSpell allocations.

**Problem:**
These Action objects are created, used once for execution, then destroyed. PassPriority in particular is identical every time for a given player.

**Proposal:**
Pool action objects in TurnSystem, reset instead of reallocate:

```cpp
// In TurnSystem
struct ActionPool {
    PassPriority pass_priority[2];           // One per player
    std::vector<PlayLand> play_lands;        // Sized to max hand size
    std::vector<CastSpell> cast_spells;      // Sized to max hand size
    std::vector<DeclareAttackerAction> attackers;  // Sized to max creatures
    std::vector<DeclareBlockerAction> blockers;    // Sized to max attackers * blockers

    void init(Game* game) {
        for (int i = 0; i < 2; i++) {
            pass_priority[i].player = game->players[i].get();
            pass_priority[i].game = game;
        }
        play_lands.resize(20);   // Max hand size
        cast_spells.resize(20);
    }
};

ActionPool action_pool;
```

ActionSpace changes to hold raw pointers:

```cpp
struct ActionSpace {
    ActionSpaceType type;
    std::vector<Action*> actions;  // Raw pointers, not unique_ptr
    Player* player;

    // Actions are valid only until next tick - no ownership
};
```

Building the ActionSpace reuses pooled objects:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::buildPriorityActionSpace(Player* player) {
    std::vector<Action*> actions;
    int land_idx = 0, spell_idx = 0;

    for (Card* card : game->zones->constHand()->cards[player->index]) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            action_pool.play_lands[land_idx].reset(card, player, game);
            actions.push_back(&action_pool.play_lands[land_idx++]);
        } else if (...) {
            action_pool.cast_spells[spell_idx].reset(card, player, game);
            actions.push_back(&action_pool.cast_spells[spell_idx++]);
        }
    }

    actions.push_back(&action_pool.pass_priority[player->index]);

    auto space = std::make_unique<ActionSpace>();
    space->type = ActionSpaceType::PRIORITY;
    space->actions = std::move(actions);
    space->player = player;
    return space;
}
```

**Implementation sketch:**
1. Add `reset()` method to each Action subclass
2. Add `ActionPool` to TurnSystem
3. Change `ActionSpace::actions` from `vector<unique_ptr<Action>>` to `vector<Action*>`
4. Update all action creation code to use pool
5. Document that actions are only valid until next step

**Impact breakdown:**
- Eliminates 87,590 PassPriority allocations: ~5% savings
- Eliminates ~10,000 PlayLand/CastSpell allocations: ~3% savings
- Reduced allocator pressure: ~5% savings
- Better cache locality (actions are adjacent in memory): ~2% savings

**Risks:**
- Actions must not be stored beyond current step
- Thread safety if multi-threaded later (need per-thread pools)
- pybind11 bindings may need adjustment for raw pointers

---

### 4. Combat Phase Skip — Expected impact: 5-10%

**Current state:**
Every turn traverses all 5 combat steps even when no creatures exist:

```cpp
// combat.cpp:11-17
CombatPhase::CombatPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new BeginningOfCombatStep(this));
    steps.emplace_back(new DeclareAttackersStep(this));
    steps.emplace_back(new DeclareBlockersStep(this));
    steps.emplace_back(new CombatDamageStep(this));
    steps.emplace_back(new EndOfCombatStep(this));
}
```

For early game turns (no creatures yet), this is 5 wasted step traversals × ~1,400 turns.

**Problem:**
The DeclareAttackersStep does check for eligible attackers, but only after the step hierarchy has been traversed:

```cpp
// combat.cpp:46-51
std::unique_ptr<ActionSpace> DeclareAttackersStep::performTurnBasedActions() {
    if (attackers_to_declare.empty()) {
        turn_based_actions_complete = true;
        return nullptr;  // No attackers, but still traversed the hierarchy
    }
    // ...
}
```

**Proposal:**
In the flat state machine, skip combat entirely when no creatures exist:

```cpp
// In advanceStep() or a pre-step check
bool TurnSystem::shouldEnterCombat() {
    // No creatures on either side = skip combat entirely
    int active_creatures = 0, opponent_creatures = 0;

    for (const auto& perm : game->zones->constBattlefield()->permanents[active_player_index]) {
        if (perm->card->types.isCreature()) active_creatures++;
    }
    for (const auto& perm : game->zones->constBattlefield()->permanents[1 - active_player_index]) {
        if (perm->card->types.isCreature()) opponent_creatures++;
    }

    // If active player has no creatures, skip to post-combat main
    if (active_creatures == 0) return false;

    return true;
}

bool TurnSystem::advanceStep() {
    // ...
    case StepType::PRECOMBAT_MAIN_STEP:
        if (shouldEnterCombat()) {
            state.step = StepType::COMBAT_BEGIN;
        } else {
            state.step = StepType::POSTCOMBAT_MAIN_STEP;  // Skip combat entirely
        }
        break;
    // ...
}
```

Also skip individual combat steps when they can't produce decisions:

```cpp
case StepType::COMBAT_DECLARE_ATTACKERS:
    if (game->zones->constBattlefield()->eligibleAttackers(activePlayer()).empty()) {
        // No eligible attackers - skip to end of combat
        state.step = StepType::COMBAT_END;
    } else {
        state.step = StepType::COMBAT_DECLARE_BLOCKERS;
    }
    break;

case StepType::COMBAT_DECLARE_BLOCKERS:
    // If no attackers were declared, skip blockers and damage
    if (combat_state.attackers.empty()) {
        state.step = StepType::COMBAT_END;
    } else {
        state.step = StepType::COMBAT_DAMAGE;
    }
    break;
```

**Implementation sketch:**
1. Add `shouldEnterCombat()` check before combat phase
2. Skip from PRECOMBAT_MAIN directly to POSTCOMBAT_MAIN when no creatures
3. Skip individual combat steps when they can't produce decisions
4. Track this in profiler to measure skip frequency

**Impact:**
- Skip ~30% of turns' combat phases (early game): ~3% savings
- Skip blockers/damage when no attackers declared: ~5% savings
- Reduced step count = fewer iterations overall

**Risks:**
- Future cards may have "at beginning of combat" triggers even without creatures
- Need to ensure this is only enabled when skip_trivial is true
- BehaviorTracker callbacks need adjustment

---

## Sequence

**Phase 1: Integrated skip-trivial (Proposal 2)**
- Moderate complexity, high impact
- Can be done incrementally without full flat state machine
- Eliminates the 6.9x tick amplification
- Expected: +25-35% throughput

**Phase 2: Flat state machine (Proposal 1)**
- Largest change, requires careful testing
- Subsumes Phase 1's benefits and adds more
- Best done after skip-trivial proves the approach
- Expected: +40-60% throughput (cumulative)

**Phase 3: Quick wins**
- Combat phase skip (Proposal 4) - 5-10%
- Can be done in parallel with Phase 2
- Low risk, straightforward implementation

**Phase 4: Action pooling (Proposal 3)**
- Moderate complexity, moderate impact
- Do after architecture is stable
- Changes Action ownership semantics
- Expected: +10-15% throughput

**Expected cumulative impact:**
- Phase 1: +25-35% throughput
- Phase 1+2: +60-80% throughput
- Phase 1+2+3: +70-90% throughput
- Phase 1+2+3+4: +90-120% throughput (potentially 2x faster)

---

## Open Questions

1. **Triggered abilities timeline?** Proposals 1 and 4 assume the turn structure is static. If "at beginning of upkeep" triggers are imminent, the feasibility checks and combat skip become more complex. When are triggered abilities planned?

2. **Multi-threading plans?** If simulation will be parallelized, action pooling needs thread-local storage. The flat state machine is more thread-friendly than the current design (less shared state).

3. **Target throughput?** Current ~340 games/sec. Is 500? 1000? Knowing the target helps prioritize the riskier refactors vs. stopping at good-enough.

4. **Compatibility requirements?** The proposals change ActionSpace ownership semantics. Does manabot store actions beyond step boundaries? Does it rely on unique_ptr ownership?

5. **Test coverage confidence?** The flat state machine is a significant refactor. What's the test coverage like for turn structure edge cases (e.g., first turn no draw, stack interactions)?

---

## Already Implemented

These optimizations are already in the codebase (verified in diagnosis.md):

- **Cached producible mana**: `ManaCache` in `game.h:36-44`, invalidated on battlefield/tap changes
- **Lazy observation creation**: Observation built only when `Game::observation()` called
- **Zone vector storage**: `Zone::cards` is `std::vector<std::vector<Card*>>`, not map
- **Battlefield vector storage**: `Battlefield::permanents` is `std::vector<std::vector<unique_ptr<Permanent>>>`
- **Observation vectors**: `std::vector<CardData>` and `std::vector<PermanentData>` instead of maps
- **playersStartingWithActive() caching**: Added `cached_active_index` to TurnSystem. Only rebuilds when active player changes. Impact: +3.8% throughput.
- **Action execution profiler scope**: Measured at 3.8% of game time - NOT a bottleneck.

---

## What Makes These Proposals Bold

The previous proposals contained good incremental ideas. These proposals go further:

1. **Proposal 1** doesn't just "reduce profiler scopes" - it **eliminates the Turn/Phase/Step object model entirely**. This is architecturally fundamental, not a tweak.

2. **Proposal 2** doesn't just "check feasibility before building ActionSpace" - it **integrates skip_trivial into the tick loop**, eliminating an entire abstraction layer (the outer skip loop).

3. **Proposal 3** doesn't just "pool PassPriority" - it **changes Action ownership semantics** from unique_ptr to raw pointers, which ripples through the entire action handling system.

4. **Proposal 4** doesn't just "skip empty steps" - it **short-circuits entire phases** based on game state, treating the turn structure as data rather than code structure.

The key insight is that the current architecture was designed for correctness and clarity. These proposals redesign it for speed, accepting that the code will be harder to understand in exchange for 2x+ throughput.
