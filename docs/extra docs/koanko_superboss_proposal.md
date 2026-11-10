# Koanko Superboss: audit and implementation proposal

Repo: `CybeastID/pokeemerald-expansion`, branch `mypatch`, shallow clone at `1f7352db` (2026-11-09). Newer than the `68c13b35` the code tour was read at.
Method: read from source with grep/sed. **Nothing compiled or run.** Items marked **[unread]** are inferred from symbol names or call sites, not from reading the function body.
Status: planning only. No files in the repo were modified.

---

## 1. Existing implementation audit

### 1.1 What the repo already gives us

| Area | Evidence | State |
|---|---|---|
| In-place team swap | `TryStartKoankoConjuring` / `ConjureKoankoTeam` (`src/battle_koanko.c:23,47`), hooked in `Cmd_checkteamslost` before `B_OUTCOME_WON`; enum `KOANKO_ORDINARY_TEAM/CONJURING/CONJURED_TEAM` | Done, single-use, guarded |
| Fake victory music precedent | `BS_KazuradropFakeVictorySting` (`battle_script_commands.c` ~15286) does `PlayBGM(MUS_VICTORY_TRAINER)`; `FadeOutAndPlayNewMapMusic(MUS_KAZURADROP_APPEARS, 4)` just above it | Reusable pattern, nothing wired for Koanko |
| Trainer slide with ad hoc text | `gBattleStruct->trainerSlideMsg` + `BattleScript_TrainerASlideMsgRet`; conjure script is `slide msg` -> `BS_ConjureKoankoTeam` -> `return` (`battle_scripts_1.s:9501`) | No enum additions needed for new lines |
| Break Bar engine | `src/battle_break_bar.c`, `BreakBarConfig` phases, `GrantSavedBreakBar`, trainer rule table (`src/trainer_break_bars.c`, `src/data/trainer_break_bars.h`) | `gKoankoConjuredBreakBars` is **empty** today |
| `noOrderChange` | `battlerState.noOrderChange`, ~16 enforcement points, AI respects it, status icon module | Cleared on every switch-in (`SwitchInClearSetData`) |
| Arcanum virtual slots | `GetBattlerMoveSlotCount/GetBattlerMoveAtSlot/GetChosenMoveFromPosition/IsArcanumAction` (`battle.h` ~1331-1370) | Castoria-only data, but generic accessors |
| Special AI hook | `AI_FLAG_DYNAMIC_FUNC`, global `sDynamicAiFunc` (`battle_ai_main.c:47`), registered by `setdynamicaifunc` (`asm/macros/event.inc:2130`) | **One function per battle.** Koanko registers none |
| Reduction | `src/battle_reduction.c`, `struct ReductionData { active, grace, nextLoss, occupant[] }`, `ReductionResetOccupant` | Shape matches the "global counter, per-occupant reset" requirement **[unread]** |
| Time Stop | `VOLATILE_TIMESTOP`, `MOVE_EFFECT_TIMESTOP` | Exists |
| Species / forms | `SPECIES_ETERNATUS_ETERNAMAX` (stats 255/115/250/125/250/130, Poison/Dragon, Pressure, canonical), `SPECIES_CALYREX_SHADOW`, `SPECIES_ZACIAN_CROWNED`; all 17 Arceus type form graphics dirs present | Exist |
| Animations | `gBattleAnimGeneral_MegaEvolution` (`battle_anim_scripts.s:31604`), `gBattleAnimMove_Polymorph` (uses `AnimTask_PolymorphMon`), `gBattleAnimMove_Transform` (`AnimTask_TransformMon`) | Exist, reusable fragments |
| Behemoth Blade | `EFFECT_DYNAMAX_DOUBLE_DMG` (`moves_info.h:18138`, handler `battle_util.c:7389`) | Whether Eternamax qualifies **[unread]** |

### 1.2 Discrepancies between the design doc and the code

1. **First six: match the handoff exactly** (`trainers.party:17033`, all Lv100, same moves). `trainers.h` is generated, so edit `trainers.party`. Trainer AI flags are `CHECK_BAD_MOVE / ACE_POKEMON / OMNISCIENT / TRY_TO_FAINT`; Music is `Elite Four`. Alcremie has `Gigantamax: Yes` but no visible Dynamax level; whether the AI can actually Dynamax it is **[unread]**.
2. **Conjured six do not match** (`src/data/koanko_party.h`):

