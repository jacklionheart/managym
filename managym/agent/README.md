# managym/agent

The agent module provides reinforcement learning APIs for the Magic: The Gathering game engine. 
It serves as the boundary between managym's C++ game logic and manabot's Python learning environment.

## Core Concepts

### Players

Players can be referenced several different related ways:

- `player_index`: Stable index [0, num_players) assigned at game start.
This can be used for indexing into num_players-length arrays.

- `player_id`: Game Object ID 
This is used as a key for accessing player data from the observation.
It is used by actions to reference things like targeting a player

- `is_agent`: Agent vs Opponent boolean 
This is the player who is responsible for the next action. All data
will be filtered to only include the perspective of the agent player.

- `is_active`: Boolean flag indicating whose turn it is
The "active player" in Magic: the Gathering is the player whose turn it is.
This is **not** necessarily the player who is responsible for the next action.

Key invariants:
- `player_index` and `player_id` never change during a game
- Exactly one player always has `is_agent=true` when an action is required

### Game Objects

Every game object (card, permanent, etc) has:
- A unique `id` that remains stable throughout its lifetime.
- A `controller_id` (the `player_id` of the player currently controlling the object)

Game objects are stored in a map indexed by `(controller_id, object_id)`.

Each game object type (card, permanent, etc) has its own map to support its own schema.


### Information Flow

managym is responsible for:
- Filtering hidden information for each player's perspective
- Setting `is_agent=true` for the acting player
- Maintaining unique, stable object IDs
- Organizing objects into appropriate zones

manabot is responsible for:
- Converting observations to tensors
- Organizing agent/opponent data
- Tracking valid actions
- Managing the learning environment

## Observation Space

The C++ observation type provides:
```cpp
struct Observation {
    bool game_over;
    bool won;
    TurnData turn;
    ActionSpaceData action_space;

    // Indexed by player_id
    std::map<int, PlayerData> players;
    std::map<int, CardData> cards;
    std::map<int, PermanentData> permanents;
};
```

Key aspects:
- Maps use `player_id` or object `id` as keys
- All data is from a single player's perspective
- Hidden cards (e.g. opponent's hand) are filtered out
- References between objects use stable IDs

## Action Space

Actions can reference game objects through focus lists:
```cpp
struct Action {
    ActionType type;
    std::vector<int> focus;  // Object IDs
};
```

Key invariants:
- All referenced objects exist in the observation
- Focus order matters (e.g. [attacker, blocker])
- IDs are stable across observations

## Test Guide

Tests should verify:
1. Player identity management
   - Stable indices
   - Correct agent flags
   - ID consistency

2. Object references
   - Valid focus lists
   - Correct owner/controller tracking
   - Stable IDs

3. Information hiding
   - Appropriate card filtering
   - Consistent zone information

4. Action validity
   - Legal game actions
   - Correct focus orders
   - Error handling