# Refactor Proposals

## Summary

The core architectural problem is **redundant computation inside the priority system**. The tick loop structure itself is sound—the cost comes from:

1. **Duplicate iteration**: `canPlayerAct()` checks every card's playability, then `availablePriorityActions()` does the exact same iteration again when building the action space
2. **Per-step allocations**: Every priority tick creates new `std::vector<std::unique_ptr<Action>>` and potentially copies player lists
3. **Per-observation allocations**: `playersStartingWithAgent()` builds a new vector on each call (3× per observation)

The tick loop runs 311K times for 500 games. Even tiny inefficiencies multiply to significant overhead.

## Proposals

### 1. Fused Priority Check — Impact: ~8% of total

**Current**: `priority.cpp:69-76` calls `canPlayerAct()` which iterates the hand, then immediately calls `makeActionSpace()` which calls `availablePriorityActions()` and iterates the hand again.

**Problem**: Every card's playability is computed twice when the player CAN act:
- `canPlayerAct()` at line 36-44: checks `card->types.isLand() && can_play_land`, `card->types.isCastable() && can_cast && game->canPayManaCost()`
- `availablePriorityActions()` at line 116-128: exact same conditions, exact same order

With 7 cards in hand and mana checks being O(battlefield_size), this duplication is expensive. The profile shows `PRECOMBAT_MAIN_STEP/priority` takes 0.078s (50% of main step time).

**Proposal**: Return actions directly from a single pass. Replace the two-method dance with one:

```cpp
// Returns pair: first = can_act (has actions beyond pass), second = actions
std::pair<bool, std::vector<std::unique_ptr<Action>>>
PrioritySystem::computePlayerActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;

    const std::vector<Card*>& hand = game->zones->constHand()->cards[player->index];
    bool can_play_land = game->canPlayLand(player);
    bool can_cast = game->canCastSorceries(player);

    for (Card* card : hand) {
        if (card->types.isLand() && can_play_land) {
            actions.emplace_back(new PlayLand(card, player, game));
        } else if (card->types.isCastable() && can_cast) {
            if (game->canPayManaCost(player, card->mana_cost.value())) {
                actions.emplace_back(new CastSpell(card, player, game));
            }
        }
    }

    bool can_act = !actions.empty();
    actions.emplace_back(new PassPriority(player, game));
    return {can_act, std::move(actions)};
}
```

Then in `tick()`:
```cpp
if (game->skip_trivial) {
    auto [can_act, actions] = computePlayerActions(player);
    if (!can_act) {
        pass_count++;
        continue;  // Actions discarded, only PassPriority
    }
    return std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY,
                                         std::move(actions), player);
}
```

**Sketch**:
1. Add `computePlayerActions()` method that returns `std::pair<bool, std::vector<...>>`
2. Replace `canPlayerAct()` call + `makeActionSpace()` with single call
3. Delete `canPlayerAct()` and `availablePriorityActions()` (now unused)
4. When `!can_act`, the actions vector is small (just PassPriority) and gets discarded

**Risk**: None—this is a pure refactor. The logic is identical, just fused.

---

### 2. Cache playersStartingWithAgent() — Impact: ~1.5% of total

**Current**: `game.cpp:103-121` creates a new `std::vector<Player*>` on every call, iterates through players with `std::find_if`, then fills the vector.

**Problem**: Called 3 times per observation in:
- `populatePlayers()` at observation.cpp:99
- `populateCards()` at observation.cpp:133, 141

Each call does: vector allocation, `std::find_if` over players, modulo arithmetic to fill. With 73K observations, that's 219K unnecessary vector allocations.

**Proposal**: Cache the result like `playersStartingWithActive()` does in `turn.cpp:175-188`. Track `cached_agent_player` and invalidate when `action_space` changes.

```cpp
// In Game class:
Player* cached_agent_player = nullptr;
std::vector<Player*> players_agent_first;

const std::vector<Player*>& Game::playersStartingWithAgent() {
    Player* agent = agentPlayer();
    if (cached_agent_player != agent) {
        int num_players = players.size();
        if (players_agent_first.size() != num_players) {
            players_agent_first.resize(num_players);
        }
        int index = (agent == players[0].get()) ? 0 : 1;
        for (int i = 0; i < num_players; i++) {
            players_agent_first[i] = players[(index + i) % num_players].get();
        }
        cached_agent_player = agent;
    }
    return players_agent_first;
}
```

