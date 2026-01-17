# Diagnosis

## Summary
The step cost is dominated by the turn system (57% of `env_step`) and, within that, repeated
priority passes that rebuild action lists and invoke mana checks. Observation building is the
second major cost (15%), with the heaviest pieces being `populatePermanents` and `populateCards`,
both of which do full `CardData` construction every step.

## Primary bottleneck
**Where**: `managym/flow/priority.cpp:20-47`
**Time**: ~8-12% of `env_step` (e.g., `PRECOMBAT_MAIN_STEP/priority` is 0.078s of 0.954s in
`profile-20260116-1731.md`)
**Why**: `computePlayerActions()` scans the entire hand every priority pass and calls
`Game::canPayManaCost()` for each castable card. `canPayManaCost()` relies on
`Zones::constBattlefield()->producibleMana()` which iterates all permanents. With ~4-5 priority
passes per step (311,397 turn ticks vs 73,652 env_steps), this compounds into the largest hot path.

## Secondary bottlenecks
**Where**: `managym/agent/observation.cpp:192-225`
**Time**: 5.6% of `env_step`
**Why**: `populatePermanents()` iterates battlefield permanents and then calls `addCard()` for each
permanent, which rebuilds `CardData` (mana cost copy + 12 type checks). This is redundant when the
card is already represented as a permanent.

**Where**: `managym/agent/observation.cpp:120-156` and `managym/agent/observation.cpp:160-189`
**Time**: 3.8% of `env_step`
**Why**: `populateCards()` walks multiple zones each step and constructs `CardData` for each card.
`addCard()` copies `ManaCost` and recomputes type flags per card on every step.

**Where**: `managym/agent/observation.cpp:64-76`
**Time**: Hidden in observation overhead (4.2% of `env_step`)
**Why**: `populateActionSpace()` copies every action and focus list into new vectors each step.
The profile does not break this out separately in earlier runs, so the cost is folded into
observation overhead.

## Unaccounted time
The stable runs show ~4% unaccounted, likely a mix of:
- auto-executed trivial actions in `Game::tick()` that bypass `action_execute` tracking
  (`managym/flow/game.cpp:253-257`)
- allocations for `ActionSpace`, `Action`, and temporary vectors
- zone operations (e.g., `Zones::move`, `Zones::pushStack`) which have no profiler scopes

`profile-20260116-1750.md` shows 55% unaccounted and `action_execute` count ≫ `env_step` count,
indicating a measurement anomaly or a run with different instrumentation. That run should not be
used as a baseline until reconciled.

## Recommendations
1. **Reduce repeated mana checks inside priority passes** (target `managym/flow/priority.cpp:20-47`)
   - Cache `canPayManaCost` results per card for the pass or precompute playable cards once per
     priority tick to avoid full battlefield scans per card.
2. **Avoid rebuilding full `CardData` for battlefield permanents** (target
   `managym/agent/observation.cpp:208-225`)
   - Either skip `addCard()` for battlefield cards or embed needed card fields into
     `PermanentData`.
3. **Pre-size observation vectors using zone counts** (target `managym/agent/observation.cpp:120-156`)
   - Use zone counts to `reserve()` for `agent_cards`, `opponent_cards`,
     `agent_permanents`, `opponent_permanents`, and `action_space.actions`.
4. **Instrument the skip_trivial auto-exec path** (target `managym/flow/game.cpp:253-257`)
   - Add a profiler scope around the auto-executed action to close the unaccounted gap.
5. **Track zone move overhead if unaccounted persists** (target `managym/state/zones.cpp:68-89`)
   - Lightweight scopes around `move()` and `pushStack()` to verify map lookup costs are negligible.
