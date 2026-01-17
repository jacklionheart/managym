// behavior_tracker.cpp
// Implementation of the BehaviorTracker system
//
// ------------------------------------------------------------------------------------------------
// EDITING INSTRUCTIONS:
// Implements methods for tracking player behavior during Magic games. 
// Each method is called at specific points in the game flow.
// ------------------------------------------------------------------------------------------------ 

#include "behavior_tracker.h"

#include "managym/flow/game.h"
#include "managym/state/battlefield.h"
#include "managym/state/card.h"
#include "managym/state/player.h"
#include "managym/state/zones.h"
#include "managym/infra/log.h"

#include <fmt/format.h>
#include <iomanip>
#include <sstream>

// -------------------------------
// BehaviorTracker Implementation
// -------------------------------

BehaviorTracker::BehaviorTracker(bool enabled) : enabled(enabled) {}

BehaviorTracker::~BehaviorTracker() {}

void BehaviorTracker::resetTurnState() {
    had_land_in_hand_this_turn = false;
    played_land_this_turn = false;
    had_castable_spell_this_turn = false;
    cast_spell_this_turn = false;
}

float BehaviorTracker::landPlayRate() const {
    if (turns_with_land_in_hand == 0) return 0.0f;
    return static_cast<float>(turns_with_land_played) / turns_with_land_in_hand;
}

float BehaviorTracker::spellCastRate() const {
    if (turns_with_castable_spell == 0) return 0.0f;
    return static_cast<float>(turns_with_spell_cast) / turns_with_castable_spell;
}

float BehaviorTracker::attackRate() const {
    if (eligible_attackers == 0) return 0.0f;
    return static_cast<float>(attacks_declared) / eligible_attackers;
}

float BehaviorTracker::blockRate() const {
    if (eligible_blockers == 0) return 0.0f;
    return static_cast<float>(blocks_declared) / eligible_blockers;
}

float BehaviorTracker::manaEfficiency() const {
    if (mana_available == 0) return 0.0f;
    return static_cast<float>(mana_spent) / mana_available;
}

float BehaviorTracker::winRate() const {
    if (games_played == 0) return 0.0f;
    return static_cast<float>(games_won) / games_played;
}

float BehaviorTracker::avgGameLength() const {
    if (games_played == 0) return 0.0f;
    return static_cast<float>(total_turns_played) / games_played;
}

void BehaviorTracker::reset() {
    lands_in_hand = 0;
    lands_played = 0;
    turns_with_land_in_hand = 0;
    turns_with_land_played = 0;
    
    castable_spells_in_hand = 0;
    turns_with_castable_spell = 0;
    turns_with_spell_cast = 0;
    spells_cast = 0;
    mana_available = 0;
    mana_spent = 0;
    
    eligible_attackers = 0;
    attacks_declared = 0;
    eligible_blockers = 0;
    blocks_declared = 0;
    damage_dealt = 0;
    damage_taken = 0;
    
    games_played = 0;
    games_won = 0;
    total_turns_played = 0;
    
    resetTurnState();
}

std::map<std::string, std::string> BehaviorTracker::getStats() const {
    std::map<std::string, std::string> result;
    
    if (!enabled) {
        return result;
    }
    
    auto format_float = [](float value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };
    
    result["land_play_rate"] = format_float(landPlayRate());
    result["spell_cast_rate"] = format_float(spellCastRate());
    result["attack_rate"] = format_float(attackRate());
    result["block_rate"] = format_float(blockRate());
    result["mana_efficiency"] = format_float(manaEfficiency());
    result["win_rate"] = format_float(winRate());
    result["avg_game_length"] = format_float(avgGameLength());
    
    // Include raw counters
    result["lands_played"] = std::to_string(lands_played);
    result["spells_cast"] = std::to_string(spells_cast);
    result["attacks_declared"] = std::to_string(attacks_declared);
    result["blocks_declared"] = std::to_string(blocks_declared);
    result["damage_taken"] = std::to_string(damage_taken);
    result["games_played"] = std::to_string(games_played);
    
    return result;
}

