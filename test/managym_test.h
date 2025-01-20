#pragma once

#include "managym/flow/game.h"
#include "managym/cardsets/card_registry.h"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

struct TestDeckEntry {
    std::string name;
    int quantity;
};

PlayerConfig makePlayerConfig(const std::string& playerName,
                              const std::vector<TestDeckEntry>& deckEntries);

// Reset card registry
void registerTestCards();

// Create a basic game
std::unique_ptr<Game> elvesVsOgres(bool headless = true, int redMountains = 4, int redOgres = 3,
                                   int greenForests = 4, int greenElves = 3);

// Put a permanent in play
void putPermanentInPlay(Game* game, Player* player, const std::string& cardName);

// Advance to a phase (main, combat, etc.) Does not advance if already in the target phase.
// Set require_action_space to true to require an action space to be returned.
bool advanceToPhase(Game* game, PhaseType targetPhase, int maxTicks = 1000);

// Advance to a phase step (declare attackers, declare blockers, etc.) Does not advance if already
// in the target step.
bool advanceToPhaseStep(Game* game, PhaseType targetPhase,
                        std::optional<StepType> targetStep = std::nullopt, int maxTicks = 1000);

// Advance to the next turn.
bool advanceToNextTurn(Game* game, int maxTicks = 1000);

struct ManagymTest : public ::testing::Test {
protected:
    void SetUp() override {    }
};