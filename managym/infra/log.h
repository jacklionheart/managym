#pragma once

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <set>
#include <string>

// Core categories for subsystem logging
enum struct LogCat {
    AGENT,    // Action execution
    STATE,    // Game state changes
    RULES,    // Rule engine
    TURN,     // Turn/phase/step transitions
    PRIORITY, // Priority system
    COMBAT,   // Combat events
    TEST      // Test cases
};

// Initialize logging with filtered categories
void initialize_logging(const std::set<LogCat>& categories, spdlog::level::level_enum level);

// Convert LogCat to display string
std::string LogCatToString(LogCat cat);

// Parse LogCat from command line arg
LogCat LogCatFromString(const std::string& str);

// Parse comma-separated LogCat list from command line
std::set<LogCat> parseLogCatString(const std::string& categories);

// Check if LogCat is enabled
bool isLogCatEnabled(LogCat cat);

// Generic logging function that forwards to spdlog
template <typename... Args>
void log(spdlog::level::level_enum level, LogCat cat, fmt::format_string<Args...> fmt,
         Args&&... args) {
    if (!isLogCatEnabled(cat))
        return;

    spdlog::log(level, "[{}] {}", LogCatToString(cat),
                fmt::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void log_debug(LogCat cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::debug, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_info(LogCat cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::info, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_warn(LogCat cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::warn, cat, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void log_error(LogCat cat, fmt::format_string<Args...> fmt, Args&&... args) {
    log(spdlog::level::err, cat, fmt, std::forward<Args>(args)...);
}

class LogScope {
public:
    explicit LogScope(spdlog::level::level_enum new_level) : old_level(spdlog::get_level()) {
        spdlog::set_level(new_level);
    }
    ~LogScope() { spdlog::set_level(old_level); }

private:
    spdlog::level::level_enum old_level;
};