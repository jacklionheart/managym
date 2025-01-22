#include "card.h"

#include "managym/state/battlefield.h"

#include <utility>

// ActivatedAbility

ActivatedAbility::ActivatedAbility() : uses_stack(true) {}

ManaAbility::ManaAbility(Mana mana) : ActivatedAbility(), mana(std::move(mana)) {
    uses_stack = false;
}

// ManaAbility

bool ManaAbility::canBeActivated(const Permanent* permanent) const { return permanent->canTap(); }

// Writes

void ManaAbility::payCost(Permanent* permanent) { permanent->tap(); }

void ManaAbility::resolve(Permanent* permanent) { permanent->controller->mana_pool.add(mana); }

// Reads

// CardTypes

CardTypes::CardTypes(const std::set<CardType>& types) : types(types) {}

// Reads
bool CardTypes::isPermanent() const {
    return types.count(CardType::CREATURE) || types.count(CardType::LAND) ||
           types.count(CardType::ARTIFACT) || types.count(CardType::ENCHANTMENT) ||
           types.count(CardType::PLANESWALKER) || types.count(CardType::BATTLE);
}

bool CardTypes::isNonLandPermanent() const { return isPermanent() && !isLand(); }

bool CardTypes::isNonCreaturePermanent() const { return isPermanent() && !isCreature(); }

bool CardTypes::isCastable() const { return !isLand() && !types.empty(); }

bool CardTypes::isSpell() const {
    return types.contains(CardType::INSTANT) || types.contains(CardType::SORCERY);
}

bool CardTypes::isCreature() const { return types.contains(CardType::CREATURE); }

bool CardTypes::isLand() const { return types.contains(CardType::LAND); }

bool CardTypes::isPlaneswalker() const { return types.count(CardType::PLANESWALKER); }

bool CardTypes::isEnchantment() const { return types.contains(CardType::ENCHANTMENT); }

bool CardTypes::isArtifact() const { return types.contains(CardType::ARTIFACT); }

bool CardTypes::isKindred() const { return types.contains(CardType::KINDRED); }

bool CardTypes::isBattle() const { return types.contains(CardType::BATTLE); }

// Card

// Only to be used by cardset to define card implementations.
Card::Card(std::string name, std::optional<ManaCost> mana_cost, CardTypes types,
           const std::vector<std::string>& supertypes, const std::vector<std::string>& subtypes,
           const std::vector<ManaAbility>& mana_abilities, std::string text_box,
           std::optional<int> power, std::optional<int> toughness)
    : GameObject(0), registry_key(0), name(std::move(name)), mana_cost(mana_cost),
      types(std::move(types)), supertypes(supertypes), subtypes(subtypes),
      mana_abilities(mana_abilities), text_box(std::move(text_box)), power(power),
      toughness(toughness), owner(nullptr) {

    if (mana_cost.has_value()) {
        colors = mana_cost->colors();
    } else {
        colors = Colors();
    }
}

// Used by the card registry to instantiate a card from the registry.
Card::Card(ObjectId id, const Card& other)
    : GameObject(id), registry_key(other.registry_key), name(other.name),
      mana_cost(other.mana_cost), colors(other.colors), types(other.types),
      supertypes(other.supertypes), subtypes(other.subtypes), mana_abilities(other.mana_abilities),
      text_box(other.text_box), power(other.power), toughness(other.toughness), owner(nullptr) {}

// Reads
std::string Card::toString() const { return "{name: " + name + "}"; }
