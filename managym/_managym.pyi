"""

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
    
"""
from __future__ import annotations
import pybind11_stubgen.typing_ext
import typing
__all__ = ['Action', 'ActionEnum', 'ActionSpace', 'ActionSpaceEnum', 'AgentError', 'BATTLEFIELD', 'BEGINNING', 'BEGINNING_DRAW', 'BEGINNING_UNTAP', 'BEGINNING_UPKEEP', 'COMBAT', 'COMBAT_BEGIN', 'COMBAT_DAMAGE', 'COMBAT_DECLARE_ATTACKERS', 'COMBAT_DECLARE_BLOCKERS', 'COMBAT_END', 'COMMAND', 'Card', 'CardTypes', 'DECLARE_ATTACKER', 'DECLARE_BLOCKER', 'ENDING', 'ENDING_CLEANUP', 'ENDING_END', 'EXILE', 'Env', 'GAME_OVER', 'GRAVEYARD', 'HAND', 'LIBRARY', 'ManaCost', 'Observation', 'POSTCOMBAT_MAIN', 'POSTCOMBAT_MAIN_STEP', 'PRECOMBAT_MAIN', 'PRECOMBAT_MAIN_STEP', 'PRIORITY', 'PRIORITY_CAST_SPELL', 'PRIORITY_PASS_PRIORITY', 'PRIORITY_PLAY_LAND', 'Permanent', 'PhaseEnum', 'Player', 'PlayerConfig', 'STACK', 'StepEnum', 'Turn', 'ZoneEnum']
class Action:
    """
    
            action_type: ActionEnum
            focus: list[int]
        
    """
    action_type: ActionEnum
    focus: list[int]
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class ActionEnum:
    """
    
              PRIORITY_PLAY_LAND: 0
              PRIORITY_CAST_SPELL: 1
              PRIORITY_PASS_PRIORITY: 2
              DECLARE_ATTACKER: 3
              DECLARE_BLOCKER: 4
        
    
    Members:
    
      PRIORITY_PLAY_LAND
    
      PRIORITY_CAST_SPELL
    
      PRIORITY_PASS_PRIORITY
    
      DECLARE_ATTACKER
    
      DECLARE_BLOCKER
    """
    DECLARE_ATTACKER: typing.ClassVar[ActionEnum]  # value = <ActionEnum.DECLARE_ATTACKER: 3>
    DECLARE_BLOCKER: typing.ClassVar[ActionEnum]  # value = <ActionEnum.DECLARE_BLOCKER: 4>
    PRIORITY_CAST_SPELL: typing.ClassVar[ActionEnum]  # value = <ActionEnum.PRIORITY_CAST_SPELL: 1>
    PRIORITY_PASS_PRIORITY: typing.ClassVar[ActionEnum]  # value = <ActionEnum.PRIORITY_PASS_PRIORITY: 2>
    PRIORITY_PLAY_LAND: typing.ClassVar[ActionEnum]  # value = <ActionEnum.PRIORITY_PLAY_LAND: 0>
    __members__: typing.ClassVar[dict[str, ActionEnum]]  # value = {'PRIORITY_PLAY_LAND': <ActionEnum.PRIORITY_PLAY_LAND: 0>, 'PRIORITY_CAST_SPELL': <ActionEnum.PRIORITY_CAST_SPELL: 1>, 'PRIORITY_PASS_PRIORITY': <ActionEnum.PRIORITY_PASS_PRIORITY: 2>, 'DECLARE_ATTACKER': <ActionEnum.DECLARE_ATTACKER: 3>, 'DECLARE_BLOCKER': <ActionEnum.DECLARE_BLOCKER: 4>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class ActionSpace:
    """
    
            action_space_type: ActionSpaceEnum
            actions: list[Action]
            focus: list[int]
        
    """
    action_space_type: ActionSpaceEnum
    actions: list[Action]
    focus: list[int]
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class ActionSpaceEnum:
    """
    
              GAME_OVER: 0
              PRIORITY: 1
              DECLARE_ATTACKER: 2
              DECLARE_BLOCKER: 3
        
    
    Members:
    
      GAME_OVER
    
      PRIORITY
    
      DECLARE_ATTACKER
    
      DECLARE_BLOCKER
    """
    DECLARE_ATTACKER: typing.ClassVar[ActionSpaceEnum]  # value = <ActionSpaceEnum.DECLARE_ATTACKER: 2>
    DECLARE_BLOCKER: typing.ClassVar[ActionSpaceEnum]  # value = <ActionSpaceEnum.DECLARE_BLOCKER: 3>
    GAME_OVER: typing.ClassVar[ActionSpaceEnum]  # value = <ActionSpaceEnum.GAME_OVER: 0>
    PRIORITY: typing.ClassVar[ActionSpaceEnum]  # value = <ActionSpaceEnum.PRIORITY: 1>
    __members__: typing.ClassVar[dict[str, ActionSpaceEnum]]  # value = {'GAME_OVER': <ActionSpaceEnum.GAME_OVER: 0>, 'PRIORITY': <ActionSpaceEnum.PRIORITY: 1>, 'DECLARE_ATTACKER': <ActionSpaceEnum.DECLARE_ATTACKER: 2>, 'DECLARE_BLOCKER': <ActionSpaceEnum.DECLARE_BLOCKER: 3>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class AgentError(RuntimeError):
    pass
