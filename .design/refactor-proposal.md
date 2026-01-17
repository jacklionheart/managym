# Refactor Proposals

## Summary

The bottleneck is **structural waste at scale**. The engine performs 556,702 priority ticks to produce 12,839 actual decisions (2.3% hit rate). For 97.7% of priority passes, it:
1. Traverses a 4-level object hierarchy (Turn→Phase→Step→Priority)
2. Builds a complete ActionSpace with allocated Action objects
3. Returns it to the caller
4. Checks if trivial (size == 1)
5. Executes action 0 and destroys everything
6. Repeats

The fix isn't caching or pooling—it's **not doing the work in the first place**. When only PassPriority is available, skip the ActionSpace construction entirely. When a phase can't produce decisions, don't enter it.

---

## Proposals

### 1. Early-Out Priority Check — Expected impact: 50-60%

**Current state:**
Every priority tick builds a full ActionSpace to determine if the player can act:

```cpp
// priority.cpp:67-93
std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    for (Card* card : hand_cards) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            actions.emplace_back(new PlayLand(...));  // ALLOCATION
        } else if (...) {
            actions.emplace_back(new CastSpell(...)); // ALLOCATION
        }
    }

    actions.emplace_back(new PassPriority(...));      // ALLOCATION (556,702 times!)
    return actions;
}
```

For 97.7% of calls, the result is a single PassPriority. We allocate objects, wrap them in unique_ptrs, create a vector, return it, check size, then destroy everything.

**Problem:**
The check for "can player do anything?" is buried inside action construction. We build the answer to throw it away.

**Proposal:**
Add a fast predicate that answers "can this player do anything?" without allocation:

```cpp
// In PrioritySystem or as a free function
bool PrioritySystem::canPlayerAct(Player* player) {
    // Empty hand = only PassPriority
    const std::vector<Card*>& hand = game->zones->constHand()->cards[player->index];
    if (hand.empty()) {
        return false;
    }

    // Check if ANY card is playable
    bool can_play_land = game->canPlayLand(player);
    bool can_cast = game->canCastSorceries(player);
    const Mana& available = game->cachedProducibleMana(player);

    for (Card* card : hand) {
        if (card->types.isLand() && can_play_land) {
            return true;
        }
        if (card->types.isCastable() && can_cast) {
            if (available.canPay(card->mana_cost.value())) {
                return true;
            }
        }
    }

    return false;
}
```

Then in the priority tick, skip ActionSpace construction when trivial:

```cpp
std::unique_ptr<ActionSpace> PrioritySystem::tick() {
    // SBA check (unchanged)
    if (!sba_done) {
        performStateBasedActions();
        sba_done = true;
        if (game->isGameOver()) return nullptr;
    }

    const std::vector<Player*>& players = game->playersStartingWithActive();

    while (pass_count < players.size()) {
        Player* player = players[pass_count];

        // FAST PATH: If player can't act, auto-pass without allocating
        if (game->skip_trivial && !canPlayerAct(player)) {
            pass_count++;
            continue;
        }

        // SLOW PATH: Build full ActionSpace
        return makeActionSpace(player);
    }

    // All passed - resolve stack or complete
    reset();
    if (!game->zones->constStack()->objects.empty()) {
        resolveTopOfStack();
        return tick();
    }

    return nullptr;
}
```

**Impact breakdown:**
- Eliminates 543,863 ActionSpace constructions (97.7% of 556,702)
- Eliminates 543,863 PassPriority allocations
- Eliminates 543,863 vector<unique_ptr<Action>> allocations
- Per-priority-tick cost drops from ~0.42μs to ~0.05μs for trivial cases

**Sketch:**
1. Add `canPlayerAct(Player*)` to PrioritySystem
2. Modify `PrioritySystem::tick()` to use fast path when `skip_trivial` enabled
3. Keep existing `availablePriorityActions` for the 2.3% that need it

**Risks:**
- `canPlayerAct` must match `availablePriorityActions` exactly or we skip real decisions
- Need regression tests that verify fast path matches slow path
- Future activated abilities on battlefield need to be added to both paths

---

### 2. Collapse Skip-Trivial into Single tick() — Expected impact: 20-30%

**Current state:**
The skip_trivial loop in `game.cpp:224-227` calls `_step(0)` repeatedly:

```cpp
bool Game::step(int action) {
    bool game_over = _step(action);

    while (!game_over && skip_trivial && actionSpaceTrivial()) {
        Profiler::Scope skip_scope = profiler->track("skip_trivial");
        game_over = _step(0);
    }
    return game_over;
}
```

Each `_step(0)` call:
1. Validates action space (redundant for trivial)
2. Executes PassPriority (increments counter)
3. Clears action space and observation
4. Calls `tick()` to get next action space
5. Returns to caller who checks if trivial again

**Problem:**
The step/tick boundary is crossed 400,128 times to handle trivial actions. Each crossing involves:
- Function call overhead
- Action space ownership transfer
- Observation invalidation
- Profiler scope creation

**Proposal:**
Move the trivial-handling loop inside `tick()`. Never return a trivial ActionSpace:

```cpp
bool Game::tick() {
    while (true) {
        // Get next action space from turn system
        current_action_space = turn_system->tick();

        // Game over check
        if (isGameOver()) {
            current_action_space = ActionSpace::createEmpty();
            return true;
        }

        // If we got an action space, check if we should auto-execute
        if (current_action_space) {
            if (!skip_trivial || current_action_space->actions.size() > 1) {
                // Non-trivial: return to caller for decision
                return false;
            }

            // Trivial: auto-execute action 0 and continue
            current_action_space->actions[0]->execute();
            current_action_space = nullptr;
            current_observation = nullptr;
            // Loop continues to get next action space
        }
    }
}
```

Now `step()` simplifies to:

```cpp
bool Game::step(int action) {
    Profiler::Scope scope = profiler->track("game");

    // Execute the agent's chosen action
    current_action_space->actions[action]->execute();
    current_action_space = nullptr;
    current_observation = nullptr;

    // Get next non-trivial action space (or game over)
    return tick();
}
```

**Impact breakdown:**
- Eliminates 400,128 `_step()` calls (replaced by tight loop)
- Eliminates 400,128 profiler scope creations for skip_trivial
- Reduces function call overhead
- Better instruction cache locality (tight loop vs function calls)

**Sketch:**
1. Modify `tick()` to loop internally until non-trivial ActionSpace or game over
2. Simplify `step()` to just execute action + call tick()
3. Remove the outer skip_trivial loop from `step()`

**Risks:**
- Changes tick() semantics (now may execute multiple trivial actions)
- Profiler output changes (skip_trivial scope disappears)
- Need to ensure game over detection still works correctly

---

### 3. Flatten Turn/Phase/Step Hierarchy — Expected impact: 15-25%

**Current state:**
Every tick traverses 4 levels of objects:

```cpp
// TurnSystem::tick() -> Turn::tick() -> Phase::tick() -> Step::tick()
std::unique_ptr<ActionSpace> Turn::tick() {
    Profiler::Scope scope = profiler->track("turn");  // 625,441 times!

    Phase* current_phase = phases[current_phase_index].get();
    std::unique_ptr<ActionSpace> result = current_phase->tick();
    // ...
}
```

Each Turn creates 5 Phase objects and ~12 Step objects on construction:

```cpp
Turn::Turn(Player* active_player, TurnSystem* turn_system) {
    phases.emplace_back(new BeginningPhase(this));    // 3 Steps
    phases.emplace_back(new PrecombatMainPhase(this)); // 1 Step
    phases.emplace_back(new CombatPhase(this));        // 5 Steps
    phases.emplace_back(new PostcombatMainPhase(this)); // 1 Step
    phases.emplace_back(new EndingPhase(this));        // 2 Steps
}
```

**Problem:**
The turn structure is static—it never changes at runtime. Yet we represent it with dynamic objects, allocate them each turn, and traverse them via virtual dispatch.

**Proposal:**
Replace object hierarchy with flat state in TurnSystem:

```cpp
// In TurnSystem
struct TurnState {
    StepType step = StepType::BEGINNING_UNTAP;
    bool step_initialized = false;
    bool turn_based_actions_complete = false;
    int priority_pass = 0;
    bool sba_done = false;
    int lands_played = 0;
    int turn_number = 0;
};

TurnState state;

// Combat state (kept separate for clarity)
struct CombatState {
    std::vector<Permanent*> attackers;
    std::vector<Permanent*> attackers_to_declare;
    std::vector<Permanent*> blockers_to_declare;
    std::map<Permanent*, std::vector<Permanent*>> attacker_to_blockers;

    void reset() {
        attackers.clear();
        attackers_to_declare.clear();
        blockers_to_declare.clear();
        attacker_to_blockers.clear();
    }
};

CombatState combat;
```

