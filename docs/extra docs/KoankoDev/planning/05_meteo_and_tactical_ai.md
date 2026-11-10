# Stage 05 — Meteo and tactical virtual-action selection

Status: **fully planned**, with explicit cooldown defaults for review. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Add shared offensive Arcanum Meteo to Castoria and both TX profiles, then compare conventional moves, the profile's five Wishes and Meteo through normal AI selection. TX alone receives a one-turn Arcanum lockout after a successful Meteo. No forced opener, rotation or scripted Meteo phase.

## 2. Existing code verified

- No MOVE_METEO definition or implementation was found in the inspected constants/move data. Existing `MOVE_METEOR_*` names are unrelated moves.
- `include/battle.h` exposes four Castoria virtual slots and one TX Wish slot. `src/castoria_name_theft.c:sArcanumPool` is Fireball/Revivify/Wish/Polymorph; `CASTORIA_ARCANUM_COUNT` is asserted against this pool. `IsArcanumSlotUsable` validates ordinary restrictions but currently assumes every TX virtual action is Wish.
- `src/battle_ai_main.c:ChooseArcanumForTarget` invokes the battle's dynamic scorer, compares its result against normal scores with strict `>` (ordinary wins ties), and commits Wish metadata only for the selected action. It is shared by singles/doubles. `sDynamicAiFunc` is one global scorer per battle; `AI_RazeluxeTrainer` currently declines non-Castoria virtual scoring, so TX production tactics are incomplete.
- `src/battle_util.c` uses numSpreadTargets, `MOVE_TARGET_BOTH`, standard multi-target damage and the normal PP exemption for virtual slots. `IsArcanumAction` distinguishes committed virtual positions from called submoves.
- `data/battle_anim_scripts.s` has Draco Meteor's staggered falling-rock/plume patterns, Meteor Beam/Cosmic Power's BG_COSMIC/scrolling/star components, and Rock Wrecker's large-rock impact/fragments. Whole move scripts also set/restore backgrounds, blending and targets; they are not interchangeable returning subroutines.

## 3. Implementation design and order

1. Add a new custom move ID/data: Stellar, Special, 150 BP, accuracy 100, priority 0, opposing actives target, ordinary hit effect, no secondary or recharge. Mark encounter-only move-caller/copy exclusions consistently with other boss spells. No new species or Tera activation.
2. Expose TX logical slots 4=Arcanum Wish and 5=Meteo; make every accessor, committed-action check, range/loop bound and legality check agree. Extend Castoria's virtual pool/count to include Meteo while preserving existing order and resources. Use per-owner slot count instead of borrowing a Castoria count as TX policy. Wish eligibility is checked only for the Wish slot.
3. Store TX cooldown by combat owner (stage 04), separate from battler-position transient state. Record one successful execution once per action, outside the per-target spread loop. Recommended success definition: at least one target reaches a successful hit resolution, including a Substitute/bar hit; full miss, all-target Protect/immunity, no targets, interruption and attack cancellation do not start cooldown. Do not derive success from animation completion or PP deduction.
4. Lock all TX Arcanum choices for its next active turn's command window; normal moves stay legal. Arm at successful action completion, make the next turn's availability snapshot locked, and clear at that turn's end exactly once. A blocked/Time Stopped/flinched next active turn still consumes the lockout; merely absent turns do not. Persist through refresh and switching; an absent owner resumes its one locked active turn when it returns. Clear on genuine owner defeat/battle cleanup. Castoria ignores this mechanism entirely.
5. Add shared outcome evaluation using `TxRollbackCanWishFor` and standard move damage/accuracy estimates, not a second execution path. Evaluate each eligible Wish outcome separately, retain best outcome/recipient, then compare that Wish score with normal moves and Meteo. Commit outcome/identity only when Wish wins; stale candidates clear every selection.
6. Register encounter dispatch for Koanko and extend existing Razeluxe dispatch for its TX. Do not overwrite a battle's global dynamic hook from a TX entry script. Keep Castoria's established route/Command Seal behavior while allowing repeated Meteo. Score normal actions with existing AI; use the stage 03 factored single-candidate damage support where needed for virtual Meteo.
7. Tactical acceptance targets: choose a secure direct KO over unnecessary setup; prefer useful persistent Reduction early when neither side has an immediate winning/losing line; prefer Reversal against meaningful boosts, Reclamation for transferable value, Calamity under favorable survival/tempo, Restoration only for the summoned profile with a depleted eligible layer, Reciprocity when meaningful player healing is expected. Wish considers missing HP/pending Wish; Protect considers delayed recovery/attacks and repetition. Never nominate ineligible/active effects. Ordinary actions win exact ties. Calibrate scoring to these scenario tests rather than forcing a turn schedule.
8. Compose a new Meteo animation from returning fragments: cosmic setup → staggered meteor falls → large impact → restore BG/blending/palettes. Bound concurrent sprites/tasks by reusing staggered waves, and animate spread targets without running extra damage scripts. Preserve allocations/cleanup on low resource headroom. No external artwork required.

