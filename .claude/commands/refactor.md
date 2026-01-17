---
requires: .design/diagnosis.md
produces: .design/refactor-proposal.md
---
Propose bold architectural changes for maximum performance impact.

## The two hot paths

1. **Tick loop**: game.cpp:205-241, turn.cpp:147-160
2. **Observation building**: observation.cpp:22-202

Bold means: not "cache this value" but "restructure how the tick loop works" or "change how observations are represented."

## Workflow

1. Read `.design/diagnosis.md`
2. Read the hot path code
3. Ask: what would make this 10x faster?
4. Write 2-4 concrete proposals to `.design/refactor-proposal.md`

## Questions to ask

**Tick loop:**
- Why do we tick multiple times per step? Could we batch?
- Does TurnSystem need to be so granular? Phases within phases within phases.
- Is the ActionSpace abstraction costing us? Building it, checking it, clearing it.

**Observation building:**
- Why rebuild from scratch every step? Incremental updates?
- Why copy data? Could Python read C++ memory directly?
- Why populate everything? Could we defer unused fields?

**The gap between them:**
- skip_trivial calls _step repeatedly. Each _step clears observation. Wasteful.
- Action execution creates/destroys objects. Pool them?

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
