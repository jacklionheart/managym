#include "observation.h"

#include "managym/flow/game.h"
#include "managym/state/mana.h"

// 3rd Party
#include <fmt/format.h>

// Built-ins
#include <sstream>

// ------------------- Observation -------------------

// Constructor: empty
Observation::Observation() : game_over(false), won(false) {
    // No-op
}

// Constructor: builds from a Game*
Observation::Observation(const Game* game) {
    // Basic flags
    if (game->isGameOver()) {
        game_over = true;
        // "won" can be true if current player is winner, or if we just
        // store global winner. Adjust as needed.
        // For example, if there's a getWinnerId() or winnerIndex():
        int winner_idx = game->winnerIndex();
        won = (winner_idx != -1 && game->players[winner_idx].get() == game->activePlayer());
    }

    // Turn data
    populateTurn(game);

    // Action space
    populateActionSpace(game);

    // Players
    populatePlayers(game);

    // Objects
    populateAllObjects(game);
}

// --------------------------------------------------
// Data
// --------------------------------------------------

// Populate the turn data
void Observation::populateTurn(const Game* game) {
    TurnData turn_data;
    turn_data.turn_number = game->turn_system->global_turn_count;
    turn_data.phase = game->turn_system->currentPhaseType();
    turn_data.step = game->turn_system->currentStepType();

    if (game->activePlayer() != nullptr) {
        turn_data.active_player_id = static_cast<int>(game->activePlayer()->id);
    } else {
        turn_data.active_player_id = -1;
    }

    if (game->current_action_space != nullptr && game->current_action_space->player != nullptr) {
        turn_data.agent_player_id = static_cast<int>(game->current_action_space->player->id);
    } else {
        turn_data.agent_player_id = -1;
    }

    turn = turn_data;
}

// Populate the action space
void Observation::populateActionSpace(const Game* game) {
    if (game->current_action_space == nullptr) {
        // If there's no action space, mark it as GAME_OVER or similar
        action_space.action_space_type = ActionSpaceType::GAME_OVER;
        return;
    }

    action_space.action_space_type = game->current_action_space->type;

    // Copy each available action
    const std::vector<std::unique_ptr<Action>>& game_actions = game->current_action_space->actions;
    for (size_t i = 0; i < game_actions.size(); i++) {
        ActionOption opt;
        opt.action_type = game_actions[i]->type;
        opt.focus = game_actions[i]->focus();
        action_space.actions.push_back(opt);
    }
}

// Populate all PlayerData
void Observation::populatePlayers(const Game* game) {
    // The game typically has a vector of unique_ptr<Player>.
    // We'll gather them into a map<int, PlayerData>, keyed by player->id
    for (int i = 0; i < static_cast<int>(game->players.size()); i++) {
        const Player* p = game->players[i].get();
        if (p == nullptr) {
            continue;
        }

        PlayerData pdat;
        pdat.id = static_cast<int>(p->id);
        pdat.player_index = i;
        pdat.is_active = (p == game->activePlayer());
        if (game->current_action_space != nullptr && game->current_action_space->player == p) {
            pdat.is_agent = true;
        } else {
            pdat.is_agent = false;
        }
        pdat.life = p->life;

        // Populate zone_counts
        // Suppose the Game or Zones system has a way to count how many
        // objects or cards are in each zone for p.
        // We'll do something like:
        for (int z = 0; z < 7; z++) {
            pdat.zone_counts[z] = game->zones->size(static_cast<ZoneType>(z), p);
        }

        players[pdat.id] = pdat;
    }
}

// Populate all object types
void Observation::populateAllObjects(const Game* game) {
    const Player* active_player = game->activePlayer();
    const Player* opponent = game->nonActivePlayer();

    // 1) HAND: visible only to the owner (observer)
    const Hand* hand = game->zones->constHand();
    for (const Card* card : hand->cards.at(active_player)) {
        addCard(card, ZoneType::HAND);
    }

    // 2) BATTLEFIELD: gather for all players
    const Battlefield* bf = game->zones->constBattlefield();
    for (const std::unique_ptr<Permanent>& perm : bf->permanents.at(active_player)) {
        addPermanent(perm.get());
    }

    // 3) GRAVEYARD: gather for all players
    const Graveyard* gy = game->zones->constGraveyard();
    for (const Player* player : game->priorityOrder()) {
        for (const Card* card : gy->cards.at(player)) {
            addCard(card, ZoneType::GRAVEYARD);
        }
    }

    // 4) EXILE: gather for all players
    const Exile* ex = game->zones->constExile();
    for (const Player* player : game->priorityOrder()) {
        for (const Card* card : ex->cards.at(player)) {
            addCard(card, ZoneType::EXILE);
        }
    }

    // 5) STACK
    const Stack* stack = game->zones->constStack();
    // 0: top of stack
    for (int i = stack->totalSize() - 1; i >= 0; i--) {
        addCard(stack->objects[i], ZoneType::STACK);
    }
}

