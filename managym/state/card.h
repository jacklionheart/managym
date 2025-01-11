#pragma once

#include "mana.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

// Forward declarations
class Game;
class Permanent;
class Zone;
class Player;

// MR113.1 An ability can be a characteristic an object has that lets it affect the game.
// An object's abilities are defined by its rules text or by the effect that created it.
// Abilities can also be granted to objects by rules or effects. (Effects that grant abilities
// usually use the words "has," "have," "gains," or "gain.") Abilities generate effects. (See rule
// 609, "Effects.")

// MR113.3b. Activated abilities have a cost and an effect. They are written as "[Cost]: [Effect.]
// [Activation instructions (if any).]" A player may activate such an ability whenever they have
// priority. Doing so puts it on the stack, where it remains until it's countered, it resolves, or
// it otherwise leaves the stack. See rule 602, "Activating Activated Abilities."
struct ActivatedAbility {
    // Data
    static int next_id;
    int id;
    bool uses_stack;

    ActivatedAbility();
    virtual ~ActivatedAbility() = default;

    // Writes
    // Pay costs required to activate this ability
    virtual void payCost(Permanent* permanent) = 0;
    // Apply this ability's effect
    virtual void resolve(Permanent* permanent) = 0;
    // Check if this ability can be activated
    virtual bool canBeActivated(const Permanent* permanent) const = 0;
};

// An activated ability that produces mana when resolved
struct ManaAbility : public ActivatedAbility {
    // Data
    Mana mana;

    ManaAbility(Mana mana);

    // Writes
    void payCost(Permanent* permanent) override;
    void resolve(Permanent* permanent) override;

    // Reads
    bool canBeActivated(const Permanent* permanent) const override;
};

// Core card types available in the game
enum struct CardType {
    CREATURE,
    INSTANT,
    SORCERY,
    PLANESWALKER,
    LAND,
    ENCHANTMENT,
    ARTIFACT,
    KINDRED,
    BATTLE
};

// Collection of types that define a card's characteristics
struct CardTypes {
    // Data
    std::set<CardType> types;

    CardTypes() = default;
    CardTypes(const std::set<CardType>& types);

    // Reads
    bool isCastable() const;
    bool isPermanent() const;
    bool isNonLandPermanent() const;
    bool isNonCreaturePermanent() const;
    bool isSpell() const;
    bool isCreature() const;
    bool isLand() const;
    bool isPlaneswalker() const;
    bool isEnchantment() const;
    bool isArtifact() const;
    bool isTribal() const;
    bool isBattle() const;
};

// Represents a single Magic card with all its characteristics
struct Card {
    // Data
    static int next_id;
    int id;
    std::string name;
    std::optional<ManaCost> mana_cost;
    Colors colors;
    CardTypes types;
    std::vector<std::string> supertypes;
    std::vector<std::string> subtypes;
    std::vector<ManaAbility> mana_abilities;
    std::string text_box;
    std::optional<int> power;
    std::optional<int> toughness;
    Player* owner;

    Card(const std::string name, std::optional<ManaCost> mana_cost, CardTypes types,
         const std::vector<std::string>& supertypes, const std::vector<std::string>& subtypes,
         const std::vector<ManaAbility>& mana_abilities, std::string text_box,
         std::optional<int> power, std::optional<int> toughness);
    Card(const Card& other);

    // Reads

    // Get string representation of card
    std::string toString() const;
    // Compare cards by ID
    bool operator==(const Card* other) const;
};

// A deck is a collection of cards.
using Deck = std::vector<std::unique_ptr<Card>>;
