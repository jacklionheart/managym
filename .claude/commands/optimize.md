---
requires: .design/diagnosis.md
produces: faster code
---
Rewrite simulation hot paths for simplicity, robustness, and performance.

## Scope

Optimize the simulation loop: `manabot/env/`, `manabot/sim/`, observation encoding, model inference.

NOT in scope: `manabot/model/train.py`, PPO updates, gradient computation. Training performance is a separate effort.

## Goal

Fix ONE bottleneck from the diagnosis, verify the improvement, then stop. The human can run `optimize` repeatedly. Each pass should make a measurable difference—if it doesn't show up in profiling, it wasn't worth doing.

## Workflow

1. Read `.design/diagnosis.md` to identify the top bottleneck
2. Read the code at the identified location
3. Implement the fix with minimal changes
4. Run tests: `pytest tests/`
5. Run a quick profile to verify improvement
6. Update `.design/diagnosis.md` to mark the bottleneck as addressed

## Optimization principles

**Measure before and after.** No optimization without numbers. Run the same simulation config before and after to compare.

**Simplicity is speed.** The fastest code is code that doesn't run. Delete unnecessary work before micro-optimizing what remains.

**Don't break the interface.** The fix should be invisible to callers. Same inputs, same outputs, faster.

**Robustness over cleverness.** A 10% gain that's fragile is worse than a 5% gain that's solid. Avoid optimizations that assume specific data patterns.

## Common optimization patterns for manabot

**Tensor reuse:**
```python
# Before: allocate every call
def encode(self, obs):
    arr = np.zeros((self.max_cards, self.card_dim))
    ...

# After: reuse buffer
def __init__(self):
    self._card_buffer = np.zeros((self.max_cards, self.card_dim))

def encode(self, obs):
    self._card_buffer.fill(0)
    ...
```

**Batch operations:**
```python
# Before: loop over items
for card in cards:
    encoded[i] = self._encode_card(card)

# After: vectorize
encoded[:len(cards)] = self._encode_cards_batch(cards)
```

**Avoid Python in hot paths:**
```python
# Before: dict lookup per item
for key in obs.keys():
    result[key] = process(obs[key])

# After: direct attribute access or pre-computed
for i, processor in enumerate(self._processors):
    result[i] = processor(obs_array[i])
```

**Lazy computation:**
```python
# Before: compute always
def get_value(self):
    return expensive_computation()

# After: compute once
@cached_property
def value(self):
    return expensive_computation()
```

## What to avoid

**Premature complexity.** Don't add caching, pooling, or parallelism unless the diagnosis specifically identifies it as the bottleneck.

**Breaking tests.** If tests fail, the optimization is wrong. Don't change tests to accommodate the optimization.

**Invisible improvements.** If you can't measure the difference, don't ship it. Revert and try something else.

**Scope creep.** Fix the ONE bottleneck identified. Other improvements go in the next `optimize` pass.

## Verification

After implementing:

```bash
# Run tests
pytest tests/

# Quick profile (same config as diagnosis)
python -m manabot.sim.sim sim.hero=simple sim.villain=random sim.num_games=50 sim.num_threads=1
```

Compare games/sec to the profile baseline. Document the improvement in a commit message.
