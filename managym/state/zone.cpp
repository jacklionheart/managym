// zone.cpp
#include "zone.h"

#include "managym/state/zones.h"

#include <algorithm> // For std::shuffle
#include <cassert>
#include <format>
#include <random> // For random number generators

Zone::Zone(Zones* zones, std::vector<Player*>& players) : zones(zones) {
    for (Player* player : players) {
        cards[player] = std::vector<Card*>();
    }
}

// Writes

void Zone::enter(Card* card) { cards[card->owner].push_back(card); }

void Zone::exit(Card* card) {
    std::vector<Card*>& player_cards = cards[card->owner];
    player_cards.erase(std::remove(player_cards.begin(), player_cards.end(), card),
                       player_cards.end());
}

// Reads

bool Zone::contains(const Card* card, Player* player) const {
    const auto& playerCards = cards.at(player);
    return std::any_of(playerCards.begin(), playerCards.end(),
                       [&card](const Card* c) { return *c == card; });
}

void Zone::shuffle(Player* player) {
    std::shuffle(cards[player].begin(), cards[player].end(), std::mt19937(std::random_device()()));
}

Card* Zone::top(Player* player) {
    std::vector<Card*>& player_cards = cards.at(player);
    return player_cards.back();
}

size_t Zone::size(Player* player) const {
    std::vector<Card*> player_cards = cards.at(player);
    return player_cards.size();
}

size_t Zone::totalSize() const {
    size_t total = 0;
    for (const auto& player_cards : cards) {
        total += player_cards.second.size();
    }
    return total;
}

// Iteration

void Zone::forEach(const std::function<void(Card*)>& func, Player* player) {
    std::vector<Card*> player_cards = cards.at(player);
    for (Card* card : player_cards) {
        func(card);
    }
}

void Zone::forEachAll(const std::function<void(Card*)>& func) {
    for (const auto& player_cards : cards) {
        for (Card* card : player_cards.second) {
            func(card);
        }
    }
}