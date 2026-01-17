// zone.cpp
#include "zone.h"

#include "managym/infra/log.h"
#include "managym/state/zones.h"

#include <algorithm>
#include <cassert>
#include <random>

Zone::Zone(Zones* zones, std::vector<Player*>& players) : zones(zones) {
    cards.resize(players.size());
}

// Writes

void Zone::enter(Card* card) {
    log_debug(LogCat::STATE, "Zone::enter - Adding card {} owned by {}", card->id, card->owner->id);
    cards[card->owner->index].push_back(card);
}

void Zone::exit(Card* card) {
    log_debug(LogCat::STATE, "Zone::exit - Removing card {} owned by {}", card->id,
              card->owner->id);
    std::vector<Card*>& player_cards = cards[card->owner->index];
    player_cards.erase(std::remove(player_cards.begin(), player_cards.end(), card),
                       player_cards.end());
    log_debug(LogCat::STATE, "Zone::exit - Removed card {} contained: {}", card->id,
              contains(card, card->owner));
}

// Reads

bool Zone::contains(const Card* card, const Player* player) const {
    const std::vector<Card*>& player_cards = cards[player->index];
    return std::any_of(player_cards.begin(), player_cards.end(),
                       [&card](const Card* c) { return c == card; });
}

void Zone::shuffle(const Player* player, std::mt19937* rng) {
    std::shuffle(cards[player->index].begin(), cards[player->index].end(), *rng);
}

Card* Zone::top(const Player* player) {
    const std::vector<Card*>& player_cards = cards[player->index];
    return player_cards.back();
}

size_t Zone::size(const Player* player) const {
    return cards[player->index].size();
}

size_t Zone::totalSize() const {
    size_t total = 0;
    for (const std::vector<Card*>& player_cards : cards) {
        total += player_cards.size();
    }
    return total;
}

// Iteration

void Zone::forEach(const std::function<void(Card*)>& func, const Player* player) {
    const std::vector<Card*>& player_cards = cards[player->index];
    for (Card* card : player_cards) {
        func(card);
    }
}

void Zone::forEachAll(const std::function<void(Card*)>& func) {
    for (const std::vector<Card*>& player_cards : cards) {
        for (Card* card : player_cards) {
            func(card);
        }
    }
}
