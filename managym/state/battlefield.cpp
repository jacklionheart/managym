#include "battlefield.h"

#include "managym/infra/log.h"
#include "managym/state/zones.h"

#include <cassert>

Permanent::Permanent(ObjectId id, Card* card)
    : GameObject(id), controller(card->owner), card(card) {
    tapped = false;
    summoning_sick = card->types.isCreature();
    damage = 0;
}

// Reads
bool Permanent::canTap() const { return !tapped && !(summoning_sick && card->types.isCreature()); }

bool Permanent::canAttack() const { return card->types.isCreature() && !tapped && !summoning_sick; }

bool Permanent::canBlock() const { return card->types.isCreature() && !tapped; }

bool Permanent::hasLethalDamage() const {
    return card->types.isCreature() && damage >= card->toughness;
}

Mana Permanent::producibleMana() const {
    Mana totalMana;
    for (const ManaAbility& ability : card->mana_abilities) {
        if (ability.canBeActivated(this)) {
            totalMana.add(ability.mana);
        }
    }
    return totalMana;
}

// Writes
void Permanent::untap() { tapped = false; }

void Permanent::tap() {
    log_debug(LogCat::STATE, "Tapping {}", card->toString());
    tapped = true;
}

void Permanent::takeDamage(int dmg) { damage += dmg; }

void Permanent::clearDamage() { damage = 0; }

void Permanent::attack() {
    log_debug(LogCat::STATE, "{} attacks", card->toString());
    attacking = true;
    tap();
}

void Permanent::activateAllManaAbilities() {
    for (ManaAbility& ability : card->mana_abilities) {
        if (ability.canBeActivated(this)) {
            activateAbility(&ability);
        }
    }
}

void Permanent::activateAbility(ActivatedAbility* ability) {
    log_debug(LogCat::STATE, "Activating ability on {}", card->toString());
    assert(ability != nullptr);
    if (!ability->canBeActivated(this)) {
        throw std::logic_error("Ability cannot be activated.");
    }
    ability->payCost(this);
    if (ability->uses_stack) {
        throw std::runtime_error("Implement me.");
        // TODO: Add to stack
        // game->addToStack(ability);
    } else {
        ability->resolve(this);
    }
}

// Battlefield implementation

Battlefield::Battlefield(Zones* zones, std::vector<Player*>& players, IDGenerator* id_generator)
    : Zone(zones, players), id_generator(id_generator) {
    for (Player* player : players) {
        cards[player] = std::vector<Card*>();
        permanents[player] = std::vector<std::unique_ptr<Permanent>>();
    }
}

// Reads
std::vector<Permanent*> Battlefield::attackers(Player* player) const {
    std::vector<Permanent*> attackers;
    const std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents.at(player);
    for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
        if (permanent->attacking) {
            attackers.push_back(permanent.get());
        }
    }
    return attackers;
}

std::vector<Permanent*> Battlefield::eligibleAttackers(Player* player) const {
    std::vector<Permanent*> eligible_attackers;
    const std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents.at(player);
    for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
        if (permanent->canAttack()) {
            eligible_attackers.push_back(permanent.get());
        }
    }
    return eligible_attackers;
}

std::vector<Permanent*> Battlefield::eligibleBlockers(Player* player) const {
    std::vector<Permanent*> eligible_blockers;
    const std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents.at(player);
    for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
        if (permanent->canBlock()) {
            eligible_blockers.push_back(permanent.get());
        }
    }
    return eligible_blockers;
}

Permanent* Battlefield::find(const Card* card) const {
    for (const auto& [player, player_permanents] : permanents) {
        for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
            if (permanent->card == card) {
                return permanent.get();
            }
        }
    }
    return nullptr;
}

Mana Battlefield::producibleMana(Player* player) const {
    Mana total_mana;
    auto it = permanents.find(player);
    if (it != permanents.end()) {
        for (const auto& permanent : it->second) {
            total_mana.add(permanent->producibleMana());
        }
    }
    return total_mana;
}

// Writes
void Battlefield::enter(Card* card) {
    Zone::enter(card);
    if (!card->types.isPermanent()) {
        throw std::invalid_argument("Card is not a permanent: " + card->toString());
    }
    log_info(LogCat::STATE, "{} enters battlefield", card->toString());
    const Player* controller = card->owner;
    permanents[controller].push_back(std::make_unique<Permanent>(id_generator->next(), card));
}

void Battlefield::exit(Card* card) {
    Zone::exit(card);
    const Player* controller = card->owner;
    permanents[controller].erase(
        std::remove_if(permanents[controller].begin(), permanents[controller].end(),
                       [&card](const std::unique_ptr<Permanent>& permanent) {
                           return permanent->card == card;
                       }),
        permanents[controller].end());
}

void Battlefield::forEachAll(const std::function<void(Permanent*)>& func) {
    for (const auto& [player, player_permanents] : permanents) {
        for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
            func(permanent.get());
        }
    }
}

void Battlefield::forEach(const std::function<void(Permanent*)>& func, Player* player) {
    std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents[player];
    for (const std::unique_ptr<Permanent>& permanent : player_permanents) {
        func(permanent.get());
    }
}

void Battlefield::produceMana(const ManaCost& mana_cost, Player* player) {
    log_debug(LogCat::RULES, "Attempting to produce {} for {}", mana_cost.toString(), player->name);

    Mana producible = producibleMana(player);
    log_debug(LogCat::RULES, "Producible mana: {}", producible.toString());

    if (!producible.canPay(mana_cost)) {
        throw std::runtime_error("Not enough producible mana to pay for mana cost.");
    }

    std::vector<std::unique_ptr<Permanent>>& player_permanents = permanents[player];

    for (auto& permanent : player_permanents) {
        if (player->mana_pool.canPay(mana_cost)) {
            break;
        }

        if (!permanent->tapped && !permanent->card->mana_abilities.empty()) {
            permanent->activateAllManaAbilities();
            log_debug(LogCat::RULES, "After activating abilities on {}, mana pool is: {}",
                      permanent->card->toString(), player->mana_pool.toString());
        }
    }

    if (!player->mana_pool.canPay(mana_cost)) {
        throw std::runtime_error("Did not generate enough mana to pay for mana cost.");
    }
}