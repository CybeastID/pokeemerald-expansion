# Stage 02 — Validate existing Wishful Thinking and stat ownership

Status: **fully planned**; existing mechanic retained, targeted hardening only. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Establish the existing Wishful Thinking implementation as a reliable shared foundation for both TX encounters. Do not add another ability, growth engine or success detector. Later stages add solo-TX eligibility, reduced maxHP and Meteo; this stage defines their integration contract.

## 2. Existing code verified

- `src/data/pokemon/species_info/gen_3_families.h:SPECIES_TX_ROLLBACK` already has HP/Atk/Def/SpA/SpD/Spe 125/70/135/130/135/80 (675 BST), `MON_TYPES(TYPE_STELLAR)`, and `ABILITY_WISHFUL_THINKING`. Front/palette are TX-specific; back/icon/cry/Dex identity retain Jirachi resources. New art/Dex work is not necessary for this encounter.
- `include/battle.h:PartyState.wishfulGrowth[NUM_STATS]` stores per-owner activation counts as u8; HP slot unused. Counts are battle-only and raw stats remain u16.
- `src/battle_util.c`: `WishfulThinkingGrowStat` mutates one raw stat, wraps Reduction mutations, increments the count; growth is `min(65535, (value * 11 + 5) / 10)`. `WishfulThinkingReapplyGrowth` iterates counts after clean base-stat reloads. `CopyMonLevelAndBaseStatsToBattleMon` reapplies before Reduction overlay restoration.
- `src/battle_script_commands.c:Cmd_switchindataupdate` reapplies after loading controller data. `RecalcBattlerStats` uses the shared base-stat-copy path. This is not a callback safe to invoke on already-grown stats.
- `src/battle_tx_wishes.c:BS_WishfulThinkingTry` checks living attacker and effective ability, chooses uniformly from STAT_ATK through STAT_SPDEF with tagged RNG, grows one stat, and requests max(1, maxHP/16) healing if damaged. It does not change stat stages.
- `data/battle_scripts_1.s:BattleScript_WishfulThinking` performs HP update and ordinary stat animation. Seven success-path calls exist: ordinary Wish, scripted first-break Wish, and five TX Wish effects. The hooks follow Wish success, not delayed healing. No Meteo hook exists.
- `test/battle/ability/wishful_thinking.c` has six tests: immediate heal/growth, untouched stages/delayed non-trigger, pending-Wish failure, switch persistence, absent ability, repeated stat reloads. `test/battle/tx_wishes.c` adds first-break Wish/Doom Desire non-trigger coverage. Existing repaired assertions include the immediate Reciprocity-cast heal.

The user reports repaired targeted tests passed. This planning task inspected their current bodies but did not rerun them or find a fresh Wishful Thinking run in the inspected Koanko logs. Old progress comments saying unrun are not authoritative current execution evidence.

Current-code addendum (2026-11-10, Stage11 reinspection): the older direct caster-only `BS_WishfulThinkingTry` description above is no longer the current dispatch shape. `BattleScript_WishfulThinking` now loops over battler targets; `BS_WishfulThinkingTry` delegates to `AbilityBattleEffects(ABILITYEFFECT_WISH, target, ...)`, whose branch grows/heals each living effective holder. Raw-stat helpers/refresh ownership remain relevant, but the intended caster-only versus all-holder trigger scope must be confirmed before the tactical tests assume either rule. Stage11 records own-Wish/other-Wish/multiple-holder validation. These are observed outside changes, preserved without implementation or a new test run by this planning task.

## 3. Implementation design and order

