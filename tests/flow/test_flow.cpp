#include "managym/flow/turn.h"
#include "managym/infra/log.h"
#include "tests/managym_test.h"

#include <fmt/core.h>
#include <gtest/gtest.h>
class TestFlow : public ManagymTest {
protected:
    std::unique_ptr<Game> game;

    void SetUp() override {
        // Create the game using our test helpers
        ManagymTest::SetUp();
        game = elvesVsOgres(true);
    }
};

TEST_F(TestFlow, TurnPhasesStepsProgression) {
    ASSERT_TRUE(game != nullptr);

    ASSERT_TRUE(advanceToNextTurn(game.get(), 1000));
}

TEST_F(TestFlow, CorrectPlayerStartsWithPriority) {
    ASSERT_TRUE(game != nullptr);

    Player* active = game->activePlayer();

    ASSERT_TRUE(advanceToPhaseStep(game.get(), PhaseType::PRECOMBAT_MAIN));
    ActionSpace* space = game->current_action_space.get();

    ASSERT_EQ(space->player, active);
}

TEST_F(TestFlow, ReachingCombatSteps) {
    ASSERT_TRUE(game != nullptr);

    auto& turnSystem = game->turn_system;

    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::COMBAT));
}

TEST_F(TestFlow, PriorityPassingTest) {
    auto game = elvesVsOgres(true);

    // Advance until we get priority in upkeep
    // Should execute the following sequence:
    // 1. Untap step (no priority)
    // 2. Upkeep step (priority)
    ASSERT_TRUE(advanceToPhaseStep(game.get(), PhaseType::BEGINNING, StepType::BEGINNING_UPKEEP));

    // Verify we have a priority action space
    auto action_space = game->actionSpace();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::PRIORITY);

    log_info(LogCat::TEST, "Action space: {}", action_space->toString());

    // Both players should pass priority
    log_info(LogCat::TEST, "Active player passing priority");
    game->step(action_space->actions.size() - 1); // Last action should be pass

    action_space = game->actionSpace();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::PRIORITY);

    log_info(LogCat::TEST, "Action space: {}", action_space->toString());
    log_info(LogCat::TEST, "Non-active player passing priority");
    game->step(action_space->actions.size() - 1); // Last action should be pass

    // After both players pass with empty stack, we should move to draw step
    auto phase = game->turn_system->currentPhaseType();
    auto step = game->turn_system->currentStepType();

    EXPECT_EQ(phase, PhaseType::BEGINNING);
    EXPECT_EQ(step, StepType::BEGINNING_DRAW);
}

TEST_F(TestFlow, ActionSpaceValidity) {
    // Create player configurations that exactly match the Python default Match:
    // hero (gaea): Mountain x12, Forest x12, Llanowar Elves x18, Grey Ogre x18
    // villain (urza): Mountain x12, Forest x12, Llanowar Elves x18, Grey Ogre x18
    PlayerConfig gaea_config(
        "gaea", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});

    PlayerConfig urza_config(
        "urza", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});

    // Create game with these configs, matching Python's Env setup
    std::mt19937 rng;
    rng.seed(std::random_device()());
    auto game = std::make_unique<Game>(std::vector<PlayerConfig>{gaea_config, urza_config}, &rng,
                                       /*skip_trivial=*/true);
    ASSERT_TRUE(game != nullptr);

    // Track progression through the game
    int steps_taken = 0;
    const int max_steps = 10000; // Match Python's max_steps
    bool game_over = false;

    // Helper to validate action space
    auto validate_action_space = [](ActionSpace* space, int step_num) {
        // Action space should never be null during normal gameplay
        ASSERT_NE(space, nullptr) << "Null action space at step " << step_num;

        // There should always be at least one action available
        ASSERT_GT(space->actions.size(), 0) << "Empty action space at step " << step_num
                                            << "\nAction space type: " << toString(space->type)
                                            << "\nFull action space: " << space->toString();

        // Player should be assigned
        ASSERT_NE(space->player, nullptr) << "Action space has no player at step " << step_num;

        // Each action should be properly initialized
        for (size_t i = 0; i < space->actions.size(); i++) {
            const auto& action = space->actions[i];
            ASSERT_NE(action, nullptr) << "Null action at index " << i << " at step " << step_num;
            ASSERT_EQ(action->player, space->player)
                << "Action player mismatch at index " << i << " at step " << step_num;
        }
    };

    // First validate initial action space
    ActionSpace* current_space = game->actionSpace();

    // Take steps through the game, validating each action space
    while (!game_over && steps_taken < max_steps) {
        validate_action_space(current_space, steps_taken);

        // Log the current game state
        log_info(LogCat::TEST, "Step {}: Phase={}, Step={}, ActivePlayer={}", steps_taken,
                 toString(game->turn_system->currentPhaseType()),
                 toString(game->turn_system->currentStepType()), game->activePlayer()->name);

        // Log the current priority if we're in a priority situation
        if (current_space->type == ActionSpaceType::PRIORITY) {
            log_info(LogCat::TEST, "Priority actions available: {}", current_space->actions.size());
        }

        game_over = game->step(0);
        current_space = game->actionSpace();
        steps_taken++;
    }

    // Game should complete within max steps
    ASSERT_TRUE(game_over) << "Game did not complete within " << max_steps << " steps";
    ASSERT_LT(steps_taken, max_steps) << "Game took maximum number of steps";
}

