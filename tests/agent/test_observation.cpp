// ------------------------------------------------------------------------------------------
// test_observation.cpp (Revised)
// ------------------------------------------------------------------------------------------

#include "managym/agent/observation.h"
#include "managym/infra/log.h"
#include "tests/managym_test.h"

#include <gtest/gtest.h>

// In the new model, the Observation no longer has a top‐level "players", "cards" or "permanents"
// map. Instead, we now have separate agent/opponent fields and maps. For example, the agent’s cards
// are stored in obs.agent_cards and the opponent’s in obs.opponent_cards.

// Helper: Given the game's players, find the agent and opponent pointers according to the new
// model.
static const Player* getGameAgent(const Game* game) { return game->agentPlayer(); }

static const Player* getGameOpponent(const Game* game) {
    const Player* game_agent = game->agentPlayer();
    for (const auto& p : game->playersStartingWithAgent()) {
        if (p != game_agent) {
            return p;
        }
    }
    return nullptr;
}

// Helper to find a card by ID in a vector of CardData
static const CardData* findCardById(const std::vector<CardData>& cards, int id) {
    for (const CardData& card : cards) {
        if (card.id == id) {
            return &card;
        }
    }
    return nullptr;
}

// Helper to find a permanent by ID in a vector of PermanentData
static const PermanentData* findPermanentById(const std::vector<PermanentData>& perms, int id) {
    for (const PermanentData& perm : perms) {
        if (perm.id == id) {
            return &perm;
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
        // Create a game where each deck has 60 total cards (for example, 30 lands and 30 creatures)
        game = elvesVsOgres(/*redMountains=*/30,
                            /*redOgres=*/30,
                            /*greenForests=*/30,
                            /*greenElves=*/30);
        green_player = game->players[0].get();
        red_player = game->players[1].get();
    }

    // Helper to verify mandatory observation fields step by step.
    // This version uses the new per-player organization.
    void verifyBasicObservation(const Observation* obs) {
        SCOPED_TRACE("verifyBasicObservation");

        log_debug(LogCat::TEST, "Starting basic observation verification");

        // 1. Verify observation pointer
        ASSERT_NE(obs, nullptr) << "Observation pointer is null";
        log_debug(LogCat::TEST, "Observation pointer valid");

        // 2. Verify global game flags
        ASSERT_FALSE(obs->game_over) << "Game should not be over in initial state";
        ASSERT_FALSE(obs->won) << "Game should not be 'won' in initial state";
        log_debug(LogCat::TEST, "Game flags valid");

        // 3. Verify turn data
        ASSERT_GE(obs->turn.turn_number, 0) << "Turn number should be >= 0";
        ASSERT_GE(obs->turn.active_player_id, 0) << "Active player ID should be >= 0";
        ASSERT_GE(obs->turn.agent_player_id, 0) << "Agent player ID should be >= 0";
        log_debug(LogCat::TEST, "Turn data valid");

        // 4. Verify player data
        // Check that the agent/opponent fields are set correctly.
        ASSERT_TRUE(obs->agent.is_agent) << "Agent player should have is_agent=true";
        ASSERT_FALSE(obs->opponent.is_agent) << "Opponent should have is_agent=false";
        ASSERT_NE(obs->agent.id, obs->opponent.id) << "Agent and opponent must have different IDs";

        // Verify that the zone counts match the game state.
        const Player* game_agent = getGameAgent(game.get());
        const Player* game_opponent = getGameOpponent(game.get());
        ASSERT_NE(game_agent, nullptr);
        ASSERT_NE(game_opponent, nullptr);

        for (size_t i = 0; i < obs->agent.zone_counts.size(); i++) {
            ZoneType zt = static_cast<ZoneType>(i);
            int expected_agent = game->zones->size(zt, game_agent);
            int expected_opponent = game->zones->size(zt, game_opponent);
            ASSERT_EQ(obs->agent.zone_counts[i], expected_agent)
                << "Zone index=" << i << " mismatch for agent (obs=" << obs->agent.zone_counts[i]
                << ", real=" << expected_agent << ")";
            ASSERT_EQ(obs->opponent.zone_counts[i], expected_opponent)
                << "Zone index=" << i
                << " mismatch for opponent (obs=" << obs->opponent.zone_counts[i]
                << ", real=" << expected_opponent << ")";
        }
        log_debug(LogCat::TEST, "Player data valid");

        // 5. Verify action space
        ASSERT_GT(obs->action_space.actions.size(), 0u)
            << "Should have at least one action in the action_space";
        for (const auto& action : obs->action_space.actions) {
            if (!action.focus.empty()) {
                for (int focus_id : action.focus) {
                    bool found = (findCardById(obs->agent_cards, focus_id) != nullptr) ||
                                 (findCardById(obs->opponent_cards, focus_id) != nullptr) ||
                                 (findPermanentById(obs->agent_permanents, focus_id) != nullptr) ||
                                 (findPermanentById(obs->opponent_permanents, focus_id) != nullptr);
                    ASSERT_TRUE(found)
                        << "Focus ID=" << focus_id << " not found in any object collection";
                }
            }
        }
        log_debug(LogCat::TEST, "Action space valid");
        log_debug(LogCat::TEST, "Basic observation verification complete");
    }
};

// ------------------------------------------------------------------------------------------
// Tests
// ------------------------------------------------------------------------------------------

TEST_F(TestObservation, InitialStateCorrectlyObserved) {
    ASSERT_NE(game, nullptr) << "Game pointer is null";
    Observation obs(game.get());
    verifyBasicObservation(&obs);

    // Additional checks: each player should have 60 total cards
    auto countCards = [](const std::array<int, 7>& counts) -> int {
        int total = 0;
        for (int cnt : counts)
            total += cnt;
        return total;
    };

    EXPECT_EQ(obs.agent.life, 20) << "Agent initial life should be 20";
    EXPECT_EQ(obs.opponent.life, 20) << "Opponent initial life should be 20";

    EXPECT_EQ(obs.agent.zone_counts[(int)ZoneType::HAND], 7)
        << "Agent initial hand size should be 7";
    EXPECT_EQ(obs.opponent.zone_counts[(int)ZoneType::HAND], 7)
        << "Opponent initial hand size should be 7";

    EXPECT_EQ(countCards(obs.agent.zone_counts), 60) << "Agent should have exactly 60 total cards";
    EXPECT_EQ(countCards(obs.opponent.zone_counts), 60)
        << "Opponent should have exactly 60 total cards";
}

TEST_F(TestObservation, CardDataCorrectlyOrganized) {
    // Play a permanent for each player to produce card data on the battlefield.
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");
    putPermanentInPlay(game.get(), red_player, "Grey Ogre");

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    // Verify agent card data.
    for (const CardData& card : obs.agent_cards) {
        EXPECT_EQ(card.owner_id, obs.agent.id) << "Agent card owner must equal agent id";

        if (card.card_types.is_creature) {
            EXPECT_TRUE(card.card_types.is_permanent) << "Creature should be permanent";
            EXPECT_TRUE(card.card_types.is_castable) << "Creature should be castable";
            EXPECT_GT(card.mana_cost.mana_value, 0) << "Creature must have mana cost > 0";
        }
        if (card.card_types.is_land) {
            EXPECT_TRUE(card.card_types.is_permanent) << "Land should be permanent";
            EXPECT_FALSE(card.card_types.is_castable) << "Land should not be castable";
            EXPECT_FALSE(card.mana_cost.mana_value > 0) << "Land should not have a mana cost";
        }
    }

    // Verify opponent card data.
    for (const CardData& card : obs.opponent_cards) {
        EXPECT_EQ(card.owner_id, obs.opponent.id) << "Opponent card owner must equal opponent id";
        // Opponent hand cards are hidden.
        EXPECT_NE(card.zone, ZoneType::HAND)
            << "Opponent hand cards should be hidden (found card: " << card.name << ")";
    }

    // For permanents on the battlefield, the corresponding card data should be present.
    for (const PermanentData& perm : obs.agent_permanents) {
        const CardData* card = findCardById(obs.agent_cards, perm.card_id);
        ASSERT_NE(card, nullptr)
            << "Agent permanent's card_id " << perm.card_id << " not found in agent_cards";
        EXPECT_EQ(card->zone, ZoneType::BATTLEFIELD)
            << "Agent permanent's card must be in BATTLEFIELD";
    }
    for (const PermanentData& perm : obs.opponent_permanents) {
        const CardData* card = findCardById(obs.opponent_cards, perm.card_id);
        ASSERT_NE(card, nullptr)
            << "Opponent permanent's card_id " << perm.card_id << " not found in opponent_cards";
        EXPECT_EQ(card->zone, ZoneType::BATTLEFIELD)
            << "Opponent permanent's card must be in BATTLEFIELD";
    }
}

TEST_F(TestObservation, PermanentDataCorrectlyOrganized) {
    // Put permanents in play for both players.
    putPermanentInPlay(game.get(), green_player, "Llanowar Elves");
    putPermanentInPlay(game.get(), red_player, "Grey Ogre");

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    // Verify agent permanents.
    for (const PermanentData& perm : obs.agent_permanents) {
        EXPECT_EQ(perm.controller_id, obs.agent.id) << "Agent permanent's controller must be agent";
    }
    // Verify opponent permanents.
    for (const PermanentData& perm : obs.opponent_permanents) {
        EXPECT_EQ(perm.controller_id, obs.opponent.id)
            << "Opponent permanent's controller must be opponent";
    }
}

TEST_F(TestObservation, HandInformationCorrectlyHidden) {
    Observation obs(game.get());
    verifyBasicObservation(&obs);

    int agentHandCount = 0;
    int opponentHandCount = 0;

    for (const CardData& card : obs.agent_cards) {
        if (card.zone == ZoneType::HAND)
            agentHandCount++;
    }
    for (const CardData& card : obs.opponent_cards) {
        if (card.zone == ZoneType::HAND)
            opponentHandCount++;
    }

    EXPECT_GT(agentHandCount, 0) << "Agent should see cards in their hand";
    EXPECT_EQ(opponentHandCount, 0) << "Opponent hand cards should be hidden";

    EXPECT_EQ(obs.agent.zone_counts[(int)ZoneType::HAND], agentHandCount)
        << "Agent zone count for HAND should match visible hand cards";
    EXPECT_GT(obs.opponent.zone_counts[(int)ZoneType::HAND], 0)
        << "Opponent zone count for HAND should be > 0 even if cards are hidden";
}

TEST_F(TestObservation, ActionSpaceCorrectlyPopulated) {
    ASSERT_TRUE(advanceToPhase(game.get(), PhaseType::PRECOMBAT_MAIN));

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    auto* actual_aspace = game->current_action_space.get();
    ASSERT_NE(actual_aspace, nullptr);

    EXPECT_EQ(static_cast<int>(obs.action_space.action_space_type),
              static_cast<int>(actual_aspace->type));
    EXPECT_EQ(obs.action_space.actions.size(), actual_aspace->actions.size());

    for (size_t i = 0; i < obs.action_space.actions.size(); i++) {
        const auto& obs_act = obs.action_space.actions[i];
        const auto& real_act = actual_aspace->actions[i];
        EXPECT_EQ(static_cast<int>(obs_act.action_type), static_cast<int>(real_act->type))
            << "Mismatch in action type at index " << i;
        EXPECT_EQ(obs_act.focus, real_act->focus()) << "Mismatch in focus vector at index " << i;
    }
}

TEST_F(TestObservation, ToJSONProducesValidString) {
    Observation obs(game.get());
    std::string json = obs.toJSON();

    EXPECT_NE(json.find("\"game_over\":"), std::string::npos) << "JSON should include 'game_over'";
    EXPECT_NE(json.find("\"won\":"), std::string::npos) << "JSON should include 'won'";
    EXPECT_NE(json.find("\"turn\":"), std::string::npos) << "JSON should include 'turn'";
    EXPECT_NE(json.find("\"agent\":"), std::string::npos) << "JSON should include 'agent'";
    EXPECT_NE(json.find("\"agent_cards\":"), std::string::npos)
        << "JSON should include 'agent_cards'";
    EXPECT_NE(json.find("\"agent_permanents\":"), std::string::npos)
        << "JSON should include 'agent_permanents'";
    EXPECT_NE(json.find("\"opponent\":"), std::string::npos) << "JSON should include 'opponent'";
    EXPECT_NE(json.find("\"opponent_cards\":"), std::string::npos)
        << "JSON should include 'opponent_cards'";
    EXPECT_NE(json.find("\"opponent_permanents\":"), std::string::npos)
        << "JSON should include 'opponent_permanents'";
}

TEST_F(TestObservation, PreservesTurnPhaseStep) {
    // Advance to a specific combat step.
    ASSERT_TRUE(
        advanceToPhaseStep(game.get(), PhaseType::COMBAT, StepType::COMBAT_DECLARE_ATTACKERS));

    Observation obs(game.get());
    verifyBasicObservation(&obs);

    EXPECT_EQ(obs.turn.phase, game->turn_system->currentPhaseType());
    EXPECT_EQ(obs.turn.step, game->turn_system->currentStepType());

    Player* active = game->activePlayer();
    EXPECT_EQ(obs.turn.active_player_id, active->id);
    if (obs.agent.id == active->id) {
        EXPECT_TRUE(obs.agent.is_active);
        EXPECT_FALSE(obs.opponent.is_active);
    } else {
        EXPECT_FALSE(obs.agent.is_active);
        EXPECT_TRUE(obs.opponent.is_active);
    }

    if (obs.turn.phase == PhaseType::COMBAT) {
        auto attackers = game->zones->constBattlefield()->attackers(active);
        for (auto* attacker : attackers) {
            const PermanentData* perm_data = nullptr;
            if (attacker->controller->id == obs.agent.id) {
                perm_data = findPermanentById(obs.agent_permanents, attacker->id);
                ASSERT_NE(perm_data, nullptr);
            } else {
                perm_data = findPermanentById(obs.opponent_permanents, attacker->id);
                ASSERT_NE(perm_data, nullptr);
            }
            EXPECT_FALSE(perm_data->is_summoning_sick)
                << "Attacker cannot be summoning sick in observation";
        }
    }
}

TEST_F(TestObservation, PlayersTakeAlternatingActions) {
    // Create a game with known deck configurations.
    PlayerConfig gaea_config(
        "gaea", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});
    PlayerConfig urza_config(
        "urza", {{"Mountain", 12}, {"Forest", 12}, {"Llanowar Elves", 18}, {"Grey Ogre", 18}});

    std::mt19937 rng;
    rng.seed(std::random_device()());
    auto game2 = std::make_unique<Game>(
        std::vector<PlayerConfig>{gaea_config, urza_config},
        &rng,
        /*skip_trivial=*/false // Ensure all actions are available for testing alternation.
    );
    ASSERT_NE(game2, nullptr);

    std::map<int, int> agent_counts;  // Map of agent id to count
    std::map<int, int> active_counts; // Map of active player id to count
    std::set<int> seen_agent_ids;
    std::set<int> seen_active_ids;
    int consecutive_same_agent = 0;
    int prev_agent_id = -1;
    const int max_steps = 1000;
    int steps = 0;
    bool game_over = false;

    while (!game_over && steps < max_steps) {
        Observation obs(game2.get());
        if (prev_agent_id == obs.agent.id)
            consecutive_same_agent++;
        else {
            consecutive_same_agent = 0;
            prev_agent_id = obs.agent.id;
        }
        agent_counts[obs.agent.id]++;
        active_counts[obs.turn.active_player_id]++;
        seen_agent_ids.insert(obs.agent.id);
        if (obs.agent.is_active)
            seen_active_ids.insert(obs.agent.id);
        if (obs.opponent.is_active)
            seen_active_ids.insert(obs.opponent.id);

        log_debug(LogCat::TEST, "Step {}: agent={}, active={}, consecutive_same={}", steps,
                  obs.agent.id, obs.turn.active_player_id, consecutive_same_agent);

        game_over = game2->step(0);
        steps++;

        if (steps > 20) {
            ASSERT_GE(seen_agent_ids.size(), 2)
                << "After " << steps << " steps, only one agent has been seen";
            float avg = 0;
            for (const auto& kv : agent_counts)
                avg += kv.second;
            avg /= agent_counts.size();
            for (const auto& kv : agent_counts)
                ASSERT_LE(std::abs(kv.second - avg), avg * 0.5)
                    << "Agent " << kv.first << " count deviates too far from average";
        }
    }
    log_info(LogCat::TEST, "Final agent counts after {} steps:", steps);
    for (const auto& kv : agent_counts) {
        log_info(LogCat::TEST, "  Player {}: {} times", kv.first, kv.second);
    }
    ASSERT_TRUE(game_over) << "Game did not complete within " << max_steps << " steps";
    ASSERT_GE(seen_agent_ids.size(), 2) << "Only one player was ever the agent";
    ASSERT_GE(seen_active_ids.size(), 2) << "Only one active player was ever seen";
}