| Slot | Handoff | In code |
|---|---|---|
| 1 | Smeargle, one real move (slot 0) | Focus Sash, 4 real moves (Seismic Toss/Night Shade/Spore/Endeavor) |
| 2 | TX Rollback: Wish/Doom Desire/Protect/empty | **`SPECIES_JIRACHI`**, Steelium Z, Iron Head/Zen Headbutt/Thunder Wave/Wish |
| 3 | Ice Rider, Fire Tera, 4 moves | matches |
| 4 | Eternatus, 4 moves | matches |
| 5 | Roar of Eternity/Flash Cannon/Earth Power/Thunderbolt | `MOVE_ROAR_OF_TIME`, Flash Cannon, Earth Power, **Stealth Rock** |
| 6 | Judgment/Earth Power/Ice Beam/Recover | Judgment/**Calm Mind**/Recover/Earth Power |

Configured levels are **100, 101, 102, 103, 104, 105** ascending. No `Music:` change exists for phase 2.
3. **Reveal line** in code is `"You thought that was all?\nThen watch what I can create!"`, longer than the handoff's. Decide which wins.
4. **"Grand Battle" has no song constant.** `songs.h` has only `MUS_VS_*`, `MUS_VS_ALICE` (561), `MUS_KAZURADROP_APPEARS` (562). Which track is meant?
5. **`SPECIES_TX_ROLLBACK` is still the Jirachi-clone prototype** (`gen_3_families.h:13083`): flat 100 stats, Steel/Psychic, Serene Grace, Jirachi cry/dex text. The handoff's 675 BST / pure Stellar / Wishful Thinking are not in the tree.
6. **Not present anywhere:** `Roar of Eternity`, `Meteo`, `Wishful Thinking`, Smeargle Any Move, `TX_WISH_REDUCTION`.
7. **All six conjured mons carry `dynamaxLevel`**, so `opponentMonCanDynamax` bits get set (`battle_main.c:2011`, consumed at `battle_gimmick.c:84`). The AI may be able to Dynamax Eternatus/Calyrex/etc. Not intended by the design, **[unread whether anything else blocks it]**.

### 1.3 Corrections to what I told you earlier in this chat
- I called the TX profile struct "the seam for Koanko's solo TX". **Wrong.** Koanko's TX is an ordinary party member in singles. The TX formation layer is hard-wired to Castoria + `TRAINER_RAZELUXE` + party index 4 + `gEnemyParty[5]` (`CanPrepareCreation`), and `TxRollbackHasWishes` requires `stage >= AMBUSH_PENDING` and `TxRollbackOwnsPartyMon`, so it is **false** for Koanko's TX. What we actually reuse: the **wish scripts and AI scoring**, not the formation.
- "Boost crushing" is not a named symbol. It is the stat-stage halving block inside `HandleEndTurnBugSpaceDecay` (`battle_end_turn.c` ~1385-1475). Details in 2.7.

---

## 2. Architecture

### 2.1 Transition and music (cheap)
Extend `BattleScript_KoankoConjureTeam`: fake-victory sting (copy the Kazuradrop pattern) -> slide message -> conjure -> crossfade to the phase-2 track. Still no `B_OUTCOME_WON`, so no rewards path runs. Add `Music` handling in script/native, not `trainers.party`.
Edit `sKoankoConjuredParty` to match the handoff, using `SPECIES_TX_ROLLBACK` in slot 2.

### 2.2 No voluntary switching for the conjured six
Reuse `noOrderChange` (already enforced for forced-switch family, AI, Eject items, etc., so Roar/Red Card/Dragon Tail would simply fail, which preserves progression). Problem: it is cleared in `SwitchInClearSetData`, so it must be **re-set on every conjured switch-in** (hook in switch-in effects keyed on `koankoPhase == KOANKO_CONJURED_TEAM`). Side effect: the No Order Change **status icon will show** on all six. Decide if that is wanted or suppressed for this phase.

### 2.3 One Special AI function for the whole fight
`sDynamicAiFunc` is a single global. Koanko needs one `AI_KoankoTrainer` that dispatches on species/phase (TX scoring, Smeargle), registered by a native from `ConjureKoankoTeam` (or a battle-start script), and cleared at cleanup (`ClearDynamicAiFunc` exists, `battle_ai_main.c:7468`).

