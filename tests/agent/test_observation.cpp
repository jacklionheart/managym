// ------------------------------------------------------------------------------------------
// test_observation.cpp (Revised)
// ------------------------------------------------------------------------------------------

#include "managym/agent/observation.h"
#include "managym/infra/log.h"
#include "tests/managym_test.h"

#include <gtest/gtest.h>

// Helper: find the actual Player* by ID
static const Player* findGamePlayerById(const Game* game, int pid) {
    for (auto& up : game->players) {
        if (up->id == pid) {
            return up.get();
        }
    }
    return nullptr;
}

class TestObservation : public ManagymTest {
protected:
    std::unique_ptr<Game> game;
    Player* green_player;
    Player* red_player;

    void SetUp() override {
        ManagymTest::SetUp();
        // Make each deck 60 total (e.g. 30 mountains, 30 ogres, etc.)
        // So that the "exactly 60 cards" checks pass
        game = elvesVsOgres(/*headless=*/true,
                            /*redMountains=*/30,
                            /*redOgres=*/30,
                            /*greenForests=*/30,
                            /*greenElves=*/30);

        green_player = game->players[0].get();
        red_player = game->players[1].get();
    }

    // Helper to verify mandatory observation fields step by step
    void verifyBasicObservation(const Observation* obs) {
        SCOPED_TRACE("verifyBasicObservation"); // gtest: prefixes all failures with this line

        managym::log::debug(Category::TEST, "Starting basic observation verification");

        // 1. Verify observation pointer
        ASSERT_NE(obs, nullptr) << "Observation pointer is null";
        managym::log::debug(Category::TEST, "Observation pointer valid");

        // 2. Verify game flags
        managym::log::debug(Category::TEST, "Checking game flags");
        ASSERT_FALSE(obs->game_over) << "Game should not be over in initial state";
        ASSERT_FALSE(obs->won) << "Game should not be 'won' in initial state";
        managym::log::debug(Category::TEST, "Game flags valid");

        // 3. Verify turn data
        managym::log::debug(Category::TEST, "Checking turn data");
        ASSERT_GE(obs->turn.turn_number, 0) << "Turn number should be >= 0";
        ASSERT_GE(obs->turn.active_player_id, 0) << "Active player ID should be >= 0";
        ASSERT_GE(obs->turn.agent_player_id, 0) << "Agent player ID should be >= 0";
        managym::log::debug(Category::TEST, "Turn data valid");

        // 4. Verify players map
        managym::log::debug(Category::TEST, "Checking players map");
        ASSERT_EQ(obs->players.size(), 2u) << "Should have exactly 2 players (Green & Red)";

        for (const auto& [pid, pdata] : obs->players) {
            managym::log::debug(Category::TEST, "Checking player {}", pid);

            // 4a. ID consistency
            ASSERT_EQ(pid, pdata.id)
                << "Player map key (" << pid << ") should match player id (" << pdata.id << ")";

            // 4b. Must find a real player with this ID
            const Player* game_player = findGamePlayerById(game.get(), pid);
            ASSERT_NE(game_player, nullptr) << "No matching Player* in the game with ID=" << pid;

            // 4c. Basic player data
            ASSERT_GE(pdata.life, 0) << "Life total should be non-negative";
            ASSERT_EQ(pdata.zone_counts.size(), 7u)
                << "Should have exactly 7 zone counts (LIB, HAND, BFIELD, etc.)";

            // 4d. Check zone counts
            for (size_t i = 0; i < pdata.zone_counts.size(); i++) {
                ZoneType zt = static_cast<ZoneType>(i);
                int expectedCount = pdata.zone_counts[i];
                int actualCount = game->zones->size(zt, game_player);
                ASSERT_EQ(expectedCount, actualCount)
                    << "Zone index=" << i << " mismatch for player " << pid
                    << " (obs=" << expectedCount << ", real=" << actualCount << ")";
            }
        }
        managym::log::debug(Category::TEST, "Players map valid");

        // 5. Verify action space
        managym::log::debug(Category::TEST, "Checking action space");
        ASSERT_GT(obs->action_space.actions.size(), 0u)
            << "Should have at least one action in the action_space";

        // 5a. Validate focus IDs
        for (const auto& action : obs->action_space.actions) {
            if (!action.focus.empty()) {
                for (int focus_id : action.focus) {
                    bool found_in_cards = (obs->cards.find(focus_id) != obs->cards.end());
                    bool found_in_permanents =
                        (obs->permanents.find(focus_id) != obs->permanents.end());
                    ASSERT_TRUE(found_in_cards || found_in_permanents)
                        << "Focus ID=" << focus_id << " not found in cards or permanents map";
                }
            }
        }
        managym::log::debug(Category::TEST, "Action space valid");
        managym::log::debug(Category::TEST, "Basic observation verification complete");
    }
};

