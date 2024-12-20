#pragma once

#include <map>
#include <set>
#include <string>

enum struct Color { COLORLESS, WHITE, BLUE, BLACK, RED, GREEN };

inline std::string toString(Color color) {
    switch (color) {
    case Color::WHITE:
        return "W";
    case Color::BLUE:
        return "U";
    case Color::BLACK:
        return "B";
    case Color::RED:
        return "R";
    case Color::GREEN:
        return "G";
    case Color::COLORLESS:
        return "C";
    default:
        return "?";
    }
}

using Colors = std::set<Color>;

class ManaCost {
  public:
    std::map<Color, int> cost;
    int generic;

    ManaCost();
    [[nodiscard]] static ManaCost parse(const std::string& mana_cost_str);
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] Colors colors() const;
    [[nodiscard]] int manaValue() const;
};

class Mana {
  public:
    std::map<Color, int> mana;
    static Mana parse(const std::string& mana_cost_str);
    static Mana single(Color color);

    Mana();
    void add(const Mana& other);
    [[nodiscard]] int total() const;
    [[nodiscard]] bool canPay(const ManaCost& mana_cost) const;
    void pay(const ManaCost& mana_cost);
    void clear();
    [[nodiscard]] std::string toString() const;
};