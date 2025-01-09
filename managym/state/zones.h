#pragma once

#include <memory>

#include "managym/state/battlefield.h"
#include "managym/state/stack.h"
#include "managym/state/zone.h"

struct Zones {
  std::vector<const Player*> players;
  std::map<Card*, Zone*> card_to_zone;

  std::unique_ptr<Graveyard> graveyard;
  std::unique_ptr<Hand> hand;
  std::unique_ptr<Library> library;
  std::unique_ptr<Battlefield> battlefield;
  std::unique_ptr<Stack> stack;
  std::unique_ptr<Exile> exile;
  std::unique_ptr<Command> command;

  Zones(std::vector<Player*>& players);

  void move(Card* card, Zone* zone);
};