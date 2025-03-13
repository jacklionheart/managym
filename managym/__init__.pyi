# __init__.pyi
"""
Type stubs for the managym module, a Magic: The Gathering reinforcement learning environment.
This defines the Python interface to the C++ managym library.
"""

from __future__ import annotations

from enum import IntEnum
from typing import Any, Dict, List, Tuple

# ---------------------------------------------------------------------
# Environment API
# ---------------------------------------------------------------------

class Env:
    """Main environment class for Magic gameplay."""
    
    def __init__(self, seed: int = 0, skip_trivial: bool = False, enable_profiler: bool = False, enable_behavior_tracking: bool = False) -> None:
        """Initialize environment.
        
        Args:
            seed: Random seed for game
            skip_trivial: Skip states with only one action
        """
        ...

    def reset(self, player_configs: List[PlayerConfig]) -> Tuple[Observation, Dict[str, Any]]:
        """Reset environment with new players.
        
        Args:
            player_configs: List of player configurations
            
        Returns:
            Tuple of (initial observation, info dict)
        """
        ...

    def step(self, action: int) -> Tuple[Observation, float, bool, bool, Dict[str, Any]]:
        """Take an action in the game.
        
        Args:
            action: Index of action to take
            
        Returns:
            Tuple of (observation, reward, terminated, truncated, info)
        """
        ...

    def info(self) -> Dict[str, Any]:
        """Get information about the environment.
        
        Returns:
            Dictionary of environment information -- profiler and behavior stats
        """
        ...

# ---------------------------------------------------------------------
# Player Config -- Used to define the match
# ---------------------------------------------------------------------

class PlayerConfig:
    """Initial configuration for a player."""
    def __init__(self, name: str, decklist: Dict[str, int]) -> None: ...
    name: str                    # Player's name
    decklist: Dict[str, int]     # Map of card name to quantity

# ---------------------------------------------------------------------
# Enums 
# ---------------------------------------------------------------------

class ZoneEnum(IntEnum):
    """Zones in Magic: The Gathering where objects can exist."""
    LIBRARY = 0      # Player's deck
    HAND = 1         # Cards in hand
    BATTLEFIELD = 2  # Cards in play
    GRAVEYARD = 3    # Discard pile
    EXILE = 4        # Removed from game
    STACK = 5        # Spells and abilities waiting to resolve
    COMMAND = 6      # Command zone for special cards

class PhaseEnum(IntEnum):
    """Major phases of a Magic turn."""
    BEGINNING = 0        # Untap, Upkeep, Draw steps
    PRECOMBAT_MAIN = 1   # First main phase
    COMBAT = 2           # All combat steps
    POSTCOMBAT_MAIN = 3  # Second main phase  
    ENDING = 4          # End and cleanup steps

class StepEnum(IntEnum):
    """Detailed steps within each phase of a turn."""
    BEGINNING_UNTAP = 0             # Untap permanents
    BEGINNING_UPKEEP = 1            # Upkeep triggers
    BEGINNING_DRAW = 2              # Draw card for turn
    PRECOMBAT_MAIN_STEP = 3         # First main phase
    COMBAT_BEGIN = 4                # Start of combat
    COMBAT_DECLARE_ATTACKERS = 5    # Choose attackers
    COMBAT_DECLARE_BLOCKERS = 6     # Choose blockers
    COMBAT_DAMAGE = 7               # Deal combat damage
    COMBAT_END = 8                  # End of combat
    POSTCOMBAT_MAIN_STEP = 9        # Second main phase
    ENDING_END = 10                 # End step triggers
    ENDING_CLEANUP = 11             # Cleanup step

class ActionEnum(IntEnum):
    """Types of actions players can take."""
    PRIORITY_PLAY_LAND = 0       # Play a land card
    PRIORITY_CAST_SPELL = 1      # Cast a spell
    PRIORITY_PASS_PRIORITY = 2   # Pass priority to next player
    DECLARE_ATTACKER = 3         # Declare an attacker
    DECLARE_BLOCKER = 4          # Declare a blocker

class ActionSpaceEnum(IntEnum):
    """Types of decision points in the game."""
    GAME_OVER = 0           # Game has ended
    PRIORITY = 1            # Player has priority to act
    DECLARE_ATTACKER = 2    # Declaring attackers
    DECLARE_BLOCKER = 3     # Declaring blockers

# ---------------------------------------------------------------------
# Core Data Classes
# ---------------------------------------------------------------------

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

class Player:
    """Player state from a perspective."""
    player_index: int    # Index in [0, num_players)
    id: int             # Unique ID for references
    is_active: bool     # Whether it's this player's turn
    is_agent: bool      # Whether this player has priority
    life: int          
    zone_counts: List[int]  # Number of objects in each zone

class Card:
    """Card state visible to the observing player."""
    zone: ZoneEnum
    owner_id: int       # Player ID of card owner
    id: int            # Unique card ID
    registry_key: int
    power: int
    toughness: int
    card_types: CardTypes
    mana_cost: ManaCost

class Permanent:
    """Permanent state visible to the observing player."""
    id: int
    controller_id: int  # Player ID of current controller
    tapped: bool
    damage: int
    is_summoning_sick: bool

class Turn:
    """Current turn information."""
    turn_number: int
    phase: PhaseEnum
    step: StepEnum
    active_player_id: int
    agent_player_id: int

class Action:
    """Single possible action."""
    action_type: ActionEnum
    focus: List[int]     # IDs of objects involved in action

class ActionSpace:
    """Collection of possible actions."""
    action_space_type: ActionSpaceEnum
    actions: List[Action]
    focus: List[int]    # Global focus objects for this decision

class Observation:
    """Game state from a single player's perspective."""
    game_over: bool
    won: bool           # True if observing player won
    turn: Turn
    action_space: ActionSpace

    # Agent (observing player) data
    agent: Player            
    agent_cards: Dict[int, Card]       # Objects owned by agent
    agent_permanents: Dict[int, Permanent]  
    
    # Opponent data (visible portion)
    opponent: Player         
    opponent_cards: Dict[int, Card]    # Visible opponent objects
    opponent_permanents: Dict[int, Permanent]

    def validate(self) -> bool: ...
    def toJSON(self) -> str: ...



# ---------------------------------------------------------------------
# Exceptions
# ---------------------------------------------------------------------

class AgentError(RuntimeError):
    """Raised when an agent attempts an invalid action."""
    ...