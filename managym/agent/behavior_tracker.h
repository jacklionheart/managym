// behavior_tracker.h
// Tracks behavioral statistics for RL agents in managym
//

#pragma once

#include <map>
#include <string>
class Game;
class Player;
class Card;
class Permanent;

// BehaviorTracker tracks statistics for a single player across multiple games
class BehaviorTracker {
public:
    BehaviorTracker(bool enabled = true);
    ~BehaviorTracker();
    
    // Data
    bool enabled;
    
    // Land Play Statistics
    int lands_in_hand = 0;
    int lands_played = 0;
    int turns_with_land_in_hand = 0;
    int turns_with_land_played = 0;
    
    // Resource Management Statistics
    int castable_spells_in_hand = 0; 
    int turns_with_castable_spell = 0;
    int turns_with_spell_cast = 0;
    int spells_cast = 0;
    int mana_available = 0;
    int mana_spent = 0;
    
    // Combat Statistics
    int eligible_attackers = 0;
    int attacks_declared = 0;
    int eligible_blockers = 0;
    int blocks_declared = 0;
    int damage_dealt = 0;
    int damage_taken = 0;
    
    // Game Outcome Statistics
    int games_played = 0;
    int games_won = 0;
    int total_turns_played = 0;
    
    // Turn context (reset each turn)
    bool had_land_in_hand_this_turn = false;
    bool played_land_this_turn = false;
    bool had_castable_spell_this_turn = false;
    bool cast_spell_this_turn = false;
    
    // Game lifecycle hooks
    void onGameStart();
    void onGameWon();
    
    // Turn lifecycle hooks
    void onTurnStart();
    void onTurnEnd();
    
    // Phase hooks
    void onMainPhaseStart(Game* game, Player* player);
    void onMainPhaseEnd();
    void onDeclareAttackersStart(Game* game, Player* player);
    void onDeclareBlockersStart(Game* game, Player* player);
    
    // Action-specific hooks
    void onLandPlayed(Card* card);
    void onSpellCast(Card* card, int mana_spent);
    void onAttackerDeclared(Permanent* attacker);
    void onBlockerDeclared(Permanent* blocker, Permanent* attacker);
    void onDamageTaken(int amount);
    
    // Reset per-turn state tracking
    void resetTurnState();
    
    // Calculate derived metrics
    float landPlayRate() const;
    float spellCastRate() const;
    float attackRate() const;
    float blockRate() const;
    float manaEfficiency() const;
    float winRate() const;
    float avgGameLength() const;
    
    // Generate statistics for the environment info dict
    std::map<std::string, std::string> getStats() const;
    
    // Reset all statistics
    void reset();
    
    // Check if tracking is enabled
    bool isEnabled() const { return enabled; }
};

// Default no-op tracker for when tracking is disabled
BehaviorTracker* defaultBehaviorTracker();