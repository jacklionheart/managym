---
requires: diff vs main
produces: profiling infrastructure
---
Build tooling to make future simulation performance investigation easier.

## Scope

Instrumentation for the simulation loop: env step timing, observation encoding, inference latency.

NOT in scope: training loop profiling (already has wandb integration). Focus on `manabot/sim/` and `manabot/env/`.

## Goal

Add instrumentation, benchmarks, or profiling infrastructure that will help identify the NEXT bottleneck. After optimizing the obvious problems, you need better tools to find the subtle ones.

This is infrastructure work—the output is tooling, not performance gains.

## Workflow

1. Review what profiling exists (check `manabot/infra/profiler.py` and sim output)
2. Identify gaps: what can't you measure today that you wish you could?
3. Add ONE piece of instrumentation
4. Verify it works and doesn't add significant overhead
5. Document how to use it

## Types of instrumentation

**Finer-grained profiling:**
```python
# Add tracking to a hot function
with self.profiler.track("encode/cards"):
    self._encode_cards(obs)
with self.profiler.track("encode/actions"):
    self._encode_actions(obs)
```

**Memory tracking:**
```python
# Track allocations in critical paths
import tracemalloc
tracemalloc.start()
# ... code ...
snapshot = tracemalloc.take_snapshot()
```

**Micro-benchmarks:**
```python
# tests/benchmarks/test_encode_perf.py
def test_encode_throughput(benchmark):
    encoder = ObservationEncoder(hypers)
    obs = create_test_observation()
    benchmark(encoder.encode, obs)
```

**Comparison tooling:**
```python
# Script to compare two simulation runs
def compare_profiles(baseline: Path, current: Path) -> dict:
    """Return percentage changes for key metrics."""
```

## What makes good instrumentation

**Low overhead.** Instrumentation that slows things down defeats the purpose. Use sampling, not logging every call.

**Easy to enable/disable.** A flag or environment variable to turn it on. Off by default in production.

**Actionable output.** Numbers that point to specific code locations, not just "it's slow."

**Reproducible.** Same inputs should give similar numbers. Control for variance.

## Manabot-specific opportunities

**Profiler hierarchy gaps:** The existing profiler tracks high-level operations. Consider adding:
- Per-encoder-type timing (player vs card vs action encoding)
- Attention mechanism breakdown
- Action masking time

**Simulation statistics:**
- Distribution of game lengths
- Action type frequencies
- Steps where model inference is slowest

**Comparative tooling:**
- Script to run same simulation with different configs and compare
- Automatic baseline tracking in CI

## Output

The instrumentation itself, plus a brief note in `.design/` explaining:
- What it measures
- How to enable it
- How to interpret the output

Example:
```markdown
## New: Per-encoder profiling

Enable with `MANABOT_PROFILE_ENCODERS=1`

Output shows time breakdown:
- encode/player: X.XXms
- encode/cards: X.XXms
- encode/permanents: X.XXms
- encode/actions: X.XXms

Use this when encode() shows up as a bottleneck to identify which object type dominates.
```

## What to avoid

**Instrumentation that requires code changes to use.** Good tooling works via flags or config, not editing source.

**Logging instead of profiling.** Print statements are not instrumentation. They add overhead and require parsing output.

**Over-engineering.** A simple timing decorator is better than a complex tracing framework. Build what you need now.
