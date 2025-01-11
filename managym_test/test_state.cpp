#include "managym/state/zones.h"
#include "managym_test/managym_test.h"

#include <gtest/gtest.h>

class TestState : public ManagymTest {
protected:
    std::unique_ptr<Game> game;

    void SetUp() override {
        ManagymTest::SetUp();
        // Build a default test game from our new helper library
        game = elvesVsOgres();
    }
};

TEST_F(TestState, CardMovementBetweenZones) {
    ASSERT_TRUE(game != nullptr);

    // Grab top card from the hand
    Player* p0 = game->players[0].get();
    Card* topCard = game->zones->top(ZoneType::HAND, p0);
    ASSERT_NE(topCard, nullptr);

    // Move it to the hand
    game->zones->moveTop(ZoneType::HAND, ZoneType::LIBRARY, p0);

    // Check it's no longer in the library but now in the hand
    EXPECT_FALSE(game->zones->contains(topCard, ZoneType::HAND, p0));
    EXPECT_TRUE(game->zones->contains(topCard, ZoneType::LIBRARY, p0));
}

TEST_F(TestState, InitialGameStateSetup) {
    ASSERT_TRUE(game != nullptr);

    // By default, each player starts with 7 in hand, 20 life, if everything is
    // correct
    for (auto& playerPtr : game->players) {
        Player* p = playerPtr.get();
        EXPECT_EQ(p->life, 20);
        EXPECT_EQ(game->zones->size(ZoneType::HAND, p), 7);
    }
}
