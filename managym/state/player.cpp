// player.cpp
#include "player.h"

#include "managym/cardsets/card_registry.h"
#include "managym/infra/log.h"

Player::Player(const ObjectId& id, int index, const PlayerConfig& config, CardRegistry* registry)
    : GameObject(id), index(index), name(config.name) {
    // Wait until 'this' is fully constructed before instantiating the deck.
    deck = instantiateDeck(config, registry);
    managym::log::debug(Category::STATE, "Created player {} (id={}) deck={}", name, id,
                        config.deckList());
}

// Writes

void Player::takeDamage(int damage) { life -= damage; }

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