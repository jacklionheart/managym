#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// A single timing node that represents a named timed block.
struct TimingNode {
    std::string label;     ///< Local label (without parent path)
    TimingNode* parent;    ///< Pointer to parent node (or nullptr for a root)
    double previous_total; ///< Accumulated elapsed time (seconds) from previous runs
    std::chrono::high_resolution_clock::time_point start_time; ///< Start time of current run
    bool running; ///< Flag indicating whether this timer is currently running
    std::unordered_map<std::string, std::unique_ptr<TimingNode>>
        children;                  ///< Child nodes keyed by label
    int count;                     ///< How many times this node was entered
    std::vector<double> durations; ///< Sampled durations (in seconds)
    int max_samples;               ///< Maximum samples to keep

    TimingNode(const std::string& label, TimingNode* parent = nullptr, int max_samples = 100);
    void start();
    double stop();
    double running_total() const;
};

class Profiler {
public:
    /// Structure holding computed statistics for a timing node.
    struct Stats {
        double total_time;    ///< Total accumulated time in seconds.
        double pct_of_parent; ///< Percentage of parent's time.
        double pct_of_total;  ///< Percentage of overall root total.
        int count;            ///< Number of times entered.
        double min;           ///< Minimum duration sample.
        double max;           ///< Maximum duration sample.
        double mean;          ///< Mean duration.
        double p5;            ///< 5th percentile.
        double p95;           ///< 95th percentile.
    };

    /// Constructs a profiler.
    explicit Profiler(bool enabled = true, int max_samples = 100);
    ~Profiler();

    /// RAII helper for timing a scope.
    class Scope {
    public:
        Scope(Profiler* profiler, TimingNode* node);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        Profiler* profiler;
        TimingNode* node;
        bool dismissed;
    };

    /// Begins a new timed scope with the given label.
    /// The returned Scope object will end the timer on destruction.
    Scope track(const std::string& label);

    /// Returns a mapping from the full hierarchical path (e.g. "A/B") to computed statistics.
    std::unordered_map<std::string, Stats> getStats() const;

    /// Reset the profiler (clears all accumulated data).
    void reset();

    bool isEnabled() const { return enabled; }

    /// Return a pretty-printed string of all profiler statistics.
    std::string toString() const;

private:
    bool enabled;
    int max_samples;
    std::vector<TimingNode*> stack;                          ///< Stack of currently running nodes.
    std::unordered_map<std::string, TimingNode*> node_cache; ///< Map full path -> node.

    /// Returns the full path for the given node (e.g. "A/B/C").
    std::string getFullPath(TimingNode* node) const;

    /// Ends the timing for the given node (called from Scope destructor).
    void endScope(TimingNode* node);
};
