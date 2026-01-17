# Open Questions

## From Refactor Analysis (2026-01-16)

### Implementation Questions

1. **Activated abilities timeline?** The `canPlayerAct()` fast-path only checks hand cards. When permanents get activated abilities, both `canPlayerAct()` and `availablePriorityActions()` need updating in sync. Is activated ability support imminent?

2. **Stack interaction validation?** The skip_trivial behavior auto-passes when the player can only pass priority (including when stack is non-empty). Need to verify this correctly handles all stack states—does a player ever have actions besides pass when responding to their own spell?

3. **First-turn draw skip?** The first player of the game doesn't draw on their first turn (MR103.7a). The current code handles this in the Turn object. After flattening, this needs to be tracked in TurnState. Verify test coverage exists.

4. **BehaviorTracker callback placement?** The flat state machine needs to call behavior callbacks (`onMainPhaseStart`, `onDeclareAttackersStart`, `onTurnEnd`, etc.) at the correct points. Should these be inline in `executeStepStart()`/`executeStepEnd()` or factored into separate methods?

5. **pybind11 Observation compatibility?** Observation reads from turn_system to populate TurnData. After flattening, it would read from TurnState instead of Turn/Phase/Step objects. Verify no Python-side code assumes specific Turn object behavior.

### Performance Questions

6. **Profiler overhead when disabled?** `Profiler::Scope` objects are created even with `enable_profiler=false`. The constructor checks `enabled` but the object still exists. Measure overhead of disabled profiler scopes in hot path.

7. **Python binding overhead?** pybind11 marshalling is not measured in C++ profiler. Could explain some unaccounted time. Worth instrumenting the full pipeline including Python side.

8. **Target throughput?** Current: 42k steps/sec. What's the target for manabot training? 50k? 100k? 200k? Knowing the target helps prioritize risky refactors vs stopping at "good enough."

### Future Compatibility Questions

9. **Triggered abilities?** "At beginning of upkeep" triggers would need the turn structure to fire at specific points. The flat state machine's `executeStepStart()` handles this, but the skip logic needs review. When are triggered abilities planned?

10. **Extra combat phases?** Some cards grant additional combat phases. The flat state machine would need to support re-entering combat. Is this on the roadmap?

11. **Multi-threading plans?** If simulation will be parallelized, the proposed changes need review for thread safety. The flat state machine is actually more thread-friendly (less shared state), but pools would need to be thread-local.

## Previously Resolved

- ~~**Why is InfoDict built every step?**~~ RESOLVED: Changed to only build at episode end. Impact: +129% steps/sec.
- ~~**Why create observations on every tick?**~~ RESOLVED: Lazy observation creation implemented.
- ~~**playersStartingWithActive() caching?**~~ RESOLVED: Caching implemented. Impact: +3.0% steps/sec.
- ~~**Skip-trivial loop visibility?**~~ RESOLVED: Added profiler scope. Shows 5.4x amplification (400,128 trivial iterations for 73,652 agent steps).
- ~~**Early-out priority check (Proposal 1)?**~~ RESOLVED: Implemented `canPlayerAct()` fast-path in priority.cpp. Impact: +66% steps/sec (42k → 71k). Reduces priority ticks from 557k to 228k.
