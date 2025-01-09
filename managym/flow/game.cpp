// game->cpp

#include "game.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "managym/state/zones.h"

Game::Game(std::vector<PlayerConfig> player_configs, bool headless)
    : turn_system(std::make_unique<TurnSystem>(this)) {
  int num_players = player_configs.size();

  if (num_players != 2) {
    throw std::invalid_argument("Game must start with 2 players.");
  }

  for (PlayerConfig player_config : player_configs) {
    players.emplace_back(std::make_unique<Player>(player_config));
  }

  std::vector<Player*> weak_players = {players[0].get(), players[1].get()};

  zones = std::make_unique<Zones>(weak_players);

  // Initialize libraries with the decks, then draw up to 7 cards
  for (std::unique_ptr<Player>& player : players) {
    Deck* deck = player->deck.get();
    for (std::unique_ptr<Card>& card : *deck) {
      zones->library->add(card.get());
    }

    int starting_hand_size =
        std::min<int>(7, zones->library->numCards(player.get()));
    for (int i = 0; i < starting_hand_size; ++i) {
      zones->move(zones->library->top(player.get()), zones->hand.get());
    }
  }

  if (!headless) {
    display = std::make_unique<GameDisplay>(this);
  }
}

void Game::play() {
  try {
    std::unique_ptr<ActionSpace> action_space = nullptr;

    while (true) {
      if (display) {
        spdlog::info("Rendering");
        display->render();
      }

      if (action_space != nullptr) {
        if (action_space->actionSelected()) {
          spdlog::info("Executing action");
          Action* selected_action =
              action_space->actions[action_space->chosen_index].get();
          selected_action->execute();
          spdlog::info("Executed action");
          action_space = nullptr;
        } else {
          spdlog::info("Waiting for action selection");
        }
      }

      if (action_space == nullptr) {
        spdlog::info("Ticking turn system");
        action_space = turn_system->tick();
        if (action_space != nullptr) {
          action_space->selectAction(0);
        }
      }
    }
  } catch (const GameOverException& e) {
    spdlog::info(e.what());
  }
}

bool Game::isGameOver() {
  return std::count_if(players.begin(), players.end(),
                       [](const std::unique_ptr<Player>& player) {
                         return player->alive;
                       }) < 2;
}

void Game::clearManaPools() {
  for (auto& [player, mana_pool] : mana_pools) {
    mana_pool.clear();
  }
}

void Game::clearDamage() {
  zones->battlefield->forEach(
      [&](Permanent* permanent) { permanent->damage = 0; });
}

void Game::untapAllPermanents(Player* player) {
  zones->battlefield->forEach([&](Permanent* permanent) { permanent->untap(); },
                              player);
}

void Game::markPermanentsNotSummoningSick(Player* player) {
  zones->battlefield->forEach(
      [&](Permanent* permanent) { permanent->summoning_sick = false; }, player);
}

void Game::drawCards(Player* player, int amount) {
  for (int i = 0; i < amount; ++i) {
    if (zones->library->numCards(player) == 0) {
      break;
    } else {
      Card* card = zones->library->top(player);
      zones->move(card, zones->hand.get());
    }
  }
}

bool Game::isPlayerAlive(Player* player) { return player->alive; }

void Game::loseGame(Player* player) {
  player->alive = false;
  if (!isPlayerAlive(player)) {
    throw GameOverException("Player " + std::to_string(player->id) +
                            " has lost the game->");
  }
}

std::vector<Card*> Game::cardsInHand(Player* player) {
  return zones->hand->cards[player];
}

Player* Game::activePlayer() { return turn_system->activePlayer(); }

Player* Game::nonActivePlayer() { return turn_system->nonActivePlayer(); }

std::vector<Player*> Game::priorityOrder() {
  return turn_system->priorityOrder();
}

bool Game::isActivePlayer(Player* player) const {
  return player == turn_system->activePlayer();
}

bool Game::canPlayLand(Player* player) const {
  return canCastSorceries(player) &&
         turn_system->current_turn->lands_played < 1;
}

bool Game::canCastSorceries(Player* player) const {
  return isActivePlayer(player) && zones->stack->size() == 0 &&
         turn_system->current_turn->currentPhase()->canCastSorceries();
}

bool Game::canPayManaCost(Player* player, const ManaCost& mana_cost) const {
  return zones->battlefield->producibleMana(player).canPay(mana_cost);
}

// Game State Mutations

void Game::addMana(Player* player, const Mana& mana) {
  mana_pools[player].add(mana);
}

void Game::spendMana(Player* player, const ManaCost& mana_cost) {
  mana_pools[player].pay(mana_cost);
}

void Game::castSpell(Player* player, Card* card) {
  if (card->types.isLand()) {
    throw std::invalid_argument("Land cards cannot be cast.");
  }
  if (card->owner != player) {
    throw std::invalid_argument("Card does not belong to player.");
  }
  zones->stack->push(card);
}

void Game::playLand(Player* player, Card* card) {
  if (!card->types.isLand()) {
    throw std::invalid_argument("Only land cards can be played.");
  }
  if (!canPlayLand(player)) {
    throw std::logic_error("Cannot play land this turn.");
  }
  spdlog::info("we canPlayLand");
  turn_system->current_turn->lands_played += 1;
  spdlog::info("{} plays a land {}", player->name, card->toString());
  zones->battlefield->enter(card);
}