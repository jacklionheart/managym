#include "card_registry.h"

#include "managym/cardsets/alpha/alpha.h"
#include "managym/cardsets/basics/basic_lands.h"

#include <format>
#include <stdexcept>

CardRegistry::CardRegistry() { registerAllCards(); }

// Writes
void CardRegistry::registerCard(const std::string& name, const Card& card) {
    if (card_map.find(name) != card_map.end()) {
        throw std::runtime_error(std::format("Card already registered: {}", name));
    }
    Card* new_card = new Card(card);
    new_card->id = next_id++;
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
        return std::make_unique<Card>(*it->second);
    } else {
        throw std::runtime_error("Card not found in registry: " + name);
    }
}
