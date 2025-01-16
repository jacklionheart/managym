// basic_lands.cpp
#include "basic_lands.h"

#include "managym/state/card_registry.h"

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

void registerBasicLands() {
    CardRegistry::instance().registerCard("Plains", basicPlains());
    CardRegistry::instance().registerCard("Island", basicIsland());
    CardRegistry::instance().registerCard("Mountain", basicMountain());
    CardRegistry::instance().registerCard("Forest", basicForest());
    CardRegistry::instance().registerCard("Swamp", basicSwamp());
}