class Card:
    """
    
            zone: ZoneEnum
            owner_id: int
            id: int
            registry_key: int
            power: int
            toughness: int
            card_types: CardTypes
            mana_cost: ManaCost
        
    """
    card_types: CardTypes
    id: int
    mana_cost: ManaCost
    owner_id: int
    power: int
    registry_key: int
    toughness: int
    zone: ZoneEnum
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class CardTypes:
    """
    
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
        
    """
    is_artifact: bool
    is_battle: bool
    is_castable: bool
    is_creature: bool
    is_enchantment: bool
    is_kindred: bool
    is_land: bool
    is_non_creature_permanent: bool
    is_non_land_permanent: bool
    is_permanent: bool
    is_planeswalker: bool
    is_spell: bool
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class Env:
    """
    
            Main environment class. Exposes reset() and step() methods
            returning Observations, along with rewards, etc.
    
            Constructor:
                Env(skip_trivial: bool = False)
        
    """
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self, skip_trivial: bool = False) -> None:
        ...
    def reset(self, player_configs: list[PlayerConfig]) -> tuple:
        """
                        Reset the environment with the given player configs.
        
                        Returns:
                            (Observation, dict) for the new state + extra info.
        """
    def step(self, action: int) -> tuple:
        """
                        Advance the environment by applying the given action index.
        
                        Returns:
                            (Observation, float reward, bool terminated, bool truncated, dict info).
        """
class ManaCost:
    """
    
            cost: list[int]  # typically length 6
            mana_value: int
        
    """
    cost: typing.Annotated[list[int], pybind11_stubgen.typing_ext.FixedSize(6)]
    mana_value: int
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class Observation:
    """
    
            game_over: bool
            won: bool
            turn: Turn
            action_space: ActionSpace
            players: dict[int, Player]
            cards: dict[int, Card]
            permanents: dict[int, Permanent]
        
    """
    action_space: ActionSpace
    cards: dict[int, Card]
    game_over: bool
    permanents: dict[int, Permanent]
    players: dict[int, Player]
    turn: Turn
    won: bool
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    def toJSON(self) -> str:
        """
        Generate a JSON-like string representation of the observation.
        """
    def validate(self) -> bool:
        """
        Perform basic consistency checks on the observation.
        """
class Permanent:
    """
    
            id: int
            controller_id: int
            tapped: bool
            damage: int
            is_creature: bool
            is_land: bool
            is_summoning_sick: bool
        
    """
    controller_id: int
    damage: int
    id: int
    is_creature: bool
    is_land: bool
    is_summoning_sick: bool
    tapped: bool
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class PhaseEnum:
    """
    
              BEGINNING: 0
              PRECOMBAT_MAIN: 1
              COMBAT: 2
              POSTCOMBAT_MAIN: 3
              ENDING: 4
        
    
    Members:
    
      BEGINNING
    
      PRECOMBAT_MAIN
    
      COMBAT
    
      POSTCOMBAT_MAIN
    
      ENDING
    """
    BEGINNING: typing.ClassVar[PhaseEnum]  # value = <PhaseEnum.BEGINNING: 0>
    COMBAT: typing.ClassVar[PhaseEnum]  # value = <PhaseEnum.COMBAT: 2>
    ENDING: typing.ClassVar[PhaseEnum]  # value = <PhaseEnum.ENDING: 4>
    POSTCOMBAT_MAIN: typing.ClassVar[PhaseEnum]  # value = <PhaseEnum.POSTCOMBAT_MAIN: 3>
    PRECOMBAT_MAIN: typing.ClassVar[PhaseEnum]  # value = <PhaseEnum.PRECOMBAT_MAIN: 1>
    __members__: typing.ClassVar[dict[str, PhaseEnum]]  # value = {'BEGINNING': <PhaseEnum.BEGINNING: 0>, 'PRECOMBAT_MAIN': <PhaseEnum.PRECOMBAT_MAIN: 1>, 'COMBAT': <PhaseEnum.COMBAT: 2>, 'POSTCOMBAT_MAIN': <PhaseEnum.POSTCOMBAT_MAIN: 3>, 'ENDING': <PhaseEnum.ENDING: 4>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class Player:
    """
    
            id: int
            player_index: int
            is_agent: bool
            is_active: bool
            life: int
            zone_counts: list[int]  # 7-element array in C++
        
    """
    id: int
    is_active: bool
    is_agent: bool
    life: int
    player_index: int
    zone_counts: typing.Annotated[list[int], pybind11_stubgen.typing_ext.FixedSize(7)]
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class PlayerConfig:
    """
    
            Configuration for a player, e.g. name and decklist.
        
    """
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self, name: str, decklist: dict[str, int]) -> None:
        """
                        Create a PlayerConfig.
        
                        Args:
                            name (str): The player's name
                            decklist (dict): card_name -> quantity
        """
