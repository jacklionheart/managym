#pragma once

#include <memory>
#include <vector>

#include "managym/state/zone.h"

class Game;  // Forward declaration
class Card;
class Zones;

class Stack : public Zone {
 public:
  using Zone::Zone;

  std::vector<Card*> objects;
  Card* top();

 protected:
  friend class Zones;
  void push(Card* card);
  Card* pop();
};