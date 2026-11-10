# Stage 08 — Validate Dialga's existing Roar of Eternity and replacement safety

Status: **fully planned**; move already implemented. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Retain existing Roar of Eternity, close focused hit/faint/replacement coverage and optionally keep Koanko's Dialga colored during grayscale. Do not recreate the move, change Time Stop duration globally or replace ordinary recharge.

## 2. Existing code verified

- `include/constants/moves.h:MOVE_ROAR_OF_ETERNITY` already exists. `src/data/moves_info.h` defines the required name, Dragon/Special 150 BP, accuracy 90, selected target, 5 PP, Roar of Time animation, mandatory self recharge and 50% MOVE_EFFECT_TIMESTOP. Boss move-call/copy exclusions are present.
- `src/data/koanko_party.h` already gives level-104 Dialga Origin Roar of Eternity/Flash Cannon/Earth Power/Thunderbolt.
- `src/battle_script_commands.c:SetMoveEffect` applies Time Stop only when the target volatile is zero; sets 3, applies grayscale, and does not refresh the protected cooldown value 1.
- `src/battle_main.c` command selection automatically commits a no-choice action for timestop>1 or recharge. End-turn volatile maintenance decrements both timers; colors restore when Time Stop becomes 1. `src/battle_util.c:CancelerRecharge/CancelerTimestop` are separate ordinary cancelers.
- Existing timeline: hit turn ends at Time Stop 2/recharge 1; next turn both skip under a proc; following turn both can act with Time Stop cooldown 1 until end-turn clears it. Preserve this established behavior.
- `Cmd_openpartyscreen` is separate from ordinary action selection and checks genuine replacement/bar/slot policy. A stopped battler is not inherently disqualified from mandatory replacement by this command; runtime edge cases still need coverage.
- `BattleTimestopApplyGreyscale` copies true palettes and grays BG/UI and object banks, exempting a battler found by MOVE_CRACK_ICE. Its current single exempt-slot variable is specific to Kazuradrop. `BattleTimestopRestore` restores all from unfaded buffers.
- `test/battle/move_effect/roar_of_eternity.c` has three repaired cases: one recharge turn (Shield Dust isolates it), 50% tagged proc, and combined recharge/Time Stop recovery. `test/battle/ability/sheer_force.c` includes the move in secondary-effect coverage. User reports targeted tests passed; no fresh run was executed here.

## 3. Implementation design and order

1. Add missing negative-path and genuine-faint replacement tests around the current move; retain existing move definition/timeline and repaired harness assumptions.
2. After stage 07 cleanup, assert Dialga enters in singles. Exercise player-left with Time Stop active, damage/residual faint and a healthy reserve; replacement must be requested through the ordinary faint path before any auto-skipped command path can consume it. Fix only the ordering/gate proven defective, if any.
3. Preserve recharge after a successful hit even when the target is stopped; never use Time Stop as a reason to clear the attacker's timer. On a miss/Protect/immunity, follow ordinary Roar of Time's existing recharge rules, verified with paired fixtures rather than inventing a custom rule.
4. Optional color polish: add a narrow Koanko-Dialga exemption query alongside the existing Crack Ice predicate, building a mask of actual live battler sprite palette banks instead of one hardcoded slot. Gray only non-exempt banks in the same existing function; no new grayscale engine. Reapply the exception after menu/graphics reconstruction only while Time Stop remains active. Keep Kazuradrop's exemption and full restore behavior intact.
5. If tests reveal stale grayscale after the last stopped occupant faints/replaces, restore from the canonical unfaded palette when no stopped effect remains. Do not clear another battler's active Time Stop in doubles. Keep cleanup local to the presentation defect, not a global volatile redesign.

Proposed changes: existing Roar tests, a Koanko Dialga encounter fixture, optional palette exemption helper in `battle_util.c` with Koanko owner query; replacement/refresh ordering only if a test demonstrates a defect. No party/move-data rewrite.

## 4. Dependencies

Stage 01 identity/progression and stage 07 return-to-singles. Existing Time Stop, recharge, faint replacement and palette buffers. Core isolated move tests are independent of stage 07.

## 5. Edge cases and invariants

Shield Dust/Sheer Force/Substitute/Protect/immunity use ordinary additional-effect rules. A successful proc cannot chain or extend during nonzero volatile state. Time Stop blocks normal command choice/action, not mandatory party replacement or battle-loss cleanup. A fainted occupant's volatile does not transfer to its reserve. Recharge works if Dialga's target faints, survives via a saved Break Bar or is replaced. Optional palette logic never identifies arbitrary Dialga outside this encounter as Koanko's source.

## 6. Test strategy

Implementation commands: `wsl make check TESTS='Roar of Eternity'`, `TESTS='Koanko Dialga'` (new prefix), and affected existing `Sheer Force`/recharge test prefixes. Reuse the three current tests; do not revert their repaired expected countdowns.

Add hit/no proc/proc, miss, Protect, Fairy immunity, Substitute and Shield Dust comparisons against ordinary recharge behavior. Test proc followed by residual faint/reserve replacement, direct KO with reserves, no reserves/player loss, simultaneous faint, Break Charm/pending bar, item/menu entry after expiry and no re-proc during cooldown. Test new Dialga after Zacian departure has no right-slot prompt. Palette acceptance requires emulator captures of Dialga in color, player/UI grayscale, menu-return continuity and full restoration, plus Kazuradrop unchanged.

## 7. Completion criteria

Existing custom move remains intact and focused tests prove recharge/proc timing, failure handling and normal mandatory replacements without hangs. Dialga's correct kit/level and singles entry are verified. Optional color polish is either verified or explicitly deferred; it does not block mandatory battle behavior.

## 8. Open questions

- **Confirmed:** no move implementation is missing; preserve current 50% effect and ordinary recharge semantics.
- **Optional default:** include owner-specific color exemption if it remains a small palette-mask change; otherwise defer it. No global Time Stop duration changes are planned.
- Passing user-reported targeted tests establish the baseline, not coverage of untested faint/menu cases; record fresh focused results during implementation.
