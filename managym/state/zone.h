#pragma once

#include "managym/state/card.h"
#include "managym/state/player.h"

#include <functional>
#include <random>
#include <vector>

// Forward declaration
class Zones;

// Base class for all game zones. A zone represents a collection of cards
// that follows specific game rules for adding and removing cards.
class Zone {
public:
    virtual ~Zone() = default;
    Zones* zones;

    Zone(Zones* zones, std::vector<Player*>& players);

    // Data
    // Cards indexed by player->index (0 or 1 for 2-player game)
    std::vector<std::vector<Card*>> cards;

    // Reads

    // Randomize the order of the cards in this zone for a player.
    void shuffle(const Player* player, std::mt19937* rng);

    // Get the top card in this zone for a player.
    Card* top(const Player* player);

    // Number of cards in this zone for a player.
    size_t size(const Player* player) const;

    // Total number of cards in this zone.
    size_t totalSize() const;

    // Check if this zone contains a specific card for a player.
    bool contains(const Card* card, const Player* player) const;

protected:
    friend class Zones;

    // Move a card into this zone. Primarily called by `Zones::move`.
    virtual void enter(Card* card);

    // Move a card out of this zone. Primarily called by `Zones::move`.
    virtual void exit(Card* card);

    // Perform a lambda on all cards in this zone for a player.
    virtual void forEach(const std::function<void(Card*)>& func, const Player* player);
    // Perform a lambda on all cards in this zone for all players.
    virtual void forEachAll(const std::function<void(Card*)>& func);
};

class Library : public Zone {
public:
    using Zone::Zone;
};

class Graveyard : public Zone {
public:
    using Zone::Zone;
};

class Hand : public Zone {
public:
    using Zone::Zone;
};

class Exile : public Zone {
public:
    using Zone::Zone;
};

class Command : public Zone {
public:
    using Zone::Zone;
};