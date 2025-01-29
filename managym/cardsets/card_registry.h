// card_registry.h
#pragma once

#include "managym/state/card.h"
#include "managym/state/game_object.h"

#include <map>
#include <memory>
#include <string>

// Central registry for card definitions and instantiation
class CardRegistry {
public:
    CardRegistry(IDGenerator* id_generator);
    // Register all available cards in the game
    void registerAllCards();
    // Register a new card definition
    void registerCard(const std::string& name, const Card& card);

    // Create a new instance of a registered card
    std::unique_ptr<Card> instantiate(const std::string& name, Player* owner);

    // Clear all registered cards
    void clear();

private:
    std::map<std::string, std::unique_ptr<Card>> card_map;
    // Unique ID amongg game objects, owned by the Game.
    IDGenerator* id_generator;
    // Card Registry Key for mapping card names to Cards.
    std::unique_ptr<IDGenerator> registry_key_generator;
};