class StepEnum:
    """
    
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
        
    
    Members:
    
      BEGINNING_UNTAP
    
      BEGINNING_UPKEEP
    
      BEGINNING_DRAW
    
      PRECOMBAT_MAIN_STEP
    
      COMBAT_BEGIN
    
      COMBAT_DECLARE_ATTACKERS
    
      COMBAT_DECLARE_BLOCKERS
    
      COMBAT_DAMAGE
    
      COMBAT_END
    
      POSTCOMBAT_MAIN_STEP
    
      ENDING_END
    
      ENDING_CLEANUP
    """
    BEGINNING_DRAW: typing.ClassVar[StepEnum]  # value = <StepEnum.BEGINNING_DRAW: 2>
    BEGINNING_UNTAP: typing.ClassVar[StepEnum]  # value = <StepEnum.BEGINNING_UNTAP: 0>
    BEGINNING_UPKEEP: typing.ClassVar[StepEnum]  # value = <StepEnum.BEGINNING_UPKEEP: 1>
    COMBAT_BEGIN: typing.ClassVar[StepEnum]  # value = <StepEnum.COMBAT_BEGIN: 4>
    COMBAT_DAMAGE: typing.ClassVar[StepEnum]  # value = <StepEnum.COMBAT_DAMAGE: 7>
    COMBAT_DECLARE_ATTACKERS: typing.ClassVar[StepEnum]  # value = <StepEnum.COMBAT_DECLARE_ATTACKERS: 5>
    COMBAT_DECLARE_BLOCKERS: typing.ClassVar[StepEnum]  # value = <StepEnum.COMBAT_DECLARE_BLOCKERS: 6>
    COMBAT_END: typing.ClassVar[StepEnum]  # value = <StepEnum.COMBAT_END: 8>
    ENDING_CLEANUP: typing.ClassVar[StepEnum]  # value = <StepEnum.ENDING_CLEANUP: 11>
    ENDING_END: typing.ClassVar[StepEnum]  # value = <StepEnum.ENDING_END: 10>
    POSTCOMBAT_MAIN_STEP: typing.ClassVar[StepEnum]  # value = <StepEnum.POSTCOMBAT_MAIN_STEP: 9>
    PRECOMBAT_MAIN_STEP: typing.ClassVar[StepEnum]  # value = <StepEnum.PRECOMBAT_MAIN_STEP: 3>
    __members__: typing.ClassVar[dict[str, StepEnum]]  # value = {'BEGINNING_UNTAP': <StepEnum.BEGINNING_UNTAP: 0>, 'BEGINNING_UPKEEP': <StepEnum.BEGINNING_UPKEEP: 1>, 'BEGINNING_DRAW': <StepEnum.BEGINNING_DRAW: 2>, 'PRECOMBAT_MAIN_STEP': <StepEnum.PRECOMBAT_MAIN_STEP: 3>, 'COMBAT_BEGIN': <StepEnum.COMBAT_BEGIN: 4>, 'COMBAT_DECLARE_ATTACKERS': <StepEnum.COMBAT_DECLARE_ATTACKERS: 5>, 'COMBAT_DECLARE_BLOCKERS': <StepEnum.COMBAT_DECLARE_BLOCKERS: 6>, 'COMBAT_DAMAGE': <StepEnum.COMBAT_DAMAGE: 7>, 'COMBAT_END': <StepEnum.COMBAT_END: 8>, 'POSTCOMBAT_MAIN_STEP': <StepEnum.POSTCOMBAT_MAIN_STEP: 9>, 'ENDING_END': <StepEnum.ENDING_END: 10>, 'ENDING_CLEANUP': <StepEnum.ENDING_CLEANUP: 11>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class Turn:
    """
    
            turn_number: int
            phase: PhaseEnum
            step: StepEnum
            active_player_id: int
            agent_player_id: int
        
    """
    active_player_id: int
    agent_player_id: int
    phase: PhaseEnum
    step: StepEnum
    turn_number: int
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class ZoneEnum:
    """
    
              LIBRARY: 0
              HAND: 1
              BATTLEFIELD: 2
              GRAVEYARD: 3
              EXILE: 4
              STACK: 5
              COMMAND: 6
        
    
    Members:
    
      LIBRARY
    
      HAND
    
      BATTLEFIELD
    
      GRAVEYARD
    
      EXILE
    
      STACK
    
      COMMAND
    """
    BATTLEFIELD: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.BATTLEFIELD: 2>
    COMMAND: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.COMMAND: 6>
    EXILE: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.EXILE: 5>
    GRAVEYARD: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.GRAVEYARD: 3>
    HAND: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.HAND: 1>
    LIBRARY: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.LIBRARY: 0>
    STACK: typing.ClassVar[ZoneEnum]  # value = <ZoneEnum.STACK: 4>
    __members__: typing.ClassVar[dict[str, ZoneEnum]]  # value = {'LIBRARY': <ZoneEnum.LIBRARY: 0>, 'HAND': <ZoneEnum.HAND: 1>, 'BATTLEFIELD': <ZoneEnum.BATTLEFIELD: 2>, 'GRAVEYARD': <ZoneEnum.GRAVEYARD: 3>, 'EXILE': <ZoneEnum.EXILE: 5>, 'STACK': <ZoneEnum.STACK: 4>, 'COMMAND': <ZoneEnum.COMMAND: 6>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
