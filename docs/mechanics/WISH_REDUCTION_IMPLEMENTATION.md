# Wish for Reduction: what changed and how it works

Written November 7, 2026. This walkthrough covers the Reduction implementation and its supporting changes to Polymorph, stat calculation, HP synchronization, switching, Break Bars and battle cleanup.

## 1. What it does in battle

Wish for Reduction places a persistent effect on the player's side. After its initial grace turn, it lowers the effective level of each living active player Pokemon at the end of each turn.

The subtraction grows by **one level per tick**: 10, 11, 12, 13 and so on. The next subtraction belongs to the side and survives switching. Each active Pokemon separately accumulates the levels it has lost during its current stay.

A Level 100 Pokemon that never switches follows this sequence:

```text
Wish is cast:             100 — no reduction this turn
Following turn ends:       90 — loses 10
Next turn ends:            79 — loses 11
Next turn ends:            67 — loses 12
Next turn ends:            54 — loses 13
Next turn ends:            40 — loses 14
Next turn ends:            25 — loses 15
Next turn ends:             9 — loses 16
Next turn ends:             1 — next loss is clamped at Level 1
```

Switching removes the outgoing Pokemon's accumulated level loss. The incoming Pokemon begins at its unreduced level and takes the next shared subtraction at the end of its entry turn.

For example, after the first Pokemon loses 10 levels, switching brings in a Level 100 replacement. That replacement loses **11** levels at the end of that turn, reaching Level 89. If the original Pokemon returns on the following turn, it starts at Level 100 and then loses **12**, reaching Level 88.

Other rules:

- The effect lasts until battle ends, independently of Castoria remaining on the field.
- It is available on the fake-name and backed-out routes. The true-name and unresolved routes cannot use it.
- It shares Wish's existing once-per-battle use with the other outcomes. An interrupted cast does not spend that use.
- A second application is rejected while Reduction is already active.
- The casting turn is the only grace turn. Switching does not create another grace period.
- In doubles, both living active player Pokemon receive the same subtraction; the shared counter advances once for the turn.
- Break Bar grants, breaks and reveals do not reset accumulated loss.
- Effective level never falls below 1, and the shared subtraction counter saturates at `MAX_LEVEL` instead of overflowing.
- Reduction is a direct side effect rather than a conventional stat-stage drop. The ordinary restrictions on casting Arcanum Wish, including Taunt, still apply.

The battle healthbox shows the reduced level and HP. Level-dependent combat calculations use the reduced battle level too.

## 2. The central design: real party data plus a temporary battle overlay

The engine has two representations that matter here:

- The party Pokemon, such as `gPlayerParty[0]`, contains its real experience, level and stats.
- The active battle Pokemon, such as `gBattleMons[0]`, contains the values combat is currently using.

Reduction changes the **battle representation**. Its lower level and derived stats are never written into the real party Pokemon, even temporarily as calculation workspace. Party screens therefore continue to show the unreduced values.

Real damage and healing still update party HP. Those updates are converted to the unreduced HP scale first. Keeping party data intact does not mean restoring HP that was genuinely lost during battle.

The new implementation lives in [src/battle_reduction.c](../src/battle_reduction.c), with its state and interface in [include/battle_reduction.h](../include/battle_reduction.h). `BattleStruct` owns that state, so it is initialized with the rest of the battle data.

There are two layers of state:

- `ReductionData` stores whether the side effect is active, whether the initial grace turn remains, and the next subtraction.
- Each `ReductionOccupant` stores accumulated level loss, the associated party index, HP fraction, and the raw stat values before and after scaling. It also tracks nested recalculation depth and Power Trick's attack/defense swap.

This separation is why switching can clear one occupant's loss without clearing the side-wide sequence.

## 3. How a tick changes stats

The effective level is calculated as:

```c
effectiveLevel = max(1, realLevel - accumulatedLevelLoss);
```

The ordinary stat formula was extracted into `CalculateMonStatsAtLevel()` in [src/pokemon.c](../src/pokemon.c). The existing `CalculateMonStats()` still obtains the real level from experience, then calls that formula.

Reduction calls the explicit-level helper on **scratch copies** of the Pokemon: one at its real level and one at its effective level. It compares those calculated stat lines and applies their ratios to the active battle stats.

For an ordinary unmodified Pokemon, that produces the normal stats for the lower level. Applying ratios to the current battle stat line also allows copied Transform stats and raw stat modifiers to be scaled without replacing the whole Pokemon. Stat stages are retained. Power Trick swaps are accounted for when applying and removing the scaling.

The tick handler is registered as `ENDTURN_REDUCTION`, after the existing residual-effect stages. The end-of-turn dispatcher already resolves pending Break Bars before continuing its work.

