// managym/infra/log.cpp
#include "log.h"

#include <sstream>

static std::set<LogCat> enabled_categories;

std::string LogCatToString(LogCat cat) {
    switch (cat) {
    case LogCat::TURN:
        return "turn";
    case LogCat::STATE:
        return "state";
    case LogCat::RULES:
        return "rules";
    case LogCat::COMBAT:
        return "combat";
    case LogCat::PRIORITY:
        return "priority";
    case LogCat::AGENT:
        return "agent";
    case LogCat::TEST:
        return "test";
    default:
        return "???";
    }
}

LogCat LogCatFromString(const std::string& input) {
    // Convert input to lowercase for comparison
    std::string str = input;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str == "turn")
        return LogCat::TURN;
    if (str == "state")
        return LogCat::STATE;
    if (str == "rules")
        return LogCat::RULES;
    if (str == "combat")
        return LogCat::COMBAT;
    if (str == "priority")
        return LogCat::PRIORITY;
    if (str == "agent")
        return LogCat::AGENT;
    if (str == "test")
        return LogCat::TEST;
    throw std::invalid_argument("Invalid LogCat: " + input);
}

std::set<LogCat> parseLogCatString(const std::string& categories) {
    std::set<LogCat> result;
    std::stringstream ss(categories);
    std::string LogCat;

    while (std::getline(ss, LogCat, ',')) {
        // Trim whitespace
        LogCat.erase(0, LogCat.find_first_not_of(" \t\r\n"));
        LogCat.erase(LogCat.find_last_not_of(" \t\r\n") + 1);
        if (!LogCat.empty()) {
            result.insert(LogCatFromString(LogCat));
        }
    }

    return result;
}

void initialize_logging(const std::set<LogCat>& categories, spdlog::level::level_enum level) {
    enabled_categories = categories;
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(level);
}

bool isLogCatEnabled(LogCat cat) {
    return enabled_categories.empty() || enabled_categories.count(cat) > 0;
}
