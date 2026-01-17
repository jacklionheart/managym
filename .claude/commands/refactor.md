---
requires: .design/diagnosis.md
produces: .design/refactor-proposal.md
---
Propose bold architectural changes for maximum performance impact.

## Scope

The simulation hot path: environment step, observation encoding, game state updates.

NOT in scope: training loop, PPO updates, API surface changes that break manabot compatibility.

## Goal

Think big. The `optimize` command makes careful, incremental fixes. This command asks: what would we rewrite from scratch? What abstractions are costing us? What would a 10x improvement require?

Output a proposal document with specific changes ranked by impact. The human decides which (if any) to pursue.

## Workflow

1. Read `.design/diagnosis.md` to understand current bottlenecks
2. Read the hot path code identified in the diagnosis
3. Step back and ask: what's the root cause behind these symptoms?
4. Propose 2-4 bold changes, ranked by expected impact
5. Write `.design/refactor-proposal.md`

## What makes a "bold" change

**Architectural rewrites.** Not "cache this result" but "restructure how we represent game state."

**Removing abstractions.** Layers of indirection that made sense for correctness but cost performance.

**Changing data layouts.** Struct-of-arrays instead of array-of-structs. Flat buffers instead of nested objects.

**Eliminating work entirely.** Question whether features are needed. Lazy vs eager computation. Incremental updates vs full rebuilds.

**Moving computation.** C++ instead of Python. Compile-time instead of runtime. GPU instead of CPU.

## Analysis approach

For each bottleneck in the diagnosis, ask:

1. **Why does this exist?** What problem was it solving?
2. **Is that problem still relevant?** Requirements change.
3. **What's the simplest possible solution?** Ignore the current implementation.
4. **What's the fastest possible solution?** Ignore maintainability.
5. **Where's the sweet spot?** Simple AND fast.

Example analysis:
```
Bottleneck: Observation encoding takes 40% of step time

Current: Build Python dict → convert to numpy → copy to tensor
Root cause: The abstraction serves Python flexibility, not performance

Bold proposal: Pre-allocate a fixed observation buffer in C++.
Update it incrementally as game state changes. Zero-copy to Python.
Expected impact: ~10x faster observation, saves 35% of step time.
Risk: Significant C++ work, harder to modify observation format.
```

## Output: .design/refactor-proposal.md

```markdown
# Refactor Proposal

## Summary
<What's the core insight? What big change would unlock performance?>

## Proposals

### 1. <Name> — Expected impact: XX%
**Current state:** <What exists now>
**Problem:** <Why it's slow>
**Proposal:** <What to change>
**Implementation sketch:** <Key steps, not full design>
**Risks:** <What could go wrong>

### 2. <Name> — Expected impact: XX%
...

## Recommended sequence
<Which to do first, dependencies between proposals>

## Questions
<Unknowns that affect feasibility>
```

## What to avoid

**Incremental improvements.** Those belong in `optimize`. This is for changes you'd need to think about before committing to.

**Vague suggestions.** "Use caching" isn't a proposal. "Cache the legal action mask and invalidate on state change" is.

**Ignoring constraints.** The API to manabot must remain stable. Python bindings are required. Don't propose changes that break the project's purpose.

**Analysis paralysis.** Propose 2-4 concrete changes, not an exhaustive list of possibilities. Pick the highest-impact ideas.

## After this command

The human reviews the proposal and decides:
- Pursue a proposal → switch to `optimize` or dedicated implementation
- Need more data → run `profile` with different config
- Proposal is too risky → continue with incremental `optimize`

This command produces a document, not code. The decision to act is human.
