// observation.cpp
// Data structures that cross the C++/Python boundary.

#include "observation.h"

#include "managym/flow/game.h"
#include "managym/infra/profiler.h"
#include "managym/state/mana.h"

// 3rd Party
#include <fmt/format.h>

// Built-ins
#include <sstream>

// ------------------- Observation -------------------

// Constructor: empty observation with defaults
Observation::Observation() = default;

// Constructor: builds from a Game*
Observation::Observation(Game* game) {
    Profiler::Scope scope = game->profiler->track("observation");

    // Basic flags
    if (game->isGameOver()) {
        game_over = true;
        int winner_idx = game->winnerIndex();
        // True if the agent won
        won = (winner_idx != -1 && game->players[winner_idx].get() == game->agentPlayer());
    }

    // Build the full state
    populateTurn(game);
    populateActionSpace(game);
    populatePlayers(game);
    populateCards(game);
    populatePermanents(game);
}

// --------------------------------------------------
// Data Population
// --------------------------------------------------

void Observation::populateTurn(Game* game) {
    Profiler::Scope scope = game->profiler->track("populateTurn");
    turn.turn_number = game->turn_system->global_turn_count;
    turn.phase = game->turn_system->currentPhaseType();
    turn.step = game->turn_system->currentStepType();

    if (game->activePlayer() != nullptr) {
        turn.active_player_id = static_cast<int>(game->activePlayer()->id);
    } else {
        turn.active_player_id = -1;
    }

    if (game->agentPlayer() != nullptr) {
        turn.agent_player_id = static_cast<int>(game->agentPlayer()->id);
    } else {
        turn.agent_player_id = -1;
    }
}

void Observation::populateActionSpace(Game* game) {
    Profiler::Scope scope = game->profiler->track("populateActionSpace");

    action_space.action_space_type = game->current_action_space->type;

    // Copy each available action
    const std::vector<std::unique_ptr<Action>>& game_actions = game->current_action_space->actions;
    for (const auto& act : game_actions) {
        ActionOption opt;
        opt.action_type = act->type;
        opt.focus = act->focus();
        action_space.actions.push_back(opt);
    }
}

void Observation::populatePlayers(Game* game) {
    Profiler::Scope scope = game->profiler->track("populatePlayers");

    const Player* agent_player = game->agentPlayer();
    assert(agent_player != nullptr);

    // Fill agent data
    agent.player_index = static_cast<int>(agent_player->index);
    agent.id = static_cast<int>(agent_player->id);
    agent.is_agent = true; // By definition
    agent.is_active = (agent_player == game->activePlayer());
    agent.life = agent_player->life;

    // Populate agent zone_counts
    for (int z = 0; z < 7; z++) {
        agent.zone_counts[z] = game->zones->size(static_cast<ZoneType>(z), agent_player);
    }

    // Find and fill opponent data
    const Player* opponent_player = nullptr;
    const std::vector<Player*>& player_order = game->playersStartingWithAgent();
    for (const Player* p : player_order) {
        if (p != agent_player) {
            opponent_player = p;
            break;
        }
    }
    assert(opponent_player != nullptr);

    opponent.player_index = static_cast<int>(opponent_player->index);
    opponent.id = static_cast<int>(opponent_player->id);
    opponent.is_agent = false; // By definition
    opponent.is_active = (opponent_player == game->activePlayer());
    opponent.life = opponent_player->life;

    // Populate opponent zone_counts
    for (int z = 0; z < 7; z++) {
        opponent.zone_counts[z] = game->zones->size(static_cast<ZoneType>(z), opponent_player);
    }
}

