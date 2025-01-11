// zones.cpp
#include "zones.h"

#include <spdlog/spdlog.h>

Zones::Zones(std::vector<Player*>& players)
    : library(std::make_unique<Library>(this, players)),
      graveyard(std::make_unique<Graveyard>(this, players)),
      hand(std::make_unique<Hand>(this, players)),
      battlefield(std::make_unique<Battlefield>(this, players)),
      stack(std::make_unique<Stack>(this, players)), exile(std::make_unique<Exile>(this, players)),
      command(std::make_unique<Command>(this, players)) {}

// Zone accessors (for read-only full acc

const Library* Zones::constLibrary() const { return library.get(); }
const Graveyard* Zones::constGraveyard() const { return graveyard.get(); }
const Hand* Zones::constHand() const { return hand.get(); }
const Battlefield* Zones::constBattlefield() const { return battlefield.get(); }
const Stack* Zones::constStack() const { return stack.get(); }
const Exile* Zones::constExile() const { return exile.get(); }
const Command* Zones::constCommand() const { return command.get(); }

Zone* Zones::getZone(ZoneType zoneType) const {
    switch (zoneType) {
    case ZoneType::LIBRARY:
        return library.get();
    case ZoneType::GRAVEYARD:
        return graveyard.get();
    case ZoneType::HAND:
        return hand.get();
    case ZoneType::BATTLEFIELD:
        return battlefield.get();
    case ZoneType::STACK:
        return stack.get();
    case ZoneType::EXILE:
        return exile.get();
    case ZoneType::COMMAND:
        return command.get();
    }
    throw std::runtime_error("Unknown ZoneType in getZone()");
}

bool Zones::contains(const Card* card, ZoneType zone, Player* player) const {
    if (!card) {
        return false;
    }
    Zone* zoneObj = getZone(zone);
    return zoneObj->contains(card, player);
}

Card* Zones::top(ZoneType zoneType, Player* player) const {
    Zone* zoneObj = getZone(zoneType);
    return zoneObj->top(player);
}

size_t Zones::size(ZoneType zoneType, Player* player) const {
    Zone* zoneObj = getZone(zoneType);
    return zoneObj->size(player);
}

size_t Zones::totalSize(ZoneType zoneType) const {
    Zone* zoneObj = getZone(zoneType);
    return zoneObj->totalSize();
}

// Writes

void Zones::move(Card* card, ZoneType toZone) {
    if (!card) {
        throw std::invalid_argument("move() called with null Card*");
    }

    Zone* newZone = getZone(toZone);

    auto it = card_to_zone.find(card);
    if (it != card_to_zone.end()) {
        Zone* oldZone = it->second;
        oldZone->exit(card);
    }

    newZone->enter(card);
    card_to_zone[card] = newZone;
}

Card* Zones::moveTop(ZoneType zoneFrom, ZoneType zoneTo, Player* player) {
    Card* card = top(zoneFrom, player);
    move(card, zoneTo);
    return card;
}

void Zones::shuffle(ZoneType zoneType, Player* player) {
    Zone* zoneObj = getZone(zoneType);
    zoneObj->shuffle(player);
}

void Zones::forEach(const std::function<void(Card*)>& func, ZoneType zoneType, Player* player) {
    Zone* zoneObj = getZone(zoneType);
    zoneObj->forEach(func, player);
}

void Zones::forEachAll(const std::function<void(Card*)>& func, ZoneType zoneType) {
    Zone* zoneObj = getZone(zoneType);
    zoneObj->forEachAll(func);
}

// Battlefield Mutations

void Zones::destroy(Permanent* permanent) {
    spdlog::info("{} is destroyed", permanent->card->toString());
    move(permanent->card, ZoneType::GRAVEYARD);
}

void Zones::produceMana(const ManaCost& mana_cost, Player* player) {
    battlefield->produceMana(mana_cost, player);
}

void Zones::forEachPermanentAll(const std::function<void(Permanent*)>& func) {
    battlefield->forEachAll(func);
}

void Zones::forEachPermanent(const std::function<void(Permanent*)>& func, Player* player) {
    battlefield->forEach(func, player);
}

// Stack Mutations

void Zones::pushStack(Card* card) { stack->push(card); }

Card* Zones::popStack() { return stack->pop(); }