### 2.4 Smeargle Any Move
- Pool: all move IDs minus an explicit exclusion predicate (Z/Max moves, `MOVE_NONE`, Struggle, custom boss moves in `MOVES_GEN_CUSTOM_START+`).
- Loop per action choice: draw without replacement, put candidate in `gBattleMons[battler].moves[0]`, score through the **existing** per-move AI path, accept if > 0, stop at 60 positives or pool exhausted, then keep the best and leave it in slot 0.
- Infinite PP: reset `pp[0]` after use. Also cover party-struct sync **[unread]**.
- Encore/lock prevention: block at inflict time (Encore effect check; Smeargle holds a Sash, so Choice items are moot unless it can be given one).
- **Performance is the real unknown.** Reaching 60 positives may take hundreds of full AI evaluations on GBA. I have not measured this. Needs a spike with a cycle counter before committing to 60.
- **RNG:** sampling consumes the RNG stream; determinism for tests needs a dedicated `RandomUniform` tag or seeded sub-stream.
- It must also satisfy "ordinary AI zero-score semantics" (what score 0 means vs. `AI_SCORE_DEFAULT`): **[unread]**.

### 2.5 TX Rollback (Koanko's solo TX)
New predicate (e.g. `TxIsKoankoSolo(battler)`) replacing `TxRollbackHasWishes` at the AI/Wish-availability call sites for this case. Add `TX_WISH_REDUCTION` (and map it to the existing Castoria reduction script path, with the 1,2,3... counter variant). `ReductionData.nextLoss` is already a global counter and `ReductionResetOccupant` is per-battler, which matches "counter persists, occupant resets" **[unread: confirm start value 10 vs 1, and what recast does]**.
Species data: replace the prototype with the agreed 125/70/135/130/135/80 Stellar mon, ability Wishful Thinking.

### 2.6 Castoria's TX: 100% -> 60% -> 30% max HP
`BreakBarConfig.phases[].onBreakScript` runs *instead of* the default `BattleScript_BreakBarRefill`. So: give each phase an `onBreakScript` that shrinks `gBattleMons[].maxHP` (battle-only), then `goto BattleScript_BreakBarRefill`. The existing `afterBreakScript`s (Wish, Doom Desire) stay as they are. Watch: exit-HP sync (`battle_main.c:5728-5733`, saved-layer scaling), `ReductionCanonicalMaxHp` if Reduction is active, and `breakBarSavedMaxHp[]` interaction with Endurance.

### 2.7 Wishful Thinking
Per-party-slot battle-only growth state (same placement idea as `PartyState` bars), applied as a multiplier on the five non-HP raw stats, not `statStages[]`. Triggers: success points of `trywish` (`BattleScript_EffectWish`) and `trycastoriawish` (`BattleScript_EffectArcanumWish`), plus the scripted first-break Wish. Stat-rewrite precedent and conflict: Reduction rewrites stats from `base[]`/`applied[]` (`ReductionBeginMutation/OnStatsCopied`). Growth must be re-applied after any Reduction recompute and after form changes **[unread, needs a read of `battle_reduction.c`]**.

### 2.8 Meteo (shared)
New custom move after `MOVE_ARCANUM_WISH` in the custom range; spread target; Arcanum slot entry for Castoria and both TXes. TX-only cooldown flag in per-battler state, set on **successful** execution, consumed at next action selection, with explicit handling of "next turn" across switch/paused turns. Animation: compose from existing Draco Meteor/Cosmic Power/Rock Wrecker pieces; mind sprite budget (`BS_TxRollbackCreate` already guards `freeSprites < 20`).

### 2.9 Calyrex Ice -> Shadow and Eternatus -> Eternamax
Both via a Break Bar phase `onBreakScript`: white flash / Mega animation, species swap, then refill.
- The existing form-change tables only cover Reins of Unity for Calyrex (`form_change_tables.h:1501-1502`). There is no battle-triggered Ice->Shadow. A new scripted form-change helper is required.
- Do **not** reuse the Mega handler as-is: it marks `GIMMICK_MEGA` as activated (`battle_script_commands.c` ~10200). Reuse only the animation script.
- Species swap needs the same controller refresh the Mega path uses (`REQUEST_SPECIES_BATTLE`), plus ability/type/stat recompute and Tera preservation (verify `GIMMICK_TERA` state survives the species swap) **[unread]**.
- **Number of bars for Calyrex and Eternatus is not specified in the handoff** ("first Break Bar break" implies at least 1).

