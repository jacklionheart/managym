#pragma once

#include <map>
#include <memory>
#include <vector>

#include "managym/state/card.h"
#include "managym/state/mana.h"
#include "managym/state/player.h"
#include "managym/state/zone.h"

class Battlefield : public Zone {
 public:
  std::map<Player*, std::vector<std::unique_ptr<Permanent>>> permanents;

  Battlefield(Zones* zones, std::vector<Player*>& players);

  std::vector<Permanent*> attackers(Player* player) const;
  std::vector<Permanent*> eligibleAttackers(Player* player) const;
  std::vector<Permanent*> eligibleBlockers(Player* player) const;
  Mana producibleMana(Player* player) const;
  Permanent* find(const Card* card) const;

 protected:
  friend class Zones;
  void destroy(Permanent* permanent);
  void forEachAll(const std::function<void(Permanent*)>& func);
  void forEach(const std::function<void(Permanent*)>& func, Player* player);
  void produceMana(const ManaCost& mana_cost, Player* player);
  void enter(Card* card) override;
  void exit(Card* card) override;
};

struct Permanent {
  int id;
  static int next_id;

  Card* card;
  Player* controller;
  bool tapped = false;
  bool summoning_sick = false;
  int damage = 0;
  bool attacking = false;

  Permanent(Card* card);

  [[nodiscard]] bool canTap() const;
  [[nodiscard]] bool canAttack() const;
  [[nodiscard]] bool canBlock() const;
  void untap();
  void tap();
  void takeDamage(int damage);
  [[nodiscard]] bool hasLethalDamage() const;
  void clearDamage();
  void attack();
  [[nodiscard]] Mana producibleMana() const;
  void activateAllManaAbilities();
  void activateAbility(ActivatedAbility* ability);

  bool operator==(const Permanent& other) const;
};