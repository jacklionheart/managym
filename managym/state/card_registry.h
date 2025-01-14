// card_registry.h
#pragma once

#include "managym/state/card.h"

#include <map>
#include <memory>
#include <string>

// Central registry for card definitions and instantiation
class CardRegistry {
public:
    // Get singleton instance of registry
    static CardRegistry& instance();

    // Register a new card definition
    void registerCard(const std::string& name, const Card& card);

    // Create a new instance of a registered card
    std::unique_ptr<Card> instantiate(const std::string& name);

    // Clear all registered cards
    void clear();

    // Delete copy constructor and assignment to enforce singleton
    CardRegistry(const CardRegistry&) = delete;
    CardRegistry& operator=(const CardRegistry&) = delete;

private:
    CardRegistry() = default;
    std::map<std::string, std::unique_ptr<Card>> card_map;
};

// Register all available cards in the game
void registerAllCards();

// Clear the card registry
void clearCardRegistry();
