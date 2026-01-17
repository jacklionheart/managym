# Refactor Proposals

## Summary

The simulation bottleneck is **architectural, not algorithmic**. After optimizations reduced observation overhead to 3.4%, the turn/phase/step hierarchy now dominates at 31% of total time—despite doing minimal actual work. The 10.1x tick amplification (98,502 internal ticks for 9,779 agent decisions) reveals the core problem: **the game engine is organized for code clarity, not for fast simulation**.

The fundamental insight: **Magic's turn structure is deterministic and sparse**. Most steps produce no decisions. Most priority passes are automatic. The engine traverses a 4-level object hierarchy and creates 4 profiler scopes to ultimately return "PassPriority" 97% of the time.

Bold refactoring means restructuring the engine around the actual workload: ~10,000 decisions per 200 games, not ~100,000 hierarchy traversals.

## Proposals

### 1. Decision-Driven Tick Loop — Expected impact: 35-50%

**Current state:**
The tick loop processes every step in sequence, regardless of whether decisions are possible:

```
Game::tick() → TurnSystem::tick() → Turn::tick() → Phase::tick() → Step::tick() → PrioritySystem::tick()
```

For 9,779 agent decisions, this hierarchy executes:
- 67,822 game ticks (skip_trivial loop iterations)
- 98,502 turn/phase/step ticks (hierarchy traversals)
- 87,590 priority ticks (97% are just PassPriority)
- 394,008 profiler scope creations/destructions

**Problem:**
The engine asks "what step are we in?" then "does this step have decisions?" The question should be inverted: "what's the next decision point?" then "advance state to there."

The polymorphic Phase/Step hierarchy exists for code organization. At runtime, Magic's turn structure is:
```
UNTAP → UPKEEP → DRAW → MAIN1 → COMBAT_BEGIN → ATTACKERS → BLOCKERS → DAMAGE → COMBAT_END → MAIN2 → END → CLEANUP
```
This never changes. A switch statement is faster than virtual dispatch through 4 object levels.

**Proposal:**
Replace the pull-based hierarchy with a push-based decision finder. The entire turn state becomes:

```cpp
struct TurnState {
    StepType step = StepType::BEGINNING_UNTAP;
    int priority_player = 0;  // 0 = active, 1 = NAP, 2 = stack resolution
    bool sba_done = false;
};
```

A single function finds the next decision:

```cpp
std::unique_ptr<ActionSpace> TurnSystem::findNextDecision() {
    while (true) {
        // Execute turn-based actions for current step
        executeTurnBasedActions(state.step);

        // Check if this step can produce a non-trivial decision
        if (canProduceDecision(state.step)) {
            ActionSpace* space = buildActionSpace(state.step);
            if (space->actions.size() > 1) {
                return std::unique_ptr<ActionSpace>(space);
            }
            // Trivial action space - auto-execute and continue
            space->actions[0]->execute();
            delete space;
        }

        // Advance to next step
        if (!advanceStep()) {
            // Turn complete, start next turn
            advanceActivePlayer();
            state.step = StepType::BEGINNING_UNTAP;
        }
    }
}
```

Key insight: **combine skip_trivial into the tick loop itself**. Don't return ActionSpaces with 1 action; auto-execute them internally. This eliminates the 6.9x game tick amplification.

**Implementation sketch:**
1. Add `TurnState` to TurnSystem with step/priority/sba fields
2. Add `executeTurnBasedActions(StepType)` - switch statement for untap/draw/cleanup
3. Add `canProduceDecision(StepType)` - checks hand size, creatures, abilities
4. Add `buildActionSpace(StepType)` - creates actions for current step
5. Add `advanceStep()` - increments step, handles phase boundaries
6. Replace `Turn`, `Phase`, `Step` classes with `TurnState` + helper methods
7. Single profiler scope for `findNextDecision()`, not nested scopes

**Impact breakdown:**
- Eliminates 4-level object hierarchy traversal: ~15% savings
- Eliminates 3 of 4 profiler scopes per tick: ~10% savings
- Combines skip_trivial loop: ~10% savings
- Reduces function call overhead: ~5% savings