void Observation::populateCards(Game* game) {
    Profiler::Scope scope = game->profiler->track("populateCards");

    const Player* agent_player = game->agentPlayer();
    assert(agent_player != nullptr);

    // 1) HAND: visible only to the owner
    const Hand* hand = game->zones->constHand();
    for (const Card* card : hand->cards[agent_player->index]) {
        addCard(card, ZoneType::HAND);
    }

    // Use cached player order for iteration
    const std::vector<Player*>& player_order = game->playersStartingWithAgent();

    // 2) GRAVEYARD: gather for all players (public info)
    const Graveyard* gy = game->zones->constGraveyard();
    for (const Player* player : player_order) {
        for (const Card* card : gy->cards[player->index]) {
            addCard(card, ZoneType::GRAVEYARD);
        }
    }

    // 3) EXILE: gather for all players (public info)
    const Exile* ex = game->zones->constExile();
    for (const Player* player : player_order) {
        for (const Card* card : ex->cards[player->index]) {
            addCard(card, ZoneType::EXILE);
        }
    }

    // 4) STACK
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
    ManaCost mc = card->mana_cost.value_or(ManaCost());
    cdata.mana_cost = mc;

    cdata.name = card->name;
    cdata.owner_id = static_cast<int>(card->owner->id);
    cdata.zone = zone;

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

    // Add to appropriate section based on owner
    if (cdata.owner_id == agent.id) {
        agent_cards.push_back(cdata);
    } else {
        opponent_cards.push_back(cdata);
    }
}

void Observation::populatePermanents(Game* game) {
    Profiler::Scope scope = game->profiler->track("populatePermanents");

    const Player* agent_player = game->agentPlayer();
    assert(agent_player != nullptr);

    // Gather from battlefield (all public info)
    const Battlefield* bf = game->zones->constBattlefield();
    const std::vector<Player*>& player_order = game->playersStartingWithAgent();
    for (const Player* player : player_order) {
        for (const auto& perm : bf->permanents[player->index]) {
            addPermanent(perm.get());
        }
    }
}

void Observation::addPermanent(const Permanent* perm) {
    PermanentData pdat;
    pdat.id = perm->id;
    pdat.controller_id = perm->controller->id;
    pdat.card_id = perm->card->id;
    pdat.tapped = perm->tapped;
    pdat.damage = perm->damage;
    pdat.is_summoning_sick = perm->summoning_sick;

    // Add to appropriate section based on controller
    if (pdat.controller_id == agent.id) {
        agent_permanents.push_back(pdat);
    } else {
        opponent_permanents.push_back(pdat);
    }

    // Also make sure we have the underlying card
    addCard(perm->card, ZoneType::BATTLEFIELD);
}

// --------------------------------------------------
// Validation
// --------------------------------------------------

bool Observation::validate() const {
    // Check agent/opponent are distinct
    if (agent.id == opponent.id) {
        return false;
    }

    // Check exactly one is_agent
    if (agent.is_agent == opponent.is_agent) {
        return false;
    }

    // Validate card ownership matches section
    for (const CardData& card : agent_cards) {
        if (card.owner_id != agent.id) {
            return false;
        }
    }
    for (const CardData& card : opponent_cards) {
        if (card.owner_id != opponent.id) {
            return false;
        }
    }

    // Validate permanent control matches section
    for (const PermanentData& perm : agent_permanents) {
        if (perm.controller_id != agent.id) {
            return false;
        }
    }
    for (const PermanentData& perm : opponent_permanents) {
        if (perm.controller_id != opponent.id) {
            return false;
        }
    }

    return true;
}

