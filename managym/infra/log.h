#pragma once

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <set>
#include <string>

namespace managym::log {

// Core categories for subsystem logging 
enum struct Category {
    AGENT,    // Action execution
    STATE,    // Game state changes
    RULES,    // Rule engine
    TURN,     // Turn/phase/step transitions
    PRIORITY, // Priority system
    COMBAT,   // Combat events
    TEST      // Test cases
};

// Initialize logging with filtered categories
void initialize(const std::set<Category>& categories, bool debug_mode);

// Convert category to display string
std::string categoryToString(Category cat);

// Parse category from command line arg
Category categoryFromString(const std::string& str);

// Parse comma-separated category list from command line
std::set<Category> parseCategoryString(const std::string& categories);

// Check if category is enabled
bool isCategoryEnabled(Category cat);

// Generic logging function that forwards to spdlog
template <typename... Args>
void log(spdlog::level::level_enum level, Category cat, fmt::format_string<Args...> fmt, Args&&... args) {
    if (!isCategoryEnabled(cat))
        return;
        
    spdlog::log(level, "[{}] {}", categoryToString(cat), fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void debug(Category cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::debug, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(Category cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::info, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(Category cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::warn, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(Category cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::err, cat, fmt, std::forward<Args>(args)...);
}

} // namespace managym::log

using Category = managym::log::Category;