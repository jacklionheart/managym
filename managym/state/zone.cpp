// zone.cpp
#include "zone.h"

#include <algorithm>  // For std::shuffle
#include <cassert>
#include <format>
#include <random>  // For random number generators

#include "managym/state/zones.h"

Zone::Zone(Zones* zones, std::vector<Player*>& players) : zones(zones) {
  for (Player* player : players) {
    cards[player] = std::vector<Card*>();
  }
}

void Zone::add(Card* card) { cards[card->owner].push_back(card); }

void Zone::remove(Card* card) {
  std::vector<Card*>& player_cards = cards[card->owner];
  player_cards.erase(
      std::remove(player_cards.begin(), player_cards.end(), card),
      player_cards.end());
}

bool Zone::contains(const Card* card, Player* player) const {
  const auto& playerCards = cards.at(player);
  return std::any_of(playerCards.begin(), playerCards.end(),
                     [&card](const Card* c) { return *c == card; });
}

void Zone::shuffle(Player* player) {
  std::shuffle(cards[player].begin(), cards[player].end(),
               std::mt19937(std::random_device()()));
}

Card* Zone::top(Player* player) {
  std::vector<Card*>& player_cards = cards.at(player);
  return player_cards.back();
}

size_t Zone::numCards(Player* player) const {
  std::vector<Card*> player_cards = cards.at(player);
  return player_cards.size();
}

size_t Zone::size() const {
  size_t total = 0;
  for (const auto& player_cards : cards) {
    total += player_cards.second.size();
  }
  return total;
}