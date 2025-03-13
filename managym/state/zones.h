#pragma once

#include "battlefield.h"
#include "managym/state/game_object.h"
#include "stack.h"
#include "zone.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

// MR400.1 A zone is a place where objects can be during a game. There are
// normally seven zones: library, hand, battlefield, graveyard, stack, exile,
// and command. Each player has their own library, hand, and graveyard. The
// other zones are shared by all players.
enum struct ZoneType { LIBRARY, HAND, BATTLEFIELD, GRAVEYARD, STACK, EXILE, COMMAND };

// `Zones` owns all of the zone objects and handles movement of cards between
// them. All mutations are performed here to support future features like undo,
// logging, etc.
class Zones {
public:
    Zones(std::vector<Player*>& players, IDGenerator* id_generator);

    // Reads

    // Check if a zone contains a specific card.
    bool contains(const Card* card, ZoneType zoneType, const Player* player) const;
    // Get the top card of a zone for a single player.
    Card* top(ZoneType zoneType, const Player* player) const;
    // Get the size of a zone for a single player.
    size_t size(ZoneType zoneType, const Player* player) const;
    // Get the total size of a zone including all players.
    size_t totalSize(ZoneType zoneType) const;

    // Accessors for individual zones
    const Library* constLibrary() const;
    const Graveyard* constGraveyard() const;
    const Hand* constHand() const;
    const Battlefield* constBattlefield() const;
    const Stack* constStack() const;
    const Exile* constExile() const;
    const Command* constCommand() const;

    // Writes

    // Move a card to a new zone.
    void move(Card* card, ZoneType toZone);
    // Move the top card from one zone to another.
    Card* moveTop(ZoneType zoneFrom, ZoneType zoneTo, Player* player);
    // Shuffle cards randomly within a single zone.
    void shuffle(ZoneType zoneType, Player* player, std::mt19937* rng);
    // Apply a function to all cards in a zone for a single player.
    void forEach(const std::function<void(Card*)>& func, ZoneType zoneType, Player* player);
    // Apply a function to all cards in a zone for all players.
    void forEachAll(const std::function<void(Card*)>& func, ZoneType zoneType);

    // Battlefield mutations

    // Destroy a permanent and move it to the graveyard.
    void destroy(Permanent* permanent);
    // Produce mana for a player by tapping permanents on the battlefield.
    void produceMana(const ManaCost& mana_cost, Player* player);
    // Apply a function to all permanents for all players.
    void forEachPermanentAll(const std::function<void(Permanent*)>& func);
    // Apply a function to all permanents for a single player.
    void forEachPermanent(const std::function<void(Permanent*)>& func, Player* player);

    // Stack mutations

    // Push a card onto the stack.
    void pushStack(Card* card);
    // Pop a card off the stack.
    Card* popStack();

protected:
    // Get a zone object by type.
    Zone* getZone(ZoneType zoneType) const;

    // Data

    // 7 Zones
    std::unique_ptr<Library> library;
    std::unique_ptr<Graveyard> graveyard;
    std::unique_ptr<Hand> hand;
    std::unique_ptr<Battlefield> battlefield;
    std::unique_ptr<Stack> stack;
    std::unique_ptr<Exile> exile;
    std::unique_ptr<Command> command;
    // Card --> Current Zone (null if none or not known).
    std::map<Card*, Zone*> card_to_zone;
    IDGenerator* id_generator;
    std::mt19937 rg;
};
