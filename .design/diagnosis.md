# Diagnosis

## Summary

Turn hierarchy traversal dominates runtime at 57% of env_step. The main phases (PRECOMBAT_MAIN_STEP + POSTCOMBAT_MAIN_STEP) account for 25% alone due to priority checks. Observation building is secondary at 15%.

## Current Throughput

**67,019 steps/sec** (post skip-trivial collapse)

## Primary Bottleneck

**Where**: `Turn::tick()` in turn.cpp:224-244
**Time**: 0.550s (57.3% of env_step)
**Why**: 311,397 turn ticks for 73,652 agent steps (4.2x amplification). Each tick traverses Turn→Phase→Step→Priority hierarchy with virtual dispatch and profiler scopes.

The main phases dominate within the turn hierarchy:
- PRECOMBAT_MAIN_STEP: 0.155s (16.2%) - 46,464 calls
- POSTCOMBAT_MAIN_STEP: 0.091s (9.5%) - 30,222 calls
- Priority checks within main phases: 0.114s combined

## Secondary Bottlenecks

### Observation building
**Where**: observation.cpp:22-39
**Time**: 0.147s (15.3% of env_step)
**Why**: Builds complete observation after every step. populatePermanents (5.8%) iterates battlefield; populateCards (3.8%) iterates multiple zones.

### Action execution
**Where**: game.cpp:208-210
**Time**: 0.082s (8.6% of env_step)
**Why**: Virtual dispatch for action execution, includes CastSpell which triggers mana production chain.

## Addressed Bottlenecks

1. ~~**Skip-trivial loop overhead**~~ — Collapsed into tick(). +3% gain (65k → 67k steps/sec)
2. ~~**Early-out priority check**~~ — canPlayerAct() fast-path. +66% gain (42k → 71k)
3. ~~**InfoDict at episode end**~~ — Deferred from every step. +129% gain
4. ~~**Phase/step profiler scopes**~~ — Removed from hot path. +8.7% gain

## Recommendations (in order of expected impact)

1. **Flatten Turn/Phase/Step hierarchy (Proposal 3)**: Replace object hierarchy with flat state machine. Eliminates ~165k object allocations per 500 games and virtual dispatch overhead. Expected: 15-25% gain.

2. **Skip empty combat phases (Proposal 4)**: When active player has no eligible attackers, skip directly to postcombat main. Expected: 5-10% gain for creature-light games.

3. **Optimize observation building**: populatePermanents iterates battlefield, then addCard iterates again. Single-pass construction could save ~3%.

## Profile Data (2026-01-16 post-optimization)

```
env_step: 0.959s (73,652 calls)
├─ game: 0.774s (80.7%)
│  ├─ tick: 0.602s (62.8%)
│  │  └─ turn: 0.550s (57.3%) - 311,397 calls
│  │     ├─ PRECOMBAT_MAIN_STEP: 0.155s (16.2%)
│  │     │  └─ priority: 0.078s
│  │     ├─ POSTCOMBAT_MAIN_STEP: 0.091s (9.5%)
│  │     │  └─ priority: 0.036s
│  │     └─ (other steps): 0.304s
│  └─ action_execute: 0.082s (8.6%)
└─ observation: 0.147s (15.3%)
   ├─ populatePermanents: 0.056s (5.8%)
   ├─ populateCards: 0.037s (3.8%)
   └─ (other): 0.054s
```
