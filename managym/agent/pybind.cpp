#include "pybind.h"

#include "managym/agent/action.h"
#include "managym/agent/env.h"
#include "managym/agent/observation.h"
#include "managym/flow/game.h"

#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

// For more convenience with array-like fields:
#include <pybind11/chrono.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

namespace py = pybind11;

// Registration functions
static void registerExceptions(py::module& m);
static void registerEnums(py::module& m);
static void registerDataClasses(py::module& m);
static void registerAPI(py::module& m);

// Register C++ enumerations to Python
static void registerEnums(py::module& m) {
    py::enum_<ZoneType>(m, "ZoneType")
        .value("LIBRARY", ZoneType::LIBRARY)
        .value("HAND", ZoneType::HAND)
        .value("BATTLEFIELD", ZoneType::BATTLEFIELD)
        .value("GRAVEYARD", ZoneType::GRAVEYARD)
        .value("EXILE", ZoneType::EXILE)
        .value("STACK", ZoneType::STACK)
        .value("COMMAND", ZoneType::COMMAND)
        .export_values();

    py::enum_<PhaseType>(m, "PhaseType")
        .value("BEGINNING", PhaseType::BEGINNING)
        .value("PRECOMBAT_MAIN", PhaseType::PRECOMBAT_MAIN)
        .value("COMBAT", PhaseType::COMBAT)
        .value("POSTCOMBAT_MAIN", PhaseType::POSTCOMBAT_MAIN)
        .value("ENDING", PhaseType::ENDING)
        .export_values();

    py::enum_<StepType>(m, "StepType")
        .value("BEGINNING_UNTAP", StepType::BEGINNING_UNTAP)
        .value("BEGINNING_UPKEEP", StepType::BEGINNING_UPKEEP)
        .value("BEGINNING_DRAW", StepType::BEGINNING_DRAW)
        .value("PRECOMBAT_MAIN_STEP", StepType::PRECOMBAT_MAIN_STEP)
        .value("COMBAT_BEGIN", StepType::COMBAT_BEGIN)
        .value("COMBAT_DECLARE_ATTACKERS", StepType::COMBAT_DECLARE_ATTACKERS)
        .value("COMBAT_DECLARE_BLOCKERS", StepType::COMBAT_DECLARE_BLOCKERS)
        .value("COMBAT_DAMAGE", StepType::COMBAT_DAMAGE)
        .value("COMBAT_END", StepType::COMBAT_END)
        .value("POSTCOMBAT_MAIN_STEP", StepType::POSTCOMBAT_MAIN_STEP)
        .value("ENDING_END", StepType::ENDING_END)
        .value("ENDING_CLEANUP", StepType::ENDING_CLEANUP)
        .export_values();

    py::enum_<ActionType>(m, "ActionType")
        .value("PRIORITY_PLAY_LAND", ActionType::PRIORITY_PLAY_LAND)
        .value("PRIORITY_CAST_SPELL", ActionType::PRIORITY_CAST_SPELL)
        .value("PRIORITY_PASS_PRIORITY", ActionType::PRIORITY_PASS_PRIORITY)
        .value("DECLARE_ATTACKER", ActionType::DECLARE_ATTACKER)
        .value("DECLARE_BLOCKER", ActionType::DECLARE_BLOCKER)
        .export_values();

    py::enum_<ActionSpaceType>(m, "ActionSpaceType")
        .value("GAME_OVER", ActionSpaceType::GAME_OVER)
        .value("PRIORITY", ActionSpaceType::PRIORITY)
        .value("DECLARE_ATTACKER", ActionSpaceType::DECLARE_ATTACKER)
        .value("DECLARE_BLOCKER", ActionSpaceType::DECLARE_BLOCKER)
        .export_values();
}

