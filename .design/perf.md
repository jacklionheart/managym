# perf

Performance optimization branch for managym. Adds profiling infrastructure and implements initial optimizations.

## Review

**Verdict:** Ready to ship

The branch adds performance command infrastructure and makes two concrete optimizations:
1. Lazy observation creation (eliminates ~7x redundant observation builds per step)
2. Maps-to-vectors conversion for observation data (faster iteration, less allocation overhead)

Both changes are well-tested and maintain API compatibility. The Python binding docstrings correctly reflect the vector-based observation format.

## Changes

### Performance Commands (`.claude/commands/`)
Five new LLM-executable commands for performance workflow:
- `profile.md` - Gather baseline performance data
- `diagnose.md` - Analyze profiler output for bottlenecks
- `optimize.md` - Implement targeted fixes
- `refactor.md` - Propose architectural changes
- `instrument.md` - Add profiling infrastructure

### Lazy Observation Creation (`game.cpp`)
**Before**: Observation created on every tick (~68k times for 9k agent steps)
**After**: Observation created lazily on first access

```cpp
Observation* Game::observation() {
    if (!current_observation) {
        current_observation = std::make_unique<Observation>(this);
    }
    return current_observation.get();
}
```

Removed observation creation from `tick()` - now only built when the API actually needs it.

### Maps to Vectors (`observation.h`, `observation.cpp`)
**Before**: `std::map<int, CardData>` and `std::map<int, PermanentData>`
**After**: `std::vector<CardData>` and `std::vector<PermanentData>`

Rationale: The maps were keyed by ID but never looked up by ID in hot paths. The observation is rebuilt each step anyway, so the O(1) lookup benefit didn't materialize. Vectors have better cache locality and lower allocation overhead.

### Test Updates (`test_observation.cpp`)
Added helper functions `findCardById()` and `findPermanentById()` to replace map lookups in tests. All tests updated to use the new vector-based API.

## Design Notes

### Baseline Performance
- 271 games/sec, 12,612 steps/sec
- 7.3x tick amplification (68k ticks for 9k agent steps)
- Turn/phase/step hierarchy: 23% of time
- Observation creation: 17% of time
- Priority system: 8% of time

### Remaining Optimization Opportunities
1. **Lazy observation creation** was implemented here. Expected to save ~17% of tick overhead.
2. **Producible mana caching** - not yet implemented. Each priority check iterates all permanents to calculate available mana.
3. **Turn structure batching** - architectural change to skip empty phases/steps entirely rather than just trivial priority passes.

### Questions Captured
See `.design/questions.md` for open questions about target throughput, Python-side overhead, and profiling gaps.
