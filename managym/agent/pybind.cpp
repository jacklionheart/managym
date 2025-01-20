// managym/agent/pybind.cpp
#include "pybind.h"

#include "managym/agent/env.h"
#include "managym/flow/game.h"

#include <fmt/format.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PyCardState::PyCardState(const Observation::CardState& internal)
    : card_id(internal.card_id), instance_id(internal.instance_id), name(internal.name),
      tapped(internal.tapped), summoning_sick(internal.summoning_sick), damage(internal.damage),
      attacking(internal.attacking), blocking(internal.blocking) {}

PyPlayerState::PyPlayerState(const Observation::PlayerState& internal)
    : life_total(internal.life_total), library_size(internal.library_size),
      mana_pool(internal.mana_pool) {
    for (const auto& [zone_type, cards] : internal.zones) {
        std::vector<PyCardState> zone_cards;
        zone_cards.reserve(cards.size());
        for (const auto& card : cards) {
            zone_cards.emplace_back(card);
        }
        zones[zone_type] = std::move(zone_cards);
    }
}

PyObservation::PyObservation(const Observation& internal)
    : turn_number(internal.turn_number), active_player_index(internal.active_player_index),
      is_game_over(internal.is_game_over), action_type(internal.action_type),
      winner_index(internal.winner_index) {
    player_states.reserve(internal.player_states.size());
    for (const auto& state : internal.player_states) {
        player_states.emplace_back(state);
    }
}

std::string PyObservation::toJSON() const {
    return ""; // Implement if needed
}

PyActionSpace::PyActionSpace(const ActionSpace* internal)
    : action_type(internal->action_type),
      valid_actions(makeValidActions(internal->actions.size())) {}

ActionType PyActionSpace::getActionType() const { return action_type; }

const std::vector<int>& PyActionSpace::getValidActions() const { return valid_actions; }

std::string PyActionSpace::toString() const {
    return fmt::format("ActionSpace(type={}, num_actions={})", ::toString(action_type),
                       valid_actions.size());
}

std::vector<int> PyActionSpace::makeValidActions(size_t n) {
    std::vector<int> actions;
    actions.reserve(n);
    for (size_t i = 0; i < n; i++) {
        actions.push_back(i);
    }
    return actions;
}

PYBIND11_MODULE(managym_py, m) {
    m.doc() = "Python bindings for managym";

    // Register exception
    //  pybind11::register_exception<ManagymActionError>(m, "ManagymActionError");

    // Enums need to use pybind11::enum_ since they're used directly
    pybind11::enum_<ZoneType>(m, "ZoneType")
        .value("LIBRARY", ZoneType::LIBRARY)
        .value("HAND", ZoneType::HAND)
        .value("BATTLEFIELD", ZoneType::BATTLEFIELD)
        .value("GRAVEYARD", ZoneType::GRAVEYARD)
        .value("STACK", ZoneType::STACK)
        .value("EXILE", ZoneType::EXILE)
        .value("COMMAND", ZoneType::COMMAND);

    pybind11::enum_<ActionType>(m, "ActionType")
        .value("INVALID", ActionType::Invalid)
        .value("PRIORITY", ActionType::Priority)
        .value("DECLARE_ATTACKER", ActionType::DeclareAttacker)
        .value("DECLARE_BLOCKER", ActionType::DeclareBlocker);

    // Bind Python mirror types
    pybind11::class_<PyCardState>(m, "CardState")
        .def_readonly("card_id", &PyCardState::card_id)
        .def_readonly("instance_id", &PyCardState::instance_id)
        .def_readonly("name", &PyCardState::name)
        .def_readonly("tapped", &PyCardState::tapped)
        .def_readonly("summoning_sick", &PyCardState::summoning_sick)
        .def_readonly("damage", &PyCardState::damage)
        .def_readonly("attacking", &PyCardState::attacking)
        .def_readonly("blocking", &PyCardState::blocking);

    pybind11::class_<PyPlayerState>(m, "PlayerState")
        .def_readonly("life_total", &PyPlayerState::life_total)
        .def_readonly("library_size", &PyPlayerState::library_size)
        .def_readonly("mana_pool", &PyPlayerState::mana_pool)
        .def_readonly("zones", &PyPlayerState::zones);

    pybind11::class_<PyObservation>(m, "Observation")
        .def_readonly("turn_number", &PyObservation::turn_number)
        .def_readonly("active_player_index", &PyObservation::active_player_index)
        .def_readonly("is_game_over", &PyObservation::is_game_over)
        .def_readonly("winner_index", &PyObservation::winner_index)
        .def_readonly("action_type", &PyObservation::action_type)
        .def_readonly("player_states", &PyObservation::player_states)
        .def("to_json", &PyObservation::toJSON);

    pybind11::class_<PyActionSpace>(m, "ActionSpace")
        .def_property_readonly("action_type", &PyActionSpace::getActionType)
        .def_property_readonly("valid_actions", &PyActionSpace::getValidActions)
        .def("__str__", &PyActionSpace::toString);

    // Configuration - keep as direct binding since it's simple value type
    pybind11::class_<PlayerConfig>(m, "PlayerConfig")
        .def(pybind11::init<std::string, const std::map<std::string, int>&>())
        .def_readwrite("name", &PlayerConfig::name)
        .def_readwrite("deck", &PlayerConfig::decklist);

    // Bind environment and create Python mirror objects in API methods
    pybind11::class_<Env>(m, "Env")
        .def(pybind11::init<>())
        .def(
            "reset",
            [](Env& env, const std::vector<PlayerConfig>& player_configs) -> pybind11::tuple {
                auto [obs, action_space] = env.reset(player_configs);
                // Create owned Python mirror objects
                return pybind11::make_tuple(PyObservation(*obs), PyActionSpace(action_space));
            },
            pybind11::arg("player_configs"))
        .def(
            "step",
            [](Env& env, int action) -> pybind11::tuple {
                auto [obs, action_space, done] = env.step(action);
                // Create owned Python mirror objects
                return pybind11::make_tuple(PyObservation(*obs), PyActionSpace(action_space), done);
            },
            pybind11::arg("action"));
}