TEST_F(TestObservation, ZoneCountsAccurate) {
    // Move a card from hand to graveyard.
    Card* card = game->zones->top(ZoneType::HAND, green_player);
    ASSERT_NE(card, nullptr);

    Observation before_obs(game.get());
    auto agent_before = before_obs.agent.zone_counts;
    auto opponent_before = before_obs.opponent.zone_counts;
    bool card_from_agent = (before_obs.agent.id == green_player->id);

    game->zones->move(card, ZoneType::GRAVEYARD);
    Observation after_obs(game.get());
    auto agent_after = after_obs.agent.zone_counts;
    auto opponent_after = after_obs.opponent.zone_counts;

    const std::array<int, 7>& owner_before = card_from_agent ? agent_before : opponent_before;
    const std::array<int, 7>& owner_after = card_from_agent ? agent_after : opponent_after;
    const std::array<int, 7>& other_before = card_from_agent ? opponent_before : agent_before;
    const std::array<int, 7>& other_after = card_from_agent ? opponent_after : agent_after;

    for (int i = 0; i < 7; i++) {
        if (i == (int)ZoneType::HAND) {
            EXPECT_EQ(owner_after[i], owner_before[i] - 1)
                << "Owner's hand should have 1 fewer card";
            EXPECT_EQ(other_after[i], other_before[i]) << "Non-owner's hand should be unchanged";
        } else if (i == (int)ZoneType::GRAVEYARD) {
            EXPECT_EQ(owner_after[i], owner_before[i] + 1)
                << "Owner's graveyard should have +1 card";
            EXPECT_EQ(other_after[i], other_before[i])
                << "Non-owner's graveyard should be unchanged";
        } else {
            EXPECT_EQ(owner_after[i], owner_before[i])
                << "Owner's zone " << i << " should be unchanged";
            EXPECT_EQ(other_after[i], other_before[i])
                << "Non-owner's zone " << i << " should be unchanged";
        }
    }
}

TEST_F(TestObservation, ValidateMethodCatchesInconsistencies) {
    Observation obs(game.get());
    // 1. Initial observation should validate
    EXPECT_TRUE(obs.validate()) << "Initial observation should be valid";

    // 2. Marking both players as agent should cause validation failure.
    Observation invalid_obs = obs;
    invalid_obs.opponent.is_agent = true;
    EXPECT_FALSE(invalid_obs.validate()) << "Should detect multiple agents";

    // 3. Alter card ownership in agent_cards to be wrong.
    invalid_obs = obs;
    if (!invalid_obs.agent_cards.empty()) {
        invalid_obs.agent_cards[0].owner_id = invalid_obs.opponent.id;
        EXPECT_FALSE(invalid_obs.validate()) << "Should detect incorrect card ownership";
    }

    // 4. Alter permanent controller in agent_permanents to be wrong.
    invalid_obs = obs;
    if (!invalid_obs.agent_permanents.empty()) {
        invalid_obs.agent_permanents[0].controller_id = invalid_obs.opponent.id;
        EXPECT_FALSE(invalid_obs.validate()) << "Should detect incorrect permanent controller";
    }
}
