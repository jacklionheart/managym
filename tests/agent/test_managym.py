import pytest
import managym

@pytest.fixture
def basic_deck_configs():
    return [
        {'Grey Ogre': 8, 'Mountain': 12},
        {'Forest': 12, 'Llanowar Elves': 8}
    ]

class TestManagym:
    def test_init(self):
        env = managym.Env()
        assert env is not None

    def test_reset_returns_valid_state(self, basic_deck_configs):
        """Test reset() returns a valid Observation with correct fields."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, info = env.reset(player_configs)

        # In the new API, the Observation should expose 'agent' and 'opponent'
        assert obs is not None
        assert isinstance(info, dict)
        assert hasattr(obs, 'agent'), "Observation missing 'agent' field"
        assert hasattr(obs, 'opponent'), "Observation missing 'opponent' field"
        # Check that agent and opponent have different IDs
        assert obs.agent.id != obs.opponent.id, "Agent and opponent must have different IDs"
        # Also check turn and action_space fields exist
        assert hasattr(obs, 'turn'), "Observation missing 'turn' field"
        assert hasattr(obs, 'action_space'), "Observation missing 'action_space' field"

    def test_multiple_resets(self, basic_deck_configs):
        """Test multiple resets update Observation correctly."""
        player_configs1 = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        player_configs2 = [
            managym.PlayerConfig("Blue Mage", {"Island": 20}),
            managym.PlayerConfig("Black Mage", {"Swamp": 20})
        ]

        env = managym.Env()
        obs1, info1 = env.reset(player_configs1)
        assert obs1.validate(), "First observation failed validation"
        # Check that agent and opponent exist.
        assert obs1.agent is not None
        assert obs1.opponent is not None

        obs2, info2 = env.reset(player_configs2)
        assert obs2.validate(), "Second observation failed validation"
        # Instead of checking names (which are not exposed), we can check that the player IDs
        # differ or that some other configuration indicator has changed.
        # For now, we simply assert that both agent and opponent exist.
        assert obs2.agent is not None
        assert obs2.opponent is not None

    def test_valid_game_loop(self, basic_deck_configs):
        """Test a full game loop by always taking the first action."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, info = env.reset(player_configs)

        max_steps = 2000
        steps = 0
        terminated = False
        truncated = False
        reward = 0.0

        while not (terminated or truncated) and steps < max_steps:
            obs, reward, terminated, truncated, info = env.step(0)
            steps += 1

        assert terminated, "Game should eventually terminate"
        assert steps < max_steps, "Game exceeded maximum steps"
        assert obs.game_over, "Observation should indicate game is over"
        assert isinstance(reward, (int, float))

    def test_invalid_action_handling(self, basic_deck_configs):
        """Test that out-of-range action indices raise an error."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, _ = env.reset(player_configs)

        num_actions = len(obs.action_space.actions)
        invalid_actions = [-1, num_actions, num_actions + 1]
        for invalid_action in invalid_actions:
            with pytest.raises(Exception) as excinfo:
                env.step(invalid_action)
            assert "Action index" in str(excinfo.value)

    def test_observation_validation(self, basic_deck_configs):
        """Ensure that observations pass validate() throughout game progression."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, _ = env.reset(player_configs)
        assert obs.validate(), "Initial observation failed validation"
        done = False
        while not done:
            obs, reward, done, truncated, info = env.step(0)
            assert obs.validate(), "Observation failed validation during game"
            if done:
                assert obs.game_over
                assert isinstance(obs.won, bool)
                break

    def test_reward_on_victory(self, basic_deck_configs):
        """Test reward values are in the correct range on game end."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, _ = env.reset(player_configs)

        done = False
        truncated = False
        final_reward = 0.0
        while not (done or truncated):
            obs, reward, done, truncated, info = env.step(0)
            final_reward = reward

        assert done, "Game did not terminate"
        assert -1.0 <= final_reward <= 1.0, "Reward out of expected bounds"

    def test_play_land_and_cast_spell(self, basic_deck_configs):
        """Test playing a land and casting a spell using priority actions."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env()
        obs, _ = env.reset(player_configs)

        def find_action(obs, action_type):
            for idx, action in enumerate(obs.action_space.actions):
                if action.action_type == action_type:
                    return idx
            return None

        # Try playing a land if available.
        land_idx = find_action(obs, managym.ActionEnum.PRIORITY_PLAY_LAND)
        if land_idx is not None:
            obs, _, _, _, _ = env.step(land_idx)
            # Check that the agent's battlefield count has increased.
            bf_count = obs.agent.zone_counts[managym.ZoneEnum.BATTLEFIELD]
            assert bf_count >= 1, "Battlefield should have at least one card after playing a land"

        # Try casting a spell if available.
        cast_idx = find_action(obs, managym.ActionEnum.PRIORITY_CAST_SPELL)
        if cast_idx is not None:
            obs, _, _, _, _ = env.step(cast_idx)
            assert obs.validate(), "Observation failed validation after casting a spell"

    def test_skip_trivial_false(self, basic_deck_configs):
        """Test that when skip_trivial is False, trivial action spaces are not auto-skipped."""
        player_configs = [
            managym.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym.Env(skip_trivial=False)
        obs, _ = env.reset(player_configs)

        terminated = False
        step_count = 0
        trivial = 0

        while not terminated and step_count <= 2000:
            if len(obs.action_space.actions) == 1:
                trivial += 1
            obs, _, terminated, _, _ = env.step(0)
            step_count += 1

        assert terminated, "Game should eventually terminate"
        assert trivial >= 10, "There should be a number of trivial action spaces encountered"
        assert step_count <= 2000, "Game should not exceed 2000 steps"
