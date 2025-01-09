#pragma once

#include <map>
#include <memory>
#include <vector>

#include "managym/state/card.h"
#include "managym/state/player.h"

// Forward declaration
class Zones;

struct Zone {
  virtual ~Zone() = default;
  Zones* zones;

  Zone(Zones* zones, std::vector<Player*>& players);
  std::map<Player*, std::vector<Card*>> cards;

  virtual void add(Card* card);
  virtual void remove(Card* card);
  void shuffle(Player* player);
  Card* top(Player* player);

  size_t numCards(Player* player) const;
  size_t size() const;
  bool contains(const Card* card, Player* player) const;
};

struct Library : public Zone {
  using Zone::Zone;
};

struct Graveyard : public Zone {
  using Zone::Zone;
};

struct Hand : public Zone {
  using Zone::Zone;
};

struct Exile : public Zone {
  using Zone::Zone;
};

struct Command : public Zone {
  using Zone::Zone;
};