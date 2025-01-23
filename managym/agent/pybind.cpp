#include "managym/agent/action.h"
#include "managym/agent/env.h"
#include "managym/agent/observation.h"

#include "managym/flow/game.h"

#include <pybind11/chrono.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// Forward declarations
static void registerExceptions(py::module& m);
static void registerEnums(py::module& m);
static void registerDataClasses(py::module& m);
static void registerAPI(py::module& m);

PYBIND11_MODULE(_managym, m) {
    m.doc() = R"docstring(
        _managym (import as `import managym`)

        Python bindings for the managym C++ library. Provides:
          - Env: the main environment class (reset, step)
          - PlayerConfig: to specify each player's name/deck
          - Observation: top-level game state snapshots
          - Enums: ZoneEnum, PhaseEnum, StepEnum, ActionEnum, ActionSpaceEnum
          - Data structs: Player, Card, Permanent, etc.

        Example usage:
            env = managym.Env()
            obs, info = env.reset([
                managym.PlayerConfig("Alice", {"Mountain": 40}),
                managym.PlayerConfig("Bob", {"Forest": 40})
            ])
            # ...
    )docstring";

    registerExceptions(m);
    registerEnums(m);
    registerDataClasses(m);
    registerAPI(m);
}

static void registerExceptions(py::module& m) {
    py::register_exception<AgentError>(m, "AgentError", PyExc_RuntimeError);
}

static void registerEnums(py::module& m) {
    // ----------------- ZoneEnum -----------------
    py::enum_<ZoneType>(m, "ZoneEnum", R"docstring(
          LIBRARY: 0
          HAND: 1
          BATTLEFIELD: 2
          GRAVEYARD: 3
          EXILE: 4
          STACK: 5
          COMMAND: 6
    )docstring")
        .value("LIBRARY", ZoneType::LIBRARY)
        .value("HAND", ZoneType::HAND)
        .value("BATTLEFIELD", ZoneType::BATTLEFIELD)
        .value("GRAVEYARD", ZoneType::GRAVEYARD)
        .value("EXILE", ZoneType::EXILE)
        .value("STACK", ZoneType::STACK)
        .value("COMMAND", ZoneType::COMMAND)
        .export_values();

    // ----------------- PhaseEnum -----------------
    py::enum_<PhaseType>(m, "PhaseEnum", R"docstring(
          BEGINNING: 0
          PRECOMBAT_MAIN: 1
          COMBAT: 2
          POSTCOMBAT_MAIN: 3
          ENDING: 4
    )docstring")
        .value("BEGINNING", PhaseType::BEGINNING)
        .value("PRECOMBAT_MAIN", PhaseType::PRECOMBAT_MAIN)
        .value("COMBAT", PhaseType::COMBAT)
        .value("POSTCOMBAT_MAIN", PhaseType::POSTCOMBAT_MAIN)
        .value("ENDING", PhaseType::ENDING)
        .export_values();

    // ----------------- StepEnum -----------------
    py::enum_<StepType>(m, "StepEnum", R"docstring(
          BEGINNING_UNTAP: 0
          BEGINNING_UPKEEP: 1
          BEGINNING_DRAW: 2
          PRECOMBAT_MAIN_STEP: 3
          COMBAT_BEGIN: 4
          COMBAT_DECLARE_ATTACKERS: 5
          COMBAT_DECLARE_BLOCKERS: 6
          COMBAT_DAMAGE: 7
          COMBAT_END: 8
          POSTCOMBAT_MAIN_STEP: 9
          ENDING_END: 10
          ENDING_CLEANUP: 11
    )docstring")
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

    // ----------------- ActionEnum -----------------
    py::enum_<ActionType>(m, "ActionEnum", R"docstring(
          PRIORITY_PLAY_LAND: 0
          PRIORITY_CAST_SPELL: 1
          PRIORITY_PASS_PRIORITY: 2
          DECLARE_ATTACKER: 3
          DECLARE_BLOCKER: 4
    )docstring")
        .value("PRIORITY_PLAY_LAND", ActionType::PRIORITY_PLAY_LAND)
        .value("PRIORITY_CAST_SPELL", ActionType::PRIORITY_CAST_SPELL)
        .value("PRIORITY_PASS_PRIORITY", ActionType::PRIORITY_PASS_PRIORITY)
        .value("DECLARE_ATTACKER", ActionType::DECLARE_ATTACKER)
        .value("DECLARE_BLOCKER", ActionType::DECLARE_BLOCKER)
        .export_values();

    // ----------------- ActionSpaceEnum -----------------
    py::enum_<ActionSpaceType>(m, "ActionSpaceEnum", R"docstring(
          GAME_OVER: 0
          PRIORITY: 1
          DECLARE_ATTACKER: 2
          DECLARE_BLOCKER: 3
    )docstring")
        .value("GAME_OVER", ActionSpaceType::GAME_OVER)
        .value("PRIORITY", ActionSpaceType::PRIORITY)
        .value("DECLARE_ATTACKER", ActionSpaceType::DECLARE_ATTACKER)
        .value("DECLARE_BLOCKER", ActionSpaceType::DECLARE_BLOCKER)
        .export_values();
}