When a tick changes the effective level, a battle script refreshes the healthbox and prints the new level. A Pokemon already at Level 1 does not repeatedly print an unchanged level, although the shared sequence continues advancing up to its cap.

## 4. HP percentages, healing and rounding

Shrinking maximum HP also rescales current HP to retain the same percentage. A Pokemon at roughly half health remains at roughly half health when its effective level changes or is restored.

The module stores HP as an integer fraction with 24 fractional bits. This retains precision that would otherwise be discarded each time HP is converted between reduced and unreduced scales. Repeated rescaling alone therefore cannot manufacture healing.

Actual damage and healing change that fraction using the current battle HP scale. A Potion adds **20 displayed battle HP**, capped at the reduced maximum. Restoring the original level afterward retains the resulting percentage.

The conversion rules also ensure that:

- A living Pokemon does not faint merely because its maximum HP shrank.
- A fainted Pokemon stays at zero HP.
- Restoring level does not erase damage or act as a full heal.

`ReductionNormalizeRequest()` is called when the engine emits party-data updates through the battle controller. It converts reduced HP and stat values before they reach party storage. It also protects real experience, level and stats during a full `REQUEST_ALL_BATTLE` update.

The healthbox and HP animation paths were adjusted to use the reduced battle values while the overlay is active. The healing-item path likewise uses displayed battle HP for an active reduced recipient.

## 5. Polymorph: protecting the originals

**Polymorph must never save Reduction's effective level, reduced stats or scaled HP as original data.**

The existing Polymorph system saves original moves, PP, nickname and experience in its first-cast backup. Original species ownership remains in `PartyState.changedSpecies`. Original level and stats are recovered from the saved experience and restored species during cleanup.

Reduction does not reuse, replace or overwrite that backup or those ownership flags.

The Polymorph changes in [src/castoria_name_theft.c](../src/castoria_name_theft.c) enforce the following order:

1. Read original species and experience from the unreduced party Pokemon.
2. Temporarily remove the Reduction overlay from the active battle view and synchronize its HP to the unreduced scale.
3. Let Polymorph perform its normal species change, growth-curve experience conversion, stat calculation and HP conversion at the real level.
4. Finish Polymorph's remaining updates, then reapply the same accumulated Reduction once.

For example, a real Level 70 Pokemon reduced to Level 40 is still a **real Level 70 Pokemon** to Polymorph. Polymorph preserves Level 70 when converting experience to the new species' growth curve. Its battle view returns to Level 40 afterward because the 30-level loss remains active.

The canonical party representation can legitimately contain Polymorph's current temporary species and its corresponding experience curve. Reduction keeps that representation unreduced. Polymorph's separate original backup remains responsible for recovering the pre-Polymorph Pokemon.

Repeated Polymorph casts retain the first original backup. Switching removes Reduction's level loss while keeping the form that Polymorph's existing lifecycle retains. Reduction never restores an old whole-Pokemon copy that could undo a later Polymorph cast.

### Nested recalculations

`ReductionBeginMutation()` and `ReductionEndMutation()` bracket operations that need unreduced battle values. Their nesting counter matters because Polymorph calls the shared stat-recalculation routine, which is also protected by these helpers.

The outer operation removes the overlay; inner recalculations finish without reapplying it prematurely. The outermost end reapplies Reduction after species, stats and HP are all consistent. This prevents double scaling or a reduced HP value being fed into Polymorph's canonical HP conversion.

### Polymorph scratch storage

Polymorph's species candidate arrays now share its existing scratch allocation with moveset construction through a union. Candidate selection finishes before moveset construction begins, so those arrays do not need simultaneous storage.

This reduces the selector's stack usage without changing the selection rules or increasing that scratch allocation. The large nested frame became visible while running the immediate snapshot checks through the test runner's 1 KiB invocation stack.

## 6. Switching, transformations and Break Bars

The integration uses shared lifecycle boundaries rather than separate copies of Reduction logic for every move.

- Genuine withdrawal, switch-in and faint cleanup clear the occupant's accumulated loss. Switch-out ability handling also runs with that occupant restored to its unreduced scale.
- Copying fresh party stats into a battler reapplies any current accumulated loss.
- Shared form/stat recalculation is bracketed by the nested mutation helpers.
- Transform and the transformed level-up path preserve the overlay around their updates.
- Dynamax reversal converts HP on the unreduced scale before reapplying Reduction. Form-change presentation is not treated as a genuine switch.
- Saved Break Bars store covered HP and maximum HP in the unreduced scale. Revealing a saved layer reapplies the current loss without resetting it.

These integrations required small additions to existing battle code. The mechanic's state, scaling and conversion logic remain in the Reduction module.

## 7. Battle cleanup and the HP-scale correction

Normal battle cleanup removes Reduction before undoing Dynamax and running the existing form/Break Bar restoration. That ensures later cleanup receives unreduced HP.

