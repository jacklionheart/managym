# Profile: 2026-01-16 (Post-InfoDict Fix)

## Config
Games: 500, Seed: 42, Threads: 1
Deck: Grey Ogre mirror, Policy: random
**Change**: InfoDict construction moved to episode end only

## Throughput
- 265.42 games/sec
- 39,097 steps/sec
- 147.3 steps/game avg

## Delta vs Previous (20260116-1800)

| Metric | Previous | Current | Change |
|--------|----------|---------|--------|
| Steps/sec | 17,039 | 39,097 | **+129%** |
| env_step total | 3.899s | 1.738s | **-55%** |
| Per-step time | 52.9μs | 23.6μs | **-55%** |
| Unaccounted | 2.130s (54.6%) | 0.032s (1.8%) | **-98%** |
| game | 1.614s (41.4%) | 1.558s (89.6%) | -3.5% |
| observation | 0.155s (4.0%) | 0.148s (8.5%) | -4.5% |

## Breakdown
| Scope | Total | Count | % |
|-------|-------|-------|---|
| env_step | 1.738s | 73,652 | 100% |
| env_step/game | 1.558s | 73,652 | 89.6% |
| env_step/game/action_execute | 0.142s | 473,780 | 8.2% |
| env_step/game/tick | 1.091s | 473,780 | 62.8% |
| env_step/game/tick/turn | 0.985s | 699,093 | 56.6% |
| env_step/game/tick/turn/phase | 0.877s | 699,093 | 50.5% |
| env_step/game/tick/turn/phase/step | 0.715s | 699,093 | 41.1% |
| env_step/game/tick/turn/phase/step/priority | 0.272s | 615,234 | 15.7% |
| env_step/game/tick/turn/phase/step/priority/priority | 0.019s | 12,839 | 1.1% |
| env_step/observation | 0.148s | 73,652 | 8.5% |
| (unaccounted) | 0.032s | - | 1.8% |

## Key Observations

1. **InfoDict overhead eliminated**: The 54.6% unaccounted time is now 1.8%. The fix was simple: only build InfoDict at episode end (`env.cpp:81-86`).

2. **Turn hierarchy now dominates**: With InfoDict fixed, the turn/phase/step hierarchy is now 56.6% of env_step time (was 26% before, but that was 26% of a much larger total).

3. **Tick amplification unchanged**: Still 9.5x internal ticks for every agent step. This is now the primary optimization target.

4. **Observation remains efficient**: 8.5% of time (was 4% before, now a larger share of smaller total).

## New Bottleneck Analysis

Now that InfoDict is fixed, the time breakdown is:
- **Turn hierarchy**: 56.6% (0.985s) — next target
- **Tick loop overhead**: 6.2% (0.106s between game and tick)
- **Observation**: 8.5% (0.148s)
- **Action execution**: 8.2% (0.142s)
- **Other**: ~20%

The turn hierarchy (Turn::tick → Phase::tick → Step::tick) remains the dominant bottleneck. The refactor proposals (flat state machine, integrated skip-trivial) would address this.

## Raw
```
env_step: total=1.73835s, count=73652
env_step/game: total=1.55841s, count=73652
env_step/game/action_execute: total=0.142266s, count=473780
env_step/game/tick: total=1.09101s, count=473780
env_step/game/tick/turn: total=0.984745s, count=699093
env_step/game/tick/turn/phase: total=0.877472s, count=699093
env_step/game/tick/turn/phase/step: total=0.714911s, count=699093
env_step/game/tick/turn/phase/step/priority: total=0.27225s, count=615234
env_step/game/tick/turn/phase/step/priority/priority: total=0.0192752s, count=12839
env_step/observation: total=0.148094s, count=73652
env_step/observation/populateCards: total=0.0370853s, count=73652
env_step/observation/populatePermanents: total=0.055086s, count=73652
env_step/observation/populatePlayers: total=0.00530917s, count=73652
env_step/observation/populateTurn: total=0.0100654s, count=147304
```

## Behavior Stats (Hero)

| Metric | Value |
|--------|-------|
| win_rate | 0.50 |
| games_played | 500 |
| avg_game_length | 19.41 turns |
| land_play_rate | 0.84 |
| lands_played | 6,318 |
| spell_cast_rate | 1.07 |
| spells_cast | 24,423 |
| attack_rate | 0.49 |
| attacks_declared | 6,040 |
| block_rate | 0.25 |
| blocks_declared | 3,063 |
| mana_efficiency | 0.18 |
