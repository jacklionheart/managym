# Diagnosis

## Summary
The tick loop is dominated by repeated priority passes inside `TurnSystem::tick()`; each pass rebuilds the priority action list and rechecks mana availability by scanning the battlefield. With ~4.2 turn ticks per env step (311,397 ticks vs 73,652 steps in `profile-20260116-1731`), the per-pass cost of `computePlayerActions()` compounds into the primary slowdown. Observation building is the next largest cost: each step fully rebuilds `CardData` and `PermanentData`, and `populatePermanents()` redundantly calls `addCard()` for battlefield cards, causing extra per-step `CardData` construction.

## Primary bottleneck
**Where**: `managym/flow/priority.cpp:20-57`
**Time**: ~8-12% of `env_step` (e.g., `PRECOMBAT_MAIN_STEP/priority` is 0.078s of 0.954s in `profile-20260116-1731.md`)
**Why**: `computePlayerActions()` scans the full hand every priority pass and, for each castable card, calls `canPay()` on a cached `Mana` computed via `Battlefield::producibleMana()` (iterates all permanents). With ~4-5 priority passes per step, this hand scan + mana check repeats excessively.
**Status**: Added a fast-path `canPlayerAct()` check for skip-trivial auto-pass to avoid constructing action vectors when no actions exist.

## Secondary bottlenecks
**Where**: `managym/agent/observation.cpp:192-225`
**Time**: 5.6% of `env_step`
**Why**: `populatePermanents()` walks battlefield permanents and then calls `addCard()` for each permanent, rebuilding `CardData` (mana cost copy + 12 type flag checks) for objects already represented as permanents.

**Where**: `managym/agent/observation.cpp:120-156` and `managym/agent/observation.cpp:160-189`
**Time**: 3.8% of `env_step`
**Why**: `populateCards()` re-walks multiple zones every step and calls `addCard()` per card, which recomputes `CardData` (including `ManaCost` copy and type flags) on every step, even for cards that are unchanged.

**Where**: `managym/agent/observation.cpp:64-76`
**Time**: Included in observation overhead (4.2% of `env_step`)
**Why**: `populateActionSpace()` copies the full action list and focus arrays into new vectors every step, which is allocation-heavy and not separately tracked in the profile.

## Unaccounted time
Stable runs show ~4% unaccounted. Likely sources are:
- auto-executed trivial actions inside `Game::tick()` (profiling only tracks `action_execute` and `action_execute/skip_trivial`, but other work in the skip loop is unscoped) (`managym/flow/game.cpp:232-262`)
- allocations for `ActionSpace`, `Action`, and per-step vectors during action/observation construction
- zone operations (`Zones::move`, `Zones::pushStack`) without dedicated profiler scopes

The 2026-01-16 17:50/17:58 profiles show 55% unaccounted time and `action_execute` counts far above `env_step`, which indicates a measurement anomaly or a run with different instrumentation; these are not reliable baselines.

## Recommendations
1. **Cache priority action availability per pass** (target `managym/flow/priority.cpp:20-57`)
   - Avoid re-scanning the hand and re-checking mana on every priority pass when no state has changed; cache playable cards per priority window or per player/pass.
2. **Remove redundant `CardData` building for battlefield permanents** (target `managym/agent/observation.cpp:208-225`)
   - Avoid `addCard()` for permanents or embed needed card fields directly into `PermanentData` to stop double work.
3. **Pre-size observation vectors and action lists** (target `managym/agent/observation.cpp:64-76`, `managym/agent/observation.cpp:120-156`)
   - Use zone counts to `reserve()` agent/opponent card and permanent vectors, and action vectors, to reduce allocation churn.
4. **Instrument skip-trivial loop and zone mutations** (targets `managym/flow/game.cpp:232-262`, `managym/state/zones.cpp:62-117`)
   - Add lightweight profiler scopes around auto-executed action paths and zone moves to close the unaccounted gap.
