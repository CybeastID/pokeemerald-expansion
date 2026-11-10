# Stage 06 — Calyrex and Eternatus Break Bar form transitions

Status: **fully planned**, bar-count recommendation awaits design review. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Implement first-original-bar transitions: Ice Rider → Shadow Rider, and Eternatus → genuine Eternamax. Calyrex has ordinary AI-controlled optional Fire Tera; preserve it through transformation when active. Reuse the shared Break Bar engine and a narrow battle-form override. Zacian arrival is a required follow-on stage, not replaced by weakening Eternamax.

Confirmed later design decision, superseding forced Fire Tera in the original handoff: Calyrex is eligible to choose Fire Tera through ordinary AI, but neither entry nor transformation forces activation. Stage 01 renews only the opponent Tera-used flag once at the team transition.

## 2. Existing code verified

- `src/data/koanko_party.h` already has Ice Rider's required initial kit/Fire Tera type at level 102 and Eternatus's initial kit at level 103. No form-transition policy exists in the inspected Koanko module; `gKoankoConjuredBreakBars` is empty.
- `src/battle_break_bar.c:TryBreakBar` consumes an original layer, executes its onBreak script and optional afterBreak continuation, and uses separate saved-layer restore behavior. `BattleScript_BreakBarRefill` uses current maxHP; multi-hit overkill cannot flow into the next layer.
- `src/battle_util.c:TryBattleFormChange` saves changedSpecies, changes **party** species and recalculates stats. It requires table eligibility; Calyrex fusion tables are item/fusion rules, not an Ice→Shadow battle phase. Do not assume that calling it with a Mega method provides a scripted boss form change.
- `RecalcBattlerStats`, `CopyMonLevelAndBaseStatsToBattleMon`, `CopyMonAbilityAndTypesToBattleMon`, and existing sprite/healthbox loading paths provide reusable pieces, but raw stat reloads must follow stage 02 ownership rules.
- `src/battle_terastal.c:ActivateTera` sets genuine Tera flags and runs its battle script. The configured Fire Tera type supplies the ordinary AI option; it does not require an entry-time activation script.
- `src/data/pokemon/species_info/gen_8_families.h` defines Shadow Rider with 100/85/80/165/100/150 and As One Shadow Rider; genuine Eternamax has HP255/Atk115/Def250/SpA125/SpD250/Spe130 and Pressure. Zacian-Crowned/Intrepid Sword also exists.
- Existing front/back/palette files are present for Calyrex shadow, Eternatus eternamax and Zacian crowned_sword. Species records reference their respective graphics. Presence was checked; visual quality and runtime allocation were not verified here.
- A subsequent read-only Pillow check confirmed indexed P front/back assets at 64×64 for Calyrex Ice/Shadow, Eternamax and Crowned Zacian. This is metadata verification, not emulator visual QA.
- `data/battle_anim_scripts.s:gBattleAnimGeneral_MegaEvolution` uses particles, a white flash and `AnimTask_HideSwapSprite`; `src/battle_anim_effects_3.c:AnimTask_HideSwapSprite` performs a graphics form swap. Transform/Polymorph tasks also exist but have distinct attacker/target assumptions.

## 3. Implementation design and order

1. Define encounter-owned form override state keyed by enemy party owner: base species, effective species, phase-completed flag and only the state needed for clean rebuild/cleanup. Implement a narrow scripted form-change helper in the Koanko module using a scratch copy of the canonical mon to calculate the target form's stats; do not invoke Mega/fusion/Transform gameplay and do not permanently write target form into the party.
2. Install the helper into supported clean battler/controller/stat refresh paths so effective species/types/ability/graphics survive refresh, without repeated growth, entry abilities or automatic item-form reversion. Preserve level/IV/EV/nature/items, statuses, stages, Tera flags, volatile state, No Order Change, bars and action ownership. Expose this helper for stage 09; do not invent a universal form framework.
3. Configure original Break Bar profiles for the two owners only. Recommended minimum is **one extra bar each**, producing one first-break transition and one final form bar. The handoff does not specify a higher total; do not silently add bars. Original scripts are keyed to phase/owner, not whichever species happens to be displayed.
4. Leave Calyrex's Fire Tera activation to ordinary AI eligibility, scoring and command-selection mechanics, using the opponent's renewed Tera opportunity from stage 01. Add no forced entry/phase Tera script or guaranteed AI opener. On original bar break: white flash → apply Shadow Rider effective species/stats/As One ability and Astral Barrage/Psyshock/Aura Sphere/Nasty Plot → refresh sprite/palette/healthbox → shared refill using the new maxHP. If Fire Tera is active, preserve it and its consumed-use bookkeeping. If inactive, leave Calyrex un-Terastallized and preserve normal eligibility for a later AI choice when the trainer's use remains available. Transformation neither activates Tera nor clears its used flag.
5. Eternatus onBreak: commit phase once, prepare genuine Eternamax stat/form state, play the existing Mega visual with the battler bound correctly so its white-flash swap shows Eternamax, restore animation context, and refill through the shared script. Do not set Mega/Dynamax active flags, a Mega slot, or a Dynamax HP multiplier merely to get the visual. Change only Dynamax Cannon→Eternabeam in the kit; Eternabeam retains ordinary recharge.
6. Add a returning after-animation/phase continuation that requests stage 07 Zacian arrival. Queue it immediately after the transform animation and before the next command selection, preserving any already-committed ordinary action queue. A form transition must not grant an extra turn or change a queued action solely by reinterpreting its old slot index.
7. Rebuild caches/UI once after each transition. Replacing a kit grants the new moves their standard starting PP once; subsequent refreshes preserve spent PP. Preserve existing EV/nature tuning rather than inventing a second build for Shadow Rider. Cleanup effective forms on genuine faint/battle end without reviving anything or altering canonical party data.

