#include "managym/agent/action.h"
#include "managym/agent/env.h"
#include "managym/agent/observation.h"
#include "managym/infra/log.h"
#include "tests/managym_test.h"

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

        log_info(LogCat::TEST, "Advancing to PRECOMBAT_MAIN");
        ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));
    }
};

TEST_F(TestAgent, PlayLandMovesCardToBattlefield) {
    ASSERT_TRUE(game != nullptr);

    // Find a land in green_player's hand
    Card* land_card = nullptr;
    for (auto* c : game->zones->constHand()->cards[green_player->index]) {
        if (c->types.isLand()) {
            land_card = c;
            break;
        }
    }
    ASSERT_NE(land_card, nullptr) << "No land found in green_player's hand.";

    // Execute the action
    log_info(LogCat::TEST, "Advancing to PRECOMBAT_MAIN");
    advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN);
    log_info(LogCat::TEST, "Playing land");
    PlayLand playLand(land_card, green_player, game.get());
    playLand.execute();

    // Check it left the hand
    EXPECT_FALSE(game->zones->contains(land_card, ZoneType::HAND, green_player));
    // Check it entered the battlefield
    EXPECT_TRUE(game->zones->contains(land_card, ZoneType::BATTLEFIELD, green_player));
    // Check land play count incremented
    EXPECT_EQ(game->turn_system->current_turn->lands_played, 1);
}

TEST_F(TestAgent, CastSpellGoesOnStack) {
    ASSERT_TRUE(game != nullptr);

    // Find a castable spell and required lands
    Card* spell_card = nullptr;
    std::vector<Card*> lands;
    const std::vector<Card*>& hand = game->zones->constHand()->cards[red_player->index];

    for (Card* c : hand) {
        if (c->types.isCastable()) {
            spell_card = c;
        } else if (c->types.isLand()) {
            lands.push_back(c);
        }
    }

    if (!spell_card || lands.empty()) {
        GTEST_SKIP() << "Required cards not found in hand. Skipping.";
    }

    // Play enough lands to cast the spell
    for (Card* land : lands) {
        game->zones->move(land, ZoneType::BATTLEFIELD);
    }

    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    // Now try to cast the spell
    CastSpell castAction(spell_card, red_player, game.get());
    castAction.execute();

    // Verify the spell is on stack
    EXPECT_TRUE(game->zones->contains(spell_card, ZoneType::STACK, red_player));
}

void verifyBasicObservation(const Observation& obs) {
    // Verify game state
    ASSERT_FALSE(obs.game_over);
    ASSERT_FALSE(obs.won);

    EXPECT_EQ(obs.agent.life, 20) << "Agent has wrong life total";
    EXPECT_EQ(obs.opponent.life, 20) << "Opponent has wrong life total";

    EXPECT_NE(obs.agent.player_index, obs.opponent.player_index)
        << "Agent and opponent have same player index";
    EXPECT_NE(obs.agent.id, obs.opponent.id) << "Agent and opponent have same player id";
}

TEST_F(TestAgent, ObservationForPriorityAction) {
    // Advance to main phase where we have priority decisions
    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    ActionSpace* action_space = game->current_action_space.get();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::PRIORITY);

    Observation obs(game.get());
    verifyBasicObservation(obs);

    // Verify action space matches reality
    ASSERT_EQ(obs.action_space.action_space_type, action_space->type);
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
    ASSERT_EQ(action_space->type, ActionSpaceType::DECLARE_ATTACKER);

    Observation obs(game.get());
    verifyBasicObservation(obs);

    // Verify we have the right phase and step
    ASSERT_EQ(obs.turn.phase, PhaseType::COMBAT);
    ASSERT_EQ(obs.turn.step, StepType::COMBAT_DECLARE_ATTACKERS);

    // Verify action space match reality
    ASSERT_EQ(obs.action_space.action_space_type, ActionSpaceType::DECLARE_ATTACKER);
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
    ASSERT_EQ(attack_space->type, ActionSpaceType::DECLARE_ATTACKER);

    // Advance to declare blockers
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_BLOCKERS));

    ASSERT_TRUE(game->zones->constBattlefield()->attackers(red_player).size() >= 1);
    ActionSpace* action_space = game->current_action_space.get();
    ASSERT_NE(action_space, nullptr);
    ASSERT_EQ(action_space->type, ActionSpaceType::DECLARE_BLOCKER);

    Observation obs(game.get());
    verifyBasicObservation(obs);

    // Verify we have the right phase and step
    ASSERT_EQ(obs.turn.phase, PhaseType::COMBAT);
    ASSERT_EQ(obs.turn.step, StepType::COMBAT_DECLARE_BLOCKERS);

    // Verify action space matches reality
    ASSERT_EQ(obs.action_space.action_space_type, ActionSpaceType::DECLARE_BLOCKER);
}

TEST_F(TestAgent, TestFullGameLoop) {
    std::map<std::string, int> redDeck{{"Grey Ogre", 8}, {"Mountain", 12}};
    std::map<std::string, int> greenDeck{{"Forest", 12}, {"Llanowar Elves", 8}};

    std::vector<PlayerConfig> playerConfigs;
    playerConfigs.emplace_back("Red Mage", redDeck);
    playerConfigs.emplace_back("Green Mage", greenDeck);

    Env env(false);

    Observation* obs = nullptr;
    InfoDict info;
    std::tie(obs, info) = env.reset(playerConfigs);

    ASSERT_NE(obs, nullptr) << "Initial Observation is null!";

    const int maxSteps = 2000;
    int steps = 0;
    bool terminated = false;
    bool truncated = false;
    double reward = 0.0;

    while (!terminated && !truncated && steps < maxSteps) {
        // The signature in C++ is:
        // step(int action, bool skip_trivial=false) -> (Observation*, double, bool, bool,
        // map<string,string>)
        Observation* newObs = nullptr;
        InfoDict newInfo;
        std::tie(newObs, reward, terminated, truncated, newInfo) = env.step(0);

        obs = newObs; // update pointer
        steps++;
    }
    // If we get here with no exception, check normal end conditions
    EXPECT_TRUE(terminated) << "Game did not terminate before maxSteps, so no IndexError happened.";
    EXPECT_LT(steps, maxSteps) << "Exceeded max steps but no exception thrown.";
    // You can add more asserts as needed
}

TEST_F(TestAgent, ReproducePriorityDeadlock) {
    std::map<std::string, int> mixed_deck{
        {"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}};

    std::vector<PlayerConfig> configs{PlayerConfig("gaea", mixed_deck),
                                      PlayerConfig("urza", mixed_deck)};

    Env env(false);
    Observation* obs = nullptr;
    InfoDict info;
    std::tie(obs, info) = env.reset(configs);
    ASSERT_NE(obs, nullptr);

    const int max_steps = 2000;
    int steps = 0;
    bool terminated = false;
    while (!terminated && steps < max_steps) {
        double reward;
        bool truncated;
        std::tie(obs, reward, terminated, truncated, info) = env.step(0);
        ASSERT_NE(obs, nullptr);
        steps++;
    }

    ASSERT_LT(steps, max_steps) << "Game did not complete before max steps";
}