---
interactive: false
produces: .design/profile-<timestamp>.md
---
Gather performance data for simulation throughput (environment + inference, NOT training).

## Scope

We're optimizing the simulation loop: `env.reset()` → `env.step()` → `player.get_action()` → repeat.

NOT in scope: PPO training, gradient computation, buffer management. Those are separate concerns.

## Goal

Run simulations and collect profiler data to establish a performance baseline. The output is raw numbers—games per second, time breakdown by component, memory usage. No analysis yet; that happens in `diagnose`.

## Workflow

1. Run a simulation with profiling enabled and capture the output
2. Record the key metrics in `.design/profile-<timestamp>.md`
3. If previous profile data exists in `.design/`, note the comparison
4. Commit the profile data

## What to capture

**Throughput metrics:**
- Games per second (overall)
- Steps per second
- Total games run, total wall time

**Time breakdown (from profiler):**
- Environment step time (`env.step()`)
- Model inference time (forward pass, action selection)
- Observation encoding time
- Any other significant contributors

**Resource usage:**
- Peak memory (if available)
- CPU/GPU utilization observations

## Running the simulation

Use the sim module with profiling:

```bash
python -m manabot.sim.sim sim.hero=simple sim.villain=random sim.num_games=100 sim.num_threads=1
```

Single-threaded first to get clean profiler data. Note the profiler output in the logs.

For model vs model:
```bash
python -m manabot.sim.sim sim.hero=<model_name> sim.villain=random sim.num_games=50 sim.num_threads=1
```

## Output format

Create `.design/profile-<YYYYMMDD-HHMM>.md`:

```markdown
# Profile: <date>

## Configuration
- Hero: <player type>
- Villain: <player type>
- Games: <N>
- Threads: 1

## Results
- **Throughput**: X.XX games/sec
- **Avg steps/game**: XX

## Time Breakdown
| Component | Avg Time | % of Total |
|-----------|----------|------------|
| env.step  | X.XXXs   | XX%        |
| inference | X.XXXs   | XX%        |
| encoding  | X.XXXs   | XX%        |

## Notes
<Any observations about the run>
```

## What NOT to do

- Don't analyze or interpret yet—just record
- Don't change code to "fix" things you notice
- Don't run multi-threaded yet (confounds profiling)
