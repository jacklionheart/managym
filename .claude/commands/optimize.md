---
requires: .design/diagnosis.md
produces: faster code
---
Fix one bottleneck. Measure improvement. Stop.

## Manabot Usage Context

Observations are consumed IMMEDIATELY after every step()—all fields are used for action selection, validation, and buffer storage. "Lazy" or "deferred" observation creation at the API boundary provides NO benefit. Focus on making observation building FAST, not on when it happens.

## The two hot paths

1. **Tick loop**: game.cpp:205-241, turn.cpp:147-160 (primary target)
2. **Observation building**: observation.cpp:22-202 (optimize for speed)

If the bottleneck is elsewhere, it's probably not worth optimizing.

## Workflow

1. Read `.design/diagnosis.md` — find the top bottleneck
2. Read the code at that location
3. Fix it with minimal changes
4. Run tests: `cd build && make run_tests`
5. Profile: `python scripts/profile.py --games 500 --seed 42`
6. Compare games/sec to baseline

If improvement is <5%, consider whether it's worth the complexity.

## Optimization targets

**Tick loop (game.cpp):**
- Reduce ticks per step — can we generate ActionSpace earlier?
- skip_trivial loop (210-212) — is it doing redundant work?
- Action execution (197) — is validate/execute doing too much?

**Observation building (observation.cpp):**
- populatePermanents (189-202) — iterates battlefield AND calls addCard which iterates again
- populateCards (119-153) — multiple zone iterations
- addCard (156-187) — copies a lot of data per card

**Data copying:**
- CardData copies string name (observation.cpp:164)
- ManaCost copied per card (observation.cpp:161-162)
- vector push_back instead of pre-sized arrays

## Principles

**Delete before optimize.** If work isn't needed, remove it.

**Measure.** No change without before/after numbers.

**One fix per pass.** This command runs repeatedly. Don't batch.

## What to avoid

**Optimizing the wrong thing.** If diagnosis says tick loop is 60% and observation is 18%, start with tick loop.

**Breaking the API.** Observation struct layout is visible to Python. Changes there require pybind.cpp updates.

**Clever hacks.** A 10% gain that's fragile is worse than 5% that's solid.

## After

If tests pass and throughput improved, commit with:
```
perf: <what you fixed>

Before: X games/sec
After: Y games/sec
```

Update `.design/diagnosis.md` to mark the bottleneck addressed.