A single function advances the state machine:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::tick() {
    while (true) {
        // Initialize step if needed
        if (!state.step_initialized) {
            executeStepStart(state.step);
            state.step_initialized = true;
        }

        // Turn-based actions (draw, attackers, blockers, damage)
        if (!state.turn_based_actions_complete) {
            std::unique_ptr<ActionSpace> tba = executeTurnBasedActions(state.step);
            if (tba) return tba;
            state.turn_based_actions_complete = true;
        }

        // Priority window
        if (stepHasPriority(state.step)) {
            std::unique_ptr<ActionSpace> priority = game->priority_system->tick();
            if (priority) return priority;
        }

        // Step complete
        executeStepEnd(state.step);
        game->clearManaPools();

        if (!advanceStep()) {
            // Turn complete
            advanceTurn();
        }

        state.step_initialized = false;
        state.turn_based_actions_complete = false;
    }
}
```

Where `stepHasPriority` is a lookup table (not function calls):

```cpp
static constexpr bool STEP_HAS_PRIORITY[] = {
    false,  // BEGINNING_UNTAP
    true,   // BEGINNING_UPKEEP
    true,   // BEGINNING_DRAW
    true,   // PRECOMBAT_MAIN_STEP
    true,   // COMBAT_BEGIN
    true,   // COMBAT_DECLARE_ATTACKERS
    true,   // COMBAT_DECLARE_BLOCKERS
    true,   // COMBAT_DAMAGE
    true,   // COMBAT_END
    true,   // POSTCOMBAT_MAIN_STEP
    true,   // ENDING_END
    false,  // ENDING_CLEANUP
};

bool TurnSystem::stepHasPriority(StepType step) {
    return STEP_HAS_PRIORITY[static_cast<int>(step)];
}
```

**Impact breakdown:**
- Eliminates ~165,000 Phase/Step object allocations per 500 games
- Eliminates 625,441 Turn::tick() profiler scope creations
- Eliminates virtual dispatch for tick() calls
- Better branch prediction (switch vs polymorphism)
- Smaller memory footprint (no object pointers to chase)

**Sketch:**
1. Add `TurnState` and `CombatState` structs to TurnSystem
2. Add `executeStepStart(StepType)` — untap logic, attacker/blocker setup, etc.
3. Add `executeTurnBasedActions(StepType)` — draw, declare attackers, etc.
4. Add `executeStepEnd(StepType)` — callbacks for BehaviorTracker
5. Add `advanceStep()` — state machine transitions
6. Keep Turn/Phase/Step classes for backward compatibility with tests, but don't use in hot path
7. Observation still works (reads step type from state)

**Risks:**
- Large refactor touching many files
- Combat state management is tricky
- BehaviorTracker callbacks must fire at correct points
- Tests assume Turn/Phase/Step objects exist

---

### 4. Skip Empty Combat Phases — Expected impact: 5-10%

**Current state:**
Every turn enters the combat phase regardless of board state:

```cpp
CombatPhase::CombatPhase(Turn* parent_turn) : Phase(parent_turn) {
    steps.emplace_back(new BeginningOfCombatStep(this));
    steps.emplace_back(new DeclareAttackersStep(this));
    steps.emplace_back(new DeclareBlockersStep(this));
    steps.emplace_back(new CombatDamageStep(this));
    steps.emplace_back(new EndOfCombatStep(this));
}
```

DeclareAttackersStep checks for eligible attackers, but only after entering the step:

```cpp
void DeclareAttackersStep::onStepStart() {
    attackers_to_declare = game()->zones->constBattlefield()->eligibleAttackers(active_player);
}
```

**Problem:**
For early-game turns with no creatures, we traverse 5 combat steps (with priority windows!) just to do nothing.

**Proposal:**
In the flat state machine, skip combat when there are no eligible attackers:

```cpp
bool TurnSystem::advanceStep() {
    state.step_initialized = false;
    state.turn_based_actions_complete = false;

    switch (state.step) {
    case StepType::PRECOMBAT_MAIN_STEP:
        // Check if combat is worth entering
        if (shouldSkipCombat()) {
            state.step = StepType::POSTCOMBAT_MAIN_STEP;
        } else {
            state.step = StepType::COMBAT_BEGIN;
        }
        break;

    case StepType::COMBAT_DECLARE_ATTACKERS:
        // If no attackers were declared, skip straight to end of combat
        if (combat.attackers.empty()) {
            state.step = StepType::COMBAT_END;
        } else {
            state.step = StepType::COMBAT_DECLARE_BLOCKERS;
        }
        break;

    case StepType::COMBAT_DECLARE_BLOCKERS:
        // If still no attackers (all declined), skip damage
        if (combat.attackers.empty()) {
            state.step = StepType::COMBAT_END;
        } else {
            state.step = StepType::COMBAT_DAMAGE;
        }
        break;

    // ... other cases as normal switch
    }
    return true;
}

