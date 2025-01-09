#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "managym/action/action.h"
#include "managym/state/card.h"
#include "managym/state/mana.h"

struct PlayerConfig {
  std::string name;
  std::map<std::string, int> decklist;

  PlayerConfig(std::string name,
               const std::map<std::string, int>& cardQuantities)
      : name(std::move(name)), decklist(cardQuantities) {}
};

class Player {
 public:
  static int next_id;
  int id;

  std::unique_ptr<Deck> deck;

  std::unique_ptr<Agent> agent;

  std::string name;
  int life = 20;
  bool alive = true;
  Mana mana_pool;

  Player(const PlayerConfig& config);

  void takeDamage(int damage);

  [[nodiscard]] std::string toString() const;

 private:
  std::unique_ptr<Deck> instantiateDeck(const PlayerConfig& config);
};
