// zone.cpp
#include "zone.h"

#include "managym/infra/log.h"
#include "managym/state/zones.h"

#include <algorithm> // For std::shuffle
#include <cassert>
#include <random> // For random number generators

Zone::Zone(Zones* zones, std::vector<Player*>& players) : zones(zones) {
    for (const Player* player : players) {
        cards[player] = std::vector<Card*>();
    }
}

// Writes

void Zone::enter(Card* card) {
    log_debug(LogCat::STATE, "Zone::enter - Adding card {} owned by {}", card->id, card->owner->id);
    cards[card->owner].push_back(card);
}

void Zone::exit(Card* card) {
    log_debug(LogCat::STATE, "Zone::exit - Removing card {} owned by {}", card->id,
              card->owner->id);
    std::vector<Card*>& player_cards = cards[card->owner];
    player_cards.erase(std::remove(player_cards.begin(), player_cards.end(), card),
                       player_cards.end());
    log_debug(LogCat::STATE, "Zone::exit - Removed card {} contained: {}", card->id,
              contains(card, card->owner));
}

// Reads

bool Zone::contains(const Card* card, const Player* player) const {
    const auto& playerCards = cards.at(player);
    return std::any_of(playerCards.begin(), playerCards.end(),
                       [&card](const Card* c) { return c == card; });
}

void Zone::shuffle(const Player* player) {
    std::shuffle(cards[player].begin(), cards[player].end(), std::mt19937(std::random_device()()));
}

Card* Zone::top(const Player* player) {
    std::vector<Card*>& player_cards = cards.at(player);
    return player_cards.back();
}

size_t Zone::size(const Player* player) const {
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

void Zone::forEach(const std::function<void(Card*)>& func, const Player* player) {
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