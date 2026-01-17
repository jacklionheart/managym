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
