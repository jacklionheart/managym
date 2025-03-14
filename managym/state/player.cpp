// player.cpp
#include "player.h"

#include "managym/agent/behavior_tracker.h"
#include "managym/cardsets/card_registry.h"
#include "managym/infra/log.h"

#include <sstream>

Player::Player(const ObjectId& id, int index, const PlayerConfig& config, CardRegistry* registry,
               BehaviorTracker* behavior_tracker)
    : GameObject(id), index(index), name(config.name),
      behavior_tracker(behavior_tracker ? behavior_tracker : defaultBehaviorTracker()) {
    // Wait until 'this' is fully constructed before instantiating the deck.
    deck = instantiateDeck(config, registry);
    log_debug(LogCat::STATE, "Created player {} (id={}) deck={}", name, id, config.deckList());
}

// Writes

void Player::takeDamage(int damage) {
    life -= damage;
    behavior_tracker->onDamageTaken(damage);
}

// Reads

std::string Player::toString() const { return name + " - Life: " + std::to_string(life) + ")"; }

// Private Helpers

std::unique_ptr<Deck> Player::instantiateDeck(const PlayerConfig& config, CardRegistry* registry) {
    std::unique_ptr<Deck> deck = std::make_unique<Deck>();

    for (const auto& [name, quantity] : config.decklist) {
        for (int i = 0; i < quantity; ++i) {
            std::unique_ptr<Card> card = registry->instantiate(name, this);
            deck->push_back(std::move(card));
        }
    }
    return deck;
}

std::string PlayerConfig::deckList() const {
    std::stringstream ss;
    for (const auto& [name, quantity] : decklist) {
        ss << name << " x" << quantity << ", ";
    }
    return ss.str();
}