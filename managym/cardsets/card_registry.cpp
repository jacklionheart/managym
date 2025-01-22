#include "card_registry.h"

#include "managym/cardsets/alpha/alpha.h"
#include "managym/cardsets/basics/basic_lands.h"

#include <format>
#include <stdexcept>

CardRegistry::CardRegistry(IDGenerator* id_generator)
    : id_generator(id_generator), registry_key_generator(std::make_unique<IDGenerator>()) {
    registerAllCards();
}

// Writes
void CardRegistry::registerCard(const std::string& name, const Card& card) {
    if (card_map.find(name) != card_map.end()) {
        throw std::runtime_error(std::format("Card already registered: {}", name));
    }
    Card* new_card = new Card(0, card);
    new_card->registry_key = registry_key_generator->next();
    card_map.insert({name, std::unique_ptr<Card>(new_card)});
}

void CardRegistry::registerAllCards() {
    managym::cardsets::registerBasicLands(this);
    managym::cardsets::registerAlpha(this);
}

void CardRegistry::clear() { card_map.clear(); }

std::unique_ptr<Card> CardRegistry::instantiate(const std::string& name) {
    auto it = card_map.find(name);
    if (it != card_map.end()) {
        // Create a new Card instance based on the stored card
        return std::make_unique<Card>(id_generator->next(), *it->second);
    } else {
        throw std::runtime_error("Card not found in registry: " + name);
    }
}
