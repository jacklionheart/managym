#pragma once

#include <map>
#include <set>
#include <string>

// Represents the five colors of mana plus colorless
enum struct Color { COLORLESS, WHITE, BLUE, BLACK, RED, GREEN };

// Convert a color to its standard one-letter representation
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

// Set of colors, used for card color identity
using Colors = std::set<Color>;

// Represents a mana cost that must be paid to cast a spell or activate an ability
struct ManaCost {
    // Data
    std::map<Color, int> cost;
    int generic;

    ManaCost();

    // Static Methods
    static ManaCost parse(const std::string& mana_cost_str);

    // Reads
    std::string toString() const;
    Colors colors() const;
    int manaValue() const;
};

// Represents actual mana in a player's mana pool
struct Mana {
    // Data
    std::map<Color, int> mana;

    // Static Methods
    static Mana parse(const std::string& mana_str);
    static Mana single(Color color);

    Mana();

    // Writes
    void add(const Mana& other);
    void pay(const ManaCost& mana_cost);
    void clear();

    // Reads
    int total() const;
    bool canPay(const ManaCost& mana_cost) const;
    std::string toString() const;
};