// Helper to add a Card to objects & cards
void Observation::addCard(const Card* card, ZoneType zone) {
    CardData cdata;
    cdata.id = static_cast<int>(card->id);
    cdata.registry_key = static_cast<int>(card->registry_key);
    ManaCost mc = ManaCost();
    if (card->mana_cost.has_value()) {
        mc = card->mana_cost.value();
    } else {
        mc = ManaCost();
        mc.mana_value = -1;
    }
    cdata.mana_cost = mc;

    cdata.card_types.is_castable = card->types.isCastable();
    cdata.card_types.is_permanent = card->types.isPermanent();
    cdata.card_types.is_non_land_permanent = card->types.isNonLandPermanent();
    cdata.card_types.is_non_creature_permanent = card->types.isNonCreaturePermanent();
    cdata.card_types.is_spell = card->types.isSpell();
    cdata.card_types.is_creature = card->types.isCreature();
    cdata.card_types.is_land = card->types.isLand();
    cdata.card_types.is_planeswalker = card->types.isPlaneswalker();
    cdata.card_types.is_enchantment = card->types.isEnchantment();
    cdata.card_types.is_artifact = card->types.isArtifact();
    cdata.card_types.is_kindred = card->types.isKindred();
    cdata.card_types.is_battle = card->types.isBattle();

    cards[cdata.id] = cdata;
}

// Helper to add a Permanent to objects & permanents (and also to cards)
void Observation::addPermanent(const Permanent* perm) {

    // Also fill PermanentData
    PermanentData pdat;
    pdat.id = perm->id;
    pdat.tapped = perm->tapped;
    pdat.damage = perm->damage;
    pdat.is_summoning_sick = perm->summoning_sick;

    permanents[perm->id] = pdat;

    addCard(perm->card, ZoneType::BATTLEFIELD);
}

// --------------------------------------------------
// Reads
// --------------------------------------------------
bool Observation::validate() const {
    // 1) Turn data check
    if (turn.turn_number < 0) {
        return false;
    }

    // 2) Players
    // Check that the map key matches the player's id field
    for (std::map<int, PlayerData>::const_iterator it = players.begin(); it != players.end();
         ++it) {
        int pid = it->first;
        const PlayerData& pdata = it->second;
        if (pdata.id != pid) {
            return false;
        }
    }

    // 4) Cards
    for (std::map<int, CardData>::const_iterator it = cards.begin(); it != cards.end(); ++it) {
        int cid = it->first;
        const CardData& cdat = it->second;
        if (cdat.id != cid) {
            return false;
        }
    }

    // 5) Permanents
    for (std::map<int, PermanentData>::const_iterator it = permanents.begin();
         it != permanents.end(); ++it) {
        int pid = it->first;
        const PermanentData& pdat = it->second;
        if (pdat.id != pid) {
            return false;
        }
    }

    // If we got here, it appears consistent
    return true;
}

