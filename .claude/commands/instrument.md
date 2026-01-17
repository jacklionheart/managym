---
produces: profiling infrastructure
---
Build tooling to make future simulation performance investigation easier.

## The two hot paths

1. **Tick loop**: game.cpp:205-241, turn.cpp:147-160
2. **Observation building**: observation.cpp:22-202

Instrumentation should reveal what's happening inside these, not around them.

## Current profiler coverage

The profiler (infra/profiler.h) already tracks:
- `env_step`, `env_reset` — top-level
- `game`, `tick` — game.cpp
- `observation`, `populateTurn`, `populatePlayers`, `populateCards`, `populatePermanents` — observation.cpp
- `turn`, `phase`, `step`, `priority` — turn.cpp

What's missing:
- Action execution time (game.cpp:197)
- Time inside skip_trivial loop vs single step
- Which phase/step types are slowest
- Memory allocation patterns

## Workflow

1. Identify a gap in current profiling
2. Add ONE piece of instrumentation
3. Verify it works and overhead is low
4. Document how to use it

## Instrumentation options

**Finer profiler scopes:**
```cpp
// game.cpp:197
{
    Profiler::Scope scope = profiler->track("action_execute");
    current_action_space->actions[action]->execute();
}
```

**Skip-trivial breakdown:**
```cpp
// game.cpp:210-212
int skip_count = 0;
while (!game_over && skip_trivial && actionSpaceTrivial()) {
    skip_count++;
    game_over = _step(0);
}
// Log skip_count or add to profiler
```

**Per-phase timing:**
```cpp
// turn.cpp - track which phase types take longest
profiler->track(fmt::format("phase/{}", phaseTypeName(type)));
```

**Memory tracking:**
```cpp
// observation.cpp - track vector allocations
size_t cards_before = agent_cards.capacity();
populateCards(game);
size_t cards_after = agent_cards.capacity();
// Log if capacity changed (indicates reallocation)
```

## Principles

**Low overhead.** Profiler scopes are cheap. Logging every call is not.

**Actionable.** Numbers should point to code locations.

**Optional.** New instrumentation should be behind a flag or compile-time option if it adds overhead.

## Output

The code change, plus a note in `.design/` explaining:
- What it measures
- How to read the output
- What to do with the information

Example:
```markdown
## Added: action_execute profiler scope

Shows time spent in Action::execute() per step.

If this dominates, look at the specific action types being executed.
Most common: PassPriority, PlayLand, CastSpell.
```
