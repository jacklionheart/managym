// battlefield.cpp
#include "battlefield.h"
#include "rules/engine/game.h"
#include "rules/zones/battlefield/permanent.h"

Battlefield::Battlefield(Game& game)
    : Zone(game) {
    // Initialize the permanents map for each player
    for (const Player& player : game.players) {
        cards[player.id] = std::vector<Card*>();
        permanents[player.id] = std::vector<std::unique_ptr<Permanent>>();
    }
}

void Battlefield::add(Card& card) {
    enter(card);     
}


void Battlefield::enter(Card& card) {
    if (!card.types.isPermanent()) {
        throw std::invalid_argument("Card is not a permanent: " + card.toString());
    }
    Zone::add(card);
    Player& controller = card.owner;
    permanents[controller.id].push_back(std::make_unique<Permanent>(card));
}

void Battlefield::remove(Card& card) {
    Zone::remove(card);
    Player& controller = card.owner;
    permanents[controller.id].erase(
        std::remove_if(permanents[controller.id].begin(), permanents[controller.id].end(),
            [&card](const std::unique_ptr<Permanent>& permanent) { return &permanent->card == &card; }),
        permanents[controller.id].end());
}

Mana Battlefield::producableMana(Player& player) const {
    Mana total_mana;
    auto it = permanents.find(player.id);
    if (it != permanents.end()) {
        for (const auto& permanent : it->second) {
            total_mana.add(permanent->producableMana(game));
        }
    }
    return total_mana;
}

void Battlefield::produceMana(const ManaCost& mana_cost, Player& player) {
    Mana producable = producableMana(player);
    if (!producable.canPay(mana_cost)) {
        throw std::runtime_error("Not enough producable mana to pay for mana cost.");
    }

    std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents[player.id];
    for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
        if (!game.mana_pools[player.id].canPay(mana_cost)) {
            permanent->activateAllManaAbilities(game);
        } else {
            break;
        }
    }

    if (!game.mana_pools[player.id].canPay(mana_cost)) {
        throw std::runtime_error("Did not generate enough mana to pay for mana cost.");
    }
}