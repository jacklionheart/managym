#include <gtest/gtest.h>

#include "managym/flow/turn.h"
#include "managym_test/managym_test.h"

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
  std::unique_ptr<ActionSpace> space = game->turn_system->tick();

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