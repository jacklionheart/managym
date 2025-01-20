#include "managym/agent/action.h"
#include "managym/agent/observation.h"
#include "managym/infra/log.h"
#include "test/managym_test.h"

#include <gtest/gtest.h>

class TestAgent : public ManagymTest {
protected:
    std::unique_ptr<Game> game;
    Player* green_player;
    Player* red_player;

    void SetUp() override {
        // Create a standard test game
        ManagymTest::SetUp();
        game = elvesVsOgres();

        green_player = game->players[0].get();
        red_player = game->players[1].get();

        managym::log::info(Category::TEST, "Advancing to PRECOMBAT_MAIN");
        ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));
    }
};

TEST_F(TestAgent, PlayLandMovesCardToBattlefield) {
    ASSERT_TRUE(game != nullptr);

    // Find a land in red_player's hand
    Card* landCard = nullptr;
    for (auto* c : game->zones->constHand()->cards.at(green_player)) {
        if (c->types.isLand()) {
            landCard = c;
            break;
        }
    }
    ASSERT_NE(landCard, nullptr) << "No land found in red_player's hand.";

    // Execute the action
    managym::log::info(Category::TEST, "Advancing to PRECOMBAT_MAIN");
    advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN);
    managym::log::info(Category::TEST, "Playing land");
    PlayLand playLand(landCard, green_player, game.get());
    playLand.execute();

    // Check it left the hand
    EXPECT_FALSE(game->zones->contains(landCard, ZoneType::HAND, green_player));
    // Check it entered the battlefield
    EXPECT_TRUE(game->zones->contains(landCard, ZoneType::BATTLEFIELD, green_player));
    // Check land play count incremented
    EXPECT_EQ(game->turn_system->current_turn->lands_played, 1);
}

TEST_F(TestAgent, CastSpellGoesOnStack) {
    ASSERT_TRUE(game != nullptr);

    // Find a castable spell and required lands
    Card* spellCard = nullptr;
    std::vector<Card*> lands;
    auto hand = game->zones->constHand()->cards.at(red_player);

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
    CastSpell castAction(spellCard, red_player, game.get());
    castAction.execute();

    // Verify the spell is on stack
    EXPECT_TRUE(game->zones->contains(spellCard, ZoneType::STACK, red_player));
}
// Helper to verify common observation properties
void verifyBasicObservation(Observation* obs) {
    // Verify players
    ASSERT_EQ(obs->player_states.size(), 2);
    ASSERT_EQ(obs->player_states[0].life_total, 20);
    ASSERT_EQ(obs->player_states[1].life_total, 20);

    // Verify game state
    ASSERT_FALSE(obs->is_game_over);
}

TEST_F(TestAgent, ObservationForPriorityAction) {
    // Advance to main phase where we have priority decisions
    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    ActionSpace* action_space = game->current_action_space.get();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->action_type, ActionType::Priority);

    Observation obs(game.get());

    // Basic observation verification
    verifyBasicObservation(game->observation());

    // Priority-specific verification
    ASSERT_EQ(game->observation()->action_type, ActionType::Priority);
}

TEST_F(TestAgent, ObservationForDeclareAttackers) {
    // Setup a non-summoning sick attacker
    putPermanentInPlay(game.get(), red_player, "Grey Ogre");

    // Make it the red player's turn
    advanceToNextTurn(game.get(), 1000);

    // Advance to declare attackers
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_ATTACKERS));

    ActionSpace* action_space = game->current_action_space.get();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->action_type, ActionType::DeclareAttacker);

    Observation obs(game.get());
    verifyBasicObservation(&obs);
    ASSERT_EQ(obs.action_type, ActionType::DeclareAttacker);
}

TEST_F(TestAgent, ObservationForDeclareBlockers) {
    // Make it the red player's turn
    advanceToNextTurn(game.get());

    // Setup a non-summoning sick attacker and blockers
    putPermanentInPlay(game.get(), red_player, "Grey Ogre");
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");

    // Get to declare attackers and declare one attacker
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_ATTACKERS));
    
    ActionSpace* attack_space = game->current_action_space.get();
    ASSERT_NE(attack_space, nullptr);
    ASSERT_EQ(attack_space->action_type, ActionType::DeclareAttacker);

    // Advance to declare blockers
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_BLOCKERS));

    ASSERT_TRUE(game->zones->constBattlefield()->attackers(red_player).size() >= 1);
    ActionSpace* action_space = game->current_action_space.get();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->action_type, ActionType::DeclareBlocker);

    Observation obs(game.get());
    verifyBasicObservation(&obs);
    ASSERT_EQ(obs.action_type, ActionType::DeclareBlocker);
}