// managym/infra/log.cpp
#include "log.h"

namespace managym::log {

static std::set<Category> enabled_categories;

std::string categoryToString(Category cat) {
    switch (cat) {
    case Category::TURN:
        return "turn";
    case Category::STATE:
        return "state";
    case Category::RULES:
        return "rules";
    case Category::COMBAT:
        return "combat";
    case Category::PRIORITY:
        return "priority";
    case Category::AGENT:
        return "agent";
    case Category::TEST:
        return "test";
    default:
        return "???";
    }
}

Category categoryFromString(const std::string& input) {
    // Convert input to lowercase for comparison
    std::string str = input;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str == "turn")
        return Category::TURN;
    if (str == "state")
        return Category::STATE;
    if (str == "rules")
        return Category::RULES;
    if (str == "combat")
        return Category::COMBAT;
    if (str == "priority")
        return Category::PRIORITY;
    if (str == "agent")
        return Category::AGENT;
    if (str == "test")
        return Category::TEST;
    throw std::invalid_argument("Invalid category: " + input);
}

std::set<Category> parseCategoryString(const std::string& categories) {
    std::set<Category> result;
    std::stringstream ss(categories);
    std::string category;

    while (std::getline(ss, category, ',')) {
        // Trim whitespace
        category.erase(0, category.find_first_not_of(" \t\r\n"));
        category.erase(category.find_last_not_of(" \t\r\n") + 1);
        if (!category.empty()) {
            result.insert(categoryFromString(category));
        }
    }

    return result;
}

void initialize(const std::set<Category>& categories, spdlog::level::level_enum level) {
    enabled_categories = categories;
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(level);
}

bool isCategoryEnabled(Category cat) {
    return enabled_categories.empty() || enabled_categories.count(cat) > 0;
}

} // namespace managym::log