#include "managym/flow/turn.h"
#include "managym/infra/log.h"
#include "test/managym_test.h"

#include <fmt/core.h>
#include <gtest/gtest.h>
class TestFlow : public ManagymTest {
protected:
    std::unique_ptr<Game> game;

    void SetUp() override {
        // Create the game using our test helpers
        ManagymTest::SetUp();
        game = elvesVsOgres(/*headless=*/true);
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

    // If the ActionSpace is for priority, the player should match 'active'
    if (space && !space->actionSelected()) {
        EXPECT_EQ(space->player, active);
    }
}

TEST_F(TestFlow, ReachingCombatSteps) {
    ASSERT_TRUE(game != nullptr);

    auto& turnSystem = game->turn_system;

    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::COMBAT));
}

TEST_F(TestFlow, PriorityPassingTest) {
    auto game = elvesVsOgres(/*headless=*/true);

    // Advance until we get priority in upkeep
    // Should execute the following sequence:
    // 1. Untap step (no priority)
    // 2. Upkeep step (priority)
    ASSERT_TRUE(advanceToPhaseStep(game.get(), PhaseType::BEGINNING, StepType::BEGINNING_UPKEEP));

    // Verify we have a priority action space
    auto action_space = game->actionSpace();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::PRIORITY);

    managym::log::info(Category::TEST, "Action space: {}", action_space->toString());

    // Both players should pass priority
    managym::log::info(Category::TEST, "Active player passing priority");
    game->step(action_space->actions.size() - 1); // Last action should be pass

    action_space = game->actionSpace();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::PRIORITY);

    managym::log::info(Category::TEST, "Action space: {}", action_space->toString());
    managym::log::info(Category::TEST, "Non-active player passing priority");
    game->step(action_space->actions.size() - 1); // Last action should be pass

    // After both players pass with empty stack, we should move to draw step
    auto phase = game->turn_system->currentPhaseType();
    auto step = game->turn_system->currentStepType();

    EXPECT_EQ(phase, PhaseType::BEGINNING);
    EXPECT_EQ(step, StepType::BEGINNING_DRAW);
}
