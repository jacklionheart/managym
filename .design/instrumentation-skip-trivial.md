# Instrumentation: skip_trivial profiler scope

## What it measures

Time and count of trivial action auto-executions in the skip_trivial loop.

**Location**: `game.cpp:224-227`

```cpp
while (!game_over && skip_trivial && actionSpaceTrivial()) {
    Profiler::Scope skip_scope = profiler->track("skip_trivial");
    game_over = _step(0);
}
```

## How to read the output

After running `python scripts/profile.py`, look for `env_step/game/skip_trivial`:

```
env_step/game/skip_trivial: total=0.280s, count=81123
```

Key metrics:
- **count**: Number of trivial actions auto-executed
- **total**: Time spent in the skip_trivial loop
- **ratio**: `skip_trivial.count / env_step.count` = amplification factor

Example interpretation (100 games):
- Agent steps: 14,940
- Skip trivial iterations: 81,123
- Amplification: 5.4x (81,123 / 14,940)
- Time in skip_trivial: 72.6% of env_step

## What to do with the information

**High amplification (>5x)**: Most of the tick loop work is handling trivial actions. The refactor proposals (integrated skip-trivial, flat state machine) target this.

**Time breakdown inside skip_trivial**:
- `skip_trivial/tick/turn` — Turn hierarchy overhead
- `skip_trivial/action_execute` — Action execution itself
- If turn hierarchy dominates, consider the flat state machine refactor

**Comparison across runs**: The ratio of skip_trivial iterations to agent steps indicates how "trivial" the game state is. Longer games with more creatures should have lower ratios (more real decisions).

## Example profile output

```
| Component | Time | % | Calls |
|-----------|------|---|-------|
| env_step | 0.386s | 100% | 14,940 |
| env_step/game | 0.347s | 90.0% | 14,940 |
| env_step/game/skip_trivial | 0.280s | 72.6% | 81,123 |
| env_step/game/skip_trivial/tick/turn | 0.191s | 49.6% | 126,802 |
| env_step/game/tick | 0.024s | 6.1% | 14,940 |
| env_step/observation | 0.031s | 8.0% | 14,940 |
```

This shows:
- 72.6% of time is in skip_trivial (auto-executing trivial actions)
- Only 6.1% is in the non-trivial tick path
- Turn hierarchy inside skip_trivial is 49.6% of total time

## Overhead

Minimal. The profiler scope is a cheap RAII wrapper that:
1. Records start timestamp on construction
2. Computes elapsed time and increments counter on destruction

The overhead is ~0.1μs per scope creation, which is negligible compared to the ~3.5μs per skip_trivial iteration.