static void registerDataClasses(py::module& m) {
    // ----------------- PlayerConfig -----------------
    py::class_<PlayerConfig>(m, "PlayerConfig", R"docstring(
        Configuration for a player, e.g. name and decklist.
    )docstring")
        .def(py::init<std::string, const std::map<std::string, int>&>(), py::arg("name"),
             py::arg("decklist"),
             R"docstring(
                Create a PlayerConfig.

                Args:
                    name (str): The player's name
                    decklist (dict): card_name -> quantity
             )docstring");

    // ----------------- Player -----------------
    py::class_<PlayerData>(m, "Player", R"docstring(
        id: int
        player_index: int
        is_agent: bool
        is_active: bool
        life: int
        zone_counts: list[int]  # 7-element array in C++
    )docstring")
        .def(py::init<>())
        .def_readwrite("id", &PlayerData::id)
        .def_readwrite("player_index", &PlayerData::player_index)
        .def_readwrite("is_agent", &PlayerData::is_agent)
        .def_readwrite("is_active", &PlayerData::is_active)
        .def_readwrite("life", &PlayerData::life)
        .def_readwrite("zone_counts", &PlayerData::zone_counts);

    // ----------------- Turn -----------------
    py::class_<TurnData>(m, "Turn", R"docstring(
        turn_number: int
        phase: PhaseEnum
        step: StepEnum
        active_player_id: int
        agent_player_id: int
    )docstring")
        .def(py::init<>())
        .def_readwrite("turn_number", &TurnData::turn_number)
        .def_readwrite("phase", &TurnData::phase)
        .def_readwrite("step", &TurnData::step)
        .def_readwrite("active_player_id", &TurnData::active_player_id)
        .def_readwrite("agent_player_id", &TurnData::agent_player_id);

    // ----------------- ManaCost -----------------
    py::class_<ManaCost>(m, "ManaCost", R"docstring(
        cost: list[int]  # typically length 6
        mana_value: int
    )docstring")
        .def(py::init<>())
        .def_readwrite("cost", &ManaCost::cost)
        .def_readwrite("mana_value", &ManaCost::mana_value);

    // ----------------- CardTypes (Flags) -----------------
    py::class_<CardTypeData>(m, "CardTypes", R"docstring(
        is_castable: bool
        is_permanent: bool
        is_non_land_permanent: bool
        is_non_creature_permanent: bool
        is_spell: bool
        is_creature: bool
        is_land: bool
        is_planeswalker: bool
        is_enchantment: bool
        is_artifact: bool
        is_kindred: bool
        is_battle: bool
    )docstring")
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
        .def_readwrite("is_kindred", &CardTypeData::is_kindred)
        .def_readwrite("is_battle", &CardTypeData::is_battle);

    // ----------------- Card -----------------
    py::class_<CardData>(m, "Card", R"docstring(
        zone: ZoneEnum
        owner_id: int
        id: int
        registry_key: int
        power: int
        toughness: int
        card_types: CardTypes
        mana_cost: ManaCost
    )docstring")
        .def(py::init<>())
        .def_readwrite("zone", &CardData::zone)
        .def_readwrite("owner_id", &CardData::owner_id)
        .def_readwrite("id", &CardData::id)
        .def_readwrite("registry_key", &CardData::registry_key)
        .def_readwrite("power", &CardData::power)
        .def_readwrite("toughness", &CardData::toughness)
        .def_readwrite("card_types", &CardData::card_types)
        .def_readwrite("mana_cost", &CardData::mana_cost);

    // ----------------- Permanent -----------------
    py::class_<PermanentData>(m, "Permanent", R"docstring(
        id: int
        controller_id: int
        tapped: bool
        damage: int
        is_creature: bool
        is_land: bool
        is_summoning_sick: bool
    )docstring")
        .def(py::init<>())
        .def_readwrite("id", &PermanentData::id)
        .def_readwrite("controller_id", &PermanentData::controller_id)
        .def_readwrite("tapped", &PermanentData::tapped)
        .def_readwrite("damage", &PermanentData::damage)
        .def_readwrite("is_creature", &PermanentData::is_creature)
        .def_readwrite("is_land", &PermanentData::is_land)
        .def_readwrite("is_summoning_sick", &PermanentData::is_summoning_sick);

    // ----------------- Action -----------------
    py::class_<ActionOption>(m, "Action", R"docstring(
        action_type: ActionEnum
        focus: list[int]
    )docstring")
        .def(py::init<>())
        .def_readwrite("action_type", &ActionOption::action_type)
        .def_readwrite("focus", &ActionOption::focus);

    // ----------------- ActionSpace -----------------
    py::class_<ActionSpaceData>(m, "ActionSpace", R"docstring(
        action_space_type: ActionSpaceEnum
        actions: list[Action]
        focus: list[int]
    )docstring")
        .def(py::init<>())
        .def_readwrite("action_space_type", &ActionSpaceData::action_space_type)
        .def_readwrite("actions", &ActionSpaceData::actions)
        .def_readwrite("focus", &ActionSpaceData::focus);

    // ----------------- Observation -----------------
    py::class_<Observation>(m, "Observation", R"docstring(
        game_over: bool
        won: bool
        turn: Turn
        action_space: ActionSpace
        players: dict[int, Player]
        cards: dict[int, Card]
        permanents: dict[int, Permanent]
    )docstring")
        .def(py::init<>())
        .def_readwrite("game_over", &Observation::game_over)
        .def_readwrite("won", &Observation::won)
        .def_readwrite("turn", &Observation::turn)
        .def_readwrite("action_space", &Observation::action_space)
        .def_readwrite("players", &Observation::players)
        .def_readwrite("cards", &Observation::cards)
        .def_readwrite("permanents", &Observation::permanents)
        .def("validate", &Observation::validate,
             R"docstring(Perform basic consistency checks on the observation.)docstring")
        .def("toJSON", &Observation::toJSON,
             R"docstring(Generate a JSON-like string representation of the observation.)docstring");
}

