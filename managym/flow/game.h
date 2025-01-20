#pragma once

#include "managym/agent/action.h"
#include "managym/flow/priority.h"
#include "managym/flow/turn.h"
#include "managym/state/card.h"
#include "managym/state/mana.h"
#include "managym/state/player.h"
#include "managym/state/zone.h"
#include "managym/state/zones.h"
#include "managym/ui/game_display.h"

#include <managym/agent/observation.h>
#include <spdlog/spdlog.h>

#include <vector>

// Core game class that manages game state and rules enforcement
class Game {
public:
    Game(std::vector<PlayerConfig> player_configs, bool headless = false);

    // Data
    std::vector<std::unique_ptr<Player>> players;
    std::unique_ptr<Zones> zones;
    // std::unique_ptr<GameDisplay> display;
    std::unique_ptr<TurnSystem> turn_system;
    std::unique_ptr<ActionSpace> current_action_space;
    std::unique_ptr<Observation> current_observation;
    std::unique_ptr<PrioritySystem> priority_system;
    std::unique_ptr<CardRegistry> card_registry;
    
    // Reads

    ActionSpace* actionSpace() const;
    Observation* observation() const;

    // Get currently active player
    Player* activePlayer() const;
    // Get non-active player
    Player* nonActivePlayer() const;
    // Get index of currently active player
    int activePlayerIndex() const;
    // Get player order for priority
    std::vector<Player*> priorityOrder() const;
    // Check if player is active player
    bool isActivePlayer(Player* player) const;
    // Check if player can play a land
    bool canPlayLand(Player* player) const;
    // Check if player can cast sorceries
    bool canCastSorceries(Player* player) const;
    // Check if player can pay a mana cost
    bool canPayManaCost(Player* player, const ManaCost& mana_cost) const;
    // Check if player is still alive
    bool isPlayerAlive(Player* player) const;
    // Check if game is over
    bool isGameOver() const;
    // Get index of the winner
    int winnerIndex() const;

    // Writes

    // Execute a single game action. Returns true
    bool step(int action);

    // Run the game to completion (using action space auto-selection)
    void play();

    // Clear all players' mana pools
    void clearManaPools();
    // Clear all damage from permanents
    void clearDamage();
    // Untap all permanents controlled by a player
    void untapAllPermanents(Player* player);
    // Mark creatures not summoning sick for a player
    void markPermanentsNotSummoningSick(Player* player);
    // Draw specified number of cards for a player
    void drawCards(Player* player, int amount);
    // Make specified player lose the game
    void loseGame(Player* player);
    // Add mana to a player's mana pool
    void addMana(Player* player, const Mana& mana);
    // Remove mana from a player's pool
    void spendMana(Player* player, const ManaCost& mana_cost);
    // Move card to stack and start casting process
    void castSpell(Player* player, Card* card);
    // Move a land card to the battlefield
    void playLand(Player* player, Card* card);

protected:
    // Move forward in the game until the next action space is available.
    // Returns true if the game is over.
    bool tick();
};
