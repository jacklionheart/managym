# Instrumentation

## skip_trivial_count

**Added**: 2026-01-16

**Location**: `Game::skip_trivial_count` (game.h:69)

**What it measures**: Number of trivial action spaces auto-executed by the skip_trivial optimization. A trivial action space has exactly 0 or 1 available actions.

**How it works**: Counter incremented in `Game::tick()` (game.cpp:254) each time a trivial action space is automatically executed without returning to the caller.

**Overhead**: Negligible. Single integer increment per skipped action.

**How to read it**:
```cpp
Game game(configs, &rng, /*skip_trivial=*/true, &profiler);
// ... run simulation ...
int skipped = game.skip_trivial_count;
```

After 500 games with Grey Ogre mirror (147 steps/game average):
- `skip_trivial_count` reveals how many additional ticks would have been external API calls
- Compare to `env_step` count: ratio shows skip_trivial efficiency
- High counts in specific phases indicate optimization opportunities

**What to do with the information**:

1. **Efficiency metric**: `skip_trivial_count / (skip_trivial_count + env_step_count)` = fraction of action spaces that were trivial. Higher is better.

2. **Optimization target**: If skip_trivial_count is low relative to turn system ticks, the skip_trivial logic might be filtering too aggressively in `computePlayerActions()`.

3. **Phase analysis**: Cross-reference with per-step profiler data. Steps with high priority pass counts but low external actions indicate efficient skipping.

**Example interpretation**:

If 500 games show:
- 73,652 env_steps (calls to `step()`)
- 300,000 skip_trivial_count

Then trivial actions are 300K/(300K+73K) = 80% of all action spaces. The optimization saved 300K external API round-trips.

If skip_trivial_count is surprisingly low (<50% of total ticks), investigate:
- Is `canPlayerAct()` returning true too often?
- Are combat steps creating many non-trivial action spaces?

## Added: action_execute/skip_trivial profiler scope

**Location**: `Game::tick()` (game.cpp:252-266)

**What it measures**: Time spent executing actions that are auto-executed by the `skip_trivial`
loop (trivial action spaces with <= 1 action).

**How to read it**:
```
env_step/game/tick/action_execute/skip_trivial: total=..., count=...
```
Compare this scope to `env_step/game/action_execute` to see how much action time is hidden inside
the skip-trivial fast path vs. explicit `step()` calls.

**Overhead**: Minimal; a single profiler scope per auto-executed trivial action.

**What to do with the information**:
- If `action_execute/skip_trivial` is large, the skip-trivial loop is doing real work and should be
  optimized just like explicit actions.
- If it is small, the unaccounted gap is likely elsewhere (allocations, zone operations, or
  action space creation).

## Added: phase/<PhaseType> profiler scope

**Location**: `Phase::tick()` (turn.cpp:146-176)

**What it measures**: Total time spent inside each phase type (BEGINNING, PRECOMBAT_MAIN, COMBAT,
POSTCOMBAT_MAIN, ENDING), inclusive of all steps within that phase.

**How to read it**:
```
env_step/game/tick/turn/phase/BEGINNING: total=..., count=...
env_step/game/tick/turn/phase/COMBAT: total=..., count=...
```
Compare totals to identify which phases dominate turn time.

**Overhead**: Minimal; one profiler scope per phase tick.

**What to do with the information**:
- If a phase dominates, drill into its `step/<StepType>` scopes to find the specific steps.
- Large time in `COMBAT` with low action counts suggests combat step logic or blockers/attackers
  enumeration is heavy.

## Added: observation realloc/<vector> profiler scopes

**Location**: `Observation::populateActionSpace`, `populateCards`, `populatePermanents`
(`managym/agent/observation.cpp`)

**What it measures**: Counts how often observation vectors grow their capacity (reallocate)
while building an observation. Each scope entry indicates a capacity increase in that vector.

**How to read it**:
```
env_step/observation/populateActionSpace/realloc/action_space/actions: total=..., count=...
env_step/observation/populateCards/realloc/agent_cards: total=..., count=...
env_step/observation/populateCards/realloc/opponent_cards: total=..., count=...
env_step/observation/populatePermanents/realloc/agent_permanents: total=..., count=...
env_step/observation/populatePermanents/realloc/opponent_permanents: total=..., count=...
```
Focus on the `count` column. Higher counts mean repeated reallocations and allocation churn.

**Overhead**: Minimal. A scope is created only when capacity increases.

**What to do with the information**:
- If counts are high, consider pre-sizing with `reserve()` or reusing buffers across steps.
- If counts are near zero, allocation churn is likely not the observation bottleneck.