**Risks:**
- Large refactor touching turn.cpp, turn.h, game.cpp, priority.cpp
- Combat phase step skipping logic needs careful handling
- Tests assume Turn/Phase/Step objects exist for assertions
- BehaviorTracker callbacks need to fire at correct points

---

### 2. Static Decision Feasibility — Expected impact: 20-30%

**Current state:**
Every step creates a full ActionSpace then checks if it's trivial:

```cpp
// priority.cpp:67-94
std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    for (Card* card : hand_cards) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            actions.emplace_back(new PlayLand(card, player, game));
        } else if (card->types.isCastable() && game->canCastSorceries(player)) {
            if (game->canPayManaCost(player, card->mana_cost.value())) {
                actions.emplace_back(new CastSpell(card, player, game));
            }
        }
    }

    actions.emplace_back(new PassPriority(player, game));  // Always
    return actions;
}
```

With 87,590 priority ticks, this creates 87,590 PassPriority objects even though 97% of these are the only action.

**Problem:**
The engine builds a complete ActionSpace to discover it has only PassPriority. This is backwards. Check feasibility first:

- Empty hand + no battlefield abilities = only PassPriority possible
- Main phase + can't play land + can't afford any spell = only PassPriority
- Combat declare attackers + no eligible attackers = skip step entirely

**Proposal:**
Add a fast feasibility check before building ActionSpace:

```cpp
enum DecisionFeasibility {
    NO_DECISIONS,        // Skip this priority pass entirely
    TRIVIAL_ONLY,        // Only PassPriority - auto-execute
    HAS_REAL_DECISIONS   // Build full ActionSpace
};

DecisionFeasibility TurnSystem::checkFeasibility(StepType step, Player* player) {
    // Steps without priority windows
    if (step == BEGINNING_UNTAP || step == ENDING_CLEANUP) {
        return NO_DECISIONS;
    }

    // Combat steps without creatures
    if (step == COMBAT_DECLARE_ATTACKERS) {
        if (game->zones->constBattlefield()->eligibleAttackers(player).empty()) {
            return NO_DECISIONS;
        }
    }

    // Priority passes - check if any non-pass actions possible
    int hand_size = game->zones->size(ZoneType::HAND, player);
    if (hand_size == 0) {
        // No hand, check battlefield abilities
        if (!hasBattlefieldAbilities(player)) {
            return TRIVIAL_ONLY;  // Only PassPriority
        }
    }

    // Has cards - check affordability
    if (!hasAffordableAction(player)) {
        return TRIVIAL_ONLY;
    }

    return HAS_REAL_DECISIONS;
}

bool TurnSystem::hasAffordableAction(Player* player) {
    const Mana& available = game->cachedProducibleMana(player);

    // Check if can play land (sorcery speed only)
    if (game->canPlayLand(player)) {
        for (Card* card : game->zones->constHand()->cards[player->index]) {
            if (card->types.isLand()) return true;
        }
    }

    // Check if can cast any spell
    if (game->canCastSorceries(player)) {
        for (Card* card : game->zones->constHand()->cards[player->index]) {
            if (card->types.isCastable() && available.canPay(card->mana_cost.value())) {
                return true;
            }
        }
    }

    return false;
}
```

