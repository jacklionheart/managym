// basic_lands.h
#pragma once

#include "managym/state/card.h"

class CardRegistry;

namespace managym::cardsets {

// Basic lands
Card basicPlains();
Card basicIsland();
Card basicMountain();
Card basicForest();
Card basicSwamp();

// Helper function to create basic land cards
Card createBasicLandCard(const std::string& name, Color color);

void registerBasicLands(CardRegistry* registry);

} // namespace managym::cardsets