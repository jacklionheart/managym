#include "game.h"

#include "managym/agent/observation.h"
#include "managym/infra/log.h"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>

Game::Game(std::vector<PlayerConfig> player_configs, bool headless, bool skip_trivial)
    : skip_trivial(skip_trivial), turn_system(std::make_unique<TurnSystem>(this)),
      priority_system(std::make_unique<PrioritySystem>(this)),
      id_generator(std::make_unique<IDGenerator>()) {

    card_registry = std::make_unique<CardRegistry>(id_generator.get());

    int num_players = player_configs.size();
    if (num_players != 2) {
        throw std::invalid_argument("Game must start with 2 players.");
    }

    // Create players first
    int index = 0;
    for (PlayerConfig& player_config : player_configs) {
        players.emplace_back(std::make_unique<Player>(id_generator->next(), index++, player_config,
                                                      card_registry.get()));
    }

    std::vector<Player*> weak_players = {players[0].get(), players[1].get()};
    zones = std::make_unique<Zones>(weak_players, id_generator.get());

    // Initialize libraries and draw starting hands
    for (std::unique_ptr<Player>& player_uptr : players) {
        Player* player = player_uptr.get();

        // Move all cards from deck to library
        for (std::unique_ptr<Card>& card : *player->deck) {
            zones->move(card.get(), ZoneType::LIBRARY);
        }

        // Shuffle before drawing
        zones->shuffle(ZoneType::LIBRARY, player);

        // Draw starting hand
        const int STARTING_HAND_SIZE = 7;
        for (int i = 0; i < STARTING_HAND_SIZE; i++) {
            if (zones->size(ZoneType::LIBRARY, player) > 0) {
                zones->moveTop(ZoneType::LIBRARY, ZoneType::HAND, player);
            }
        }
    }

    if (!headless) {
        display = std::make_unique<GameDisplay>();
    }

    // Start the game
    tick();
    while (skip_trivial && actionSpaceTrivial()) {
        step(0);
    }
}

// Reads

ActionSpace* Game::actionSpace() const { return current_action_space.get(); }

bool Game::actionSpaceTrivial() const {
    return !current_action_space || current_action_space->actions.size() <= 1;
}

Observation* Game::observation() const { return current_observation.get(); }

Player* Game::agentPlayer() const {
    if (current_action_space && current_action_space->player) {
        return current_action_space->player;
    }
    return players[0].get();
}

Player* Game::activePlayer() const { return turn_system->activePlayer(); }

Player* Game::nonActivePlayer() const { return turn_system->nonActivePlayer(); }

std::vector<Player*> Game::playersStartingWithActive() const {
    return turn_system->playersStartingWithActive();
}

// Get players, starting with the agent player (or the first player if no agent)
std::vector<Player*> Game::playersStartingWithAgent() const {
    Player* first = players[0].get();
    if (current_action_space && current_action_space->player) {
        first = current_action_space->player;
    }

    // Find the index of the agent player
    auto it = std::find_if(players.begin(), players.end(),
                           [first](const std::unique_ptr<Player>& p) { return p.get() == first; });
    int index = (it != players.end()) ? std::distance(players.begin(), it) : 0;

    int num_players = players.size();
    std::vector<Player*> order;
    for (int i = 0; i < num_players; i++) {
        order.push_back(players[(index + i) % num_players].get());
    }

    return order;
}

bool Game::isActivePlayer(Player* player) const { return player == turn_system->activePlayer(); }

bool Game::canPlayLand(Player* player) const {
    return canCastSorceries(player) && turn_system->current_turn->lands_played < 1;
}

bool Game::canCastSorceries(Player* player) const {
    return isActivePlayer(player) && zones->size(ZoneType::STACK, player) == 0 &&
           turn_system->current_turn->currentPhase()->canCastSorceries();
}

bool Game::canPayManaCost(Player* player, const ManaCost& mana_cost) const {
    return zones->constBattlefield()->producibleMana(player).canPay(mana_cost);
}

bool Game::isPlayerAlive(Player* player) const { return player->alive; }

bool Game::isGameOver() const {
    return std::count_if(players.begin(), players.end(),
                         [](const std::unique_ptr<Player>& player) { return player->alive; }) < 2;
}

