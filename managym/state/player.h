#pragma once

#include "managym/cardsets/card_registry.h"
#include "managym/state/card.h"
#include "managym/state/game_object.h"
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

    std::string deckList() const;
};

// Represents a player in the game, managing their resources and game state
class Player : public GameObject {
public:
    Player(const ObjectId& id, int index, const PlayerConfig& config, CardRegistry* registry);

    // Data

    // 0-indexed index of the player in the game (according to initial turn order)
    // Used by manabot to identify the player
    int index;
    std::unique_ptr<Deck> deck;
    std::string name;
    int life = 20;
    bool drew_when_empty = false;
    bool alive = true;
    Mana mana_pool;

    // Reads
    std::string toString() const;

    // Writes
    void takeDamage(int damage);

    // Delete copy/assignment to prevent ownership issues
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = default;

private:
    // Helpers
    std::unique_ptr<Deck> instantiateDeck(const PlayerConfig& config, CardRegistry* registry);
};