// Register the Observation object and its sub-objects, along with configuration input objects.
static void registerDataClasses(py::module& m) {
    py::class_<PlayerConfig>(m, "PlayerConfig")
        .def(py::init<std::string, const std::map<std::string, int>&>(), py::arg("name"),
             py::arg("decklist"));

    py::class_<PlayerData>(m, "Player")
        .def(py::init<>())
        .def_readwrite("id", &PlayerData::id)
        .def_readwrite("player_index", &PlayerData::player_index)
        .def_readwrite("is_active", &PlayerData::is_active)
        .def_readwrite("is_agent", &PlayerData::is_agent)
        .def_readwrite("life", &PlayerData::life)
        .def_readwrite("zone_counts", &PlayerData::zone_counts);

    // TurnData
    py::class_<TurnData>(m, "Turn")
        .def(py::init<>())
        .def_readwrite("turn_number", &TurnData::turn_number)
        .def_readwrite("phase", &TurnData::phase)
        .def_readwrite("step", &TurnData::step)
        .def_readwrite("active_player_id", &TurnData::active_player_id)
        .def_readwrite("agent_player_id", &TurnData::agent_player_id);

    py::class_<ManaCost>(m, "ManaCost")
        .def(py::init<>())
        .def_readwrite("cost", &ManaCost::cost)
        .def_readwrite("mana_value", &ManaCost::mana_value);

    py::class_<CardTypeData>(m, "CardType")
        .def(py::init<>())
        .def_readwrite("is_castable", &CardTypeData::is_castable)
        .def_readwrite("is_permanent", &CardTypeData::is_permanent)
        .def_readwrite("is_non_land_permanent", &CardTypeData::is_non_land_permanent)
        .def_readwrite("is_non_creature_permanent", &CardTypeData::is_non_creature_permanent)
        .def_readwrite("is_spell", &CardTypeData::is_spell)
        .def_readwrite("is_creature", &CardTypeData::is_creature)
        .def_readwrite("is_land", &CardTypeData::is_land)
        .def_readwrite("is_planeswalker", &CardTypeData::is_planeswalker)
        .def_readwrite("is_enchantment", &CardTypeData::is_enchantment)
        .def_readwrite("is_artifact", &CardTypeData::is_artifact)
        .def_readwrite("is_kindred", &CardTypeData::is_kindred);

    py::class_<CardData>(m, "Card")
        .def(py::init<>())
        .def_readwrite("zone", &CardData::zone)
        .def_readwrite("owner_id", &CardData::owner_id)
        .def_readwrite("id", &CardData::id)
        .def_readwrite("registry_key", &CardData::registry_key)
        .def_readwrite("power", &CardData::power)
        .def_readwrite("toughness", &CardData::toughness)
        .def_readwrite("card_type", &CardData::card_type)
        .def_readwrite("mana_cost", &CardData::mana_cost);

    py::class_<PermanentData>(m, "Permanent")
        .def(py::init<>())
        .def_readwrite("id", &PermanentData::id)
        .def_readwrite("controller_id", &PermanentData::controller_id)
        .def_readwrite("tapped", &PermanentData::tapped)
        .def_readwrite("damage", &PermanentData::damage)
        .def_readwrite("is_creature", &PermanentData::is_creature)
        .def_readwrite("is_land", &PermanentData::is_land)
        .def_readwrite("is_summoning_sick", &PermanentData::is_summoning_sick);

    py::class_<ActionOption>(m, "Action")
        .def(py::init<>())
        .def_readwrite("action_type", &ActionOption::action_type)
        .def_readwrite("focus", &ActionOption::focus);

    py::class_<ActionSpaceData>(m, "ActionSpace")
        .def(py::init<>())
        .def_readwrite("action_space_type", &ActionSpaceData::action_space_type)
        .def_readwrite("actions", &ActionSpaceData::actions)
        .def_readwrite("focus", &ActionSpaceData::focus);

    py::class_<Observation>(m, "Observation")
        .def(py::init<>())
        .def(py::init<const Game*>())
        .def_readwrite("game_over", &Observation::game_over)
        .def_readwrite("won", &Observation::won)
        .def_readwrite("turn", &Observation::turn)
        .def_readwrite("action_space", &Observation::action_space)
        .def_readwrite("players", &Observation::players)
        .def_readwrite("cards", &Observation::cards)
        .def_readwrite("permanents", &Observation::permanents)
        .def("validate", &Observation::validate)
        .def("toJSON", &Observation::toJSON);
}

// The primary API for managym is the Env api, which has two methods:
// reset: called once to start a new game
// step:
//
// The API is meant to mostly follow the conventions of `gymnasium`. See:
// https://gymnasium.farama.org/api/env/
static void registerAPI(py::module& m) {
    // Env
    py::class_<Env>(m, "Env")
        .def(py::init<bool>(), py::arg("skip_trivial") = false)
        .def(
            "reset",
            [](Env& env, const std::vector<PlayerConfig>& configs) {
                auto [obs_ptr, info_map] = env.reset(configs);

                // Convert std::map<std::string,std::string> to Python dict
                py::dict info;
                for (auto& kv : info_map) {
                    info[py::str(kv.first)] = py::str(kv.second);
                }
                // Return (Observation*, info)
                return py::make_tuple(obs_ptr, info);
            },
            py::arg("player_configs"), "Reset the environment returning (observation, info).")
        .def(
            "step",
            [](Env& env, int action) {
                auto [obs_ptr, reward, terminated, truncated, info_map] = env.step(action);

                py::dict info;
                for (auto& kv : info_map) {
                    info[py::str(kv.first)] = py::str(kv.second);
                }
                // Return (observation, reward, terminated, truncated, info)
                return py::make_tuple(obs_ptr, reward, terminated, truncated, info);
            },
            py::arg("action"),
            "Run one environment step returning (observation, reward, terminated, truncated, "
            "info).");
}

// Register exceptions that can be thrown by managym to manabot.
static void registerExceptions(py::module& m) {
    // Register AgentError exception
    py::exception<AgentError> exc(m, "AgentError");
}

PYBIND11_MODULE(managym_py, m) {
    m.doc() = "Python bindings for managym. See: https://github.com/jacklionheart/managym";

    registerExceptions(m);
    registerEnums(m);
    registerDataClasses(m);
    registerAPI(m);

    // Simple function to test exception throwing
    m.def("throw_action_error", []() { throw AgentError("Test in python bindings"); });
}

// (Keep other registration functions as they are...)