# Stage 01 — Encounter lifecycle and fixed progression

Status: **fully planned**, not implemented by this task. Source inspected on `mypatch`, 2026-11-09.

## 1. Objective and scope

Complete presentation and progression around the existing uninterrupted twelve-opponent Trainer battle. Preserve first-team switching, player resources, field/side conditions, per-opponent EXP processing and final-only victory/rewards. This stage does not implement individual boss attacks, forms, temporary allies, or Arceus.

Confirmed later design decision, superseding the original handoff: clear only Koanko's opponent-side Tera-used flag once at the team transition. Calyrex's Fire Tera is an ordinary optional AI choice (stage 06), not a forced entry event.

## 2. Existing code verified

- `src/battle_koanko.c`: `TryStartKoankoConjuring` guards encounter eligibility, controller/script-stack availability, player survival and one-time phase entry. `ConjureKoankoTeam` replaces the enemy party, clears reused enemy slot state, preserves player/field/trainer-wide state, and nominates Smeargle. `include/battle_koanko.h` defines ORDINARY/CONJURING/CONJURED phases.
- `src/battle_script_commands.c`: `Cmd_checkteamslost` calls the transition before setting WON. The script runs after ordinary faint/EXP processing. `data/battle_scripts_1.s:BattleScript_KoankoConjureTeam` currently only calls the trainer slide and replacement native; no music commands.
- `src/data/trainers.party:TRAINER_KOANKO`: all six ordinary species/moves match the handoff, all level 100. Alcremie has Dynamax Level 10 and Gigantamax enabled. `src/data/trainers.h` is generated output, not the authoring source.
- `src/data/koanko_party.h`: corrected intended species/moves, Smeargle has only slot 0, TX slot 3 is empty; configured levels are 100/101/102/103/104/105. `.teraType = TYPE_FIRE` records Calyrex's intended Tera type but does not prove entry is already Terastallized.
- `src/data/trainer_break_bars.h`: initial and conjured Koanko rules are separate and both empty. Later stages add only required conjured rules.
- `src/battle_ai_switch_items.c`: `ShouldSwitch` declines sequence-switching AI; `GetMostSuitableMonToSwitchInto` uses `GetNextMonInParty` with `AI_FLAG_SEQUENCE_SWITCHING`. `CanBattlerConsiderSwitch` respects `HasNoOrderChange`.
- `src/battle_util.c:HasNoOrderChange`, script phazing/item/pivot gates, and `Cmd_openpartyscreen` distinguish living trapped occupants from genuine faints. No Order Change clears with occupant initialization; reapply after switch-in clearing.
- `src/battle_hold_effects.c:HOLD_EFFECT_BREAK_CHARM` uses `GrantSavedBreakBar` once per owner. Preserve its full saved layer and no repeat-grant rule.
- `src/trainer_slide.c` supports ad hoc `trainerSlideMsg`; use existing returning slide script. Current reveal text is `You thought that was all?\nThen watch what I can create!`.
- `include/constants/songs.h` contains `MUS_VICTORY_TRAINER` and `MUS_KAZURADROP_APPEARS`, no `MUS_GRAND_BATTLE`. Kazuradrop has a fake-victory/music-change precedent in `battle_script_commands.c`.
- `test/battle/koanko.c` covers replacement state preservation, twelve faints, residual/recoil/Destiny Bond, simultaneous faints with/without player reserves, hazards and new AI moves. `TEMPSCRIPTS/koanko-tests.log` records these nine passing historically; no fresh run performed here.

Current-code addendum (2026-11-10, Stage11 reinspection): `ConjureKoankoTeam` now contains the one-time opponent-left Tera-used reset and SEQUENCE_SWITCHING assignment; `KoankoOnSwitchIn`, declared in the Koanko header and called by `SwitchInClearSetData`, reapplies No Order Change during the conjured phase. The Koanko unit test now asserts preservation of player Tera/opponent Dynamax and no repeated Tera renewal. These outside changes supersede the earlier snapshot's missing-policy assessment. Preserve and validate them rather than recreate them; this addendum does not claim a new passing test run or completion of the presentation/other lifecycle work below.

## 3. Implementation design and order

