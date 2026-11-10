# Stage 09 — Arceus staged Bug Space and Command Seal Conversion 2

Status: **fully planned**, explicit fallback/first-turn defaults below. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Deliver the mandatory final boss sequence: fake `Alter Ego's Bug Space` presentation with genuine Multitype; survival through its first complete turn; dismiss Doom Clock while retaining boost crushing; grant one full saved layer over remaining HP; on that saved layer's break spend Koanko's seal and adapt actual Arceus form/Judgment to the effective breaking attack type. No healing, copied opponent mechanics, Castoria Command YOU effects, Plate dependency or Kazuradrop regressions.

## 2. Existing code verified

- `src/data/koanko_party.h`: level-105 Arceus already has Judgment/Earth Power/Ice Beam/Recover, Multitype, no Plate. No Arceus encounter state/sequence exists in `battle_koanko.c`.
- `include/battle.h` owns `bugSpace`; `battle_util.c:ABILITY_BUG_SPACE` initializes Kazuradrop-specific flags, threshold160/decay10, a configured bar, UI and legend-punishment script. Calling the real ability path would add wrong mechanics and dialogue.
- `battle_end_turn.c:HandleEndTurnBugSpaceDecay` currently combines threshold decay/tier changes **and** positive-stage crushing. Crushing halves positive stages toward neutral. It can choose `BattleScript_BugSpaceStatDecayFirst`, which invokes Kazura dialogue and FLAG_TEMP_1. `HandleEndTurnBugSpacePassiveOHKO` handles Doom death.
- `battle_script_commands.c:IsMoveBlockedByBugSpace` blocks move BP >= threshold. Thus 254 is not an unconditional disable. Other tier-gated damage/item/faint behavior also reads bugSpace. `battle_util.c` can issue Kazura legend-punishment switch-in dialogue whenever Bug Space is active.
- `kazuradrop_battle_ui.c` offers Create/Destroy/SlideOut/SlideIn panel functions. `ReshowKazuradropBattleUI` recreates the panel whenever bugSpace.active, so a slide-out alone does not keep it hidden after a menu.
- `battle_interface.c:CreateAbilityPopUp` supports `gBattleScripting.abilityPopupOverwrite`; presentation can show Bug Space without writing Arceus's ability.
- `battle_break_bar.c:GrantSavedBreakBar` saves canonical covered HP/maxHP. Fill/animate/restore scripts already exist. `TryBreakBar` deliberately bypasses configured original onBreak/afterBreak scripts for saved layers; Arceus therefore needs a targeted saved-layer event continuation, not an ordinary configured phase callback.
- HP updates have separate move/passive/instant-death paths, and `Cmd_tryfaintmon` queues bars before genuine faint effects. A clamp only in the ordinary damage formula cannot cover this requirement.
- `battle_script_commands.c:Cmd_settypetorandomresistance` reuses `GetTypeModifier`, but uses mutable last-move globals, changes typing only, excludes special types, and has ordinary Conversion 2 legality rules. Copy its chart-driven idea, not the whole command. `battle_util.c:GetTypeModifier` respects inverse matchup configuration.
- `GetBattleMoveType` uses current dynamic type; `SetTypeBeforeUsingMove` resolves move/ability/item overrides. Last-hit globals are updated later and may not describe the saved-layer-breaking event after nested/delayed scripts.
- `battle_main.c:GetDynamicMoveType` currently resolves EFFECT_CHANGE_TYPE_ON_ITEM from held item. Judgment requires a narrow encounter-form override; changing Arceus's types alone will leave no-Plate Judgment Normal.
- `gen_4_families.h:ARCEUS_SPECIES_INFO` defines all **18 ordinary forms**, all with Multitype and shared front/back pixel data, distinct type species and normal/shiny palettes. Seventeen type directories and palette declarations exist in `src/data/graphics/pokemon.h`. There is no Stellar/Divine/Caster/Alter Ego Arceus form. Shared indexed shape is intentional resource reuse; switching the actual species record plus its palette is the real typed form, unlike recoloring SPECIES_ARCEUS_NORMAL.
- `battle_anim_effects_3.c:AnimTask_TransformMon` mosaics the attacker and swaps graphics by a supplied mode. `AnimTask_PolymorphMon` mosaics the target, loads party graphics and updates its healthbox; blindly calling it would reload canonical Normal graphics unless adapted. `battle_anim_scripts.s` has both scripts. Existing Castoria seal presentation is text/charge spending, not evidence of a reusable standalone seal graphic.

## 3. Implementation design and order

### A. Owned state and opening turn