### 2.10 Zacian-Crowned (largest unknown)
The TX layer is a *slot-policy overlay that disables the **player** right slot and populates the **opponent** right slot* (`TxRollbackGetSlotPolicy`, `BS_TxRollbackCreate`). Zacian needs the mirror image, with a mon that is not in `gPlayerParty`. Two candidate designs:
- **A. Mid-battle `BATTLE_TYPE_INGAME_PARTNER`.** Reuses `SetControllerToPlayerPartner` (AI controller) and partner-faint handling (`battle_main.c:4801-4814`), but 67 references across ~15 files assume `gPartnerParty`/multi flows, and win/loss and party screens assume that structure.
- **B. Mirrored slot-policy layer** (new `ALLY_*` stage machine, player-right populated, opponent-right disabled). Cleaner ownership ("not a party mon, not a loss") but all new, and every `TxRollback*` call site that hardcodes player-right-disabled needs an audit.
Neither has been read deeply enough to commit. Recommend a **time-boxed spike** (summon a stub ally in a test, faint it, tear it down, confirm `NoAliveMonsForPlayer` ignores it) before approving the rest of the Eternatus phase. Also needed: Behemoth Blade vs. Eternamax qualification (`EFFECT_DYNAMAX_DOUBLE_DMG` at `battle_util.c:7389`; read what it tests), and Bug Space crush interaction (crush hits the whole opposing side, so Zacian must be gone before Arceus; it will be, by design).

### 2.11 Dialga / Roar of Eternity
New custom move: 150 BP, 90 acc, Roar of Time recharge, 50% `MOVE_EFFECT_TIMESTOP`. Reuse Roar of Time's animation. Replace `MOVE_STEALTH_ROCK` with Thunderbolt in the party. Check the faint-replacement path under Time Stop (softlock case).

### 2.12 Arceus: fake Bug Space, saved bar, Conversion 2
**Bug Space reuse: feasible but must not run through the ability switch-in path.** `ABILITY_BUG_SPACE` activation (`battle_util.c:4757-4782`) also does things that would be wrong for Arceus:
- sets `FLAG_KAZURADROP_ENTERED_BATTLE`, consumed by `kazuradrop_gameover.c`, `battle_setup.c:1331` (whiteout logic) and `battle_script_commands.c:17267/17288/17311`, so a loss would show the **Kazuradrop game-over**;
- sets `VAR_TEMP_0`, clears `FLAG_KAZU_LEGENDARY_PUNISH`;
- calls `ActivateBreakBar(&gKazuradropBreakBarConfig)` and `CreateBugSpacePanel`;
- afterwards, on **player switch-in**, `bugSpace.active` + `GetKazuraLegendTier(battler) != 0` fires `BattleScript_KazuraLegendPunishSwitchIn` (`battle_util.c:4783-4789`). If Arceus uses `bugSpace.active`, **a player switching in a legendary during the Arceus fight gets Kazuradrop's punishment** unless that check is gated.
Recommended shape: a native `BS_StartArceusBugSpace` that sets only the fields it needs, plus a new `bugSpace` field (e.g. `fake`/`borrowed`) that the punish check, game-over flag set, and Kazura-text paths test.

**Crush.** In `HandleEndTurnBugSpaceDecay`, crush halves positive stat stages of the *opposing* side of `sourceBattler` every end-turn, after the decay block, and the function returns early only if `bpThreshold == 0`. So `decayRate = 0` and a large threshold keeps crush running with tier NORMAL. The handoff's `254` is nearly inert for the **BP-block** (`IsMoveBlockedByBugSpace`, `battle_script_commands.c:1385`, blocks moves with BP >= threshold), but `0xFFFF` is safer than 254 unless the UI or `RefreshBugSpaceNumber` depends on the value. `bugSpace` is `memset` at battle start only (`battle_main.c:3045`, **[unread which function]**), so it does **not** clear on Arceus faint: explicit cleanup is needed.

**HP floor for the first turn.** `BugSpaceData` already has `gutsActive` (Endure-style) and `entryDefenseTimer`; check whether either can be reused before building a new clamp **[unread]**. Passive damage already has `ClampBreakBarPassiveDamage`, but instant-KO paths bypass it by design (code tour 1.4).