static void registerAPI(py::module& m) {
    py::class_<Env>(m, "Env", R"docstring(
        Main environment class. Exposes reset() and step() methods
        returning Observations, along with rewards, etc.

        Constructor:
            Env(skip_trivial: bool = False)
    )docstring")
        .def(py::init<bool>(), py::arg("skip_trivial") = false)
        .def(
            "reset",
            [](Env& env, const std::vector<PlayerConfig>& configs) {
                auto [obs_ptr, info_map] = env.reset(configs);
                py::dict info;
                for (auto& kv : info_map) {
                    info[py::str(kv.first)] = py::str(kv.second);
                }
                return py::make_tuple(obs_ptr, info);
            },
            py::arg("player_configs"),
            R"docstring(
                Reset the environment with the given player configs.

                Returns:
                    (Observation, dict) for the new state + extra info.
             )docstring")
        .def(
            "step",
            [](Env& env, int action) {
                auto [obs_ptr, reward, terminated, truncated, info_map] = env.step(action);
                py::dict info;
                for (auto& kv : info_map) {
                    info[py::str(kv.first)] = py::str(kv.second);
                }
                return py::make_tuple(obs_ptr, reward, terminated, truncated, info);
            },
            py::arg("action"),
            R"docstring(
                Advance the environment by applying the given action index.

                Returns:
                    (Observation, float reward, bool terminated, bool truncated, dict info).
             )docstring");
}
