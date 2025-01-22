// player.cpp
#include "player.h"

#include "managym/cardsets/card_registry.h"

Player::Player(const ObjectId& id, const PlayerConfig& config, CardRegistry* registry)
    : GameObject(id), name(config.name), deck(instantiateDeck(config, registry)) {}

// Writes

void Player::takeDamage(int damage) { life -= damage; }

// Reads

std::string Player::toString() const { return name + " - Life: " + std::to_string(life) + ")"; }

// Private Helpers

std::unique_ptr<Deck> Player::instantiateDeck(const PlayerConfig& config, CardRegistry* registry) {
    std::unique_ptr<Deck> deck = std::make_unique<Deck>();

    for (const auto& [name, quantity] : config.decklist) {
        for (int i = 0; i < quantity; ++i) {
            std::unique_ptr<Card> card = registry->instantiate(name);
            card->owner = this;
            deck->push_back(std::move(card));
        }
    }
    return deck;
}