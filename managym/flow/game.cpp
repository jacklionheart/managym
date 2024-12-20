// game->cpp

#include "game.h"

#include "rules/game.h"
#include "rules/turns/priority.h"
#include "rules/turns/combat.h"
#include "rules/zones/battlefield.h"
#include "rules/zones/stack.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <spdlog/spdlog.h>


Game::Game(std::vector<PlayerConfig> player_configs, bool headless) : zones(this), turn_system(std::make_unique<TurnSystem>(this)) {
    int num_players = player_configs.size();

    if (num_players!= 2) {
        throw std::invalid_argument("Game must start with 2 players.");
    }

    for (PlayerConfig player_config : player_configs) {
        players.emplace_back(new Player(player_config));
    }

    // Initialize libraries with the decks, then draw up to 7 cards
    for (Player& player : this->players) {
        Deck* deck = player->deck.get();
        for (std::unique_ptr<Card>& card : deck) {
            zones->library.move(*card);
        }

        int starting_hand_size = std::min<int>(7, zones->library.numCards(player.id));
        for (int i = 0; i < starting_hand_size; ++i) {
            zones->hand.move(*zones->library.top(player.id));
        }
    }

    if (!headless) {
        display = std::make_unique<GameDisplay>(*this);
    }
}

void Game::play() {
    try {
        std::unique_ptr<ActionSpace> action_space = nullptr;
        std::unique_ptr<Action> selected_action = nullptr;

        while (true) {        
            if (display) {
                display->render();
                selected_action = active_player.agent.chosenAction();
            }

            if (selected_action != nullptr) {
                if (action_space == nullptr) {
                    throw std::logic_error("No action space to select action");
                }
                selected_action->execute();
                action_space = nullptr;
            }

            if (action_space == nullptr) {
                action_space = turn_system.tick();
            }
        }
    } catch (const GameOverException& e) {
        spdlog::info(e.what());
    }   
}

bool Game::isGameOver() {
    return std::count_if(players.begin(), players.end(), [](const Player& player) {
        return player.alive;
    }) < 2;
}

void Game::clearManaPools() {
    for (auto& [player, mana_pool] : mana_pools) {
        mana_pool.clear();
    }
}

void Game::clearDamage() {
    zones->battlefield.forEach([&](Permanent* permanent) {
        permanent.damage = 0;
    });
}

void Game::untapAllPermanents(Player* player) {
    zones->battlefield.forEach([&](Permanent* permanent) {
        permanent.untap();
    }, player->id);
}

void Game::markPermanentsNotSummoningSick(Player* player) {
    zones->battlefield.forEach([&](Permanent* permanent) {
        permanent.summoning_sick = false;
    }, player->id);
}

void Game::drawCards(Player* player, int amount) {

    for (int i = 0; i < amount; ++i) {
        if (zones->library.numCards(player->id) == 0) {
            // TODO
            //            loseGame(player);
            break;
        } else {
            Card* card = zones->library.top(player->id);
            zones->hand.move(*card);
        }
    }
}

Player* Game::player(int player_id) {
    return &*std::find_if(players.begin(), players.end(), [&](const Player& player) {
        return player.id == player_id;
    });
}

bool Game::isPlayerAlive(Player* player) {
    return player->alive;
}

void Game::loseGame(Player* player) {
    player->alive = false;
    if (!isPlayerAlive(player)) {
        throw GameOverException("Player " + std::to_string(player->id) + " has lost the game->");
    }
}

std::vector<Card*> Game::cardsInHand(Player* player) {
    return zones->hand.cards[player->id];
}

Player* Game::activePlayer() {
    return turn_system.activePlayer();
}

Player* Game::nonActivePlayer() {
    return turn_system.nonActivePlayer();
}

std::vector<Player*> Game::priorityOrder() {
    return turn_system.priorityOrder();
}

Card* Game::card(int id) {
    return *cards.at(id);
}

bool Game::isActivePlayer(Player* player) const {
    return player->id == turn_system.activePlayer().id;
}

bool Game::canPlayLand(Player* player) const {
    return canCastSorceries(player) && turn_system.lands_played < 1;
}

bool Game::canCastSorceries(Player* player) const {
    return isActivePlayer(player) && zones->stack.size() == 0 && current_turn->current_phase->can_cast_sorceries;
}

bool Game::canPayManaCost(Player* player, const ManaCost& mana_cost) const {
    return zones->battlefield.producibleMana(player->id).canPay(mana_cost);
}

// Game State Mutations

void Game::addMana(Player* player, const Mana& mana) {
    mana_pools[player].add(mana);
}

void Game::spendMana(Player* player, const ManaCost& mana_cost) {
    mana_pools[player->id].pay(mana_cost);
}

void Game::castSpell(Player* player, Card* card) {
    if (card->types.isLand()) {
        throw std::invalid_argument("Land cards cannot be cast.");
    }
    if (card->owner_id != player->id) {
        throw std::invalid_argument("Card does not belong to player.");
    }
    zones->stack.cast(card);   
}

void Game::playLand(Player* player, Card* card) {
    if (!card->types.isLand()) {
        throw std::invalid_argument("Only land cards can be played.");
    }
    if (!canPlayLand(player)) {
        throw std::logic_error("Cannot play land this turn.");
    }
        spdlog::info("we canPlayLand");
    turn_system.current_turn->lands_played += 1;
    spdlog::info("{} plays a land {}", player->name, card->toString());
    zones->battlefield.enter(card);
}