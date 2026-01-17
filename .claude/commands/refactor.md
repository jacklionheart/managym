---
requires: .design/diagnosis.md
produces: .design/refactor-proposal.md
---
Propose bold architectural changes for maximum performance impact.

## Manabot Usage Patterns

**Critical context**: manabot consumes observations IMMEDIATELY after every step(). The training loop uses ALL observation fields right away for action selection, validation, and buffer storage. There is zero slack—observations cannot be deferred or lazy-loaded at the API boundary.

**What this means for refactoring:**
- "Defer unused fields" is NOT an option—all fields are used
- "Lazy observation" at API level provides NO benefit
- Focus on reducing work INSIDE observation building, not on when it happens
- The tick loop is the primary target for bold refactoring

## The two hot paths

1. **Tick loop**: game.cpp:205-241, turn.cpp:147-160 (PRIMARY TARGET)
2. **Observation building**: observation.cpp:22-202 (optimize for speed, not deferral)

Bold means: not "cache this value" but "restructure how the tick loop works" or "eliminate unnecessary iteration in observations."

## Workflow

1. Read `.design/diagnosis.md`
2. Read the hot path code
3. Ask: what would make this 10x faster?
4. Write 2-4 concrete proposals to `.design/refactor-proposal.md`

## Questions to ask

**Tick loop (primary target):**
- Why do we tick multiple times per step? Could we batch?
- Does TurnSystem need to be so granular? Phases within phases within phases.
- Is the ActionSpace abstraction costing us? Building it, checking it, clearing it.
- Can we skip steps/phases that cannot produce decisions?

**Observation building (optimize for speed):**
- Why rebuild from scratch every step? Incremental updates could help.
- Why copy data? Could Python read C++ memory directly?
- Are we iterating collections multiple times? Single-pass population?
- Note: ALL fields are used by manabot—cannot defer or skip fields.

**The gap between them:**
- skip_trivial calls _step repeatedly—good, avoids observation creation during internal ticks
- Action execution creates/destroys objects. Pool them?
- InfoDict construction on every step—should only build when requested

## What makes a proposal

A proposal is specific enough to implement:

```markdown
### Incremental observation updates
**Current**: Observation rebuilt from scratch every step (observation.cpp:22-39)
**Problem**: Most state doesn't change between steps
**Proposal**: Track dirty flags on game state. Only rebuild changed sections.
**Sketch**:
1. Add dirty bits to Player, Zones, TurnSystem
2. Observation caches previous values
3. populate* checks dirty flag, skips if clean
**Impact**: ~15% of step time (observation is 18%, most is redundant)
**Risk**: Dirty tracking bugs cause stale observations
```

Not a proposal: "Use caching." "Optimize the hot path." "Consider incremental updates."

## Output

`.design/refactor-proposal.md`:

```markdown
# Refactor Proposals

## Summary
<Core insight: what's fundamentally wrong with current design?>

## Proposals

### 1. <Name> — Impact: ~X%
**Current**: ...
**Problem**: ...
**Proposal**: ...
**Sketch**: ...
**Risk**: ...

### 2. <Name> — Impact: ~X%
...

## Sequence
<Which first? Dependencies?>

## Open questions
<What would you need to know before committing?>
```

## What to avoid

**Incremental fixes.** Those go in `optimize`.

**Vague ideas.** Every proposal needs a sketch.

**Breaking constraints.** Python bindings must work. The Observation struct is the API.

This produces a document. Implementation happens in `implement` after human review.
