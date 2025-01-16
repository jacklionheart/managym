// basic_lands.h
#pragma once

#include "managym/state/card.h"

Card createBasicLandCard(const std::string& name, Color color);

// Basic lands
Card basicPlains();
Card basicIsland();
Card basicMountain();
Card basicForest();
Card basicSwamp();

void registerBasicLands();