1. Preserve 10% tuning and current round-to-nearest integer behavior. Add missing behavioral coverage first; change production code only for demonstrated contract gaps.
2. Keep authoritative activation at successful script branches. A future Koanko Reduction Wish must call the same returning ability script once after Reduction is actually activated. Eligibility/AI scoring, cast animation alone, failed Wish and delayed recovery must not trigger it.
3. Document the clean-stat-reload contract next to shared helpers. Later maxHP/form overlays must compose as clean form/base stats → raw Wishful growth → encounter maxHP overlay → Reduction overlay, with HP/controller synchronization handled once. If a path already has grown stats, remove/rebuild its overlay rather than calling reapply directly.
4. Test original party stats/HP independently from battle displays and controller writes. Add normalization only where a reduced maxHP/form override would otherwise leak into permanent party data; do not redesign all stat calculation.
5. Audit count saturation explicitly with low base stats and future base/form changes. u16 saturation is a representation limit, not a proposed gameplay cap. A u8 count can lose activation history after 255, especially when low rounded stats do not grow or the base later changes; the comment that the count can never bind is too broad. If those scenarios are supported, replace count-only reconstruction with bounded exact transformation metadata or a wider checked count whose saturation is proven behaviorally equivalent for every supported base. Never silently wrap or introduce a lower cap.

Proposed changes: existing ability/TX tests, helper comments and any proven overlay/count defect in `battle_util.c`/`battle.h`; only affected controller normalization sites. Keep the existing public helper names and returning script contract.

## 4. Dependencies

Existing success scripts, party ownership, RNG tags and Reduction mutation API. Stage 01 supplies phase identity; stages 04–06 must follow this contract when changing maxHP or forms.

## 5. Edge cases and invariants

- Full HP still grows one stat; tiny maxHP healing rounds to at least 1 and caps at missing HP. Heal Block cannot prevent this immediate ability heal after an otherwise successful Wish.
- Confirmed user clarification: native Stellar TX has no weaknesses or resistances against player-accessible attack types. TYPE_DIVINE remains boss-exclusive and unavailable through legitimate player gameplay; its existing 2× damage against Stellar is intentional and must remain unchanged.
- An ordinary Wish attempt may itself fail under existing restrictions; ability healing does not bypass Wish legality.
- Haze clears stages without clearing raw growth; Accuracy/Evasion/HP are never growth candidates. Uniform choice includes Speed despite its enum ordering.
- No growth on Healing Wish/Lunar Dance, failed/interrupted Wish, delayed heal, Doom Desire or Meteo; successful first-break Wish triggers once.
- No depleted bar restoration or pending-break revival. Ability loss prevents new activations without erasing already-earned raw growth.
- Switches/reloads do not apply growth twice or transfer counts to replacements; real battle cleanup clears it. Transform/Power Trick/Polymorph interactions need explicit tests before asserting broad compatibility.

## 6. Test strategy

Implementation commands: `wsl make check TESTS='Wishful Thinking'`, `TESTS='TX Wishes'`, `TESTS='Arcanum Wish More Wishes'`; run separately. Add tagged-RNG coverage of all five stats, integer boundaries, u16 saturation, >255 activation/rebuild scenarios, Haze, Heal Block with successful virtual Wish, full HP, tiny maxHP, ability suppression, repeated refresh, battle cleanup and byte-for-byte unchanged canonical stats.

Reuse the existing scripted-break and Reciprocity fixtures. Later stage 04 adds 60%/30% maxHP interaction tests instead of duplicating them here. Use existing text-width checks only if new text actually changes; do not rename the ability based on a hypothetical width failure.

For native-Stellar defense validation, assert neutral chart effectiveness for player-accessible attack types and separately preserve TYPE_DIVINE→Stellar at 2×. A direct Divine regression fixture does not make Divine player-accessible; retain its boss-only move/access restrictions and do not add it to legitimate player pools.

## 7. Completion criteria

Existing ability remains active with unchanged intended tuning; missing negative/overlay cases are proven. Growth and healing occur once per successful qualifying cast, survive supported reloads, never alter stages or permanent stats, and remain overflow-safe. Any finite history representation has a documented proof or a regression exposing the limitation; unresolved limitations are not hidden behind the existing passing tests.

## 8. Open questions

- **Confirmed:** 10% already exists; preserve it unless the user requests retuning. No tuning confirmation is needed merely to retain current behavior.
- **Implementation investigation:** long-horizon growth history and Transform/Power Trick semantics require the focused cases above. This is shared hardening, not permission to change intended battle behavior.
- **Optional:** Jirachi back/icon/Dex placeholders can remain; they do not block this plan.
