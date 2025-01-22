// basic_lands.h
#pragma once

#include "managym/state/card.h"

class CardRegistry;

namespace managym::cardsets {

// Basic lands
Card basicPlains(ObjectId registry_key);
Card basicIsland(ObjectId registry_key);
Card basicMountain(ObjectId registry_key);
Card basicForest(ObjectId registry_key);
Card basicSwamp(ObjectId registry_key);

// Helper function to create basic land cards
Card createBasicLandCard(const std::string& name, Color color);

void registerBasicLands(CardRegistry* registry);

} // namespace managym::cardsets