1. Add an owner-bound Arceus lifecycle: NOT_ENTERED, OPENING_PROTECTED, CRUSH_ONLY_SAVED, ADAPTING, ADAPTED, FINISHED. Store first-full-turn eligibility, saved-layer tag/index, one seal-available flag, breaking-damage context and effective form override. Commit state before yielding. Keep this separate from ability/stages and Castoria's charges.
2. At entry, arm the HP floor **before entry hazards/delayed entry damage**, then run the stage 01 Arceus slide. Initialize real Bug Space data explicitly for this owner using a Koanko mode, not the ability initializer: source=Arceus, ordinary opening threshold/decay baseline, Doom panel visible and crushing enabled. Do not set Kazuradrop encounter flags or grant its configured bar/Guts/legend punishment.
3. Preserve the exact requested fake presentation text `Alter Ego's Bug Space`, using abilityPopupOverwrite and the existing presentation pattern. If the ordinary popup owner label would instead identify Arceus, supply a presentation-only label override so the complete requested wording is shown; do not change actual species, typing or ability to achieve the label. Restore popup/label context afterward. Effective ability remains Multitype throughout. Use Koanko-specific returning messages where existing scripts contain Kazura taunts.
4. Define the protected complete turn as the first command-selected full battle turn with Arceus present. If it arrives mid-turn, protect the arrival remainder and the following complete turn. If it arrives at a replacement boundary before selection, that next turn is its first. Item-only/idle/Time Stopped actions still count; scripts/bar processing do not. Keep floor until all end-turn effects for that turn finish.
5. Add one owner-scoped clamp shared by HP preview/commit paths for move, passive and instant-death updates; harden the genuine-faint gate as the last defense for direct zero-HP writes. Clamp to 1 before recording faint/break/reward effects or syncing zero HP. Cover OHKO, Perish Song, Destiny Bond, recoil/self-KO, hazards, poison/weather and delayed attacks. This is protection, not a restore/heal event; never trigger healing benefits.

### B. Dismiss clock and cover underlying HP

6. Before the following selection, run the exact dismissal line from the handoff, slide the Doom Clock out, and set persistent owner-specific panel-hidden state. Gate Create/SlideIn/Reshow as well as initial drawing. Keep bugSpace.active for crushing, but explicitly disable threshold blocking, threshold decay/tier transitions, Doom/passive OHKO and Kazura-specific dialogue/legend punishment.
7. Extract the existing crushing operation into a reusable branch/helper, preserving original Kazuradrop call order and messages. Koanko mode uses a plain crushing message/animation without ability misidentification or global FLAG_TEMP_1 ownership. Keep candidates 254/0/NORMAL only as data values if convenient; behavior must be controlled by explicit mode, not sentinel constants.
8. Use `GrantSavedBreakBar`, existing fill and animate helpers to cover the remaining underlying HP with exactly one full maxHP layer. Record which saved layer belongs to the seal. This does not heal the covered HP. Remove the floor after the saved layer is successfully registered/filled, before commands resume. Any pre-existing covered layers must retain their own identities and scripts.

### C. Breaking type, seal and genuine form override

9. At the authoritative HP commit that empties the tagged saved layer, capture this event's resolved damage type/source/validity into owner state. Use actual action damage context after dynamic type resolution; for delayed attacks use their stored resolved type. Do not read gCurrentMove or last-hit globals later. Non-attack instant deaths/residual sources have no attack type; they must not inherit stale context. Multi-hit records the hit that actually empties the layer once.
10. Extend the saved-layer resolution seam to optionally push a Koanko-owned returning continuation **after the shared saved HP restoration and before returning to battle**. Include the extra stack entry in preflight capacity checks. Gate it to the recorded layer/owner and unspent seal; all other saved bars preserve existing behavior. Do not replay original onBreak scripts or add another bar/refill.
11. Commit ADAPTING/spent seal before the trainer slide. Choose uniformly from an explicit type→Arceus species mapping restricted to the existing 18 forms whose `GetTypeModifier(breakType, candidateType) < 1`. Use tagged RNG over the finite eligible list, no rejection loop or hardcoded matchup lists. Ordinary current form may remain eligible; valid prior typing does not force an unrelated nonresistant result.
12. For unknown/typeless damage or an empty resistance list (notably native Stellar/Divine under current chart), **safe proposed fallback is retain current form/type, spend the one seal, and complete the presentation without a heal**. Divine is boss-exclusive and unavailable to the player through legitimate gameplay; its case is defensive fallback coverage, not an expected player attack. Never invent a boss-only Arceus form or claim resistance where none exists. This is an explicit exception path for review.
13. Apply stage 06's battle-only form override to the chosen actual type-specific species, update typing/graphics/palette while retaining Multitype, and preserve restored underlying HP/maxHP exactly. Add a narrow GetDynamicMoveType/AI query so owned Arceus Judgment uses the override type independently of Plate; ordinary Arceus/item logic stays unchanged. Protect the override from item/Multitype refresh by rebuilding effective data after canonical loads, not by mutating held items or permanent species.
14. Compose a seal-slide → distortion → timed species/palette swap → reveal sequence. Reuse Transform mosaic or parameterized Polymorph visual components, adapted to load the effective form through the owner override. Do not call Transform/Polymorph gameplay commands or copy opponent stats/moves/abilities. Restore mosaic/blending/animation battler context and healthbox/bar visuals even if resource allocation fails.
15. On genuine Arceus defeat or battle exit, remove only its owned Bug Space data/UI, survival floor, tagged saved-layer event, seal/override context and deferred callbacks. Preserve ordinary battle cleanup and avoid setting Kazura game-over flags. Cleanup is safe when interrupted, repeated or after another controller refresh.