TEST_F(TestFlow, CombatActionSpaceAfterDamage) {
    // Create player configurations matching the Python test:
    PlayerConfig gaea_config(
        "gaea", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});

    PlayerConfig urza_config(
        "urza", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});

    // Create game with these configs
    std::mt19937 rng;
    rng.seed(std::random_device()());
    auto game =
        std::make_unique<Game>(std::vector<PlayerConfig>{gaea_config, urza_config}, &rng, true);
    ASSERT_TRUE(game != nullptr);

    // Helper to validate action space
    auto validate_action_space = [](ActionSpace* space, int step_num) {
        ASSERT_NE(space, nullptr) << "Null action space at step " << step_num;

        // There should always be at least one action available
        ASSERT_GT(space->actions.size(), 0) << "Empty action space at step " << step_num
                                            << "\nAction space type: " << toString(space->type);

        // Player should be assigned
        ASSERT_NE(space->player, nullptr) << "Action space has no player at step " << step_num;

        // Each action should be properly initialized
        for (size_t i = 0; i < space->actions.size(); i++) {
            const auto& action = space->actions[i];
            ASSERT_NE(action, nullptr) << "Null action at index " << i << " at step " << step_num;
            ASSERT_EQ(action->player, space->player)
                << "Action player mismatch at index " << i << " at step " << step_num;
        }

        // Log the action space for debugging
        log_info(LogCat::TEST, "Action space at step {}: {}", step_num, space->toString());
    };

    // Set up combat scenario:
    // 1. Get to precombat main
    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    // 2. Put a creature into play for each player
    putPermanentInPlay(game.get(), game->activePlayer(), "Llanowar Elves");
    advanceToNextTurn(game.get());
    putPermanentInPlay(game.get(), game->activePlayer(), "Llanowar Elves");

    // 3. Advance through summoning sickness
    advanceToNextTurn(game.get());
    advanceToNextTurn(game.get());

    // 4. Get to combat declare attackers step
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_ATTACKERS));

    log_info(LogCat::TEST, "Starting combat sequence");

    int step_count = 0;
    const int max_steps = 20; // Should complete combat in fewer steps
    bool combat_complete = false;

    // Run through the combat sequence
    while (!combat_complete && step_count < max_steps) {
        // Validate current action space
        ActionSpace* current_space = game->actionSpace();
        validate_action_space(current_space, step_count);

        // Log state before step
        log_info(LogCat::TEST, "Step {}: Phase={}, Step={}, ActivePlayer={}", step_count,
                 toString(game->turn_system->currentPhaseType()),
                 toString(game->turn_system->currentStepType()), game->activePlayer()->name);

        // Take step
        bool game_over = game->step(0);
        ASSERT_FALSE(game_over) << "Game ended unexpectedly during combat";
        step_count++;

        // Check if we've moved past combat
        if (game->turn_system->currentPhaseType() != PhaseType::COMBAT) {
            combat_complete = true;
        }

        // After step, validate the next action space
        current_space = game->actionSpace();
        validate_action_space(current_space, step_count);
    }

    ASSERT_TRUE(combat_complete) << "Combat did not complete within " << max_steps << " steps";
}
