#include "profiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <fmt/format.h>

// -------------------- TimingNode Implementation --------------------

TimingNode::TimingNode(const std::string& label, TimingNode* parent, int max_samples)
    : label(label), parent(parent), previous_total(0.0), running(false), count(0),
      max_samples(max_samples) {}

void TimingNode::start() {
    if (running) {
        throw std::runtime_error("Timer '" + label + "' is already running.");
    }
    start_time = std::chrono::high_resolution_clock::now();
    running = true;
    count++;
}

double TimingNode::stop() {
    if (!running) {
        throw std::runtime_error("Timer '" + label + "' was not started.");
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    double elapsed_sec = elapsed.count();
    previous_total += elapsed_sec;
    if (durations.size() < static_cast<std::size_t>(max_samples)) {
        durations.push_back(elapsed_sec);
    } else {
        int r = std::rand() % count;
        if (r < max_samples) {
            durations[r] = elapsed_sec;
        }
    }
    running = false;
    return elapsed_sec;
}

double TimingNode::running_total() const {
    double total = previous_total;
    if (running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> current = now - start_time;
        total += current.count();
    }
    return total;
}

// -------------------- Profiler Implementation --------------------

Profiler::Profiler(bool enabled, int max_samples) : enabled(enabled), max_samples(max_samples) {}

Profiler::~Profiler() {
    // Nodes are owned by unique_ptr in children maps; no manual deletion is required.
}

std::string Profiler::getFullPath(TimingNode* node) const {
    if (!node) {
        return "";
    }
    std::vector<std::string> parts;
    while (node != nullptr) {
        parts.push_back(node->label);
        node = node->parent;
    }
    std::reverse(parts.begin(), parts.end());
    std::string path;
    for (std::size_t i = 0; i < parts.size(); i++) {
        if (i > 0) {
            path += "/";
        }
        path += parts[i];
    }
    return path;
}

Profiler::Scope Profiler::track(const std::string& label) {
    if (!enabled) {
        return Scope(this, nullptr);
    }
    if (label.empty()) {
        throw std::runtime_error("Empty label is not allowed");
    }
    std::string full_label;
    TimingNode* parent = nullptr;
    if (stack.empty()) {
        full_label = label;
    } else {
        parent = stack.back();
        full_label = getFullPath(parent) + "/" + label;
    }
    TimingNode* node = nullptr;
    auto it = node_cache.find(full_label);
    if (it != node_cache.end()) {
        node = it->second;
        if (node->running) {
            throw std::runtime_error("Node '" + full_label + "' is already running.");
        }
    } else {
        node = new TimingNode(label, parent, max_samples);
        node_cache[full_label] = node;
        if (parent != nullptr) {
            parent->children[label] = std::unique_ptr<TimingNode>(node);
        }
    }
    node->start();
    stack.push_back(node);
    return Scope(this, node);
}

void Profiler::endScope(TimingNode* node) {
    if (!stack.empty() && stack.back() == node) {
        node->stop();
        stack.pop_back();
    } else {
        throw std::runtime_error("Profiler stack corruption detected while ending scope for '" +
                                 node->label + "'");
    }
}

void Profiler::reset() {
    stack.clear();
    node_cache.clear();
}

std::unordered_map<std::string, Profiler::Stats> Profiler::getStats() const {
    std::unordered_map<std::string, Stats> stats_map;
    double overall_total = 0.0;
    for (const auto& kv : node_cache) {
        if (kv.second->parent == nullptr) {
            overall_total += kv.second->running_total();
        }
    }
    for (const auto& kv : node_cache) {
        std::string path = kv.first;
        TimingNode* node = kv.second;
        double node_total = node->running_total();
        double parent_total = (node->parent) ? node->parent->running_total() : overall_total;
        Stats s;
        s.total_time = node_total;
        s.pct_of_parent = (parent_total > 0) ? (node_total / parent_total * 100.0) : 0.0;
        s.pct_of_total = (overall_total > 0) ? (node_total / overall_total * 100.0) : 0.0;
        s.count = node->count;
        if (!node->durations.empty()) {
            std::vector<double> sorted = node->durations;
            std::sort(sorted.begin(), sorted.end());
            s.min = sorted.front();
            s.max = sorted.back();
            s.mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
            std::size_t idx_p5 = std::max<size_t>(std::ceil(0.05 * sorted.size()) - 1, 0);
            std::size_t idx_p95 = std::max<size_t>(std::ceil(0.95 * sorted.size()) - 1, 0);
            s.p5 = sorted[idx_p5];
            s.p95 = sorted[idx_p95];
        } else {
            s.min = s.max = s.mean = s.p5 = s.p95 = 0.0;
        }
        stats_map[path] = s;
    }
    return stats_map;
}

// -------------------- Profiler::Scope Implementation --------------------

Profiler::Scope::Scope(Profiler* profiler, TimingNode* node)
    : profiler(profiler), node(node), dismissed(false) {}

Profiler::Scope::~Scope() {
    if (!dismissed && profiler && node) {
        profiler->endScope(node);
    }
}

// -------------------- Pretty Print API --------------------
// This function produces a nicely formatted, multi-line string representation of the profiler stats.
std::string Profiler::toString() const {
    std::unordered_map<std::string, Stats> stats = getStats();
    std::vector<std::string> keys;
    for (const std::pair<const std::string, Stats>& kv : stats) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    std::string output;
    output += "Profiler Statistics:\n";
    for (const std::string& key : keys) {
        const Stats& s = stats.at(key);
        output += fmt::format("  {}\n    Total Time: {:.6f} s\n    Count: {}\n    %% of Parent: {:.2f}%\n    %% of Total: {:.2f}%\n    Min: {:.6f} s\n    Max: {:.6f} s\n    Mean: {:.6f} s\n    5th Percentile: {:.6f} s\n    95th Percentile: {:.6f} s\n",
                              key, s.total_time, s.count, s.pct_of_parent, s.pct_of_total,
                              s.min, s.max, s.mean, s.p5, s.p95);
    }
    return output;
}

// -------------------- Baseline Export/Compare API --------------------

std::string Profiler::exportBaseline() const {
    std::unordered_map<std::string, Stats> stats = getStats();
    std::vector<std::string> keys;
    for (const std::pair<const std::string, Stats>& kv : stats) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    std::ostringstream output;
    for (const std::string& key : keys) {
        const Stats& s = stats.at(key);
        output << key << "\t" << s.total_time << "\t" << s.count << "\n";
    }
    return output.str();
}

std::unordered_map<std::string, std::pair<double, int>> Profiler::parseBaseline(
    const std::string& baseline) {
    std::unordered_map<std::string, std::pair<double, int>> result;
    std::istringstream input(baseline);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        // Parse tab-separated: path\ttotal_time\tcount
        size_t tab1 = line.find('\t');
        if (tab1 == std::string::npos) {
            continue;
        }
        size_t tab2 = line.find('\t', tab1 + 1);
        if (tab2 == std::string::npos) {
            continue;
        }

        std::string path = line.substr(0, tab1);
        double total_time = std::stod(line.substr(tab1 + 1, tab2 - tab1 - 1));
        int count = std::stoi(line.substr(tab2 + 1));

        result[path] = std::make_pair(total_time, count);
    }

    return result;
}