**Saved bar + Conversion 2 hook.** Code tour 1.3 says saved layers **skip both configured hooks** (`onBreakScript` and `afterBreakScript`), so the Command Seal trigger cannot be a phase script. It needs a new hook in the saved-layer branch of `TryBreakBar` / `BS_RestoreSavedBreakBar` (`battle_break_bar.c` ~259-294, 567).
- **Breaking type capture:** record the effective type at the `QueueBreakBar` call sites (`MoveDamageDataHpUpdate`, `PassiveDataHpUpdate`, `Cmd_setatkhptozero`, `Cmd_tryfaintmon`). Passive/self paths have no move type, so define a fallback (suggest: Normal, or "no resist pick, random type").
- **Type selection:** use the existing type-effectiveness lookup (`CalcTypeEffectivenessMultiplier`-family) against each Arceus form species' types; do not hardcode a matchup list.
- **Form override:** battle-only species swap like 2.9, with an override record that survives `TryBattleFormChange(..., FORM_CHANGE_BATTLE_SWITCH)` and Multitype/plate logic (`form_change_tables.h:725`). Judgment's type source must be checked **[unread]**.
- **Presentation:** compose from `AnimTask_PolymorphMon` (Polymorph) and `AnimTask_TransformMon` (Transform) plus a Koanko slide; invoke only the visual tasks, never the move effects.
- Assets: 17 type form dirs present; nothing missing for non-Stellar types.

### 2.13 Aurora Beam gag
**Recommend cutting.** It requires either a fake attacker battler or a scheduler change for a non-action damage event. The only adjacent precedent (TX ambush) borrows a real battler. Not worth the fragility.

---

## 3. File-by-file change plan (no code yet)

| File | Change |
|---|---|
| `src/data/koanko_party.h` | Conform conjured six; TX in slot 2; Thunderbolt for Dialga; remove Calm Mind etc.; decide `dynamaxLevel` |
| `src/data/trainers.party` | Only if first-six edits are needed (none found). Do not add Music here |
| `src/battle_koanko.c` | Fake-victory + reveal text + phase-2 music; per-switch-in `noOrderChange`; register Koanko dynamic AI; cleanup of all conjured-phase state |
| `data/battle_scripts_1.s` | Extend `BattleScript_KoankoConjureTeam`; add TX/Arceus slide scripts, Calyrex/Eternamax/saved-bar scripts, Meteo and Roar of Eternity scripts |
| `src/data/trainer_break_bars.h`, `src/trainer_break_bars.c` | Fill `gKoankoConjuredBreakBars` rules (Calyrex, Eternatus, Arceus saved-bar grant, optionally TX) |
| `src/battle_break_bar.c` / `.h` | Saved-layer post-break hook; `lastBreakType` capture |
| `src/battle_tx_wishes.c`, `include/battle_tx_rollback.h` | `TX_WISH_REDUCTION`; Koanko-solo availability predicate; Wishful Thinking trigger points |
| `src/battle_ai_main.c` | `AI_KoankoTrainer` (TX scoring, Smeargle pass); keep ties going to ordinary moves |
| `src/battle_reduction.c` / `.h` | Koanko 1,2,3 variant parameter, reuse global counter **[unread]** |
| `src/data/pokemon/species_info/gen_3_families.h` | Real TX data (types, stats, ability) |
| `include/constants/{moves,abilities}.h`, `src/data/moves_info.h`, `src/data/abilities_info.h` | `MOVE_ROAR_OF_ETERNITY`, `MOVE_METEO`, `ABILITY_WISHFUL_THINKING` |
| `data/battle_anim_scripts.s`, `src/battle_anim_new.c` | Meteo animation, Arceus morph composition, Roar of Eternity (reuse Roar of Time) |
| `src/battle_util.c`, `src/battle_end_turn.c`, `src/battle_script_commands.c` | Gate Kazuradrop-only paths on a `borrowed` flag; Arceus Bug Space start/stop natives; Wishful Thinking growth; Encore guard for Smeargle |
| `src/battle_main.c` | Zacian/ally lifecycle (spike dependent); battle-exit cleanup |
| `src/battle_controllers.c`, `src/battle_interface.c`, `src/battle_gfx_sfx_util.c` | Ally healthbox/controller (spike dependent) |
| `src/kazuradrop_battle_ui.c` | Panel hide/suppress for Arceus path |
| `test/battle/...` | See section 5 |
| `docs/mechanics/tx_rollback.md`, `break_bars.md` | Update (code tour also noted `MOVEEND_BREAK_BARS` is documented but does not exist) |

---

## 4. Staged order

