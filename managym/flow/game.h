#include <spdlog/spdlog.h>

#include <vector>

#include "managym/action/action.h"
#include "managym/flow/priority.h"
#include "managym/flow/turn.h"
#include "managym/render/game_display.h"
#include "managym/state/battlefield.h"
#include "managym/state/card.h"
#include "managym/state/mana.h"
#include "managym/state/player.h"
#include "managym/state/zone.h"

class Game {
 public:
  std::vector<std::unique_ptr<Player>> players;
  std::map<Player *, Mana> mana_pools;

  std::unique_ptr<Zones> zones;
  std::unique_ptr<GameDisplay> display;
  std::unique_ptr<TurnSystem> turn_system;

  std::unique_ptr<Graveyard> graveyard;

  Game(std::vector<PlayerConfig> player_configs, bool headless = false);

  void play();
  bool isGameOver();
  void nextStep();
  void clearManaPools();
  void grantPriority();
  void allowPlayerActions();

  // Lookups
  Card *card(int id);
  std::vector<Card *> cardsInHand(Player *player);
  Player *activePlayer();
  Player *nonActivePlayer();
  std::vector<Player *> priorityOrder();

  bool isActivePlayer(Player *player) const;
  bool canPlayLand(Player *player) const;
  bool canCastSorceries(Player *player) const;
  bool canPayManaCost(Player *player, const ManaCost &mana_cost) const;
  bool isPlayerAlive(Player *player);

  // Automatic Actions
  void untapAllPermanents(Player *player);
  void markPermanentsNotSummoningSick(Player *player);
  void drawCards(Player *player, int amount);
  void loseGame(Player *player);
  void performStateBasedActions();
  void clearDamage();

  // Game State Mutations
  void addMana(Player *player, const Mana &mana);
  void spendMana(Player *player, const ManaCost &mana_cost);
  void castSpell(Player *player, Card *card);
  void playLand(Player *player, Card *card);
};

class GameOverException : public std::exception {
 public:
  std::string message;
  GameOverException(const std::string &msg) : message(msg) {}
  const char *what() const noexcept override { return message.c_str(); }
};