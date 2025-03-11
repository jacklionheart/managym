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
    auto stats = getStats();
    std::vector<std::string> keys;
    for (const auto& kv : stats) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());
    std::string output;
    output += "Profiler Statistics:\n";
    for (const auto& key : keys) {
        const Stats& s = stats.at(key);
        output += fmt::format("  {}\n    Total Time: {:.6f} s\n    Count: {}\n    %% of Parent: {:.2f}%\n    %% of Total: {:.2f}%\n    Min: {:.6f} s\n    Max: {:.6f} s\n    Mean: {:.6f} s\n    5th Percentile: {:.6f} s\n    95th Percentile: {:.6f} s\n",
                              key, s.total_time, s.count, s.pct_of_parent, s.pct_of_total,
                              s.min, s.max, s.mean, s.p5, s.p95);
    }
    return output;
}
