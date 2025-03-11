#include "mana.h"

#include "managym/infra/log.h"

#include <cctype>
#include <numeric>
#include <stdexcept>

ManaCost::ManaCost() : cost{0, 0, 0, 0, 0, 0}, mana_value(0) {}

ManaCost ManaCost::parse(const std::string& mana_str) {
    ManaCost result;
    int generic = 0;

    size_t i = 0;
    while (i < mana_str.length()) {
        char c = mana_str[i];
        if (std::isdigit(c)) {
            int num = 0;
            while (i < mana_str.length() && std::isdigit(mana_str[i])) {
                num = num * 10 + (mana_str[i] - '0');
                i++;
            }
            result.cost[6] = num;
        } else {
            switch (c) {
            case 'W':
                result.cost[0]++;
                break;
            case 'U':
                result.cost[1]++;
                break;
            case 'B':
                result.cost[2]++;
                break;
            case 'R':
                result.cost[3]++;
                break;
            case 'G':
                result.cost[4]++;
                break;
            case 'C':
                result.cost[5]++;
                break;

            default:
                throw std::invalid_argument("Invalid mana symbol: " + std::string(1, c));
            }
            i++;
        }
    }

    result.mana_value = std::accumulate(result.cost.begin(), result.cost.end(), 0);
    return result;
}

std::string ManaCost::toString() const {
    std::string result;

    // Generic first (as colorless)
    if (cost[0] > 0) {
        result += std::to_string(cost[0]);
    }

    // Colored symbols
    static const char* symbols = "CWUBRG";
    for (int i = 1; i < 6; i++) {
        for (int j = 0; j < cost[i]; j++) {
            result += symbols[i];
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
    Colors result;
    for (int i = 0; i < 5; i++) {
        if (cost[i] > 0) {
            result.insert(static_cast<Color>(i));
        }
    }
    return result;
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
    log_debug(LogCat::RULES, "Mana: {}", toString());
    log_debug(LogCat::RULES, "Checking if can pay Mana cost: {}", mana_cost.toString());
    if (total() < mana_cost.mana_value) {
        log_debug(LogCat::RULES, "Not enough total mana (have {}, need {})", total(),
                  mana_cost.mana_value);
        return false;
    }

    // Work with a copy for simulation
    auto remaining = mana;

    // First pay colored costs (include colorless)
    for (int i = 0; i < 6; i++) {
        if (remaining[static_cast<Color>(i)] < mana_cost.cost[i]) {
            log_debug(LogCat::RULES, "Not enough {} mana (have {}, need {})",
                      ::toString(static_cast<Color>(i)), remaining[static_cast<Color>(i)],
                      mana_cost.cost[i]);
            return false;
        }
        remaining[static_cast<Color>(i)] -= mana_cost.cost[i];
    }

    // Then check for generic mana needed
    int generic_needed = mana_cost.cost[6];
    int available_mana = 0;
    for (const auto& [color, amount] : remaining) {
        available_mana += amount;
    }

    log_debug(LogCat::RULES, "For generic cost: need {}, have {} available", generic_needed,
              available_mana);
    log_debug(LogCat::RULES, "Can pay: {}", generic_needed <= available_mana);
    return generic_needed <= available_mana;
}

void Mana::pay(const ManaCost& mana_cost) {
    if (!canPay(mana_cost)) {
        throw std::runtime_error("Not enough mana to pay for mana cost.");
    }

    // Pay colored mana costs
    for (int i = 0; i < 6; i++) {
        mana[static_cast<Color>(i)] -= mana_cost.cost[i];
    }

    // Pay generic mana costs
    int generic_needed = mana_cost.cost[6];
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