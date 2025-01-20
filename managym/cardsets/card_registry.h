// card_registry.h
#pragma once

#include "managym/state/card.h"

#include <map>
#include <memory>
#include <string>

// Central registry for card definitions and instantiation
class CardRegistry {
public:
    CardRegistry();
    // Register all available cards in the game
    void registerAllCards();
    // Register a new card definition
    void registerCard(const std::string& name, const Card& card);

    // Create a new instance of a registered card
    std::unique_ptr<Card> instantiate(const std::string& name);

    // Clear all registered cards
    void clear();

private:
    std::map<std::string, std::unique_ptr<Card>> card_map;
    uint32_t next_id = 0;
};