// card_registry.h
#pragma once

#include <map>
#include <memory>
#include <string>

#include "managym/state/card.h"

class CardRegistry {
  public:
    static CardRegistry& instance();

    void registerCard(const std::string& name, const Card& card);
    std::unique_ptr<Card> instantiate(const std::string& name);

    // Deleting copy constructor and assignment operator to enforce singleton
    CardRegistry(const CardRegistry&) = delete;
    CardRegistry& operator=(const CardRegistry&) = delete;

  private:
    CardRegistry() = default; // Private constructor
    std::map<std::string, std::unique_ptr<Card>> card_map;
};

void registerAllCards();