void BehaviorTracker::onGameStart() {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Game started");
    games_played++;
}

void BehaviorTracker::onGameWon() {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Game won");
    games_won++;
}

void BehaviorTracker::onTurnStart() {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Turn started");
    
    // Increment turn counter
    total_turns_played++;
    
    // Reset per-turn state
    resetTurnState();
}

void BehaviorTracker::onTurnEnd() {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Turn ended");
    
    // Update turn-level statistics
    if (had_land_in_hand_this_turn) {
        turns_with_land_in_hand++;
    }
    
    if (played_land_this_turn) {
        turns_with_land_played++;
    }
    
    if (had_castable_spell_this_turn) {
        turns_with_castable_spell++;
    }
    
    if (cast_spell_this_turn) {
        turns_with_spell_cast++;
    }
}

void BehaviorTracker::onMainPhaseStart(Game* game, Player* player) {
    if (!enabled || !game || !player) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Main phase started");
    
    // Count lands in hand
    int lands_in_hand_count = 0;
    bool has_castable_spell = false;
    
    // Check cards in hand for lands and castable spells
    for (Card* card : game->zones->constHand()->cards[player->index]) {
        if (card->types.isLand()) {
            lands_in_hand_count++;
        } else if (card->types.isCastable() && game->canPayManaCost(player, card->mana_cost.value())) {
            has_castable_spell = true;
        }
    }
    
    // Update statistics
    lands_in_hand += lands_in_hand_count;
    if (lands_in_hand_count > 0) {
        had_land_in_hand_this_turn = true;
    }
    
    if (has_castable_spell) {
        had_castable_spell_this_turn = true;
        castable_spells_in_hand++;
    }
    
    // Track available mana
    Mana available_mana = game->zones->constBattlefield()->producibleMana(player);
    int total_available = available_mana.total();
    mana_available += total_available;
}

void BehaviorTracker::onMainPhaseEnd() {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Main phase ended");
    // Currently a no-op, but kept for potential future use
}

void BehaviorTracker::onDeclareAttackersStart(Game* game, Player* player) {
    if (!enabled || !game || !player) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Combat started");
    
    // Count eligible attackers
    std::vector<Permanent*> eligible_attackers_vec = game->zones->constBattlefield()->eligibleAttackers(player);
    eligible_attackers += eligible_attackers_vec.size();
}

void BehaviorTracker::onDeclareBlockersStart(Game* game, Player* player) {
    if (!enabled || !game || !player) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Combat started");
    
    std::vector<Permanent*> eligible_blockers_vec = game->zones->constBattlefield()->eligibleBlockers(player);
    eligible_blockers += eligible_blockers_vec.size();
}   

void BehaviorTracker::onLandPlayed(Card* card) {
    if (!enabled || !card) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Land played: {}", card->name);
    
    lands_played++;
    played_land_this_turn = true;
}

void BehaviorTracker::onSpellCast(Card* card, int mana_spent) {
    if (!enabled || !card) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Spell cast: {} (cost: {})", 
              card->name, mana_spent);
    
    spells_cast++;
    cast_spell_this_turn = true;
    this->mana_spent += mana_spent;
}

void BehaviorTracker::onAttackerDeclared(Permanent* attacker) {
    if (!enabled || !attacker) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Attacker declared: {}", 
              attacker->card->name);
    
    attacks_declared++;
}

void BehaviorTracker::onBlockerDeclared(Permanent* blocker, Permanent* attacker) {
    if (!enabled || !blocker) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Blocker declared: {}", 
              blocker->card->name);
    
    blocks_declared++;
}

void BehaviorTracker::onDamageTaken(int amount) {
    if (!enabled) return;
    
    log_debug(LogCat::AGENT, "BehaviorTracker: Damage taken: {}", amount);
    damage_taken += amount;
}

// Default no-op tracker instance
BehaviorTracker* defaultBehaviorTracker() {
    static BehaviorTracker instance(false);
    return &instance;
}