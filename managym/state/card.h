#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "mana.h"

// Forward declarations
class Game;
class Permanent;
class Zone;
class Player;

struct ActivatedAbility {
    static int next_id;
    int id;
    bool uses_stack;

    ActivatedAbility();

    virtual void payCost(Permanent* permanent) = 0;
    virtual bool canBeActivated(const Permanent* permanent) const = 0;
    virtual void resolve(Permanent* permanent) = 0;
    virtual ~ActivatedAbility() = default;
};

struct ManaAbility : public ActivatedAbility {
    Mana mana;

    ManaAbility(Mana mana);

    void payCost(Permanent* permanent) override;
    bool canBeActivated(const Permanent* permanent) const override;
    void resolve(Permanent* permanent) override;
};

enum struct CardType { CREATURE, INSTANT, SORCERY, PLANESWALKER, LAND, ENCHANTMENT, ARTIFACT, KINDRED, BATTLE };

struct CardTypes {
    std::set<CardType> types;
    CardTypes() = default;
    CardTypes(const std::set<CardType>& types);
    [[nodiscard]] bool isCastable() const;
    [[nodiscard]] bool isPermanent() const;
    [[nodiscard]] bool isNonLandPermanent() const;
    [[nodiscard]] bool isNonCreaturePermanent() const;
    [[nodiscard]] bool isSpell() const;
    [[nodiscard]] bool isCreature() const;
    [[nodiscard]] bool isLand() const;
    [[nodiscard]] bool isPlaneswalker() const;
    [[nodiscard]] bool isEnchantment() const;
    [[nodiscard]] bool isArtifact() const;
    [[nodiscard]] bool isTribal() const;
    [[nodiscard]] bool isBattle() const;
};

struct Card {
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
    std::optional<int> power; // Use std::optional for optional values
    std::optional<int> toughness;
    Player* owner;

    [[nodiscard]] std::string toString() const;
    bool operator==(const Card* other) const;

    Card(const std::string name, std::optional<ManaCost> mana_cost, CardTypes types,
         const std::vector<std::string>& supertypes, const std::vector<std::string>& subtypes,
         const std::vector<ManaAbility>& mana_abilities, std::string text_box, std::optional<int> power,
         std::optional<int> toughness);

    Card(const Card& other);
};

using Deck = std::vector<std::unique_ptr<Card>>;
