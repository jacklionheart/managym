#include "mana.h"

#include <spdlog/spdlog.h>

#include <cctype>
#include <iostream>
#include <stdexcept>

ManaCost::ManaCost() : generic(0) {
    cost[Color::COLORLESS] = 0;
    cost[Color::WHITE] = 0;
    cost[Color::BLUE] = 0;
    cost[Color::BLACK] = 0;
    cost[Color::RED] = 0;
    cost[Color::GREEN] = 0;
}

ManaCost ManaCost::parse(const std::string& mana_cost_str) {
    ManaCost mana_cost;
    size_t i = 0;
    while (i < mana_cost_str.length()) {
        char c = mana_cost_str[i];
        if (std::isdigit(c)) {
            int num = 0;
            while (i < mana_cost_str.length() && std::isdigit(mana_cost_str[i])) {
                num = num * 10 + (mana_cost_str[i] - '0');
                i++;
            }
            mana_cost.generic += num;
        } else {
            switch (c) {
            case 'W':
                mana_cost.cost[Color::WHITE] += 1;
                break;
            case 'U':
                mana_cost.cost[Color::BLUE] += 1;
                break;
            case 'B':
                mana_cost.cost[Color::BLACK] += 1;
                break;
            case 'R':
                mana_cost.cost[Color::RED] += 1;
                break;
            case 'G':
                mana_cost.cost[Color::GREEN] += 1;
                break;
            case 'C':
                mana_cost.cost[Color::COLORLESS] += 1;
                break;
            default:
                throw std::invalid_argument(std::string("Invalid character in mana cost: ") + c);
            }
            i++;
        }
    }
    return mana_cost;
}

std::string ManaCost::toString() const {
    std::string result;
    if (generic > 0) {
        result += std::to_string(generic);
    }
    for (const auto& [color, quantity] : cost) {
        for (int i = 0; i < quantity; i++) {
            char symbol;
            switch (color) {
            case Color::COLORLESS:
                symbol = 'C';
                break;
            case Color::WHITE:
                symbol = 'W';
                break;
            case Color::BLUE:
                symbol = 'U';
                break;
            case Color::BLACK:
                symbol = 'B';
                break;
            case Color::RED:
                symbol = 'R';
                break;
            case Color::GREEN:
                symbol = 'G';
                break;
            }
            result += symbol;
        }
    }
    return result;
}

Mana Mana::parse(const std::string& mana_str) {
    Mana mana;
    size_t i = 0;
    while (i < mana_str.length()) {
        char c = mana_str[i];
        if (std::isdigit(c)) {
            int num = 0;
            while (i < mana_str.length() && std::isdigit(mana_str[i])) {
                num = num * 10 + (mana_str[i] - '0');
                i++;
            }
            mana.mana[Color::COLORLESS] += num;
        } else {
            switch (c) {
            case 'C':
                mana.mana[Color::COLORLESS] += 1;
                break;
            case 'W':
                mana.mana[Color::WHITE] += 1;
                break;
            case 'U':
                mana.mana[Color::BLUE] += 1;
                break;
            case 'B':
                mana.mana[Color::BLACK] += 1;
                break;
            case 'R':
                mana.mana[Color::RED] += 1;
                break;
            case 'G':
                mana.mana[Color::GREEN] += 1;
                break;
            default:
                throw std::invalid_argument(std::string("Invalid character in mana cost: ") + c);
            }
            i++;
        }
    }
    return mana;
}

Mana Mana::single(Color color) {
    Mana mana;
    mana.mana[color] = 1;
    return mana;
}
Colors ManaCost::colors() const {
    Colors colors_set;
    for (const auto& [color, quantity] : cost) {
        if (quantity > 0 && color != Color::COLORLESS) {
            colors_set.insert(color);
        }
    }
    return colors_set;
}

int ManaCost::manaValue() const {
    int total = generic;
    for (const auto& [color, quantity] : cost) {
        total += quantity;
    }
    return total;
}

Mana::Mana() {
    mana[Color::COLORLESS] = 0;
    mana[Color::WHITE] = 0;
    mana[Color::BLUE] = 0;
    mana[Color::BLACK] = 0;
    mana[Color::RED] = 0;
    mana[Color::GREEN] = 0;
}

void Mana::add(const Mana& other) {
    for (const auto& [color, amount] : other.mana) {
        mana[color] += amount;
    }
}

int Mana::total() const {
    int sum = 0;
    for (const auto& [color, amount] : mana) {
        sum += amount;
    }
    return sum;
}

bool Mana::canPay(const ManaCost& mana_cost) const {
    spdlog::debug("Mana: {}", toString());
    spdlog::debug("Checking if can pay Mana cost: {}", mana_cost.toString());
    if (total() < mana_cost.manaValue()) {
        spdlog::debug("Not enough total mana (have {}, need {})", total(), mana_cost.manaValue());
        return false;
    }

    // Work with a copy for simulation
    auto remaining = mana;

    // First pay colored costs
    for (const auto& [color, required] : mana_cost.cost) {
        if (remaining[color] < required) {
            spdlog::debug("Not enough {} mana (have {}, need {})", ::toString(color),
                          remaining[color], required);
            return false;
        }
        remaining[color] -= required;
    }

    // Then check for generic mana needed
    int generic_needed = mana_cost.generic;
    int available_mana = 0;
    for (const auto& [color, amount] : remaining) {
        available_mana += amount;
    }

    spdlog::debug("For generic cost: need {}, have {} available", generic_needed, available_mana);

    spdlog::debug("Can pay: {}", generic_needed <= available_mana);
    return generic_needed <= available_mana;
}

void Mana::pay(const ManaCost& mana_cost) {
    if (!canPay(mana_cost)) {
        throw std::runtime_error("Not enough mana to pay for mana cost.");
    }

    // Pay colored mana costs
    for (const auto& [color, required] : mana_cost.cost) {
        mana[color] -= required;
    }

    // Pay generic mana costs
    int generic_needed = mana_cost.generic;
    while (generic_needed > 0) {
        for (auto& [color, amount] : mana) {
            if (amount > 0) {
                int to_pay = std::min(amount, generic_needed);
                amount -= to_pay;
                generic_needed -= to_pay;
                if (generic_needed == 0) {
                    break;
                }
            }
        }
        if (generic_needed > 0) {
            throw std::runtime_error("Error during payment.");
        }
    }
}

void Mana::clear() {
    for (auto& [color, amount] : mana) {
        amount = 0;
    }
}

std::string Mana::toString() const {
    std::string result = "{ ";
    for (const auto& [color, amount] : mana) {
        if (amount > 0) {
            result += ::toString(color) + ": " + std::to_string(amount) + ", ";
        }
    }
    result += "}";
    return result;
}