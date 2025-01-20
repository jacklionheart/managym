// basic_lands.cpp
#include "managym/cardsets/basics/basic_lands.h"

#include "managym/cardsets/card_registry.h"

namespace managym::cardsets {
Card createBasicLandCard(const std::string& name, Color color) {
    return Card(name,
                std::nullopt, // No mana cost for basic lands
                CardTypes({CardType::LAND}), {"basic"}, {name}, {ManaAbility(Mana::single(color))},
                "{T}: Add {" + toString(color) + "}.",
                std::nullopt, // No power
                std::nullopt);
}
Card basicPlains() { return createBasicLandCard("Plains", Color::WHITE); }

Card basicIsland() { return createBasicLandCard("Island", Color::BLUE); }

Card basicMountain() { return createBasicLandCard("Mountain", Color::RED); }

Card basicForest() { return createBasicLandCard("Forest", Color::GREEN); }

Card basicSwamp() { return createBasicLandCard("Swamp", Color::BLACK); }

void registerBasicLands(CardRegistry* registry) {
    registry->registerCard("Plains", basicPlains());
    registry->registerCard("Island", basicIsland());
    registry->registerCard("Mountain", basicMountain());
    registry->registerCard("Forest", basicForest());
    registry->registerCard("Swamp", basicSwamp());
}

} // namespace managym::cardsets