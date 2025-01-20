#pragma once

#include "managym/agent/agent.h"
#include "managym/cardsets/card_registry.h"
#include "managym/state/card.h"
#include "managym/state/mana.h"

#include <map>
#include <memory>
#include <string>

// Configuration for creating a new player, including their name and decklist
struct PlayerConfig {
    std::string name;
    std::map<std::string, int> decklist;

    PlayerConfig(std::string name, const std::map<std::string, int>& cardQuantities)
        : name(std::move(name)), decklist(cardQuantities) {}
};

// Represents a player in the game, managing their resources and game state
class Player {
public:
    Player(const PlayerConfig& config, CardRegistry* registry);

    // Data
    std::unique_ptr<Deck> deck;
    std::unique_ptr<Agent> agent;
    std::string name;
    int life = 20;
    bool alive = true;
    Mana mana_pool;

    // Reads
    std::string toString() const;

    // Writes
    void takeDamage(int damage);

private:
    // Helpers
    std::unique_ptr<Deck> instantiateDeck(const PlayerConfig& config, CardRegistry* registry);
};