// Convert to a JSON-like string for debugging or serialization
std::string Observation::toJSON() const {
    std::ostringstream out;
    out << "{\n";

    // game_over / won
    out << fmt::format(R"(  "game_over": {},)", (game_over ? "true" : "false")) << "\n";
    out << fmt::format(R"(  "won": {},)", (won ? "true" : "false")) << "\n";

    // Turn
    out << R"(  "turn": {)" << "\n";
    out << fmt::format(R"(    "turn_number": {},)", turn.turn_number) << "\n";
    out << fmt::format(R"(    "phase": {},)", static_cast<int>(turn.phase)) << "\n";
    out << fmt::format(R"(    "step": {},)", static_cast<int>(turn.step)) << "\n";
    out << fmt::format(R"(    "active_player_id": {},)", turn.active_player_id) << "\n";
    out << fmt::format(R"(    "agent_player_id": {})", turn.agent_player_id) << "\n";
    out << "  },\n";

    // ActionSpace
    out << R"(  "action_space": {)" << "\n";
    out << fmt::format(R"(    "action_space_type": {},)",
                       static_cast<int>(action_space.action_space_type))
        << "\n";
    // actions
    out << R"(    "actions": [)" << "\n";
    for (size_t i = 0; i < action_space.actions.size(); i++) {
        const ActionOption& act = action_space.actions[i];
        out << "      {\n";
        out << fmt::format(R"(        "action_type": {},)", static_cast<int>(act.action_type))
            << "\n";
        out << R"(        "focus": [)";
        for (size_t j = 0; j < act.focus.size(); j++) {
            out << act.focus[j];
            if (j + 1 < act.focus.size()) {
                out << ",";
            }
        }
        out << "]\n      }";
        if (i + 1 < action_space.actions.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "    ],\n";

    // focus
    out << R"(    "focus": [)";
    for (size_t i = 0; i < action_space.focus.size(); i++) {
        out << action_space.focus[i];
        if (i + 1 < action_space.focus.size()) {
            out << ",";
        }
    }
    out << "]\n  },\n";

    // Helper lambda to serialize a map<int, T>
    auto serializeMap = [&](const std::string& name, auto& mp, auto formatter) {
        out << fmt::format(R"(  "{}": {{)", name) << "\n";
        bool first_item = true;
        for (typename std::decay<decltype(mp)>::type::const_iterator it = mp.begin();
             it != mp.end(); ++it) {
            int the_id = it->first;
            const auto& val = it->second;
            if (!first_item) {
                out << ",\n";
            }
            first_item = false;
            formatter(the_id, val);
        }
        out << "\n  }";
    };

    // players
    serializeMap("players", players, [&](int pid, const PlayerData& p) {
        out << fmt::format(R"(    "{}": {{)", pid) << "\n";
        out << fmt::format(R"(      "id": {},)", p.id) << "\n";
        out << fmt::format(R"(      "player_index": {},)", p.player_index) << "\n";
        out << fmt::format(R"(      "is_active": {},)", (p.is_active ? "true" : "false")) << "\n";
        out << fmt::format(R"(      "is_agent": {},)", (p.is_agent ? "true" : "false")) << "\n";
        out << fmt::format(R"(      "life": {},)", p.life) << "\n";
        // zone_counts
        out << R"(      "zone_counts": [)";
        for (size_t i = 0; i < p.zone_counts.size(); i++) {
            out << p.zone_counts[i];
            if (i + 1 < p.zone_counts.size()) {
                out << ",";
            }
        }
        out << "],\n";
        out << "]\n    }";
    });
    out << ",\n";

    // cards
    serializeMap("cards", cards, [&](int cid, const CardData& cdat) {
        out << fmt::format(R"(    "{}": {{)", cid) << "\n";
        out << fmt::format(R"(      "id": {},)", cdat.id) << "\n";
        out << fmt::format(R"(      "registry_key": {},)", cdat.registry_key) << "\n";
        out << R"(      "mana_cost": {)" << "\n";
        out << R"(        "cost": [)";
        for (size_t i = 0; i < cdat.mana_cost.cost.size(); i++) {
            out << cdat.mana_cost.cost[i];
            if (i + 1 < cdat.mana_cost.cost.size()) {
                out << ",";
            }
        }
        out << "],\n";
        out << fmt::format(R"(        "mana_value": {})", cdat.mana_cost.mana_value) << "\n";
        out << "      }\n    }";
    });
    out << ",\n";

    // permanents
    serializeMap("permanents", permanents, [&](int pid, const PermanentData& pdat) {
        out << fmt::format(R"(    "{}": {{)", pid) << "\n";
        out << fmt::format(R"(      "id": {},)", pdat.id) << "\n";
        out << fmt::format(R"(      "tapped": {},)", (pdat.tapped ? "true" : "false")) << "\n";
        out << fmt::format(R"(      "damage": {},)", pdat.damage) << "\n";
        out << fmt::format(R"(      "is_summoning_sick": {})",
                           (pdat.is_summoning_sick ? "true" : "false"))
            << "\n";
        out << "    }";
    });
    out << ",\n";

    out << "\n}\n";

    return out.str();
}
