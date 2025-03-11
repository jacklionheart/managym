#include "card_registry.h"

#include "managym/cardsets/alpha/alpha.h"
#include "managym/cardsets/basics/basic_lands.h"
#include "managym/infra/log.h"
#include "managym/state/player.h"

#include <stdexcept>

CardRegistry::CardRegistry(IDGenerator* id_generator)
    : id_generator(id_generator), registry_key_generator(std::make_unique<IDGenerator>()) {
    registerAllCards();
}

// Writes
void CardRegistry::registerCard(const std::string& name, const Card& card) {
    if (card_map.find(name) != card_map.end()) {
        throw std::runtime_error(fmt::format("Card already registered: {}", name));
    }
    Card* new_card = new Card(0, card, nullptr);
    new_card->registry_key = registry_key_generator->next();
    card_map.insert({name, std::unique_ptr<Card>(new_card)});
}

void CardRegistry::registerAllCards() {
    managym::cardsets::registerBasicLands(this);
    managym::cardsets::registerAlpha(this);
}

void CardRegistry::clear() { card_map.clear(); }

std::unique_ptr<Card> CardRegistry::instantiate(const std::string& name, Player* owner) {
    if (!owner) {
        throw std::invalid_argument("Cannot instantiate card with null owner");
    }

    auto it = card_map.find(name);
    if (it == card_map.end()) {
        throw std::runtime_error("Card not found in registry: " + name);
    }

    // Create new Card instance with proper owner and ID
    ObjectId new_id = id_generator->next();
    log_debug(LogCat::STATE, "Instantiating card {} (id={}) for player {} (id={})", name, new_id,
              owner->name, owner->id);

    auto card = std::make_unique<Card>(new_id, *it->second, owner);

    // Validate ownership was set correctly
    if (card->owner != owner) {
        throw std::runtime_error(
            fmt::format("Card ownership mismatch during instantiation: expected={}, actual={}",
                        owner->id, card->owner ? card->owner->id : -1));
    }
    if (card->owner->id != owner->id) {
        throw std::runtime_error(
            fmt::format("Card owner ID mismatch during instantiation: expected={}, actual={}",
                        owner->id, card->owner->id));
    }

    return card;
}