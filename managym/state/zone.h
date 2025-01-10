#pragma once

#include <map>
#include <memory>
#include <vector>

#include "managym/state/card.h"
#include "managym/state/player.h"

// Forward declaration
class Zones;

class Zone {
 public:
  virtual ~Zone() = default;
  Zones* zones;

  Zone(Zones* zones, std::vector<Player*>& players);
  std::map<Player*, std::vector<Card*>> cards;

  void shuffle(Player* player);
  Card* top(Player* player);

  size_t size(Player* player) const;
  size_t totalSize() const;
  bool contains(const Card* card, Player* player) const;

 protected:
  friend class Zones;
  virtual void enter(Card* card);
  virtual void exit(Card* card);
  virtual void forEach(const std::function<void(Card*)>& func, Player* player);
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