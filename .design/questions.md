# Open Questions

## Performance Targets

1. **What's the target throughput?** Current 270 games/sec (12.6k steps/sec) - is this sufficient for training requirements?

2. **What's the Python-side overhead?** This diagnosis covers only the C++ managym side. Need to profile the full manabot pipeline including model inference to understand the complete picture.

## Profiling Gaps

3. **Need profile with larger board states.** Current test uses simple 20-card decks. Performance characteristics may differ significantly with:
   - More permanents on battlefield
   - Larger hand sizes
   - More complex mana costs
   - Cards with activated abilities

4. **Need multi-threaded profile.** Single-threaded gives clean data but doesn't reflect production use. Potential contention points:
   - Thread-local RNG
   - Profiler data structures (if shared)
   - Memory allocation

## Architecture Clarifications

5. **Is the 7x tick amplification expected?** For 9,304 agent steps, we see 68,210 game ticks. Even with skip_trivial=True, Magic's turn structure causes significant internal work. Is there room to skip more phases/steps that have no decisions?

6. **Why create observations on every tick?** The current implementation creates a new Observation in Game::tick() even for internal ticks that won't be returned to the agent. Was this intentional for debugging, or an oversight?

7. **Should action allocation be pooled?** availablePriorityActions() allocates new vectors and unique_ptrs per call. With ~90k priority checks per profile run, this is significant allocation churn. Worth pooling?

## Instrumentation Notes

8. **Profile comparison is additive.** The new `compareToBaseline()` compares cumulative profiler data. To measure optimization impact accurately, reset profiler between runs or create fresh Env instances. The current design accumulates stats across the lifetime of the Profiler object.

8b. ~~**Skip-trivial loop visibility?**~~ **RESOLVED**: Added `skip_trivial` profiler scope at `game.cpp:225`. Shows 5.4x amplification (400,128 trivial iterations for 73,652 agent steps) consuming 72.3% of env_step time. See `.design/instrumentation-skip-trivial.md`.

## Refactor-Specific Questions

9. **Training mode vs evaluation mode?** The lazy priority and policy hints in the refactor proposals are most valuable for training. Should there be a fast training mode that assumes simple policies, or should the engine always enumerate all actions?

10. **Triggered abilities timeline?** The decision-driven tick loop and static feasibility proposals assume the turn structure is static. If "at beginning of upkeep" triggers are imminent, the feasibility checks become more complex. When are triggered abilities planned?

11. ~~**playersStartingWithActive() caching status?**~~ **RESOLVED**: Caching implemented at `turn.cpp:175-188`. Only rebuilds when `active_player_index` changes. Impact: +3.8% throughput.

12. **Combat phase behavior expectations?** Proposal 2 would skip the entire combat phase when there are no creatures. This matches current behavior with skip_trivial=True, but may need flags for future features. Is this acceptable?

## InfoDict Questions (NEW - 2026-01-16)

13. **Why is InfoDict built every step?** `env.cpp:81-82` calls `addProfilerInfo()` and `addBehaviorInfo()` on every step, consuming 54.6% of env_step time. Is there a requirement for per-step profiler/behavior data, or can this be deferred to episode end or explicit request?

14. ~~**Profiler overhead when disabled?**~~ Partially addressed: Phase/step scopes removed. Turn scope remains for useful profiling data. Full zero-overhead disabled profiling would require preprocessor macros.

## Resolved (2026-01-16)

13. ~~**Why is InfoDict built every step?**~~ **RESOLVED**: Changed to only build at episode end. Impact: +129% steps/sec.

6. ~~**Why create observations on every tick?**~~ **RESOLVED**: Lazy observation creation implemented. Observations only built when `Game::observation()` called, not during internal ticks.

## New Questions (Post-Optimization)

15. **Vector copy in priority.cpp**: `priority.cpp:32, 52, 97` copies the cached player vector into a local variable on every call. Should use `const std::vector<Player*>&` reference instead. Minor impact (~1-2%) but easy fix.

16. **Action allocation still high**: 556,702 PassPriority allocations per 500 games. The flyweight/pool pattern in refactor-proposal.md would eliminate these, but changes Action ownership semantics. Is the API change acceptable?

17. **Should skip_trivial integrate into PrioritySystem?**: Currently skip_trivial is a loop in `game.cpp:224-227` that calls `_step(0)` repeatedly. Integrating the trivial-check into `PrioritySystem::tick()` could avoid building ActionSpaces entirely for 97.9% of priority passes. Risk: must be 100% correct or real decisions get skipped.
