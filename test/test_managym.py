import pytest
import managym_py

@pytest.fixture
def basic_deck_configs():
    """Each config is a dict of {card_name: count}."""
    return [
        {"Grey Ogre": 8, "Mountain": 12},  # Red deck
        {"Forest": 12, "Llanowar Elves": 8}  # Green deck
    ]


class TestManagym:
    def test_init(self):
        """Smoke test environment initialization."""
        env = managym_py.Env()
        assert env is not None

    def test_reset_returns_valid_state(self, basic_deck_configs):
        """Test reset() returns valid initial Observation with correct fields."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, info = env.reset(player_configs)

        assert obs is not None
        assert isinstance(info, dict)
        assert hasattr(obs, 'players')
        assert len(obs.players) == 2
        assert hasattr(obs, 'action_space')
        assert hasattr(obs.action_space, 'actions')
        assert len(obs.action_space.actions) > 0

        assert hasattr(obs.turn, 'active_player_id')
        assert 0 <= obs.turn.active_player_id < 2

        assert obs.validate()

    def test_valid_game_loop(self, basic_deck_configs):
        """Test a full game loop by repeatedly taking the first action."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, info = env.reset(player_configs)

        max_steps = 2000
        steps = 0
        terminated = False
        truncated = False
        reward = 0.0

        while not (terminated or truncated) and steps < max_steps:
            obs, reward, terminated, truncated, info = env.step(0)
            steps += 1

        assert terminated
        assert steps < max_steps
        assert obs.game_over
        assert isinstance(reward, (int, float))

    def test_invalid_action_handling(self, basic_deck_configs):
        """Test that out-of-range action indices raise an error."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, _ = env.reset(player_configs)

        num_actions = len(obs.action_space.actions)
        invalid_actions = [-1, num_actions, num_actions + 1]

        for invalid_action in invalid_actions:
            with pytest.raises(Exception) as excinfo:
                env.step(invalid_action)
            assert "Action index" in str(excinfo.value)

    def test_multiple_resets(self, basic_deck_configs):
        """Test multiple resets with different configurations."""
        player_configs1 = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        player_configs2 = [
            managym_py.PlayerConfig("Blue Mage", {"Island": 20}),
            managym_py.PlayerConfig("Black Mage", {"Swamp": 20})
        ]

        env = managym_py.Env()

        obs1, info1 = env.reset(player_configs1)
        assert obs1.validate()
        assert len(obs1.players) == 2

        obs2, info2 = env.reset(player_configs2)
        assert obs2.validate()
        assert len(obs2.players) == 2
        player_names = [p.id for p in obs2.players.values()]
        assert len(player_names) == 2

    def test_observation_validation(self, basic_deck_configs):
        """Verifies observation validation and game over conditions."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, _ = env.reset(player_configs)
        assert obs.validate()

        done = False
        while not done:
            obs, reward, done, truncated, info = env.step(0)
            assert obs.validate()
            
            if done:
                assert obs.game_over
                assert isinstance(obs.won, bool)
                break

    def test_reward_on_victory(self, basic_deck_configs):
        """Test reward values at game end."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, _ = env.reset(player_configs)

        done = False
        truncated = False
        final_reward = 0.0

        while not (done or truncated):
            obs, reward, done, truncated, info = env.step(0)
            final_reward = reward

        assert done
        assert -1.0 <= final_reward <= 1.0

    def test_play_land_and_cast_spell(self, basic_deck_configs):
        """Test playing a land and casting a spell."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env()
        obs, _ = env.reset(player_configs)

        def find_action(obs, action_type):
            return next(
                (i for i, act in enumerate(obs.action_space.actions) 
                 if act.action_type == action_type),
                None
            )

        land_idx = find_action(obs, managym_py.ActionType.PRIORITY_PLAY_LAND)
        if land_idx is not None:
            obs, _, _, _, _ = env.step(land_idx)
            agent_player_id = obs.turn.agent_player_id
            agent_player_data = obs.players[agent_player_id]
            bf_count = agent_player_data.zone_counts[managym_py.ZoneType.BATTLEFIELD]
            assert bf_count >= 1

        cast_idx = find_action(obs, managym_py.ActionType.PRIORITY_CAST_SPELL)
        if cast_idx is not None:
            obs, _, _, _, _ = env.step(cast_idx)
            assert obs.validate()

    def test_throw_action_error(self):
        """Test exception handling."""
        with pytest.raises(Exception) as excinfo:
            managym_py.throw_action_error()
        assert "Test in python bindings" in str(excinfo.value)

    def test_skip_trivial_false(self, basic_deck_configs):
        """Test non-skipping of trivial actions."""
        player_configs = [
            managym_py.PlayerConfig("Red Mage", basic_deck_configs[0]),
            managym_py.PlayerConfig("Green Mage", basic_deck_configs[1])
        ]
        env = managym_py.Env(skip_trivial=False)
        obs, _ = env.reset(player_configs)

        terminated = False
        truncated = False
        step_count = 0
        trivial = 0

        while not (terminated or truncated) and step_count <= 2000:
            if len(obs.action_space.actions) == 1:
                trivial += 1
            obs, _, terminated, truncated, _ = env.step(0)
            step_count += 1

        assert terminated
        assert trivial >= 10
        assert step_count <= 2000