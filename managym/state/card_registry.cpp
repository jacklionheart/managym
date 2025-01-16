#include "card_registry.h"

#include "managym/cardsets/alpha/alpha.h"
#include "managym/cardsets/basics/basic_lands.h"

#include <format>
#include <stdexcept>

CardRegistry& CardRegistry::instance() {
    static CardRegistry registry;
    return registry;
}

// Writes
void CardRegistry::registerCard(const std::string& name, const Card& card) {
    if (card_map.find(name) != card_map.end()) {
        throw std::runtime_error(std::format("Card already registered: {}", name));
    }
    card_map.insert({name, std::make_unique<Card>(card)});
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

// Global registration functions
void registerAllCards() {
    registerBasicLands();
    registerAlpha();
}

void clearCardRegistry() { CardRegistry::instance().clear(); }