**Implementation sketch:**
1. Add `checkFeasibility()` method to TurnSystem
2. Add `hasAffordableAction()` helper using cached mana
3. In tick loop: if `TRIVIAL_ONLY`, auto-advance priority without building ActionSpace
4. If `NO_DECISIONS`, skip step entirely (don't even enter priority system)
5. Only build full ActionSpace when `HAS_REAL_DECISIONS`

**Impact breakdown:**
- Skip ~60% of priority passes (empty hand, no affordable spells): ~12% savings
- Avoid Action object allocation for trivial passes: ~8% savings
- Reduce priority system iterations: ~5% savings

**Risks:**
- Must be 100% correct - skipping a step that has a real decision breaks the game
- Need extensive test coverage for edge cases
- Future cards with triggered abilities complicate this

---

### 3. Pooled Action Objects — Expected impact: 8-12%

**Current state:**
Every priority check allocates new Action objects:

```cpp
actions.emplace_back(new PlayLand(card, player, game));
actions.emplace_back(new CastSpell(card, player, game));
actions.emplace_back(new PassPriority(player, game));  // 87,590 times!
```

For 200 games: ~90,000 PassPriority allocations, ~5,000 PlayLand allocations, ~5,000 CastSpell allocations.

**Problem:**
PassPriority is identical every time—same player, same game, same effect. Yet we allocate 87,590 of them. PlayLand and CastSpell are parameterized but still follow predictable patterns.

**Proposal:**
Pre-allocate action objects and reset them instead of creating new ones:

```cpp
// In PrioritySystem
struct ActionCache {
    PassPriority pass_priority[2];     // One per player
    std::vector<PlayLand> play_lands;  // Sized to max hand size
    std::vector<CastSpell> cast_spells;

    void reset(Game* game) {
        for (int i = 0; i < 2; i++) {
            pass_priority[i].player = game->players[i].get();
            pass_priority[i].game = game;
        }
        // play_lands and cast_spells resized as needed
    }
};
```

Return raw pointers to cached actions instead of unique_ptr:

```cpp
std::vector<Action*> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<Action*> actions;

    int land_idx = 0;
    int spell_idx = 0;

    for (Card* card : hand_cards) {
        if (card->types.isLand() && game->canPlayLand(player)) {
            cache.play_lands[land_idx].card = card;
            cache.play_lands[land_idx].player = player;
            actions.push_back(&cache.play_lands[land_idx++]);
        } else if (card->types.isCastable() && game->canPayManaCost(player, card->mana_cost.value())) {
            cache.cast_spells[spell_idx].card = card;
            cache.cast_spells[spell_idx].player = player;
            actions.push_back(&cache.cast_spells[spell_idx++]);
        }
    }

    actions.push_back(&cache.pass_priority[player->index]);
    return actions;
}
```

**Alternative: Flyweight pattern**
If pooling is too complex, use a simpler flyweight for PassPriority:

```cpp
// Single shared PassPriority per player, never deallocated
PassPriority* PrioritySystem::getPassPriority(Player* player) {
    static PassPriority pass[2];  // Thread-local if needed
    pass[player->index].player = player;
    pass[player->index].game = game;
    return &pass[player->index];
}
```

**Implementation sketch:**
1. Change ActionSpace to store `std::vector<Action*>` not `std::vector<unique_ptr<Action>>`
2. Add ActionCache to PrioritySystem
3. Reset cache on game reset
4. Update action creation to use cached objects
5. Ensure actions are not stored beyond the current step

**Impact:**
- Eliminates 87,590 PassPriority heap allocations: ~4% savings
- Eliminates ~10,000 PlayLand/CastSpell allocations: ~2% savings
- Reduces allocator pressure: ~3% savings

**Risks:**
- Action lifetime must be carefully managed (valid only until next step)
- ActionSpace can't own actions anymore - semantic change
- Thread safety if multi-threaded later

---

### 4. Lazy Priority with Early Exit — Expected impact: 10-15%

**Current state:**
Priority system creates all possible actions upfront, then the agent picks one:

```cpp
// Build all actions
std::vector<Action> actions = availablePriorityActions(player);
// Agent picks
int choice = agent->selectAction(actions);
// Execute one
actions[choice]->execute();
```

For a training agent that always picks action 0 (or uses a simple heuristic), we're building 5-10 actions when only 1 will be used.

**Problem:**
The engine doesn't know if the agent will:
1. Always pick PassPriority (97% of steps in skip_trivial mode)
2. Pick the first land if one is playable
3. Pick the cheapest castable spell

If the engine could predict the agent's choice, it could skip building other actions.

**Proposal:**
Add an "eager agent" mode where simple policies are evaluated inline:

```cpp
// For skip_trivial mode: if only PassPriority or 1 other action, auto-resolve
std::unique_ptr<ActionSpace> PrioritySystem::tick() {
    // Check for trivial cases first
    int hand_size = game->zones->size(ZoneType::HAND, player);
    bool can_play_land = hand_size > 0 && game->canPlayLand(player);
    bool can_cast = hand_size > 0 && game->canCastSorceries(player) && hasAffordableSpell(player);

    // If no real actions, auto-pass
    if (!can_play_land && !can_cast) {
        passPriority();  // Don't even build ActionSpace
        return nullptr;  // Continue tick loop
    }

    // Build minimal ActionSpace
    return makeActionSpace(player);
}
```

For training, add a callback hook:

```cpp
// Agent provides a policy hint
enum PolicyHint {
    FULL_ACTIONS,      // Build all actions, agent will choose
    PREFER_FIRST,      // Auto-pick first non-pass action if available
    PREFER_PASS,       // Auto-pass unless forced
    PREFER_AGGRESSIVE  // Prefer attacks/casts over pass
};

void Env::setPolicyHint(PolicyHint hint);
```

**Implementation sketch:**
1. Add `PolicyHint` to Env configuration
2. In PrioritySystem::tick(), check hint before building full ActionSpace
3. For `PREFER_PASS`: immediately pass if PassPriority is available
4. For `PREFER_FIRST`: build actions lazily until first non-pass found
5. Return full ActionSpace only for `FULL_ACTIONS` or when hint doesn't apply

**Impact:**
- Skip ActionSpace building for 97% of steps in training: ~10% savings
- Reduce action object creation: ~3% savings

**Risks:**
- Changes agent interface semantics
- Training quality may differ if actions aren't fully enumerated
- Need to ensure hint doesn't break game correctness

---

## Sequence

**Phase 1: Quick wins (implement together)**
1. ~~Cache `playersStartingWithActive()` with `cached_active_index` — 5-10% impact~~ ✓ DONE (+3.8% measured)
2. Pooled PassPriority flyweight — 3-5% impact
3. Single profiler scope at step level — 3-5% impact

**Phase 2: Decision feasibility**
4. Static decision feasibility checks — 20-30% impact
   - This can be done independently of the flat state machine
   - Lower risk, easier to test

**Phase 3: Architectural refactor**
5. Decision-driven tick loop — 35-50% impact
   - Largest change, requires Phase 2 to be solid
   - Subsumes several smaller optimizations

**Phase 4: Advanced optimizations (if needed)**
6. Pooled action objects — 8-12% impact
7. Lazy priority with early exit — 10-15% impact

**Expected cumulative impact:**
- Phase 1: +15-25% throughput (quick, low risk)
- Phase 1+2: +40-60% throughput
- Phase 1+2+3: +80-120% throughput (potentially 2x faster)
- Phase 1+2+3+4: +100-150% throughput

## Open Questions

1. **Training mode vs evaluation mode?** The lazy priority and policy hints are most valuable for training. Should there be a fast training mode that assumes simple policies?

2. **Multi-threading plans?** If simulation will be parallelized, action pooling needs thread-local storage. The flat state machine approach is more thread-friendly than the current design.

3. **Triggered abilities timeline?** Proposals 1 and 2 assume the turn structure is static. If "at beginning of upkeep" triggers are imminent, the feasibility checks become more complex.

4. **Target throughput?** Current ~340 games/sec. Is 500? 1000? Knowing the target helps prioritize the riskier refactors.

## Already Implemented

These optimizations are already in the codebase (verified in diagnosis.md):

- **Cached producible mana**: `ManaCache` in `game.h:36-44`, invalidated on battlefield/tap changes
- **Lazy observation creation**: Observation built only when `Game::observation()` called
- **Zone vector storage**: `Zone::cards` is `std::vector<std::vector<Card*>>`, not map
- **Battlefield vector storage**: `Battlefield::permanents` is `std::vector<std::vector<unique_ptr<Permanent>>>`
- **Observation vectors**: `std::vector<CardData>` and `std::vector<PermanentData>` instead of maps
- **playersStartingWithActive() caching**: Added `cached_active_index` to TurnSystem (`turn.h:104`, `turn.cpp:175-188`). Only rebuilds when active player changes. Impact: +3.8% throughput (339 → 352 games/sec).
