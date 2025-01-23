from __future__ import annotations
from managym._managym import Action
from managym._managym import ActionEnum
from managym._managym import ActionSpace
from managym._managym import ActionSpaceEnum
from managym._managym import AgentError
from managym._managym import Card
from managym._managym import CardTypes
from managym._managym import Env
from managym._managym import ManaCost
from managym._managym import Observation
from managym._managym import Permanent
from managym._managym import PhaseEnum
from managym._managym import Player
from managym._managym import PlayerConfig
from managym._managym import StepEnum
from managym._managym import Turn
from managym._managym import ZoneEnum
from . import _managym
__all__ = ['Action', 'ActionEnum', 'ActionSpace', 'ActionSpaceEnum', 'AgentError', 'BATTLEFIELD', 'BEGINNING', 'BEGINNING_DRAW', 'BEGINNING_UNTAP', 'BEGINNING_UPKEEP', 'COMBAT', 'COMBAT_BEGIN', 'COMBAT_DAMAGE', 'COMBAT_DECLARE_ATTACKERS', 'COMBAT_DECLARE_BLOCKERS', 'COMBAT_END', 'COMMAND', 'Card', 'CardTypes', 'DECLARE_ATTACKER', 'DECLARE_BLOCKER', 'ENDING', 'ENDING_CLEANUP', 'ENDING_END', 'EXILE', 'Env', 'GAME_OVER', 'GRAVEYARD', 'HAND', 'LIBRARY', 'ManaCost', 'Observation', 'POSTCOMBAT_MAIN', 'POSTCOMBAT_MAIN_STEP', 'PRECOMBAT_MAIN', 'PRECOMBAT_MAIN_STEP', 'PRIORITY', 'PRIORITY_CAST_SPELL', 'PRIORITY_PASS_PRIORITY', 'PRIORITY_PLAY_LAND', 'Permanent', 'PhaseEnum', 'Player', 'PlayerConfig', 'STACK', 'StepEnum', 'Turn', 'ZoneEnum']
BATTLEFIELD: _managym.ZoneEnum  # value = <ZoneEnum.BATTLEFIELD: 2>
BEGINNING: _managym.PhaseEnum  # value = <PhaseEnum.BEGINNING: 0>
BEGINNING_DRAW: _managym.StepEnum  # value = <StepEnum.BEGINNING_DRAW: 2>
BEGINNING_UNTAP: _managym.StepEnum  # value = <StepEnum.BEGINNING_UNTAP: 0>
BEGINNING_UPKEEP: _managym.StepEnum  # value = <StepEnum.BEGINNING_UPKEEP: 1>
COMBAT: _managym.PhaseEnum  # value = <PhaseEnum.COMBAT: 2>
COMBAT_BEGIN: _managym.StepEnum  # value = <StepEnum.COMBAT_BEGIN: 4>
COMBAT_DAMAGE: _managym.StepEnum  # value = <StepEnum.COMBAT_DAMAGE: 7>
COMBAT_DECLARE_ATTACKERS: _managym.StepEnum  # value = <StepEnum.COMBAT_DECLARE_ATTACKERS: 5>
COMBAT_DECLARE_BLOCKERS: _managym.StepEnum  # value = <StepEnum.COMBAT_DECLARE_BLOCKERS: 6>
COMBAT_END: _managym.StepEnum  # value = <StepEnum.COMBAT_END: 8>
COMMAND: _managym.ZoneEnum  # value = <ZoneEnum.COMMAND: 6>
DECLARE_ATTACKER: _managym.ActionSpaceEnum  # value = <ActionSpaceEnum.DECLARE_ATTACKER: 2>
DECLARE_BLOCKER: _managym.ActionSpaceEnum  # value = <ActionSpaceEnum.DECLARE_BLOCKER: 3>
ENDING: _managym.PhaseEnum  # value = <PhaseEnum.ENDING: 4>
ENDING_CLEANUP: _managym.StepEnum  # value = <StepEnum.ENDING_CLEANUP: 11>
ENDING_END: _managym.StepEnum  # value = <StepEnum.ENDING_END: 10>
EXILE: _managym.ZoneEnum  # value = <ZoneEnum.EXILE: 5>
GAME_OVER: _managym.ActionSpaceEnum  # value = <ActionSpaceEnum.GAME_OVER: 0>
GRAVEYARD: _managym.ZoneEnum  # value = <ZoneEnum.GRAVEYARD: 3>
HAND: _managym.ZoneEnum  # value = <ZoneEnum.HAND: 1>
LIBRARY: _managym.ZoneEnum  # value = <ZoneEnum.LIBRARY: 0>
POSTCOMBAT_MAIN: _managym.PhaseEnum  # value = <PhaseEnum.POSTCOMBAT_MAIN: 3>
POSTCOMBAT_MAIN_STEP: _managym.StepEnum  # value = <StepEnum.POSTCOMBAT_MAIN_STEP: 9>
PRECOMBAT_MAIN: _managym.PhaseEnum  # value = <PhaseEnum.PRECOMBAT_MAIN: 1>
PRECOMBAT_MAIN_STEP: _managym.StepEnum  # value = <StepEnum.PRECOMBAT_MAIN_STEP: 3>
PRIORITY: _managym.ActionSpaceEnum  # value = <ActionSpaceEnum.PRIORITY: 1>
PRIORITY_CAST_SPELL: _managym.ActionEnum  # value = <ActionEnum.PRIORITY_CAST_SPELL: 1>
PRIORITY_PASS_PRIORITY: _managym.ActionEnum  # value = <ActionEnum.PRIORITY_PASS_PRIORITY: 2>
PRIORITY_PLAY_LAND: _managym.ActionEnum  # value = <ActionEnum.PRIORITY_PLAY_LAND: 0>
STACK: _managym.ZoneEnum  # value = <ZoneEnum.STACK: 4>
