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

// ------------------- Turn data ----------------------
struct TurnData {
    int turn_number;
    PhaseType phase;
    StepType step;
    // Player whose turn it is (may not have priority)
    int active_player_id;
    // Player who has priority (may not be active player)
    int agent_player_id;
};

// ------------------- Player data --------------------
struct PlayerData {
    // Stable index in [0, num_players) assigned at game start
    int player_index;
    int id;         // Unique ID for references
    bool is_agent;  // Whether this player has priority
    bool is_active; // Whether it's this player's turn
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
    std::string name;
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
    int card_id;
    int controller_id;
    bool tapped = false;
    int damage = 0;
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
    Observation();                    // default constructor (empty)
    explicit Observation(Game* game); // constructor from game state

    // Data - Global State
    bool game_over = false;
    bool won = false; // True if observing player won
    TurnData turn;
    ActionSpaceData action_space;

    // Agent (observing player) data
    PlayerData agent;
    std::vector<CardData> agent_cards; // Objects owned by agent
    std::vector<PermanentData> agent_permanents;

    // Opponent data (visible portion)
    PlayerData opponent;
    std::vector<CardData> opponent_cards; // Visible opponent objects
    std::vector<PermanentData> opponent_permanents;

    // Reads
    bool validate() const;      // Basic consistency checks
    std::string toJSON() const; // Convert to a JSON-like string

private:
    // Writes
    void populateTurn(Game* game);
    void populateActionSpace(Game* game);
    void populatePlayers(Game* game);
    void populateCards(Game* game);
    void populatePermanents(Game* game);

    // Helper for populating card data into agent/opponent sections
    void addCard(const Card* card, ZoneType zone);

    // Helper for populating permanent data into agent/opponent sections
    void addPermanent(const Permanent* perm);
};