std::string Profiler::compareToBaseline(const std::string& baseline) const {
    std::unordered_map<std::string, std::pair<double, int>> baseline_stats = parseBaseline(baseline);
    std::unordered_map<std::string, Stats> current_stats = getStats();

    // Collect all keys from both
    std::vector<std::string> keys;
    for (const std::pair<const std::string, std::pair<double, int>>& kv : baseline_stats) {
        keys.push_back(kv.first);
    }
    for (const std::pair<const std::string, Stats>& kv : current_stats) {
        if (baseline_stats.find(kv.first) == baseline_stats.end()) {
            keys.push_back(kv.first);
        }
    }
    std::sort(keys.begin(), keys.end());

    std::ostringstream output;
    output << "Profile Comparison (baseline vs current):\n";
    output << fmt::format("{:<50} {:>12} {:>12} {:>10} {:>10}\n",
                          "Path", "Baseline", "Current", "Change", "Count");
    output << std::string(94, '-') << "\n";

    for (const std::string& key : keys) {
        bool in_baseline = baseline_stats.find(key) != baseline_stats.end();
        bool in_current = current_stats.find(key) != current_stats.end();

        if (in_baseline && in_current) {
            double base_time = baseline_stats[key].first;
            int base_count = baseline_stats[key].second;
            double curr_time = current_stats[key].total_time;
            int curr_count = current_stats[key].count;

            double pct_change = 0.0;
            if (base_time > 0) {
                pct_change = ((curr_time - base_time) / base_time) * 100.0;
            }

            std::string change_str;
            if (pct_change > 1.0) {
                change_str = fmt::format("+{:.1f}%", pct_change);
            } else if (pct_change < -1.0) {
                change_str = fmt::format("{:.1f}%", pct_change);
            } else {
                change_str = "~0%";
            }

            output << fmt::format("{:<50} {:>10.4f}s {:>10.4f}s {:>10} {:>10}\n",
                                  key, base_time, curr_time, change_str,
                                  curr_count - base_count);
        } else if (in_baseline) {
            double base_time = baseline_stats[key].first;
            output << fmt::format("{:<50} {:>10.4f}s {:>12} {:>10} {:>10}\n",
                                  key, base_time, "(removed)", "-100%", "N/A");
        } else {
            double curr_time = current_stats[key].total_time;
            int curr_count = current_stats[key].count;
            output << fmt::format("{:<50} {:>12} {:>10.4f}s {:>10} {:>10}\n",
                                  key, "(new)", curr_time, "+NEW", curr_count);
        }
    }

    return output.str();
}