1. **Party and data only:** conjured party conformance, `Roar of Eternity`, Dialga set, real TX species. Smallest blast radius, immediately testable.
2. **Transition polish:** fake victory + reveal + music + `noOrderChange` per switch-in. Test rewards/music/state continuity.
3. **Koanko dynamic AI skeleton** (registered, dispatching, all branches returning baseline) plus tests that existing Razeluxe/Alice AI are unaffected.
4. **Meteo + Wishful Thinking + Koanko Reduction wish** (shared TX work). Resolve Reduction/stat-rewrite interaction first.
5. **Castoria TX 100/60/30 max-HP phases.** Regression-heavy; run all Castoria/Razeluxe tests.
6. **Calyrex form change.** Establishes the reusable scripted form-change helper.
7. **Smeargle Any Move**, after a performance spike (can be moved earlier or later freely; it is independent).
8. **Eternatus -> Eternamax** (form change only).
9. **Zacian spike**, then decision A vs B, then implementation.
10. **Arceus**: borrowed Bug Space, saved bar hook, Conversion 2, cleanup. Last because it touches Kazuradrop paths and needs the most regression coverage.

Dependencies: 6 -> 8, 6 -> 10 (form helper); 3 -> 4, 7; 9 gates the Eternatus phase's "complete" status.

---

## 5. Test strategy

Add to the handoff's list, based on what the audit found:
- **Kazuradrop isolation:** with Arceus Bug Space active, player switching in a legendary must **not** trigger `KazuraLegendPunish`; a player loss must **not** show the Kazuradrop game-over; `FLAG_KAZURADROP_ENTERED_BATTLE` stays clear.
- **`bugSpace` cleanup** after Arceus faints (state is not auto-cleared by faint).
- **`IsBattlerAlive` while a break is pending** (code tour risk 7) in every new script that checks "alive": TX shrink, Calyrex/Eternamax transform, saved-bar hook.
- **Break-type capture:** direct move, multi-hit (only the breaking hit), recoil/passive, self-KO, no-type fallback.
- **Smeargle:** RNG determinism under the test runner, empty pool, <60 positives, performance probe (cycles), Encore can't land, PP stays infinite.
- **Dynamax:** AI never Dynamaxes conjured mons (or intentionally does).
- **noOrderChange re-application** after every conjured switch-in, including the replacement after a faint.
- **Zacian:** faint is not a loss; `NoAliveMonsForPlayer` ignores it; teardown works whether it fainted earlier or is still active; no extra turn before Dialga.
- Standing regressions: Castoria/Razeluxe, Kazuradrop (including the known failing `kazuradrop_stack_breaker.c:35`, do not count as new), Break Bars, Time Stop, ordinary doubles.

---

## 6. Risks and open questions

**Blockers / need your answer**
1. **Bars per mon:** how many Break Bars do Calyrex and Eternatus get? (Only TX's two and Arceus's one saved bar are specified.)
2. **"Grand Battle"**: which song? No such constant exists.
3. **Reveal line:** keep the current two-line version or use the handoff's?
4. **TX stats/type:** confirm 125/70/135/130/135/80 with `TYPE_STELLAR` replaces the 100-flat Steel/Psychic placeholder; confirm Wishful Thinking at 10%.
5. **Dynamax on conjured mons:** intended or should `dynamaxLevel` be dropped?
6. **No-Order-Change icon** on all six conjured mons: show or suppress?

**Technical unknowns (need a read or spike)**
- Zacian: design A vs B (section 2.10).
- Smeargle 60-positive CPU cost.
- Reduction vs Wishful Thinking vs max-HP-shrink stat/HP rewriting interplay.
- Whether Eternamax qualifies for `EFFECT_DYNAMAX_DOUBLE_DMG`.
- Judgment's type source under a battle-only species override.
- Whether the `gutsActive`/`entryDefenseTimer` fields can supply the one-turn HP floor.

**Essential vs optional.** Essential: everything except Aurora Beam. Optional/cut: Aurora Beam; Dialga staying in colour under Time Stop grayscale (polish).

---

## 7. Readiness

| Can build as-is | Needs a spike or a read first | Needs your decision |
|---|---|---|
| Party/data conformance, Roar of Eternity, Dialga | Zacian ally | Bars per mon, Grand Battle track |
| Transition polish, `noOrderChange` per switch-in | Smeargle performance | Reveal text, Dynamax, status icon |
| Koanko AI skeleton | Wishful Thinking vs Reduction | TX stats/WT percent |
| Castoria TX max-HP phases | Arceus type-override persistence | Aurora Beam cut |
| Calyrex form change | Eternamax vs Behemoth Blade | |
| Borrowed Bug Space (with the Kazura gating) | | |

Not compiled, not run, and I only read what I cite. Treat every **[unread]** as a to-do before implementation.