Proposed files: move constants/data and declarations, `battle.h` accessors, Castoria virtual pool/usability, TX combat data, AI dispatcher/scoring, ordinary move-end success hook and end-turn cooldown hook, `battle_anim_scripts.s`, focused Meteo/AI tests. Do not modify all spread moves or count meteors as hits.

## 4. Dependencies

Stages 02–04 and stage 01 owner/turn timing. Stage 07 later exercises two live opposing targets; Meteo can first be tested with ordinary doubles fixtures.

## 5. Edge cases and invariants

One damage event per living opponent; standard 75% spread reduction only when standard engine targeting makes it applicable. Disabled/retired slots are not phantom targets. Protect, Wide Guard, redirection and target fainting use ordinary spread rules. A partial successful spread result starts only one cooldown. Meteo never invokes Wishful Thinking. Normal TX slots remain usable during cooldown, even when no Wish outcome is valid. No ordinary four-slot array is indexed with 4/5.

## 6. Test strategy

New prefixes: `Meteo`, `Koanko TX AI`, `TX Arcanum cooldown`. Existing: `TX Wishes`, `Arcanum Wish More Wishes`, `Arcanum`, `Wishful Thinking`. Use separate `wsl make check TESTS='<prefix>'` invocations.

Test one/two targets with measured damage ratios, one event per target, one or both Protect/Wide Guard, Substitute, pending bar/faint and zero valid targets. Verify native-Stellar STAB/defense using the existing chart and that TX acquires no Stellar-Tera flags or one-use boost counters. Preserve the existing Stellar-versus-Terastallized-target multiplier where the ordinary damage engine applies it; no separate chart rewrite. Test success/failure/partial hit cooldown, blocked next turn, absence/re-entry and refresh; Castoria can cast repeatedly. Test normal/virtual tie conservation, no outcome commitment when Meteo/normal wins, direct KO versus Reduction, useful buff reversal versus idle wishes, profile outcome differences and no stale target ownership. Emulator-review animation in singles, Castoria's asymmetric doubles and later Zacian doubles; inspect peak sprite/task use and palette/BG restoration.

## 7. Completion criteria

Meteo has required data, standard spread execution and composed reusable-resource visuals. TX's next active turn locks only Arcanum; Castoria is exempt. AI compares all intended actions tactically, preserves normal ties and never indexes virtual slots into ordinary arrays. Existing Castoria route and TX Wish tests remain passing.

## 8. Open questions

- **Defaults to review:** success means a resolved successful target hit; lockout is the next active battle turn, consumed even if action-paused. These definitions fill handoff ambiguity and must be documented in player-facing mechanics docs.
- **Tuning:** numerical AI weights require scenario validation; no new damage/BST/level retuning is authorized. Tactical criteria above are the acceptance contract.
- Animation resources exist, but visual timing and allocation headroom require emulator evidence at implementation time.