1. Extend existing Koanko encounter state in `include/battle_koanko.h`/`include/battle.h` with named encounter event flags and a completed-conjured-turn counter. Keep battle-wide state outside `PartyState`; per-boss state is keyed to enemy party identity, never just current species. Initialize/cleanup with the existing battle lifecycle.
2. Extend `battle_koanko.c` and `BattleScript_KoankoConjureTeam`: play fake trainer-victory BGM, run the returning reveal slide, change to the confirmed Grand Battle track, then call existing replacement. Keep outcome zero and return to ordinary replacement; do not call actual battle-win/results code. Preserve existing reveal's second sentence by default because it contains the required line.
3. Inside the existing guarded, one-time team transition, clear only Koanko's opponent-side trainer Tera-used bookkeeping so the conjured team has a fresh ordinary Tera opportunity. Preserve the player's Tera-used flag and every other trainer-wide gimmick-used flag, including opponent Dynamax usage. This reset must not run on switch-in, form change, repeated transition calls or controller re-entry, and does not activate Tera. After conjuring, apply `AI_FLAG_SEQUENCE_SWITCHING` to Koanko's active AI flags and preserve/reinstall it on subsequent enemy switch-ins. Do not modify first-team trainer AI flags. Smeargle remains forced lead; remaining live party slots enter in ascending order.
4. Add a Koanko post-switch-in policy helper, called after `SwitchInClearSetData` and before entry mechanics can request switching. Apply No Order Change only to living conjured enemy owners. Recommended policy is immunity to forced switching using existing infrastructure: Roar fails, damaging phazing still deals damage, pivots keep their non-switch effects, switching items are not incorrectly consumed. Genuine faint replacement remains ordinary.
5. Reserve returning, once-only entry events for TX's `Look at this cutie I found in the files!` and Arceus's `You remember this one, right?`. Dispatch at entry script boundaries with controllers idle; no new general trainer-slide enum is necessary. Arceus's actual opening effects belong to stage 09.
6. Expose narrow Koanko queries for owner/phase/entry-event eligibility and an end-of-completed-turn hook for later Arceus timing. Do not scatter trainer-ID checks across damage/AI/UI code. Increment conjured turns once per full end-turn pass, including idle/Time Stopped turns, never on scripted extra actions or bar transitions.

Proposed file changes: `battle_koanko.c`/header (state/policy/events), `battle.h` (owned state), `battle_script_commands.c` and `battle_main.c` (entry/turn boundaries), `battle_ai_switch_items.c` only if existing sequence flags cannot cover the selection path, `battle_scripts_1.s` (returning presentation), `test/battle/koanko.c`. Do not edit party content already conforming to the handoff.

## 4. Dependencies

Existing transition, trainer-slide, No Order Change, party-state and Break Charm systems. Individual boss stages consume these entry/timing hooks. No temporary-ally implementation is needed to review this stage.

## 5. Edge cases and invariants

- No new battle instance, intermediate payout, trainer completion, win text or cleanup. Normal per-KO EXP remains; do not award an extra six-mon payout.
- Preserve player party/PP/items/bars, side hazards/screens/weather, delayed effects, Reduction state, trainer inventory and consumed gimmick resources, with exactly one exception: clear only the opponent trainer's Tera-used flag once at the ordinary→conjured transition. Player Tera usage and opponent Dynamax/other gimmick usage remain unchanged.
- Player exhaustion on a simultaneous final-team-one KO prevents conjuring. A surviving reserve allows ordinary mandatory replacement before continuation.
- No Order Change must not block empty-slot replacement, poison/recoil/Destiny Bond faints, or saved-bar processing. Entry hazards can defeat multiple successive bosses without reordering or repeating dialogue.
- Deferred events cannot give a fainted/replaced occupant a queued action. Flags are committed before yielding to controllers.
- Keep current levels and ordinary party definitions. Confirm Alcremie actually G-Maxes through existing gimmick activation; its flags alone are not runtime proof.

## 6. Test strategy

At implementation time: `wsl make check TESTS=Koanko`; adjacent groups `TESTS='Arcanum No Order Change'`, `TESTS='Break Bar Charm'`, `TESTS='Trainer Break Bar'` (use actual name prefixes, separate invocations).

Extend existing fixtures with fixed order despite matchup preferences, living phazing immunity/damaging phazing, mandatory faint replacement, first-team switching unchanged, once-only entry dialogue, repeated transition calls, simultaneous KO and controller re-entry. Assert battle identity/outcome/rewards and preserved pending Wish/Future Sight, player resources and bars. Seed both trainers' Tera-used flags and opponent Dynamax/other gimmick-used flags: only the opponent Tera-used flag clears. After consuming the renewed opponent Tera opportunity, repeat transition calls, switch-ins and controller re-entry and prove it stays consumed. Also cover an initially unused opponent Tera flag. Add music command/event assertions if the harness exposes them; otherwise emulator-check fake victory → reveal → confirmed Grand Battle without win text. Verify first-team Alcremie G-Max and Calyrex's renewed ordinary AI Tera eligibility without forced activation.

## 7. Completion criteria

Twelve opponents are defeated in one battle; first team behaves ordinarily, second team stays in order and cannot be moved while alive, and genuine faints replace normally. Music and designated slides occur once at safe script boundaries. Only the opponent Tera-used flag resets, exactly once at transition; all other player/trainer/field resources follow the preservation rules and no duplicate rewards occur. All existing Koanko and affected No Order Change tests pass.

## 8. Open questions and readiness

- **Essential presentation decision:** confirm which existing song is Grand Battle. `MUS_KAZURADROP_APPEARS` is a candidate referenced by an older progress note, not verified track identity. Mechanics can proceed independently; do not import new music or silently choose a different track.
- **Recommended design approval:** forced-switch immunity is the safest policy preserving strict boss order and reuses tested No Order Change. The handoff requires investigation rather than explicitly choosing immunity; record any rejection before implementation.
- **Default:** retain the existing two-sentence reveal, unchanged configured levels and item/stat tuning. Entry slide lines above are confirmed requirements.
- Ready for implementation after these policy/presentation choices are accepted; planning does not authorize code edits.
