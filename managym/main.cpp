#include "managym/state/battlefield.h"
#include "managym/state/card.h"
#include "managym/state/card_registry.h"
#include "managym/state/mana.h"
#include "managym/state/player.h"
#include "managym/state/zone.h"
#include "managym/state/zones.h"
#include <iostream>
#include <memory>

void testCardCreation() {
    std::cout << "\nTesting card creation..." << std::endl;
    
    // Create a basic creature card
    Card creature("Test Creature", 
                 ManaCost::parse("2G"),
                 CardTypes({CardType::CREATURE}),
                 {},  // supertypes
                 {"Beast"},  // subtypes
                 {},  // mana abilities
                 "Test creature text",
                 2,  // power
                 3); // toughness
                 
    std::cout << "Created creature: " << creature.toString() << std::endl;
    std::cout << "Mana cost: " << creature.mana_cost->toString() << std::endl;
    std::cout << "Is creature: " << creature.types.isCreature() << std::endl;
    std::cout << "Power/Toughness: " << *creature.power << "/" << *creature.toughness << std::endl;
}

void testManaSystem() {
    std::cout << "\nTesting mana system..." << std::endl;
    
    // Test mana cost parsing
    ManaCost cost = ManaCost::parse("2RG");
    std::cout << "Parsed mana cost: " << cost.toString() << std::endl;
    
    // Test mana pool operations
    Mana pool;
    pool.mana[Color::RED] = 2;
    pool.mana[Color::GREEN] = 1;
    pool.mana[Color::COLORLESS] = 2;
    
    std::cout << "Can pay RG: " << pool.canPay(ManaCost::parse("RG")) << std::endl;
    std::cout << "Can pay 2RG: " << pool.canPay(cost) << std::endl;
    std::cout << "Can pay 3RG: " << pool.canPay(ManaCost::parse("3RG")) << std::endl;
}

void testZoneSystem() {
    std::cout << "\nTesting zone system..." << std::endl;
    
    // Create players
    PlayerConfig p1_config("Player 1", {});
    PlayerConfig p2_config("Player 2", {});
    
    auto p1 = std::make_unique<Player>(p1_config);
    auto p2 = std::make_unique<Player>(p2_config);
    
    std::vector<Player*> players = {p1.get(), p2.get()};
    
    // Create zones
    auto zones = std::make_unique<Zones>(players);
    
    // Create a test card
    auto card = std::make_unique<Card>("Test Card", 
                                     ManaCost::parse("1R"),
                                     CardTypes({CardType::CREATURE}),
                                     {},
                                     {"Warrior"},
                                     {},
                                     "",
                                     2,
                                     2);
    card->owner = p1.get();
    
    // Test zone transitions
    std::cout << "Moving card through zones..." << std::endl;
    
    zones->move(card.get(), zones->hand.get());
    std::cout << "Card in hand" << std::endl;
    
    zones->move(card.get(), zones->battlefield.get());
    std::cout << "Card on battlefield" << std::endl;
    
    auto permanent = zones->battlefield->find(card.get());
    if (permanent) {
        std::cout << "Found permanent on battlefield" << std::endl;
        permanent->tap();
        std::cout << "Permanent tapped: " << permanent->tapped << std::endl;
    }
}

int main() {
    std::cout << "Testing managym state system..." << std::endl;
    
    testCardCreation();
    testManaSystem();
    testZoneSystem();
    
    std::cout << "\nAll tests completed!" << std::endl;
    return 0;
}