**Sketch**:
1. Add `cached_agent_player` and `players_agent_first` to Game
2. Change return type from `std::vector<Player*>` to `const std::vector<Player*>&`
3. Cache invalidates naturally when agent changes (checked via `agentPlayer()`)

**Risk**: Low. Cache invalidation is implicit via `agentPlayer()` comparison.

---

### 3. Pre-allocate Action vectors — Impact: ~2% of total

**Current**: `availablePriorityActions()` creates a new `std::vector<std::unique_ptr<Action>>` with default capacity (typically 0), then grows it via `emplace_back`.

**Problem**: With 7-card hands, this causes 3-4 reallocations per priority pass. With 46K PRECOMBAT_MAIN_STEP counts and ~5 priority passes each, that's ~1M unnecessary reallocations.

**Proposal**: Reserve capacity upfront. Hand size is known, and actions ≤ hand_size + 1.

```cpp
std::vector<std::unique_ptr<Action>> actions;
actions.reserve(hand_cards.size() + 1);  // +1 for PassPriority
```

Better yet, if we implement Proposal #1 (fused priority check), we can also pre-allocate:

```cpp
auto [can_act, actions] = computePlayerActions(player);
// Inside computePlayerActions:
actions.reserve(hand.size() + 1);
```

**Sketch**:
1. Add `reserve()` call at start of action building
2. Size hint = hand_size + 1 (worst case: all cards playable + pass)

**Risk**: None. `reserve()` is always safe; just reduces allocations.

---

### 4. Single-pass observation population — Impact: ~3% of total

**Current**: `populateCards()` iterates through hand, graveyard, exile, and stack separately. `populatePermanents()` iterates battlefield. For each permanent, `addPermanent()` calls `addCard()` which constructs a full `CardData`.

**Problem**:
- `addCard()` copies `card->name` (string allocation)
- `addCard()` checks 12 type flags via method calls
- `addPermanent()` creates both `PermanentData` and `CardData` for battlefield cards

With ~10 permanents per game average and 73K observations, that's 730K redundant CardData constructions for battlefield cards.

**Proposal**: Inline card data into `PermanentData` for battlefield cards. Don't call `addCard()` from `addPermanent()`.

```cpp
struct PermanentData {
    ObjectId id;
    ObjectId controller_id;
    ObjectId card_id;
    int registry_key;        // NEW: for card lookup
    bool tapped;
    int damage;
    bool is_summoning_sick;
    // Card-derived fields (avoid separate CardData)
    CardTypes card_types;    // NEW: embed type flags
    ManaCost mana_cost;      // NEW: embed mana cost
};
```

Then in Python, battlefield card data comes from `PermanentData` directly. The `agent_cards` and `opponent_cards` vectors only contain non-battlefield cards.

**Sketch**:
1. Expand `PermanentData` to include card fields needed by manabot
2. Remove `addCard(perm->card, ZoneType::BATTLEFIELD)` call in `addPermanent()`
3. Update Python bindings to handle new structure
4. Non-battlefield cards (hand, graveyard, exile, stack) still use `CardData`

**Risk**: API change. Python code needs updating. But manabot uses all these fields anyway, so the data is just reorganized.

---

## Sequence

1. **Proposal #1 (Fused Priority Check)** — Biggest win, no API change, pure refactor
2. **Proposal #3 (Pre-allocate Actions)** — Trivial change, pairs with #1
3. **Proposal #2 (Cache playersStartingWithAgent)** — Easy win, no API change
4. **Proposal #4 (Single-pass observation)** — Requires API coordination with manabot

## Open questions

1. **Manabot field usage**: Does manabot actually need all 12 type flags? If some are unused, we could skip populating them.

2. **String interning for card names**: `card->name` is copied on every `addCard()` call. Could we use string_view or index into a name table?

3. **Action object reuse**: Could we pool Action objects instead of creating new ones each tick? The current pattern creates ~500K Action objects per 500 games.

4. **Producible mana caching scope**: The mana cache is per-player per-game. Could we cache at a finer granularity (per permanent) to avoid iterating all permanents on each `canPayManaCost()` check?
