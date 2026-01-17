# Open Questions

## Build Environment

1. **Cannot verify build**: The build requires Python 3.12 and cmake, which aren't available in the current environment. The CMakeLists.txt requires `find_package(Python 3.12 EXACT ...)`. Need to verify the code compiles and tests pass.

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
