#pragma once

#include "managym/state/card.h"
#include "managym/state/mana.h"
#include "managym/state/player.h"
#include "managym/state/zone.h"

#include <map>
#include <memory>
#include <vector>

// A permanent on the battlefield with additional game state
struct Permanent {
    // Data
    static int next_id;
    int id;
    Card* card;
    Player* controller;
    bool tapped = false;
    bool summoning_sick = false;
    int damage = 0;
    bool attacking = false;

    // Create a new permanent from a card
    Permanent(Card* card);

    // Reads
    // Check if this permanent can be tapped
    bool canTap() const;
    // Check if this permanent can attack
    bool canAttack() const;
    // Check if this permanent can block
    bool canBlock() const;
    // Check if this permanent has taken lethal damage
    bool hasLethalDamage() const;
    // Calculate total mana this permanent could produce
    Mana producibleMana() const;
    // Compare permanents by ID
    bool operator==(const Permanent& other) const;

    // Writes
    // Reset this permanent's tapped state
    void untap();
    // Tap this permanent
    void tap();
    // Add damage to this permanent
    void takeDamage(int damage);
    // Remove all damage from this permanent
    void clearDamage();
    // Declare this permanent as an attacker
    void attack();
    // Activate all available mana abilities
    void activateAllManaAbilities();
    // Activate a specific ability on this permanent
    void activateAbility(ActivatedAbility* ability);
};

// Zone representing the main game area where permanents exist
class Battlefield : public Zone {
public:
    Battlefield(Zones* zones, std::vector<Player*>& players);

    // Data
    std::map<Player*, std::vector<std::unique_ptr<Permanent>>> permanents;

    // Reads

    // Get all attacking creatures controlled by a player
    std::vector<Permanent*> attackers(Player* player) const;
    // Get all creatures that can attack for a player
    std::vector<Permanent*> eligibleAttackers(Player* player) const;
    // Get all creatures that can block for a player
    std::vector<Permanent*> eligibleBlockers(Player* player) const;
    // Calculate total mana available to a player
    Mana producibleMana(Player* player) const;
    // Find a permanent by the corresponding card.
    Permanent* find(const Card* card) const;

protected:
    friend class Zones;

    // Writes
    // Move a permanent to the graveyard
    void destroy(Permanent* permanent);
    // Apply lambda to all permanents on battlefield
    void forEachAll(const std::function<void(Permanent*)>& func);
    // Apply lambda to all permanents controlled by a player.
    void forEach(const std::function<void(Permanent*)>& func, Player* player);
    // Tap permanents to produce required mana.
    void produceMana(const ManaCost& mana_cost, Player* player);
    // Add a card to the battlefield as a permanent.
    void enter(Card* card) override;
    // Remove a card from the battlefield.
    void exit(Card* card) override;
};
