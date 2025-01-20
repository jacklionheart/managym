// managym/agent/pybind.h
#pragma once

#include "managym/agent/observation.h"
#include "managym/state/zones.h"

// Mirror structs for Python bindings

struct PyCardState {
    PyCardState(const Observation::CardState& internal);

    int card_id;
    std::optional<int> instance_id;
    std::string name;
    bool tapped;
    bool summoning_sick;
    int damage;
    bool attacking;
    bool blocking;
};

struct PyPlayerState {
    PyPlayerState(const Observation::PlayerState& internal);

    int life_total;
    int library_size;
    Mana mana_pool;
    std::map<ZoneType, std::vector<PyCardState>> zones;
};

struct PyObservation {
    PyObservation(const Observation& internal);
    std::string toJSON() const;

    int turn_number;
    int active_player_index;
    bool is_game_over;
    ActionType action_type;
    int winner_index;
    std::vector<PyPlayerState> player_states;
};

struct PyActionSpace {
    explicit PyActionSpace(const ActionSpace* internal);
    ActionType getActionType() const;
    const std::vector<int>& getValidActions() const;
    std::string toString() const;

private:
    static std::vector<int> makeValidActions(size_t n);

    ActionType action_type;
    std::vector<int> valid_actions;
};