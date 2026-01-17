# Open Questions

## Build Environment

1. **Cannot verify build**: The build requires Python 3.12 and cmake, which aren't available in the current environment. The CMakeLists.txt requires `find_package(Python 3.12 EXACT ...)`. Need to verify the code compiles and tests pass.
2. **Build tools missing**: `cmake` and `python` are not available in this environment, so `make run_tests` and `scripts/profile.py` could not be run.
3. **Profile run blocked**: `python3 scripts/profile.py --games 500 --seed 42` failed with `ModuleNotFoundError: No module named 'managym'`. Need an environment with the `managym` module installed (or a local build) to capture throughput/profiler numbers.
4. **Profile run blocked (2026-01-16)**: `python scripts/profile.py --games 500 --seed 42` failed because `python` is not available; `python3` exists but cannot import `managym` (`ModuleNotFoundError`). Need a built/installed `managym` module to capture the requested profile.
5. **Profile run blocked (current)**: `/usr/bin/python3` is Python 3.9.6 and cannot import `managym`. `python` is not available. `CMakeLists.txt` requires Python 3.12 EXACT, so local build/install is blocked. Need a Python 3.12 environment with `managym` built/installed to run `scripts/profile.py --games 500 --seed 42`.

## Design Inputs

1. **Missing design doc for implement step**: No `.design/<branch>.md` or equivalent implementation plan is present in `.design/`. Unsure which spec to implement.
2. **Implement step assumption**: No implementation doc found, so no code changes were made. Provide the intended design doc or target changes to proceed.

## Implementation Decisions

2. **Observation constructor signature change**: Changed `Observation(const Game* game)` to `Observation(Game* game)` to support the cached `playersStartingWithAgent()` method. This is a minor API change but observation building doesn't require const access to the game.

3. **Proposals #4 and #5 deferred**: The refactor proposal mentioned two additional optimizations that were not implemented:
   - Proposal #4 (Single-pass observation population): Would require API changes to manabot
   - Proposal #5 (Convert `card_to_zone` from `std::map` to `std::vector`): Would require changes to zones.h/cpp

## Verification Needed

4. **Profile to verify improvements**: Run `/profile` after building to measure:
   - Expected ~8% improvement from fused priority check
   - Expected ~1.5% improvement from cached playersStartingWithAgent
   - Expected ~2% improvement from pre-allocated action vectors
   - Expected ~1-2% improvement from removed string copy in CardData
   - Total expected improvement: ~12-14% reduction in execution time

5. **CardData.name removal**: Removed `std::string name` field from CardData to avoid string copies. This was safe because:
   - The field was NOT exposed to Python via pybind11 (see pybind.cpp lines 236-254)
   - Only used by `toJSON()` debug method, which now omits it
   - Python code uses `registry_key` to look up card names from the card registry

6. **Profiler missing populateActionSpace**: The latest profiler output did not include an `env_step/observation/populateActionSpace` entry. Should this scope be present, or is it intentionally disabled/filtered?

7. **Profile-20260116-1750 anomaly**: That run shows 55% unaccounted time and `action_execute`
count (473,780) far above `env_step` count (73,652). Was this captured with different
instrumentation or a different branch/config?

## Refactor Proposals

1. Which step transitions are guaranteed to have no choices (e.g., cleanup, untap) for all current cardsets, and can we codify them for fast-forward?
2. What are the minimal mutation hooks needed to safely invalidate cached priority windows?
3. Can we keep the public Observation API intact while adding a cached internal representation, or do we need a versioned Observation struct?
4. Are Action objects ever referenced outside the action space lifetime (e.g., in logs or trackers)?
