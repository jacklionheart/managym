#pragma once

#include "managym/agent/action.h"
#include "managym/agent/action_space.h"
#include "managym/flow/turn.h"
#include "managym/state/mana.h"
#include "managym/state/zones.h"

#include <array>
#include <map>
#include <vector>

class Game;
class Card;
class Permanent;
class StackObject;
class ManaValue;

// ------------------- Turn data ----------------------
struct TurnData {
    int turn_number;
    PhaseType phase;
    StepType step;
    // The "agent player" is the players who is taking the next action.
    int agent_player_id;
    // The "active player" in magic is the player whose turn it is,
    // but not necessarily the player currently taking 
    int active_player_id;
};

// ------------------- Player data --------------------
struct PlayerData {
    int id;           // Unique ID for the player
    int player_index; // 0 for first, 1 for second, etc.
    // Is the player taking the next action
    bool is_agent = false;
    // Is the player whose turn it is
    bool is_active = false;
    int life = 20;

    // [LIB, HAND, BATTLEFIELD, GRAVEYARD, EXILE, STACK, COMMAND]
    std::array<int, 7> zone_counts = {0, 0, 0, 0, 0, 0, 0};
};

// ------------------- Card data ----------------------

struct CardTypeData {
    bool is_castable = false;
    bool is_permanent = false;
    bool is_non_land_permanent = false;
    bool is_non_creature_permanent = false;
    bool is_spell = false;
    bool is_creature = false;
    bool is_land = false;
    bool is_planeswalker = false;
    bool is_enchantment = false;
    bool is_artifact = false;
    bool is_kindred = false;
    bool is_battle = false;
};


struct CardData {
    ZoneType zone;
    int owner_id;
    int id;
    int registry_key;
    int power;
    int toughness;
    CardTypeData card_types;
    ManaCost mana_cost;
};

// ------------------- Permanent data -----------------
struct PermanentData {
    int id;
    int controller_id;
    bool tapped = false;
    int damage = 0;
    bool is_creature = false;
    bool is_land = false;
    bool is_summoning_sick = false;
};

// ------------------- Actions ------------------------
struct ActionOption {
    ActionType action_type;
    std::vector<int> focus; // references to objects or players by ID
};

struct ActionSpaceData {
    ActionSpaceType action_space_type = ActionSpaceType::GAME_OVER;
    std::vector<ActionOption> actions;
    std::vector<int> focus; // references to objects or players by ID
};

// ------------------- Full Observation -----------------
// MR405.1 Observes all public game state for a single player viewpoint
struct Observation {
    Observation();                          // default constructor (empty)
    explicit Observation(const Game* game); // constructor from game state

    // Data
    bool game_over = false;
    bool won = false;
    TurnData turn;
    ActionSpaceData action_space;

    // Each map is keyed by its ID, as in the Python code.
    std::map<int, PlayerData> players;
    std::map<int, CardData> cards;
    std::map<int, PermanentData> permanents;

    // Reads
    bool validate() const;      // Basic consistency checks
    std::string toJSON() const; // Convert to a JSON-like string

private:
    // Writes
    void populateTurn(const Game* game);
    void populateActionSpace(const Game* game);
    void populatePlayers(const Game* game);
    void populateAllObjects(const Game* game);

    void addCard(const Card* card, ZoneType zone);
    void addPermanent(const Permanent* perm);
};
