#include "zones.h"
Zones::Zones(std::vector<Player*>& players)
    : graveyard(std::make_unique<Graveyard>(this, players)), hand(std::make_unique<Hand>(this, players)),
      library(std::make_unique<Library>(this, players)), battlefield(std::make_unique<Battlefield>(this, players)),
      stack(std::make_unique<Stack>(this, players)), exile(std::make_unique<Exile>(this, players)),
      command(std::make_unique<Command>(this, players)) {}

void Zones::move(Card* card, Zone* zone) {
    Zone* previous = card_to_zone[card];
    if (previous) {
        previous->remove(card);
    }
    zone->add(card);
    card_to_zone[card] = zone;
}