Proposed files: Koanko state/helpers/header; `battle.h` Bug Space mode/owner state; `battle_end_turn.c` separated crushing/gates; `battle_script_commands.c` HP/type event and scripted adapters; `battle_break_bar.c` targeted saved-layer continuation; `battle_main.c` dynamic Judgment type; Kazuradrop UI/popup/graphics integration and animation scripts/task adaptation; focused Arceus tests. No new external assets required.

## 4. Dependencies

Stages 01–02 and 06; stage 07 cleanup guarantees singles before the final bosses. Existing saved bars, type chart, Multitype forms, popup override and mosaic tasks. Stage 08 must leave no stale grayscale or disabled-slot state.

## 5. Edge cases and invariants

Exactly one first-full-turn protection window, one dismissal, one saved-layer grant and one seal spend. Pending bars are resolved before faint/reward processing. Stored covered HP returns without extra healing and is unchanged by adapting. Haze/stage changes do not invalidate the form; menus/healthbox updates cannot bring back Doom UI. No Kazura legend punish, dialogue, Guts, move swaps, item restriction or custom game-over leaks into Koanko. Ordinary Kazuradrop remains behaviorally identical.

## 6. Test strategy

New prefixes `Koanko Arceus` and `Koanko Command Seal`; existing `Break Bar`, `Arcanum Wish Endurance`, `Conversion 2`, `Multitype`, `Kazuradrop`, and narrow Polymorph/Transform tests. Use separate focused `wsl make check TESTS='<prefix>'` invocations.

Test the exact `Alter Ego's Bug Space` opening presentation, genuine Multitype throughout, and restoration of temporary popup/label overrides. Test mid-turn versus before-selection entry, entry hazards, first-turn exact 1HP floor across every HP source/instant KO, genuine survival without false faint/EXP, dismissal before next selection, saved layer covering a damaged base and revealing exact HP, menu round trips while hidden, continued crushing without BP blocking/tier death or Kazura text. Break with Weather Ball/Tera Blast/ability-converted moves, called moves, delayed attack, multihit and no typed source; verify captured type differs from stale globals. Enumerate eligible forms against chart for all ordinary attacks, tagged uniform choice, custom-type empty set, exactly one seal, actual species/palette/typing/Judgment, genuine Multitype and unchanged stats/moves/party/underlying HP. Test refresh/item/ability interactions, interrupted animation and battle-loss cleanup. Inspect original Kazuradrop regressions separately, including its existing `kazuradrop_stack_breaker.c` KNOWN_FAILING AnimTask_ShakePlatforms teardown exception.

## 7. Completion criteria

The whole staged sequence is observable at correct selection boundaries, survives unusual HP sources and refreshes, and implements genuine typed Arceus/Judgment adaptation with one seal and no extra heal/bar. Shared Kazuradrop behavior and ordinary Arceus/Conversion 2 remain unchanged. Visual distortion is composed from existing resources and cleans up completely.

## 8. Open questions and discrepancies

- **Default for approval:** first complete turn includes the arrival remainder plus one full selected turn when entry was mid-turn; fallback retains current form and spends the seal when no valid resisting form exists.
- **Opening data default:** use existing opening Bug Space threshold160/decay10 while the clock is displayed, then explicit crush-only mode. The handoff does not set opening threshold values; no unapproved global tuning change is implied.
- **Assets clarified:** genuine typed species share base sprite pixels by design; their type/palette records exist. A demand for distinct new silhouettes would require new artwork and is not assumed.
- **Confirmed matchup clarification:** native Stellar TX has no weaknesses or resistances against player-accessible attack types. TYPE_DIVINE intentionally deals 2× to Stellar, but Divine is boss-exclusive and unavailable through legitimate player gameplay. Preserve the interaction and access restriction; this is not a discrepancy or an open design decision.
- **Seal presentation:** a returning Koanko slide plus existing distortion is sufficient. An additional reusable seal glyph was not established by the inspected code and must not be invented as an existing asset.