int Game::winnerIndex() const {
    if (!isGameOver()) {
        return -1;
    }

    for (int i = 0; i < players.size(); i++) {
        if (players[i]->alive) {
            return i;
        }
    }

    return -1;
}

// Writes

void Game::play() {
    while (true) {
        if (step(0)) {
            break;
        }
    }
}

bool Game::_step(int action) {
    // Validate current action space exists
    if (!current_action_space) {
        throw std::logic_error("No active action space");
    }

    if (isGameOver()) {
        throw AgentError("Game is over");
    }

    if (current_action_space->actions.empty()) {
        throw std::logic_error("No actions available");
    }

    // Check if the action is within the valid range
    if (action < 0 || action >= current_action_space->actions.size()) {
        throw AgentError(fmt::format("Action index {} out of bound: {}.", action,
                                     current_action_space->actions.size()));
    }

    if (isGameOver()) {
        throw AgentError("Game is over");
    }

    // Execute action and clear current action space
    managym::log::debug(Category::AGENT, "Available actions: {}", current_action_space->toString());
    managym::log::debug(Category::AGENT, "Executing action: {}",
                        current_action_space->actions[action]->toString());
    current_action_space->actions[action]->execute();
    current_action_space = nullptr;
    current_observation = nullptr;

    // Progress game until next action space needed
    return tick();
}

bool Game::step(int action) {
    bool game_over = _step(action);

    while (!game_over && skip_trivial && actionSpaceTrivial()) {
        game_over = _step(0);
    }

    return game_over;
}

bool Game::tick() {
    // Keep ticking until we have an action space or game ends
    while (!current_action_space) {
        // Handle display updates
        if (display) {
            display->processEvents();
            if (display->isOpen()) {
                display->render(current_observation.get());
            }
        }

        // Get next action space
        current_action_space = turn_system->tick();

        // Check for game over
        if (isGameOver()) {
            current_action_space = ActionSpace::createEmpty();
            current_observation = std::make_unique<Observation>(this);
            return true;
        }
    }

    // Create new observation for current state
    current_observation = std::make_unique<Observation>(this);
    return false;
}

void Game::clearManaPools() {
    for (std::unique_ptr<Player>& player_uptr : players) {
        Player* player = player_uptr.get();
        player->mana_pool.clear();
    }
}

void Game::clearDamage() {
    zones->forEachPermanentAll([&](Permanent* permanent) { permanent->damage = 0; });
}

void Game::untapAllPermanents(Player* player) {
    zones->forEachPermanent([&](Permanent* permanent) { permanent->untap(); }, player);
}

void Game::markPermanentsNotSummoningSick(Player* player) {
    zones->forEachPermanent([&](Permanent* permanent) { permanent->summoning_sick = false; },
                            player);
}

void Game::drawCards(Player* player, int amount) {
    for (int i = 0; i < amount; ++i) {
        if (zones->size(ZoneType::LIBRARY, player) == 0) {
            managym::log::info(Category::RULES,
                               "{} drew a card from an empty library, will die next SBA",
                               player->name);
            player->drew_when_empty = true;
            break;
        } else {
            Card* card = zones->top(ZoneType::LIBRARY, player);
            zones->moveTop(ZoneType::LIBRARY, ZoneType::HAND, player);
        }
    }
}

void Game::loseGame(Player* player) { player->alive = false; }

void Game::addMana(Player* player, const Mana& mana) { player->mana_pool.add(mana); }

void Game::spendMana(Player* player, const ManaCost& mana_cost) {
    player->mana_pool.pay(mana_cost);
}

void Game::castSpell(Player* player, Card* card) {
    if (card->types.isLand()) {
        throw std::invalid_argument("Land cards cannot be cast.");
    }
    if (card->owner != player) {
        throw std::invalid_argument("Card does not belong to player.");
    }
    zones->pushStack(card);
}

void Game::playLand(Player* player, Card* card) {
    if (!card->types.isLand()) {
        throw std::invalid_argument("Only land cards can be played.");
    }
    if (!canPlayLand(player)) {
        throw std::logic_error("Cannot play land this turn.");
    }
    managym::log::debug(Category::AGENT, "we canPlayLand");
    turn_system->current_turn->lands_played += 1;
    managym::log::debug(Category::AGENT, "{} plays a land {}", player->name, card->toString());
    zones->move(card, ZoneType::BATTLEFIELD);
}