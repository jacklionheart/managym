---
interactive: false
requires: .design/profile-*.md
produces: .design/diagnosis.md
---
Identify the hottest paths and root causes of simulation performance bottlenecks.

## Scope

Focus on the simulation loop: environment step, observation encoding, model inference, action selection.

NOT in scope: training loop, PPO updates, gradient computation. Those are separate optimization targets.

## Goal

Turn raw profiler data into actionable insights. Find where time actually goes, not where you assume it goes. The output is a diagnosis document that identifies specific functions, patterns, or architectural choices causing slowdowns.

## Workflow

1. Read the most recent profile data in `.design/profile-*.md`
2. Identify the dominant time sinks
3. Drill into the code to understand WHY those paths are slow
4. Document findings in `.design/diagnosis.md`

## Analysis approach

**Follow the numbers.** Start with whatever takes the most time. If env.step is 80% of runtime, that's where to look. If model inference dominates, focus there.

**Distinguish symptoms from causes:**
- Symptom: "env.step takes 50ms"
- Cause: "observation encoding converts dict to tensor every step, allocating 10KB"

**Look for these patterns:**

1. **Unnecessary work per step**
   - Repeated tensor allocations
   - Redundant conversions (numpy ↔ torch)
   - Validation that could happen once

2. **Python overhead in hot paths**
   - Dict lookups in inner loops
   - Function call overhead
   - Type checking at runtime

3. **Memory allocation patterns**
   - Creating new tensors instead of reusing
   - Growing lists instead of pre-allocating
   - Copying data unnecessarily

4. **Architectural bottlenecks**
   - Single-threaded where parallelism possible
   - Synchronous where async would help
   - Full observations when partial would suffice

## Drilling into code

For each suspected bottleneck:

1. Find the exact function/method
2. Read it and understand the implementation
3. Identify the specific inefficiency
4. Note the file and line number

Example:
```
## Bottleneck: Observation Encoding

**Location**: manabot/env/observation.py:ObservationEncoder.encode()
**Time**: ~40% of simulation time
**Root cause**: Creates new numpy arrays for every field on every step.
The encoder allocates max_cards_per_player * card_dim floats per call,
even when most slots are empty.

**Evidence**: Profile shows 10KB allocation per encode() call
```

## Output: .design/diagnosis.md

```markdown
# Performance Diagnosis

## Summary
<One paragraph: what's slow and why>

## Primary Bottleneck
**Component**: <name>
**Location**: <file:line>
**Time share**: XX%
**Root cause**: <specific explanation>

## Secondary Bottlenecks
<Same format, ranked by impact>

## Quick Wins
<Low-effort changes with measurable impact>

## Architectural Issues
<Changes that require design work>

## Questions
<Uncertainties that need profiling or investigation>
```

## When uncertain

If the profile data is incomplete, note what's missing in `.design/questions.md` and proceed with best-effort analysis.

Don't guess—but don't block either. If a bottleneck is ambiguous, note the uncertainty and suggest experiments for the next `profile` pass.
