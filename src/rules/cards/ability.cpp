// ability.cpp
#include "ability.h"
#include "rules/zones/battlefield/permanent.h"
#include "rules/engine/game.h"
#include "rules/cards/card.h"

ActivatedAbility::ActivatedAbility(Card& card) : card(card) {
    uses_stack = true;
}

ManaAbility::ManaAbility(const Mana& mana, Card& card) : mana(mana), ActivatedAbility(card) {
    uses_stack = false;
}

void ManaAbility::payCost(Permanent& permanent, Game& game) {
    permanent.tap();
}

bool ManaAbility::canBeActivated(const Permanent& permanent, const Game& game) const {
    return permanent.canTap();
}

void ManaAbility::resolve(Permanent& permanent, Game& game) {
    game.addMana(permanent.controller, mana);
}