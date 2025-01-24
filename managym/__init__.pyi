# __init__.pyi
# Stubs for the managym module exposed by your pybind11 extension.
# This file is NOT executed at runtime; it's purely for linters and IDEs.

from __future__ import annotations

from enum import IntEnum
from typing import Dict, List, Tuple

# ---------------------------------------------------------------------
# Enums
# ---------------------------------------------------------------------
class ZoneEnum(IntEnum):
    LIBRARY: int
    HAND: int
    BATTLEFIELD: int
    GRAVEYARD: int
    EXILE: int
    STACK: int
    COMMAND: int

class PhaseEnum(IntEnum):
    BEGINNING: int
    PRECOMBAT_MAIN: int
    COMBAT: int
    POSTCOMBAT_MAIN: int
    ENDING: int

class StepEnum(IntEnum):
    BEGINNING_UNTAP: int
    BEGINNING_UPKEEP: int
    BEGINNING_DRAW: int
    PRECOMBAT_MAIN_STEP: int
    COMBAT_BEGIN: int
    COMBAT_DECLARE_ATTACKERS: int
    COMBAT_DECLARE_BLOCKERS: int
    COMBAT_DAMAGE: int
    COMBAT_END: int
    POSTCOMBAT_MAIN_STEP: int
    ENDING_END: int
    ENDING_CLEANUP: int

class ActionEnum(IntEnum):
    PRIORITY_PLAY_LAND: int
    PRIORITY_CAST_SPELL: int
    PRIORITY_PASS_PRIORITY: int
    DECLARE_ATTACKER: int
    DECLARE_BLOCKER: int

class ActionSpaceEnum(IntEnum):
    GAME_OVER: int
    PRIORITY: int
    DECLARE_ATTACKER: int
    DECLARE_BLOCKER: int

# ---------------------------------------------------------------------
# Data Classes: Player, Turn, ManaCost, etc.
# (As exposed by pybind11 .def_readwrite)
# ---------------------------------------------------------------------

class PlayerConfig:
    def __init__(self, name: str, decklist: Dict[str, int]) -> None: ...
    name: str 
    decklist: Dict[str, int]

class Player:
    id: int
    player_index: int
    is_agent: bool
    is_active: bool
    life: int
    zone_counts: List[int]

class Turn:
    turn_number: int
    phase: PhaseEnum
    step: StepEnum
    active_player_id: int
    agent_player_id: int

class ManaCost:
    cost: List[int]
    mana_value: int

class CardTypes:
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

class Card:
    zone: ZoneEnum
    owner_id: int
    id: int
    registry_key: int
    power: int
    toughness: int
    card_types: CardTypes
    mana_cost: ManaCost

class Permanent:
    id: int
    controller_id: int
    tapped: bool
    damage: int
    is_creature: bool
    is_land: bool
    is_summoning_sick: bool

class Action:
    action_type: ActionEnum
    focus: List[int]

class ActionSpace:
    action_space_type: ActionSpaceEnum
    actions: List[Action]
    focus: List[int]

class Observation:
    game_over: bool
    won: bool
    turn: Turn
    action_space: ActionSpace
    players: Dict[int, Player]
    cards: Dict[int, Card]
    permanents: Dict[int, Permanent]

    def validate(self) -> bool: ...
    def toJSON(self) -> str: ...

# ---------------------------------------------------------------------
# Environment API
# ---------------------------------------------------------------------
class Env:
    def __init__(self, skip_trivial: bool = False) -> None: ...
    def reset(self, player_configs: List[PlayerConfig]) -> Tuple[Observation, Dict[str, str]]: ...
    def step(self, action: int) -> Tuple[Observation, float, bool, bool, Dict[str, str]]: ...
    # Optionally define close(), etc., if you have them.

# ---------------------------------------------------------------------
# Exceptions
# ---------------------------------------------------------------------
class AgentError(RuntimeError):
    """Exception thrown on invalid agent actions."""