std::string Observation::toJSON() const {

    std::ostringstream out;
    out << "{\n";

    // Global state
    out << fmt::format("  \"game_over\": {},\n", game_over ? "true" : "false");
    out << fmt::format("  \"won\": {},\n", won ? "true" : "false");

    // Turn state
    out << "  \"turn\": {\n";
    out << fmt::format("    \"turn_number\": {},\n", turn.turn_number);
    out << fmt::format("    \"phase\": {},\n", static_cast<int>(turn.phase));
    out << fmt::format("    \"step\": {},\n", static_cast<int>(turn.step));
    out << fmt::format("    \"active_player_id\": {},\n", turn.active_player_id);
    out << fmt::format("    \"agent_player_id\": {}\n", turn.agent_player_id);
    out << "  },\n";

    // Actions
    out << "  \"action_space\": {\n";
    out << fmt::format("    \"type\": {},\n", static_cast<int>(action_space.action_space_type));
    out << "    \"actions\": [\n";
    for (size_t i = 0; i < action_space.actions.size(); ++i) {
        const auto& action = action_space.actions[i];
        out << "      {\n";
        out << fmt::format("        \"type\": {},\n", static_cast<int>(action.action_type));
        out << "        \"focus\": [";
        for (size_t j = 0; j < action.focus.size(); ++j) {
            out << action.focus[j];
            if (j + 1 < action.focus.size())
                out << ", ";
        }
        out << "]\n      }";
        if (i + 1 < action_space.actions.size())
            out << ",";
        out << "\n";
    }
    out << "    ]\n  },\n";

    // Helper for player data
    auto writePlayer = [&out](const std::string& name, const PlayerData& player) {
        out << fmt::format("  \"{}\": {{\n", name);
        out << fmt::format("    \"player_index\": {},\n", player.player_index);
        out << fmt::format("    \"id\": {},\n", player.id);
        out << fmt::format("    \"is_active\": {},\n", player.is_active ? "true" : "false");
        out << fmt::format("    \"is_agent\": {},\n", player.is_agent ? "true" : "false");
        out << fmt::format("    \"life\": {},\n", player.life);
        out << "    \"zone_counts\": [";
        for (size_t i = 0; i < player.zone_counts.size(); ++i) {
            out << player.zone_counts[i];
            if (i + 1 < player.zone_counts.size())
                out << ", ";
        }
        out << "]\n  },\n";
    };

    // Helper for card data
    auto writeCards = [&out](const std::string& name, const std::vector<CardData>& cards) {
        out << fmt::format("  \"{}\": [\n", name);
        for (size_t i = 0; i < cards.size(); ++i) {
            const CardData& card = cards[i];
            out << "    {\n";
            out << fmt::format("      \"id\": {},\n", card.id);
            out << fmt::format("      \"registry_key\": {},\n", card.registry_key);
            out << fmt::format("      \"name\": \"{}\",\n", card.name);
            out << fmt::format("      \"zone\": {},\n", static_cast<int>(card.zone));
            out << fmt::format("      \"owner_id\": {},\n", card.owner_id);
            out << fmt::format("      \"power\": {},\n", card.power);
            out << fmt::format("      \"toughness\": {},\n", card.toughness);
            out << "      \"mana_cost\": {\n";
            out << "        \"cost\": [";
            for (size_t j = 0; j < card.mana_cost.cost.size(); ++j) {
                out << card.mana_cost.cost[j];
                if (j + 1 < card.mana_cost.cost.size())
                    out << ", ";
            }
            out << "],\n";
            out << fmt::format("        \"mana_value\": {}\n", card.mana_cost.mana_value);
            out << "      }\n    }";
            if (i + 1 < cards.size())
                out << ",";
            out << "\n";
        }
        out << "  ],\n";
    };

    // Helper for permanent data
    auto writePermanents = [&out](const std::string& name,
                                  const std::vector<PermanentData>& perms) {
        out << fmt::format("  \"{}\": [\n", name);
        for (size_t i = 0; i < perms.size(); ++i) {
            const PermanentData& perm = perms[i];
            out << "    {\n";
            out << fmt::format("      \"id\": {},\n", perm.id);
            out << fmt::format("      \"card_id\": {},\n", perm.card_id);
            out << fmt::format("      \"controller_id\": {},\n", perm.controller_id);
            out << fmt::format("      \"tapped\": {},\n", perm.tapped ? "true" : "false");
            out << fmt::format("      \"damage\": {},\n", perm.damage);
            out << fmt::format("      \"is_summoning_sick\": {}\n",
                               perm.is_summoning_sick ? "true" : "false");
            out << "    }";
            if (i + 1 < perms.size())
                out << ",";
            out << "\n";
        }
        out << "  ]";
        if (name != "opponent_permanents")
            out << ","; // No comma after last field
        out << "\n";
    };

    // Write all data using helpers
    writePlayer("agent", agent);
    writeCards("agent_cards", agent_cards);
    writePermanents("agent_permanents", agent_permanents);
    writePlayer("opponent", opponent);
    writeCards("opponent_cards", opponent_cards);
    writePermanents("opponent_permanents", opponent_permanents);

    out << "}";
    return out.str();
}