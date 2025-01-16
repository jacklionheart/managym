#include "managym/agent/observation.h"

#include "managym/agent/action_space.h"
#include "managym/flow/game.h"
#include "managym/state/battlefield.h"
#include "managym/state/zones.h"

#include <fmt/format.h>

Observation::Observation(const Game* game, const ActionSpace* action_space) {
    // Initialize game flow state
    turn_number = game->turn_system->global_turn_count;
    active_player_id = game->activePlayer()->id;
    is_game_over = game->isGameOver();
    action_type = action_space ? action_space->action_type : ActionType::Invalid;
    winner_id = -1;

    // Initialize player states
    for (const auto& player_ptr : game->players) {
        Player* player = player_ptr.get();
        PlayerState state;

        // Basic player state
        state.life_total = player->life;
        state.mana_pool = player->mana_pool;
        state.library_size = game->zones->size(ZoneType::LIBRARY, player);

        // Initialize all zones
        // Hand
        const auto& hand_cards = game->zones->constHand()->cards.at(player);
        for (const Card* card : hand_cards) {
            CardState card_state;
            card_state.card_id = card->id;
            card_state.name = card->name;
            card_state.tapped = false;
            card_state.summoning_sick = false;
            card_state.damage = 0;
            card_state.attacking = false;
            card_state.blocking = false;
            state.zones[ZoneType::HAND].push_back(card_state);
        }

        // Battlefield
        const auto& player_permanents = game->zones->constBattlefield()->permanents.at(player);
        for (const auto& permanent : player_permanents) {
            CardState card_state;
            card_state.card_id = permanent->card->id;
            card_state.instance_id = permanent->id;
            card_state.name = permanent->card->name;
            card_state.tapped = permanent->tapped;
            card_state.summoning_sick = permanent->summoning_sick;
            card_state.damage = permanent->damage;
            card_state.attacking = permanent->attacking;
            card_state.blocking = false; // TODO: Add blocking state to Permanent
            state.zones[ZoneType::BATTLEFIELD].push_back(card_state);
        }

        // Other zones can be added similarly if needed
        player_states.push_back(state);
    }
}

std::string Observation::CardState::toJSON() const {
    return fmt::format(R"({{
        "card_id": {},
        "instance_id": {},
        "name": "{}",
        "tapped": {},
        "summoning_sick": {},
        "damage": {},
        "attacking": {},
        "blocking": {}
    }})",
        card_id, 
        instance_id ? std::to_string(*instance_id) : "null",
        name,
        tapped ? "true" : "false",
        summoning_sick ? "true" : "false", 
        damage,
        attacking ? "true" : "false",
        blocking ? "true" : "false");
}

std::string Observation::PlayerState::toJSON() const {
    // Convert zones to JSON
    std::string zones_json = "{";
    bool first_zone = true;
    for (const auto& [zone_type, cards] : zones) {
        if (!first_zone) {
            zones_json += ",";
        }
        first_zone = false;

        zones_json += fmt::format(R"("{}": [)", static_cast<int>(zone_type));
        
        // Add cards for this zone
        for (size_t i = 0; i < cards.size(); ++i) {
            if (i > 0) {
                zones_json += ",";
            }
            zones_json += cards[i].toJSON();
        }
        zones_json += "]";
    }
    zones_json += "}";

    return fmt::format(R"({{
        "life_total": {},
        "library_size": {},
        "mana_pool": "{}",
        "zones": {}
    }})",
        life_total,
        library_size,
        mana_pool.toString(),
        zones_json);
}

std::string Observation::toJSON() const {
    // Convert player states to JSON array
    std::string player_states_json = "[";
    for (size_t i = 0; i < player_states.size(); ++i) {
        if (i > 0)
            player_states_json += ",";
        player_states_json += player_states[i].toJSON();
    }
    player_states_json += "]";

    return fmt::format(R"({{
        "turn_number": {},
        "active_player_id": {},
        "is_game_over": {},
        "winner_id": {},
        "action_type": "{}",
        "player_states": {}
    }})",
        turn_number,
        active_player_id,
        is_game_over ? "true" : "false",
        winner_id == -1 ? "null" : std::to_string(winner_id),
        static_cast<int>(action_type),
        player_states_json);
}