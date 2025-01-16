#include "game.h"

#include "managym/agent/observation.h"
#include "managym/state/zones.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <stdexcept>

Game::Game(std::vector<PlayerConfig> player_configs, bool headless)
    : turn_system(std::make_unique<TurnSystem>(this)) {
    int num_players = player_configs.size();
    if (num_players != 2) {
        throw std::invalid_argument("Game must start with 2 players.");
    }

    // Create players first
    for (PlayerConfig& player_config : player_configs) {
        players.emplace_back(std::make_unique<Player>(player_config));
    }

    std::vector<Player*> weak_players = {players[0].get(), players[1].get()};
    zones = std::make_unique<Zones>(weak_players);

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
}

// Reads

Player* Game::activePlayer() const { return turn_system->activePlayer(); }

Player* Game::nonActivePlayer() const { return turn_system->nonActivePlayer(); }

std::vector<Player*> Game::priorityOrder() const { return turn_system->priorityOrder(); }

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

// Writes

void Game::play() {
    while (true) {
        bool done = tick();
        if (done) {
            spdlog::debug("game over in play");
            break;
        }
    }
}

bool Game::tick() {
    try {
        //        sf::sleep(sf::milliseconds(100));
        std::unique_ptr<Observation> obs =
            std::make_unique<Observation>(this, current_action_space.get());

        // Handle display if it exists
        if (display) {
            display->processEvents();
            if (display->isOpen()) {
                display->render(obs.get());
            }
        }

        // Handle current action space if it exists
        if (current_action_space) {
            if (current_action_space->actionSelected()) {
                Action* selected_action =
                    current_action_space->actions[current_action_space->chosen_index].get();
                selected_action->execute();
                current_action_space = nullptr;
            }
        }

        // Get next action space if needed
        if (!current_action_space) {
            current_action_space = turn_system->tick();

            // Auto-select action for now
            if (current_action_space) {
                current_action_space->selectAction(0);
            }
        }

        return false;

    } catch (const GameOverException& e) {
        spdlog::debug("game over");
        spdlog::info(e.what());
        return true;
    }
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
            break;
        } else {
            Card* card = zones->top(ZoneType::LIBRARY, player);
            zones->moveTop(ZoneType::LIBRARY, ZoneType::HAND, player);
        }
    }
}

void Game::loseGame(Player* player) {
    player->alive = false;
    if (!isPlayerAlive(player)) {
        throw GameOverException("Player " + std::to_string(player->id) + " has lost the game->");
    }
}

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
    spdlog::debug("we canPlayLand");
    turn_system->current_turn->lands_played += 1;
    spdlog::debug("{} plays a land {}", player->name, card->toString());
    zones->move(card, ZoneType::BATTLEFIELD);
}