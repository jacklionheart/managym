#include "managym/cardsets/alpha/alpha.h"

#include "managym/state/card.h"
#include "managym/state/card_registry.h"

Card llanowarElves() {
    return Card("Llanowar Elves", ManaCost::parse("G"), CardTypes({CardType::CREATURE}), {},
                {"Elf", "Druid"}, {ManaAbility(Mana::single(Color::GREEN))}, "{T}: Add {G}.", 1, 1);
}

Card greyOgre() {
    return Card("Grey Ogre", ManaCost::parse("2R"), CardTypes({CardType::CREATURE}), {}, {"Ogre"},
                {}, "", 2, 2);
}

void registerAlpha() {
    CardRegistry::instance().registerCard("Llanowar Elves", llanowarElves());
    CardRegistry::instance().registerCard("Grey Ogre", greyOgre());
}
