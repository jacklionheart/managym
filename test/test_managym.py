import pytest
import managym_py

@pytest.fixture
def basic_deck_configs():
    return [
        {"Grey Ogre": 8, "Mountain": 12},
        {"Forest": 12, "Llanowar Elves": 8}
    ]

class TestManagym:
    def test_init(self):
        """Test environment initialization"""
        env = managym_py.Env()
        assert env is not None

    def test_reset_returns_valid_state(self, basic_deck_configs):
        """Test reset returns valid initial state"""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, action_space = env.reset(player_configs)

        assert obs is not None
        assert action_space is not None
        assert hasattr(action_space, 'valid_actions')
        assert len(action_space.valid_actions) > 0

    def test_valid_game_loop(self, basic_deck_configs):
        """Test full game loop executes properly"""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        _, action_space = env.reset(player_configs)

        steps = 0
        max_steps = 100000 # Prevent infinite loop
        done = False

        while not done and steps < max_steps:
            _, _, done = env.step(0)
            steps += 1

        assert done is True, "Game did not complete within max steps"
        assert steps < max_steps, "Reached maximum steps before game completion"

    def test_invalid_action_handling(self, basic_deck_configs):
        """Test handling of invalid actions"""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, action_space = env.reset(player_configs)

        # Test with actions outside the valid range
        invalid_actions = [-1, len(action_space.valid_actions), 
                           max(action_space.valid_actions) + 1]
        
        for invalid_action in invalid_actions:
            with pytest.raises(RuntimeError) as excinfo:
                env.step(invalid_action)
            
            # Check the error message contains the invalid action
            assert str(invalid_action) in str(excinfo.value)

    def test_multiple_resets(self, basic_deck_configs):
        """Test multiple resets with different configurations"""
        player_configs1 = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        player_configs2 = [
            managym_py.PlayerConfig("Blue Mage", {"Island": 20}),
            managym_py.PlayerConfig("Black Mage", {"Swamp": 20})
        ]

        env = managym_py.Env()
        
        # First reset
        obs1, action_space1 = env.reset(player_configs1)
        assert obs1 is not None
        assert action_space1 is not None
        
        # Second reset with different configs
        obs2, action_space2 = env.reset(player_configs2)
        assert obs2 is not None
        assert action_space2 is not None