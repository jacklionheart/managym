---
requires: .design/profile-*.md
produces: .design/diagnosis.md
---
Find the root cause of slowdowns in the tick loop and observation building.

## The two hot paths

1. **Tick loop**: `Game::step()` → `_step()` → `tick()` → `TurnSystem::tick()`
   - game.cpp:205-241 — step() calls _step(), which executes the action and calls tick()
   - game.cpp:225-241 — tick() loops calling TurnSystem::tick() until ActionSpace available
   - turn.cpp:147-160 — TurnSystem::tick() advances turn/phase/step state

2. **Observation building**: Built lazily when `Game::observation()` is called
   - observation.cpp:22-39 — constructor calls five populate* functions
   - populateTurn (45-62): turn/phase/step/player IDs
   - populateActionSpace (64-77): copies action options
   - populatePlayers (79-117): player state and zone counts
   - populateCards (119-153): hand, graveyard, exile, stack
   - populatePermanents (189-202): battlefield state

## Workflow

1. Read the latest `.design/profile-*.md`
2. Identify where time goes: tick loop vs observation vs unaccounted
3. Read the relevant code and find WHY it's slow
4. Write `.design/diagnosis.md`

## What to look for

**In the tick loop:**
- How many ticks per step? (profile shows tick count vs step count)
- What happens in each tick? Phase transitions? Priority checks?
- Is skip_trivial doing too much work? (game.cpp:210-212)

**In observation building:**
- Which populate* function dominates?
- Are we copying data that could be referenced?
- Are we iterating collections multiple times?

**In the unaccounted gap:**
- Action execution time (game.cpp:197)
- The skip_trivial loop (game.cpp:210-212 calls _step repeatedly)
- Zone lookups, player iteration, other per-step overhead

## Output

`.design/diagnosis.md`:

```markdown
# Diagnosis

## Summary
<One paragraph: what's slow and why>

## Primary bottleneck
**Where**: <file:line>
**Time**: X% of step
**Why**: <specific cause>

## Secondary bottlenecks
<Same format, in order of impact>

## Unaccounted time
<What's in the gap? What instrumentation would reveal it?>

## Recommendations
<Ordered by impact, with specific file:line targets>
```

Be specific. "Observation is slow" is useless. "populatePermanents iterates battlefield twice (observation.cpp:197, 221)" is actionable.