Proposed files: Koanko module/header and battle-owned state; trainer Break Bar assignments; shared form/stat/controller refresh integration; battle/animation scripts and declarations; focused `koanko_forms.c` tests. No species stat nerfs, new sprites or global form-table changes are required.

## 4. Dependencies

Stages 01–02; existing Break Bar and Tera/graphics resources. Stage 07 consumes the Eternamax-animation completion event. Stage 09 reuses the battle-only form helper with a no-heal policy.

## 5. Edge cases and invariants

Saved layers on top of an original phase restore covered HP without replaying transformation. Multi-hit/OHKO/Destiny Bond/self-KO and delayed damage resolve bars at the existing move/end-turn boundaries. Pending scripted transitions are single-use across controller yields. Preserve status, stages and queued action identity; do not copy opponent data. Calyrex's underlying form/ability/kit changes independently of Tera: active Fire Tera persists, while an inactive state stays inactive unless ordinary AI subsequently activates it. Entry, refresh and transformation never reset trainer Tera usage or force the choice. Eternamax has genuine form stats and Recover calculates from genuine current maxHP. No automatic three-turn Dynamax expiration applies.

## 6. Test strategy

New prefixes `Koanko Calyrex` and `Koanko Eternatus`; existing `Koanko`, `Break Bar`, `Trainer Break Bar`, `Wishful Thinking`, and relevant Tera/recharge tests. Run focused `wsl make check TESTS='<prefix>'` invocations separately.

Cover ordinary AI choosing Fire Tera and deferring it; entry must not activate it automatically. Transform both an active Fire-Tera Calyrex and an un-Terastallized Calyrex: the former retains Fire Tera and consumed-use state; the latter remains inactive, retains unused eligibility and can choose Fire Tera later through ordinary AI. Also prove that already-consumed opponent Tera usage blocks another activation and is not cleared by transformation/refresh. Cover first-original-bar trigger, saved-layer non-trigger, same-turn queued action, new stats/ability/moves/PP, repeated refresh, statuses/stages preserved, new maxHP refill and multi-hit isolation. Update existing Koanko/trainer-rule unit assertions that currently require every conjured slot's Break Bar configuration to be NULL/empty: only Calyrex/Eternatus acquire the approved original profiles; solo TX stays barless. Assert no Mega/Dynamax flags or extra resource consumption, Eternabeam recharge, Recover amount, canonical species/stats unchanged and cleanup on loss. Emulator-review white flash in both Tera states, Mega visual target, sprite position/palette and resource restoration.

## 7. Completion criteria

Both first-break form transitions occur once without switching, use correct form stats/kits and existing bar timing, and preserve the stated state. Calyrex's Fire Tera is optional under ordinary AI, persists when already active, and is neither forced nor renewed by transformation. Eternamax has 250 defenses and no fake gimmick mechanics. The Zacian request is available at the required animation boundary, ready for stage 07.

## 8. Open questions

- **Essential design choice:** total bars are not configured today. Recommend one extra each; a higher approved number requires explicit configuration but no new engine.
- **Default:** preserve current EVs/nature/items/levels and all statuses/stages; no unrequested full restore/cures. New-form starting PP is assigned once.
- Visual use of the Mega symbol is intended by the requested Mega animation; it must not imply or activate Mega gameplay.