// ------------------------------------------------------------------------------------------
// Tests
// ------------------------------------------------------------------------------------------

TEST_F(TestObservation, InitialStateCorrectlyObserved) {
    ASSERT_TRUE(game != nullptr) << "Game pointer is null";
    Observation obs(game.get());
    verifyBasicObservation(&obs);

    // Additional checks: e.g. each player has 60 total cards
    for (const auto& [pid, pdata] : obs.players) {
        EXPECT_EQ(pdata.life, 20) << "Player " << pid << " initial life should be 20";
        // By default, 7 in hand:
        EXPECT_EQ(pdata.zone_counts[(int)ZoneType::HAND], 7)
            << "Player " << pid << " initial hand size should be 7";

        // If you really want 60 total:
        int total = 0;
        for (int i = 0; i < 7; i++) {
            total += pdata.zone_counts[i];
        }
        EXPECT_EQ(total, 60) << "Player " << pid
                             << " should have exactly 60 total cards (deck + hand + etc.)";
    }
}

TEST_F(TestObservation, CardDataCorrectlyPopulated) {
    // Put a test permanent in play
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    ASSERT_FALSE(obs.cards.empty()) << "Should have at least some CardData in the observation";

    for (const auto& [cid, cdata] : obs.cards) {
        // Key consistency
        EXPECT_EQ(cid, cdata.id) << "Card map key " << cid << " should match the card data's ID "
                                 << cdata.id;

        // Owner must be valid
        auto pfound = obs.players.find(cdata.owner_id);
        EXPECT_TRUE(pfound != obs.players.end())
            << "Card owner id=" << cdata.owner_id << " is not in obs.players map";

        // Card types checks
        if (cdata.card_types.is_creature) {
            EXPECT_TRUE(cdata.card_types.is_permanent) << "Creatures must be permanents";
            EXPECT_TRUE(cdata.card_types.is_castable)
                << "Creatures must be castable (unless your engine says otherwise)";
            EXPECT_GT(cdata.mana_cost.mana_value, 0) << "Creature must have a mana cost > 0";
        }
        if (cdata.card_types.is_land) {
            EXPECT_TRUE(cdata.card_types.is_permanent) << "Lands must be permanents";
            EXPECT_FALSE(cdata.card_types.is_castable) << "Lands cannot be cast";
            EXPECT_FALSE(cdata.mana_cost.mana_value > 0) << "Lands cannot have a mana cost";
        }

        // If it's on battlefield, then a PermanentData entry should exist
        if (cdata.zone == ZoneType::BATTLEFIELD) {
            EXPECT_TRUE(cdata.card_types.is_permanent);
            auto perm_it =
                std::find_if(obs.permanents.begin(), obs.permanents.end(),
                             [card_id = cdata.id](const auto& pdat) { 
                                 return pdat.second.card_id == card_id; 
                             });
            EXPECT_TRUE(perm_it != obs.permanents.end())
                << "We found a card in BATTLEFIELD with ID=" << cid << " name=" << cdata.name
                << ", but no matching permanent in obs.permanents!";
        }
    }
}

TEST_F(TestObservation, PermanentDataCorrectlyPopulated) {
    // Put multiple test permanents in play
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");
    putPermanentInPlay(game.get(), red_player, "Grey Ogre");

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    ASSERT_FALSE(obs.permanents.empty()) << "We should have at least 2 permanents now.";

    for (const auto& [perm_id, pdat] : obs.permanents) {
        // Key consistency
        EXPECT_EQ(perm_id, pdat.id);

        // Must find the underlying card.
        auto cdit = obs.cards.find(pdat.card_id);
        if (cdit == obs.cards.end()) {
            ADD_FAILURE() << "Permanent ID=" << perm_id
                          << " has no matching CardData in obs.cards!";
            continue;
        }

        const auto& cdata = cdit->second;
        EXPECT_EQ(cdata.zone, ZoneType::BATTLEFIELD)
            << "A permanent's card must be in BATTLEFIELD zone name=" << cdata.name;

        // Controller must be valid
        auto pfound = obs.players.find(pdat.controller_id);
        EXPECT_TRUE(pfound != obs.players.end())
            << "Permanent's controller_id=" << pdat.controller_id << " is not in obs.players";

        // If the permanent is a creature, it must come from a creature card
        if (pdat.is_creature) {
            EXPECT_TRUE(cdata.card_types.is_creature)
                << "Permanent claims is_creature==true, but underlying card doesn't have "
                   "is_creature set.";
            if (pdat.damage > 0) {
                // Then cdata.toughness must be > 0
                EXPECT_GT(cdata.toughness, 0)
                    << "Damaged creature should have > 0 toughness or it'd be destroyed";
                EXPECT_LT(pdat.damage, cdata.toughness)
                    << "Damage >= toughness => it should be destroyed (SBA). Possibly a mismatch.";
            }
        }

        // If the permanent is a land, it cannot have summoning sickness
        if (pdat.is_land) {
            EXPECT_TRUE(cdata.card_types.is_land);
            EXPECT_FALSE(pdat.is_summoning_sick)
                << "By definition, lands shouldn't have summoning sickness in your ruleset.";
        }
    }
}

