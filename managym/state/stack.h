#pragma once

#include <memory>
#include <vector>

#include "managym/state/zone.h"

class Game;  // Forward declaration
class Card;

struct Stack : public Zone {
  using Zone::Zone;

  std::vector<Card* > objects;

  void push(Card* card);
  Card* pop();

  Card* top();
};