bool TurnSystem::shouldSkipCombat() {
    // Only skip if skip_trivial is enabled
    if (!game->skip_trivial) return false;

    // Skip if active player has no eligible attackers
    return game->zones->constBattlefield()->eligibleAttackers(activePlayer()).empty();
}
```

**Impact:**
- Skip ~30-50% of turns' combat phases (depends on game length)
- Each skipped combat saves 5 step traversals + priority windows
- Reduces tick count by ~10-15% for creature-light games

**Risks:**
- Future "at beginning of combat" triggers may need combat phase even without creatures
- Only enable when skip_trivial is true
- BehaviorTracker may expect combat callbacks

---

## Sequence

**Phase 1: Early-Out Priority Check (Proposal 1)** — Do first
- Highest impact (~50-60%)
- Localized change (priority.cpp only)
- Easy to test in isolation
- Enables measuring remaining bottlenecks

**Phase 2: Collapse Skip-Trivial (Proposal 2)** — Do second
- High impact (~20-30%)
- Small change (game.cpp only)
- Simplifies profiler output

**Phase 3: Flatten Hierarchy (Proposal 3)** — Do third
- Moderate impact (~15-25%)
- Large change but well-defined
- Depends on Proposals 1 & 2 proving the approach

**Phase 4: Skip Empty Combat (Proposal 4)** — Do last
- Lower impact (~5-10%)
- Simple once Proposal 3 is done
- Natural extension of flat state machine

**Expected cumulative throughput:**
- Baseline: 42,513 steps/sec
- After Proposal 1: ~85,000 steps/sec (+100%)
- After Proposal 2: ~110,000 steps/sec (+160%)
- After Proposal 3: ~135,000 steps/sec (+220%)
- After Proposal 4: ~145,000 steps/sec (+240%)

Target: **3x current throughput** (from 42k to ~125k+ steps/sec)

---

## Open Questions

1. **Activated abilities timeline?** Proposal 1's `canPlayerAct()` only checks hand cards. When permanents get activated abilities, it needs updating. Is this imminent?

2. **Stack interaction complexity?** The current skip_trivial behavior auto-passes when stack is non-empty (since players can only pass). Verify this is correct for all stack states.

3. **BehaviorTracker integration?** The flat state machine needs to call `onMainPhaseStart`, `onDeclareAttackersStart`, etc. at the right points. Should these be inline in `executeStepStart()` or separate?

4. **Test coverage for turn structure?** Proposal 3 is a significant refactor. What's the test coverage for edge cases like first-turn-no-draw, multiple combat phases (via extra turns), and stack interactions during phases?

5. **pybind11 compatibility?** The Observation struct reads from Turn/Phase/Step. After flattening, it reads from TurnState. Are there Python-side assumptions about these structures?

---

## Already Implemented

These optimizations are in the codebase:

- **Lazy observation**: Built only when `observation()` called
- **Vector zones**: `cards` is `vector<vector<Card*>>`
- **Cached producible mana**: `ManaCache` in game.h
- **Cached player order**: `playersStartingWithActive()` returns reference
- **InfoDict only at episode end**: Removed per-step overhead
- **Removed phase/step profiler scopes**: Only turn/priority remain

---

## What Makes These Proposals Bold

1. **Proposal 1** doesn't optimize action construction—it **eliminates 97.7% of action constructions entirely** by asking "can player act?" before building the answer.

2. **Proposal 2** doesn't speed up skip_trivial—it **removes the step/tick boundary** for trivial actions, collapsing a loop of function calls into a tight inner loop.

3. **Proposal 3** doesn't cache Turn objects—it **eliminates the Turn/Phase/Step object model** in the hot path, replacing dynamic objects with static state.

4. **Proposal 4** doesn't speed up combat—it **skips combat entirely** when there's nothing to fight with.

The theme: **don't optimize slow code; eliminate the code**.
