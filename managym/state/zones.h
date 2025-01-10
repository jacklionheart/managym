#pragma once

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "battlefield.h"
#include "stack.h"
#include "zone.h"

// MR400.1. A zone is a place where objects can be during a game. There are
// normally seven zones: library, hand, battlefield, graveyard, stack, exile,
// and command.
//
// Some older cards also use the ante zone. Each player has their own library,
// hand, and graveyard. The other zones are shared by all players.
enum struct ZoneType {
  LIBRARY,
  HAND,
  BATTLEFIELD,
  GRAVEYARD,
  STACK,
  EXILE,
  COMMAND
};

// Zones manager: Owns all of the zone objects and handles movement of cards
// between them. All state mutations go here, and all card state is read from
// here.
class Zones {
 public:
  // Constructor
  Zones(std::vector<Player*>& players);

  // Reads
  //

  // Generic zone queries
  bool contains(const Card* card, ZoneType zoneType, Player* player) const;
  Card* top(ZoneType zoneType, Player* player) const;
  size_t size(ZoneType zoneType, Player* player) const;
  size_t totalSize(ZoneType zoneType) const;

  // Const Zone* accessories
  const Library* constLibrary() const;
  const Graveyard* constGraveyard() const;
  const Hand* constHand() const;
  const Battlefield* constBattlefield() const;
  const Stack* constStack() const;
  const Exile* constExile() const;
  const Command* constCommand() const;

  // Writes
  //

  // Generic zone movement
  void move(Card* card, ZoneType toZone);
  Card* moveTop(ZoneType zoneFrom, ZoneType zoneTo, Player* player);
  void shuffle(ZoneType zoneType, Player* player);
  void forEach(const std::function<void(Card*)>& func, ZoneType zoneType,
               Player* player);
  void forEachAll(const std::function<void(Card*)>& func, ZoneType zoneType);

  // Battlefield mutations
  void destroy(Permanent* permanent);
  void produceMana(const ManaCost& mana_cost, Player* player);
  void forEachPermanentAll(const std::function<void(Permanent*)>& func);
  void forEachPermanent(const std::function<void(Permanent*)>& func,
                        Player* player);

  // Stack mutations
  void pushStack(Card* card);
  Card* popStack();

 protected:
  // Helpers
  Zone* getZone(ZoneType zoneType) const;

  // Data
  //

  // Zone* subobjects
  std::unique_ptr<Library> library;
  std::unique_ptr<Graveyard> graveyard;
  std::unique_ptr<Hand> hand;
  std::unique_ptr<Battlefield> battlefield;
  std::unique_ptr<Stack> stack;
  std::unique_ptr<Exile> exile;
  std::unique_ptr<Command> command;

  // Current zone
  std::map<Card*, Zone*> card_to_zone;
};
