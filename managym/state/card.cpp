// card.cpp
#include "card.h"

#include "managym/state/battlefield.h"
#include <utility>

int ActivatedAbility::next_id = 0;

ActivatedAbility::ActivatedAbility() : id(next_id++) {
    uses_stack = true;
}

ManaAbility::ManaAbility(Mana mana) : ActivatedAbility(), mana(std::move(mana)) {
    uses_stack = false;
}

void ManaAbility::payCost(Permanent* permanent) {
    permanent->tap();
}

bool ManaAbility::canBeActivated(const Permanent* permanent) const {
    return permanent->canTap();
}

void ManaAbility::resolve(Permanent* permanent) {
    permanent->controller->mana_pool.add(mana);
}

ManaAbility tapForMana(const std::string& mana_str) {
    return {Mana::parse(mana_str)};
}

CardTypes::CardTypes(const std::set<CardType>& types) : types(types) {}

bool CardTypes::isPermanent() const {
    return types.count(CardType::CREATURE) || types.count(CardType::LAND) || types.count(CardType::ARTIFACT) ||
           types.count(CardType::ENCHANTMENT) || types.count(CardType::PLANESWALKER) || types.count(CardType::BATTLE);
}

bool CardTypes::isNonLandPermanent() const {
    return isPermanent() && !isLand();
}

bool CardTypes::isNonCreaturePermanent() const {
    return isPermanent() && !isCreature();
}

bool CardTypes::isCastable() const {
    return !isLand() && !types.empty();
}

bool CardTypes::isSpell() const {
    return types.contains(CardType::INSTANT) || types.contains(CardType::SORCERY);
}

bool CardTypes::isCreature() const {
    return types.contains(CardType::CREATURE);
}

bool CardTypes::isLand() const {
    return types.contains(CardType::LAND);
}

bool CardTypes::isPlaneswalker() const {
    return types.count(CardType::PLANESWALKER);
}

bool CardTypes::isEnchantment() const {
    return types.contains(CardType::ENCHANTMENT);
}

bool CardTypes::isArtifact() const {
    return types.contains(CardType::ARTIFACT);
}

bool CardTypes::isTribal() const {
    return types.contains(CardType::KINDRED);
}

bool CardTypes::isBattle() const {
    return types.contains(CardType::BATTLE);
}

int Card::next_id = 0;

Card::Card(std::string name, std::optional<ManaCost> mana_cost, CardTypes types,
           const std::vector<std::string>& supertypes, const std::vector<std::string>& subtypes,
           const std::vector<ManaAbility>& mana_abilities, std::string text_box, std::optional<int> power,
           std::optional<int> toughness)
    : id(next_id++), name(std::move(name)), mana_cost(mana_cost), types(std::move(types)), supertypes(supertypes),
      subtypes(subtypes), mana_abilities(mana_abilities), text_box(std::move(text_box)), power(power),
      toughness(toughness), owner(nullptr) {

    if (mana_cost.has_value()) {
        colors = mana_cost->colors();
    } else {
        colors = Colors();
    }
}

Card::Card(const Card& other)
    : id(next_id++), name(other.name), mana_cost(other.mana_cost), colors(other.colors), types(other.types),
      supertypes(other.supertypes), subtypes(other.subtypes), mana_abilities(other.mana_abilities),
      text_box(other.text_box), power(other.power), toughness(other.toughness), owner(nullptr) {}

bool Card::operator==(const Card* other) const {
    return this->id == other->id;
}

std::string Card::toString() const {
    return "{name: " + name + "}";
}