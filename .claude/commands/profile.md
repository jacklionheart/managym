---
produces: .design/profile-<timestamp>.md
---
Measure simulation throughput. Raw numbers only.

## The two hot paths

1. **Tick loop**: `Game::step()` → `_step()` → `tick()` → `TurnSystem::tick()`
   - game.cpp:205-241 (step and tick)
   - turn.cpp:147-160 (TurnSystem::tick)

2. **Observation building**: `Observation` constructor and populate* functions
   - observation.cpp:22-39 (constructor)
   - observation.cpp:45-202 (populateTurn, populateActionSpace, populatePlayers, populateCards, populatePermanents)

Everything else is noise.

## Run the benchmark

```bash
python scripts/profile.py --games 500 --seed 42
```

Grey Ogre mirror, random policy, single thread, profiler enabled.

## Record these numbers

**Throughput:**
- Games/sec
- Steps/sec
- Avg steps/game

**Profiler breakdown:**
- `env_step` total
- `env_step/game` — the tick loop wrapper
- `env_step/game/tick/observation` — observation building
- `env_step/game/tick/turn` — turn system logic
- Each populate* function under observation

**Derived:**
- % in observation building
- % in turn logic
- % unaccounted (the gap between game and tick+observation)

## Output

`.design/profile-<YYYYMMDD-HHMM>.md`:

```markdown
# Profile: <date>

## Config
Games: 500, Seed: 42, Threads: 1
Deck: Grey Ogre mirror, Policy: random

## Throughput
- X games/sec
- Y steps/sec
- Z steps/game avg

## Breakdown
| Scope | Total | Count | % |
|-------|-------|-------|---|
| env_step | | | 100% |
| → game | | | |
| → tick/observation | | | |
| → tick/turn | | | |
| (unaccounted) | | | |

## Raw
<profiler dump>
```

Don't analyze. Don't suggest fixes. Record numbers.

Note delta vs previous profile if one exists.