A combined Polymorph/Reduction/Break Bar regression exposed another necessary correction: Break Bar cleanup previously summed absolute HP from a Polymorph form, restored the original species, then applied that old-form HP total to the original species' maximum HP.

For Polymorphed Pokemon, the cleanup now retains each surviving layer as a percentage and converts the total to the restored species' HP scale. The result remains capped at its ordinary maximum HP. This preserves the intended surviving health across the species change.

The cleanup tests explicitly call the production restoration routine through a test-only helper. Recorded test battles can exit through a path that bypasses the normal cleanup callback, so relying only on an `AFTER_CLEANUP` block would not reliably exercise these assertions.

## 8. Where to read the changes

- [battle_reduction.c](../src/battle_reduction.c) and [battle_reduction.h](../include/battle_reduction.h): state, activation, ticking, stat overlay, HP conversion and controller normalization.
- [castoria_name_theft.c](../src/castoria_name_theft.c): Wish eligibility/execution, protected Polymorph updates, snapshot accessor for tests and shared scratch storage.
- [pokemon.c](../src/pokemon.c): reusable explicit-level stat calculation.
- [battle_util.c](../src/battle_util.c), [battle_dynamax.c](../src/battle_dynamax.c) and [battle_script_commands.c](../src/battle_script_commands.c): stat refreshes, transformations, switching and active-target healing.
- [battle_main.c](../src/battle_main.c) and [battle_break_bar.c](../src/battle_break_bar.c): occupant cleanup, battle restoration and saved HP layers.
- [battle_controllers.c](../src/battle_controllers.c) and [battle_interface.c](../src/battle_interface.c): protected party updates and reduced battle displays.
- [battle_end_turn.c](../src/battle_end_turn.c), [battle_scripts_1.s](../data/battle_scripts_1.s), and the corresponding constants/message definitions: tick dispatch, announcements and healthbox refresh.
- [arcanum_reduction.c](../test/battle/arcanum_reduction.c): the new regression coverage. The existing Arcanum eligibility test was also updated to recognize Reduction as implemented.
- [arcanum.md](../docs/mechanics/arcanum.md): the maintained mechanics reference.

## 9. Verification and the remaining test issue

The completed implementation was checked with headless `wsl make check` runs and a production `wsl make` build. The existing logs record these results; writing this walkthrough did not rerun the tests.

- **Arcanum:** 132 passed, including 20 Reduction test definitions with parameterized cases. See [reduction-arcanum-tests.log](reduction-arcanum-tests.log).
- **Break Bars:** 37 passed. See [reduction-breakbar-tests.log](reduction-breakbar-tests.log).
- **Dynamax:** 75 passed; 5 existing TODO cases. See [reduction-dynamax-tests.log](reduction-dynamax-tests.log).
- **Terastallization:** 39 passed. See [reduction-tera-tests.log](reduction-tera-tests.log).
- **Mega Evolution:** 10 passed; **Primal Reversion:** 13 passed; **Ultra Burst:** 6 passed. See the corresponding `reduction-mega`, `reduction-primal` and `reduction-ultra` test logs here.
- **Transform:** 1 passed; 1 existing TODO case. See [reduction-transform-tests.log](reduction-transform-tests.log).
- **Production ROM:** built successfully. See [reduction-rom-build.log](reduction-rom-build.log).

The new Reduction tests cover the decay sequence, initial grace, switching/re-entry, doubles, stat calculation, HP rounding, healing, controller writes, route restrictions, interruption, form changes, Transform/Power Trick, level gains, Break Bar storage/reveal and cleanup.

The Polymorph cases inspect its backup **immediately after the first capture**, check that repeated casts leave it unchanged, and verify restoration of species, experience, level, stats, nickname, moves and PP. They also check HP scale and genuine faint preservation.

### One unchanged legacy Polymorph test fails

The separate old [move_effect/polymorph.c](../test/battle/move_effect/polymorph.c) suite reports 3 passes and 1 failure. Its first test casts Polymorph with the opponent but expects the move animation on the player; the recorded failure is `Unmatched ANIMATION` at that assertion. It subsequently expects the player to become Castoria, although the existing species guard excludes that boss species.

That test file was left unchanged. Its failure is recorded in [reduction-polymorph-tests.log](reduction-polymorph-tests.log). The new Reduction/Polymorph snapshot and restoration cases pass, but the full set of neighboring tests is therefore not entirely green.

## 10. What remains deferred

Reduction's effect and execution are implemented. Production tactical selection of this Wish outcome remains deferred, consistent with the other unfinished Wish policies. Tests nominate it through Special AI and exercise the normal cast path.

No MP cost was introduced; the MP system remains unimplemented. No commit was made for this work.