TEST_F(TestObservation, ActionSpaceCorrectlyPopulated) {
    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    // Compare observation with the actual Game::current_action_space
    Observation obs(game.get());
    verifyBasicObservation(&obs);

    auto* actual_aspace = game->current_action_space.get();
    ASSERT_TRUE(actual_aspace != nullptr);

    EXPECT_EQ(static_cast<int>(obs.action_space.action_space_type),
              static_cast<int>(actual_aspace->type));

    EXPECT_EQ(obs.action_space.actions.size(), actual_aspace->actions.size());

    for (size_t i = 0; i < obs.action_space.actions.size(); i++) {
        const auto& obs_act = obs.action_space.actions[i];
        const auto& real_act = actual_aspace->actions[i];

        EXPECT_EQ(static_cast<int>(obs_act.action_type), static_cast<int>(real_act->type))
            << "Mismatch in action type at index=" << i;

        EXPECT_EQ(obs_act.focus, real_act->focus()) << "Mismatch in focus vector at index=" << i;
    }
}

TEST_F(TestObservation, ZoneCountsAccurate) {
    // Move a card from hand -> graveyard
    Card* card = game->zones->top(ZoneType::HAND, green_player);
    ASSERT_NE(card, nullptr);

    Observation before_obs(game.get());
    auto before_counts = before_obs.players[green_player->id].zone_counts;

    game->zones->move(card, ZoneType::GRAVEYARD);

    Observation after_obs(game.get());
    auto after_counts = after_obs.players[green_player->id].zone_counts;

    for (int i = 0; i < 7; i++) {
        if (i == (int)ZoneType::HAND) {
            EXPECT_EQ(after_counts[i], before_counts[i] - 1) << "Hand should have 1 fewer card";
        } else if (i == (int)ZoneType::GRAVEYARD) {
            EXPECT_EQ(after_counts[i], before_counts[i] + 1) << "Graveyard should have +1 card";
        } else {
            EXPECT_EQ(after_counts[i], before_counts[i])
                << "Zone " << i << " should not have changed";
        }
    }
}

TEST_F(TestObservation, ToJSONProducesValidString) {
    Observation obs(game.get());
    std::string json = obs.toJSON();

    // Spot-check some fields
    EXPECT_TRUE(json.find("\"game_over\":") != std::string::npos)
        << "Should have 'game_over' field in JSON";
    EXPECT_TRUE(json.find("\"won\":") != std::string::npos) << "Should have 'won' field in JSON";
    EXPECT_TRUE(json.find("\"turn\":") != std::string::npos) << "Should have 'turn' object in JSON";
    EXPECT_TRUE(json.find("\"players\":") != std::string::npos) << "Should list 'players' in JSON";
    EXPECT_TRUE(json.find("\"cards\":") != std::string::npos) << "Should list 'cards' in JSON";
    EXPECT_TRUE(json.find("\"permanents\":") != std::string::npos)
        << "Should list 'permanents' in JSON";
}

TEST_F(TestObservation, PreservesTurnPhaseStep) {
    // Advance to, e.g., COMBAT_DECLARE_ATTACKERS
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_ATTACKERS));

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    // Check turn data matches game
    EXPECT_EQ(obs.turn.phase, game->turn_system->currentPhaseType());
    EXPECT_EQ(obs.turn.step, game->turn_system->currentStepType());

    // Active player checks
    Player* active = game->activePlayer();
    EXPECT_EQ(obs.turn.active_player_id, active->id);
    EXPECT_TRUE(obs.players.at(active->id).is_active)
        << "is_active should be true for the active player ID";

    // If we are in combat, check attackers
    if (obs.turn.phase == PhaseType::COMBAT) {
        auto attackers = game->zones->constBattlefield()->attackers(active);
        for (auto* attacker : attackers) {
            auto& perm_data = obs.permanents.at(attacker->id);
            EXPECT_TRUE(perm_data.is_creature)
                << "Attacker must be creature flagged in observation";
            EXPECT_FALSE(perm_data.is_summoning_sick)
                << "Attacker can't be summoning sick in observation";
        }
    }
}