BATTLEFIELD: ZoneEnum  # value = <ZoneEnum.BATTLEFIELD: 2>
BEGINNING: PhaseEnum  # value = <PhaseEnum.BEGINNING: 0>
BEGINNING_DRAW: StepEnum  # value = <StepEnum.BEGINNING_DRAW: 2>
BEGINNING_UNTAP: StepEnum  # value = <StepEnum.BEGINNING_UNTAP: 0>
BEGINNING_UPKEEP: StepEnum  # value = <StepEnum.BEGINNING_UPKEEP: 1>
COMBAT: PhaseEnum  # value = <PhaseEnum.COMBAT: 2>
COMBAT_BEGIN: StepEnum  # value = <StepEnum.COMBAT_BEGIN: 4>
COMBAT_DAMAGE: StepEnum  # value = <StepEnum.COMBAT_DAMAGE: 7>
COMBAT_DECLARE_ATTACKERS: StepEnum  # value = <StepEnum.COMBAT_DECLARE_ATTACKERS: 5>
COMBAT_DECLARE_BLOCKERS: StepEnum  # value = <StepEnum.COMBAT_DECLARE_BLOCKERS: 6>
COMBAT_END: StepEnum  # value = <StepEnum.COMBAT_END: 8>
COMMAND: ZoneEnum  # value = <ZoneEnum.COMMAND: 6>
DECLARE_ATTACKER: ActionSpaceEnum  # value = <ActionSpaceEnum.DECLARE_ATTACKER: 2>
DECLARE_BLOCKER: ActionSpaceEnum  # value = <ActionSpaceEnum.DECLARE_BLOCKER: 3>
ENDING: PhaseEnum  # value = <PhaseEnum.ENDING: 4>
ENDING_CLEANUP: StepEnum  # value = <StepEnum.ENDING_CLEANUP: 11>
ENDING_END: StepEnum  # value = <StepEnum.ENDING_END: 10>
EXILE: ZoneEnum  # value = <ZoneEnum.EXILE: 5>
GAME_OVER: ActionSpaceEnum  # value = <ActionSpaceEnum.GAME_OVER: 0>
GRAVEYARD: ZoneEnum  # value = <ZoneEnum.GRAVEYARD: 3>
HAND: ZoneEnum  # value = <ZoneEnum.HAND: 1>
LIBRARY: ZoneEnum  # value = <ZoneEnum.LIBRARY: 0>
POSTCOMBAT_MAIN: PhaseEnum  # value = <PhaseEnum.POSTCOMBAT_MAIN: 3>
POSTCOMBAT_MAIN_STEP: StepEnum  # value = <StepEnum.POSTCOMBAT_MAIN_STEP: 9>
PRECOMBAT_MAIN: PhaseEnum  # value = <PhaseEnum.PRECOMBAT_MAIN: 1>
PRECOMBAT_MAIN_STEP: StepEnum  # value = <StepEnum.PRECOMBAT_MAIN_STEP: 3>
PRIORITY: ActionSpaceEnum  # value = <ActionSpaceEnum.PRIORITY: 1>
PRIORITY_CAST_SPELL: ActionEnum  # value = <ActionEnum.PRIORITY_CAST_SPELL: 1>
PRIORITY_PASS_PRIORITY: ActionEnum  # value = <ActionEnum.PRIORITY_PASS_PRIORITY: 2>
PRIORITY_PLAY_LAND: ActionEnum  # value = <ActionEnum.PRIORITY_PLAY_LAND: 0>
STACK: ZoneEnum  # value = <ZoneEnum.STACK: 4>
