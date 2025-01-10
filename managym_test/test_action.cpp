#include <gtest/gtest.h>

#include "managym/action/action.h"
#include "managym_test/managym_test.h"

class TestAction : public ManagymTest {
 protected:
  std::unique_ptr<Game> game;
  Player* p0;
  Player* p1;

  void SetUp() override {
    // Create a standard test game
    ManagymTest::SetUp();
    game = elvesVsOgres();
    p0 = game->players[0].get();
    p1 = game->players[1].get();

    game->turn_system->startNextTurn();
  }
};

TEST_F(TestAction, PlayLandMovesCardToBattlefield) {
  ASSERT_TRUE(game != nullptr);

  // Find a land in p0's hand
  Card* landCard = nullptr;
  for (auto* c : game->zones->constHand()->cards.at(p0)) {
    if (c->types.isLand()) {
      landCard = c;
      break;
    }
  }
  ASSERT_NE(landCard, nullptr) << "No land found in p0's hand.";

  // Execute the action
  spdlog::info("Advancing to PRECOMBAT_MAIN");
  advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN);
  spdlog::info("Playing land");
  PlayLand playLand(landCard, p0, game.get());
  playLand.execute();

  // Check it left the hand
  EXPECT_FALSE(game->zones->contains(landCard, ZoneType::HAND, p0));
  // Check it entered the battlefield
  EXPECT_TRUE(game->zones->contains(landCard, ZoneType::BATTLEFIELD, p0));
  // Check land play count incremented
  EXPECT_EQ(game->turn_system->current_turn->lands_played, 1);
}

TEST_F(TestAction, CastSpellGoesOnStack) {
  ASSERT_TRUE(game != nullptr);

  // Find a castable spell and required lands
  Card* spellCard = nullptr;
  std::vector<Card*> lands;
  auto hand = game->zones->constHand()->cards.at(p0);

  for (Card* c : hand) {
    if (c->types.isCastable()) {
      spellCard = c;
    } else if (c->types.isLand()) {
      lands.push_back(c);
    }
  }

  if (!spellCard || lands.empty()) {
    GTEST_SKIP() << "Required cards not found in hand. Skipping.";
  }

  // Play enough lands to cast the spell
  for (Card* land : lands) {
    game->zones->move(land, ZoneType::BATTLEFIELD);
  }

  ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

  // Now try to cast the spell
  CastSpell castAction(spellCard, p0, game.get());
  castAction.execute();

  // Verify the spell is on stack
  EXPECT_TRUE(game->zones->contains(spellCard, ZoneType::STACK, p0));
}