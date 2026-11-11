# Chapter 12 — Comparative engine architecture review

Status: **comparison complete; recommendations await approval, implementation not authorized**. Reviewed against the local `mypatch` worktree on 2026-11-10.

## 1. Purpose, authority and conclusion

Chapters [00–11](00_MASTER_PLAN.md) remain the authoritative existing design. This chapter compares their localized approach with alternatives that permit deeper engine changes. It neither replaces those chapters nor approves their outstanding gameplay defaults. This review changed no source, scripts, tests or assets, and ran no battle tests or builds.

**Recommendation: adopt several bounded shared engine interfaces, retain the current executor and encounter policies, and defer broad rewrites.** The original restriction did encourage some repeated cache, ownership and action-context handling. However, the plans already call for substantial shared work where necessary, particularly Stage07's independent ownership and Stage11's candidate/outcome helpers. Removing the restriction mostly strengthens those interfaces; it does not justify replacing the battle engine.

The most valuable changes are shared HP-outcome policy, ordinary/virtual candidate evaluation, a defined fresh-choice cache boundary, explicit actor ownership, transactional formation changes, and durable committed-action metadata. These address concrete source gaps. Generic simulation, arbitrary party sizes, a new scheduler and a universal effect framework have much weaker justification for this encounter.

### Preserved gameplay contract

All levels, stats, kits, items, sequence, bars, effects and tuning decisions in Chapters00–11 remain in force. In particular:

- One uninterrupted twelve-opponent Trainer battle; first six switch normally, conjured six retain their fixed order/no-living-switch policy. Player items, real-party replacement, per-KO EXP and final-only victory/rewards remain ordinary.
- Clear **only the opponent Tera-used flag once** at the team transition. Preserve player Tera, opponent Dynamax and every other trainer gimmick-used flag. Calyrex's Fire Tera stays **optional under ordinary AI**; preserve active Tera through Ice→Shadow, or preserve inactivity and remaining eligibility. Transformation never forces or renews it.
- Native Stellar TX is neutral defensively against player-accessible attack types. Preserve the existing **Divine→Stellar 2×** interaction and Divine's boss-exclusive, legitimately player-unavailable status. Native Stellar is not Stellar Terastallization.
- Smeargle still samples until 60 distinct positive, legal candidates or pool exhaustion and executes the winner through real slot0. There is no exhaustive best-of-all-moves policy or extra action.
- Castoria-summoned and solo TX remain different profiles. Preserve summoned 100→60→30% maxHP, original 10+ Reduction, saved-layer behavior and summon topology; solo TX stays barless with its different Wish list and persistent 1+ Reduction. Preserve Meteo's proposed standard spread execution and TX-only cooldown semantics, subject to their existing approvals.
- Calyrex and genuine Eternamax keep their required forms/stats/kits. Mega visuals add no Mega/Dynamax mechanics. Independent AI Zacian is mandatory, occupies no real player slot, and leaves before Dialga. Its tuning and anti-Eternamax damage proposal still require their existing approvals.
- Preserve Dialga's existing recharge/Time Stop timeline. Arceus retains Multitype and the exact **`Alter Ego's Bug Space`** presentation, opening protection, clock dismissal with continued crushing, saved HP and one seal adaptation without healing.
- The newly observed all-living-holder Wishful Thinking dispatch remains unchanged by this review. Caster-only versus all-holder benefit is the open scope decision recorded in Chapters02/11, not an architectural choice to resolve silently.

Unapproved defaults remain unapproved: boss bar totals, Zacian tuning/damage qualification, Meteo success timing, Arceus opening/fallback details, AI flag changes and other Chapter10 release decisions. Optional Aurora Beam remains recommended for omission.

## 2. Evidence and limits

All twelve existing chapters were read. Relevant current source was inspected through semantic search and exact symbol reads. The worktree already contains implementation and documentation edits from earlier work; this review does not revert or adopt them as new approvals. An older plan's implementation snapshot is not current-code evidence.

### Engine seams actually present

| Area | Current source evidence | Architectural implication |
|---|---|---|
| Damage estimates | [battle_ai_util.c](../../../../src/battle_ai_util.c): `AI_CalcDamage` resolves dynamic type/category, constructs `DamageContext`, uses shared type/damage calculations and temporarily simulates gimmicks | Damage arithmetic is already shared. The missing layer is the consequence of the resulting damage, not a second damage formula. Simulation still touches globals, so a new API cannot simply declare it pure. |
| HP and layers | [battle_script_commands.c](../../../../src/battle_script_commands.c): `Cmd_healthbarupdate`, `MoveDamageDataHpUpdate`, `PassiveDataHpUpdate`, `Cmd_tryfaintmon`; [battle_break_bar.c](../../../../src/battle_break_bar.c): `QueueBreakBar`, `ClampBreakBarPassiveDamage`, `TryBreakBar`, `GrantSavedBreakBar` | Preview, attack HP, passive HP, instant death and direct-zero writes have different paths. Saved layers and original phases already have a useful shared resolver. |
| AI selection | [battle_ai_main.c](../../../../src/battle_ai_main.c): `ComputeBattlerDecisions`, `BattleAI_ChooseMoveIndex`, `CalcBattlerAiMovesData`, `ChooseArcanumForTarget`, singles/doubles selectors | Ordinary caches are real-slot indexed; virtual actions use the dynamic callback without ordinary scoring passes. Singles temporarily narrows signed scores to `u8`. |
| Cache/state | [battle.h](../../../../include/battle.h): `AiLogicData`, `AiThinkingStruct`, `AiBattleData`; `SetAiLogicDataForTurn` rebuilds at normal turn boundaries | Fresh-choice refresh can be made explicit without replacing all caches with a dependency graph. |
| Action identity | `gChosenMoveByBattler`, chosen positions, targets and AI choices already exist; `ClearWishSelection`/`CommitWishSelection` handle additional Wish metadata | There is existing move identity to preserve. The gap is coherent lifetime and ownership of all related metadata across transitions, not total absence of committed moves. |
| Ownership | `GetBattlerMon`, `GetBattlerParty`, `GetBattlerPartyState` map side/index to the real parties; `CopyBattlerDataToAIParty` writes side/index history | Battler position, battle side and persistent owner are conflated. Redirecting only one accessor cannot isolate Zacian. |
| Controllers/switching | [battle_controllers.c](../../../../src/battle_controllers.c) common handlers use party accessors and six-bit masks; [battle_controller_player_partner.c](../../../../src/battle_controller_player_partner.c) calls `AI_TrySwitchOrUseItem` and directly checks `gPlayerParty` for replacement | A partner controller is reusable machinery, not independent ownership or a moves-only permission model. |
| Gimmicks | [battle_gimmick.c](../../../../src/battle_gimmick.c): active state is side/party indexed; trainer use can be shared with a same-trainer partner | Actor storage and trainer resource ownership need separate answers. Zacian must not inherit the player's active gimmick or resource owner. |
| Live topology | [battle_tx_rollback.c](../../../../src/battle_tx_rollback.c): `BS_TxRollbackCreate` initializes two new positions, adds DOUBLE, copies AI flags and preserves old choices; no return-to-singles implementation | A proven expansion seam exists. A reversible shared transition boundary does not yet exist. |
| Stats/abilities | [battle_reduction.c](../../../../src/battle_reduction.c) mutation/normalization helpers; `CopyMonLevelAndBaseStatsToBattleMon`, Wishful growth and `TryBattleFormChange` in [battle_util.c](../../../../src/battle_util.c) | Canonical/effective stat separation already exists in pieces. Ordinary form change writes party species; it is not automatically suitable for battle-only boss overrides. |
| Encounter progress | [battle_koanko.c](../../../../src/battle_koanko.c) already resets opponent Tera once and adds sequence switching; `KoankoOnSwitchIn` reapplies No Order Change | Preserve these changes and validate them. The older Stage01 snapshot does not justify rebuilding the transition. |
| Bug Space | [battle_end_turn.c](../../../../src/battle_end_turn.c) combines decay/crushing; threshold0 returns before crushing. [kazuradrop_battle_ui.c](../../../../src/kazuradrop_battle_ui.c) reshow uses `bugSpace.active` | Explicit behavior/presentation modes are needed; magic threshold values cannot express the intended final phase. |

Current authored Koanko AI flags omit CHECK_VIABILITY and SMART_TERA. `BattlerHasAi` does not recognize DOUBLE-only independent player-right AI. `CanIndexMoveFaintTarget`/`CanEndureHit` do not model Break Bars or the planned opening floor. These are inspected gaps, not claims that a newly run test failed. Existing TX injected-score tests establish plumbing, not production tactical quality. Historical passing logs and the existing map file do not establish acceptance or current resource headroom.

## 3. Comparison matrix

**Adopt** means recommend a specific alternative for approval; **retain** means the original is already preferable; **defer** applies to the broader option described. Effort is relative implementation/review effort, not an estimate of elapsed days. Risk describes exposure before the proposed controls.

| ID / decision | Existing Chapters00–11 approach | Engine-integrated alternative | Benefit / cost / regression exposure | Recommendation |
|---|---|---|---|---|
| A1 Damage consequences | Small AI outcome helper plus owned HP clamps | Shared read-only HP/layer policy used by execution adapters and AI projection | Removes conflicting KO/floor definitions; medium-high effort, high HP-path exposure | **Adopt bounded policy**, retain damage formula/resolver |
| A2 Candidate scoring | Factor slot0 candidate scoring; add virtual damage support and profile scoring | Explicit candidate descriptor with shared ordinary evaluator and scratch cache | One legality/context contract; medium-high effort, high ordinary AI exposure | **Adopt incrementally**, retain legacy adapters |
| A3 Cache freshness | Refresh listed state at each next fresh choice | Central decision boundary with revision/occupant-generation validation | Makes invalidation reviewable; medium effort, medium-high AI exposure | **Adopt coarse boundary**, defer incremental dependency graph |
| A4 Independent ownership | Temporary six-entry backing party, routed accessors/history | Small explicit owner binding and action permissions used by those accessors | Avoids side/index aliases across all actor state; high effort/exposure | **Adopt formal owner contract**, retain compatible backing storage |
| A5 Topology transitions | Generalize slot policy, implement safe Zacian entry/departure locally | Shared preflight/commit/cleanup formation service with encounter requests | One checklist for 2→4→2; high effort/exposure | **Adopt bounded service**, no arbitrary formations |
| A6 Commitment/context | Preserve move identity, save event globals, commit Wish result | Per-action committed metadata and bounded nested event context | Prevents slot reinterpretation and stale outcomes; medium-high effort/exposure | **Adopt bridge**, defer scheduler/global rewrite |
| A7 Effective forms/stats | Narrow form override, growth reapply, separate TX/Reduction normalization | Shared ordered rebuild and canonical writeback seam | Reduces inconsistent reload hooks; high effort, high stat/controller exposure | **Adopt bounded seam**, defer universal modifier system |
| A8 Profiles/dispatch | TX owner/profile split and encounter dynamic callback | Capability-driven candidate providers beneath compatible encounter dispatch | Reduces slot/species coupling; medium effort, medium exposure | **Adopt small provider interface**, retain local effect execution |
| A9 Bars/phase execution | Reuse original/saved resolver with targeted saved continuation | General phase graph or event bus | More expressiveness, substantial ordering/stack cost without present need | **Retain original**; defer general graph |
| A10 Abilities/cooldowns | Existing ability event; owner-local cooldown/turn hooks | Universal effect/timer registry | Generality adds scope and dispatch overhead; little demonstrated reuse | **Retain original**, use shared success context from A6 |
| A11 Encounter/gimmicks/switching | In-place team replacement; ordinary gimmicks and narrow No Order Change | Generic wave framework, synthetic trainer resources, new switch engine | More abstraction risks unintended resets and changed ordinary rules | **Retain original**, route ownership through A4 |
| A12 Bug Space | Explicit Koanko mode, extracted crushing, persistent UI visibility | Generic field-effect/component framework | Existing mode fixes exact coupling; broader rewrite has high Kazura exposure | **Retain original** |
| A13 Presentation/Time Stop | Returning composed animations; existing volatile/recharge; narrow palette mask | Animation graph or global turn-status redesign | Much larger change than missing behavior requires | **Retain original** |
| A14 Tactical policy/search | Ordinary flags, bounded sampler, narrow tested utility adapters | New global planner/full simulator/thirteen bespoke scorers | Potential strength gains, major latency, determinism and maintenance costs | **Retain original**; defer broader AI project |

## 4. Detailed comparisons

### A1. Shared damage and HP-outcome estimation

**Existing approach.** Stage11 proposes a small AI query distinguishing ordinary damage, true KO, original-layer break, saved reveal and floor protection. Stage09 adds owned clamps across HP paths and a faint backstop. This already rejects a second phase engine, but separately implementing these rules in AI and execution would still permit drift.

**Integrated alternative and impact.** Share a read-only policy over current owner/layer/protection state, requested HP loss and source kind. Execution adapters apply its allowed HP result and classify the event; AI projects through the same policy without mutating bars or executing scripts. Affect `DamageContext` consumers, `AI_CalcDamage` callers/KO utilities, HP preview/commit commands and `battle_break_bar.c` queries. Dynamic type and delayed-attack provenance travel with the damage event; residual/typeless events explicitly have no attack type. Keep Substitute, Sash/Sturdy and ordinary prevention ordering intact rather than hiding every defense inside a new clamp.

**Advantages/cost.** One definition of reachable current-layer damage supports both intelligent KO bonuses and Arceus survival. Original versus saved layer identity remains owned by the existing bar record. Medium-high implementation cost comes from enumerating every HP path, including instant-death and direct-zero writes; a helper called only from the normal damage formula is insufficient. The policy should return facts, not tactical weights or script pointers, which keeps maintenance divided sensibly.

**Risk and recommendation — adopt bounded policy.** HP ordering affects ordinary battles globally. Default unowned behavior must match existing arithmetic/side effects; route adapters in small reviewed steps. Do not replace `TryBreakBar`, refill/restore scripts or the shared damage calculator. For multihit AI, a summed damage range cannot establish exact hit-by-hit phase behavior: use available strike information or an explicitly conservative estimate. Never propagate overkill into covered HP or claim a guarantee from an aggregate roll. Exact future phase simulation is deferred.

### A2. A candidate is not a real move-array slot

**Existing approach.** Stages03/05 factor ordinary single-candidate scoring, refresh scratch slot0 for sampled moves, and provide virtual Meteo damage support. Stage11 requires parity and signed comparisons. However, current `ChooseArcanumForTarget` calls only `RunDynamicAiFunc`, while ordinary scoring depends on four-slot caches and comparison passes.

**Integrated alternative and impact.** Give the evaluator an explicit candidate description: actor/owner, move ID, origin (real, sampled or virtual), logical action ID, target policy, legality/resource state and optional Wish outcome/recipient. Share damage, accuracy, effective type, relevant ordinary scoring and final signed comparison. Refactor `CalcBattlerAiMovesData`, slot-based AI utilities, `BattleAI_DoAIProcessing`, `AI_CompareDamagingMoves` integration and `ChooseArcanumForTarget`; introduce a small candidate scratch cache beside `AiLogicData`. Legacy four-slot actions use an adapter. Execution still commits a real slot0 sampled move or an existing virtual action through the ordinary executor.

**Advantages/cost.** Avoids treating logical slot4/5 as an array index and makes new offensive virtual moves comparable without duplicating normal scoring. Cost is substantial: many heuristics inspect sibling moves, PP or cached slot values. Their dependencies must be audited; simply replacing one slot accessor is not parity. For ordinary actors preserve their real comparison set; Smeargle retains the planned one-slot context and streaming best-of-accepted selection. Wish outcomes remain utility candidates with exact effect eligibility, not fake damaging moves.

**Risk and recommendation — adopt incrementally.** Ordinary score ordering and RNG consumption can change. Preserve enabled flags, score baseline, utility increments, ordinary-versus-virtual ties and Smeargle uniform best ties. Fix the existing `u8` temporary score narrowing with signed comparisons rather than capping bonuses. Start with shared context and scratch evaluation, then route virtual attacks; do not rewrite all heuristics at once or increase `MAX_MON_MOVES`. Validate a candidate against an equivalent ordinary action and actual execution, not just a copied test scorer.

### A3. Central fresh-choice invalidation instead of scattered cache repair

**Existing approach.** Stage11 specifies a comprehensive next-fresh-choice refresh after forms, growth, Reduction, items, bars and topology. Stage03 requests per-candidate refresh. The list is correct, but localized callbacks can forget a dependent speed, type, gimmick or target cache.

**Integrated alternative and impact.** Make a shared decision-entry boundary validate a coarse battle/decision revision plus owner and occupancy generation. A changed effective combat state invalidates the affected decision snapshot; topology changes invalidate membership and all active-pair data. Reuse `SetAiLogicDataForTurn`/`SetBattlerAiData`/move-cache builders and instrument mutations at their authoritative sites. Within one stable decision, recompute only each sampled/virtual candidate's scratch facts. Preserve AI knowledge policy and prediction state; invalidation is not permission to expose information unavailable under the actor's flags.

**Advantages/cost.** Centralizes the freshness contract and makes a stale cache detectable instead of merely plausible. A conservative full active-state refresh at a safe boundary is easier to maintain than a fine dependency graph. Its cost is extra work when changes occur, particularly with doubles and gimmick comparisons; redundant per-candidate full rebuilds would be unacceptable. A single epoch is an invalidation signal, not proof that every mutator has been covered.

**Risk and recommendation — adopt coarse boundary; defer dependency graph.** Changes to `ComputeBattlerDecisions`, turn setup and cache builders can alter predictions/RNG or repeatedly clear data. Rebuild only for a fresh decision, retain actual committed commands, and keep invalidation separate from selection. Include party reuse, HP/status/item changes and menu reconstruction that changes effective state. Compare post-mutation cache values with a fresh calculation. Do not introduce a persistent per-move/per-field dependency system until measurements show that the simpler boundary is too costly.

### A4. Independent AI ownership, side and permissions

**Existing approach.** Stage07 already requires owner-aware accessors, isolated PartyState/AI history/gimmicks and a six-entry temporary backing party. This is necessary engine work, not a purely local workaround. The danger is implementing each exception through a separate `isKoankoZacian` branch with slightly different identity semantics.

**Integrated alternative and impact.** Formalize a small battle actor binding: combat side, position, backing owner/local index, occupancy generation, controller/AI policy, trainer-resource owner and permitted actions. Default bindings map to today's real parties. Route `GetBattlerMon/Party/PartyState`, active AI party/history access, item ownership and gimmick getters through it where the data is actor-owned. Retain real-side queries for weather/screens and `GetSideParty` for actual party menus. `BattlerHasAi`, `AI_TrySwitchOrUseItem`, partner ChoosePokemon, EXP/revival/loss paths and trainer-use sharing must consult the appropriate ownership/permission dimension.

**Advantages/cost.** One identity contract prevents aliases even when Zacian local index0 and player local index0 coexist. Moves-only permission closes switch, item and gimmick override paths, not just selection. This is the highest audit burden: direct array references remain across controllers, faint processing and history. A formal owner binding does not automatically make every ordinary multi-trainer path generic.

**Risk and recommendation — adopt explicit binding, retain compatible storage.** Use defaults preserving ordinary semantics and one opt-in auxiliary owner; do not change PARTY_SIZE, active battler limits, save data or link protocols. Keep Stage07's six-entry backing allocation initially: existing mask loops require a compatible party view. A one-mon allocation is safe only after all consumers use a verified count/mask API, an independent migration with little immediate payoff. Current common GetMonData response storage only accommodates limited full-mon transfers; six addressable entries do not authorize sending six full records in one response.

Player loss still counts the real six even if Zacian lives. Side membership must not imply player inventory/EXP/gimmick ownership. Generation checks reject stale actor-owned work after retirement or party replacement. Owner-local storage is also necessary for Reduction's ally baseline, not merely AI history.

### A5. Transactional 2→4→2 formation changes

**Existing approach.** Stage07 calls for generalized encounter slot policy and controller-safe entry/collapse, using TX expansion as evidence. It correctly preserves the chosen queue and requires singles before Dialga; writing a separate initialization/cleanup checklist in each encounter would duplicate fragile engine state.

**Integrated alternative and impact.** A bounded formation service handles prepare, commit and retirement at explicit script/controller barriers. Preflight validates owner bindings, idle required controllers, stack/callback capacity and sprite/task/palette resources. Commit initializes positions, controllers, layout, absent/reserved policy, AI recognition/flags and cache membership together. Retirement excludes future actions/targets, resolves necessary effects, normalizes cursor state, releases resources and restores the intended formation. Encounter modules request policies and timing; the service does not choose reinforcements or battle phases.

Affected seams include `BS_TxRollbackCreate`, Koanko ally scripts, `battle_main.c` action/end-turn cursors, controller initialization, reshow/layout modules, absent-slot queries and `AssignUsableGimmicks`. Keep original left battler IDs stable. DOUBLE versus MULTI/INGAME_PARTNER remains an explicit policy; neither latter flag substitutes for ownership.

**Advantages/cost.** A single invariant checklist prevents half-initialized positions and inconsistent doubles targeting. Cost is high because shrinking live iteration bounds is fundamentally more dangerous than allocating two absent slots. Ordinary doubles, partner battles and Castoria reshow are adjacent regression surfaces. Preparation can release resources on failure before exposure; after an entry ability or script has observable effects, arbitrary rollback is unsafe, so use explicit idempotent completion/cleanup states.

**Recommendation — adopt bounded service.** Implement only existing supported layouts with compatible TX adapters; migrate TX mutation only behind equivalent-behavior tests. Do not convert the whole engine to arbitrary formation graphs. No new actor gets an attack in a previously committed queue, and no cache refresh may reselect retained actors. Resource failure cannot silently omit mandatory Zacian and call the encounter complete; recover/retry at a safe boundary or record a mandatory implementation blocker. Departure, including already-fainted/simultaneous cases, completes before Dialga send-out.

### A6. Commitment and nested event context

**Existing approach.** Stages06/10 preserve committed move identity and returning-script globals; Stage05 commits Wish metadata only for the winning candidate. TX ambush already saves a large subset of globals and explicitly resumes pending bars/faints. That works for one event but repeated bespoke save lists are difficult to audit when forms, spread hits and saved-layer callbacks nest.

**Integrated alternative and impact.** Maintain a compact per-action commitment record linking the existing chosen move ID, source/slot, actor owner/generation, target policy, selected outcome/recipient, gimmick intent and an action sequence identity. A bounded event-context bridge saves/restores only declared transient execution fields around returning events. Continue feeding existing script globals/controller responses; do not replace the interpreter. Touch AI commitment, opponent/partner ChooseMove, action loading in `battle_main.c`, virtual execution validation and TX/Koanko returning events.

**Advantages/cost.** Slot rewrites cannot reinterpret an already-chosen move; stale Wish recipients cannot attach to another owner. A once-per-action success summary can arm Meteo cooldown after the target loop, independently of visual impacts. Arceus captures the breaking hit's resolved type at HP commit rather than reading overwritten globals later. Cost is medium-high: the bridge must cover callback/cursor and source semantics, not just attacker/target. Large full-BattleStruct copies would be wasteful and still miss external globals.

**Risk and recommendation — adopt bridge; defer scheduler rewrite.** Move callers may replace executing submoves while retaining the originating virtual action/cost. Multi-turn continuations keep ordinary engine rules. Actor-owned recipients use generation checks; position-based Wish/delayed attacks retain their position semantics and are not silently converted to owner-following effects. A retired positional target expires as specified. Post-commit Disable, faint or target change resolves through ordinary failure/redirection/cancel rules, not a replacement candidate. Keep irreversible effects and costs at their existing authoritative execution points; do not move all resource consumption to selection. A full command bus or elimination of global script state is a separate, much larger project.

### A7. Shared effective-form/stat rebuild seam

**Existing approach.** Stages02/04/06 order clean form stats → Wishful non-HP growth → owned TX maxHP overlay → Reduction. They propose scratch form calculation, battle-only overrides and separate controller normalization. Existing Reduction already brackets mutations and preserves HP fractions. Ordinary `TryBattleFormChange` instead changes the party mon and recalculates it.

**Integrated alternative and impact.** Centralize a small effective-battler rebuild and canonical-writeback seam used by clean stat loads, owned form refresh and controller reconstruction. Encounter state supplies effective species/ability/type/kit policy and owned maxHP; existing growth and Reduction hooks compose in their defined order. Touch `CopyMonLevelAndBaseStatsToBattleMon`, `CopyMonAbilityAndTypesToBattleMon`, `RecalcBattlerStats`, controller normalization, owner-aware graphics queries and dynamic Judgment typing. Track kit initialization separately so rebuilding does not restore spent PP.

**Advantages/cost.** One ordering contract avoids multiplying growth or losing an override on a menu return. AI receives the same effective stats/types as execution. Maintenance improves if the seam replaces duplicate reload work, not if a new pipeline runs alongside the old hooks. Cost and regression exposure are high: Transform/Polymorph, Power Trick, Dynamax HP, ordinary forms, healing and saved-layer canonical HP have existing semantics to preserve.

**Recommendation — adopt bounded seam, defer universal modifier system.** Keep the proven Reduction normalization/mutation model and use an opt-in effective form provider rather than inventing a complete modifier graph. Preserve legal copied stats and mutation behavior; a blanket canonical rebuild after every change would erase them. Do not recompute expensive mon stats for each AI candidate. Calyrex's form change and Tera state remain independent, Eternamax receives no gimmick multiplier, and Arceus adaptation preserves exact restored HP. Original-bar refill is an explicit event, not a side effect of rebuilding. Count saturation/rounding issues in Wishful growth remain Stage02 correctness questions, not permission to change growth magnitude or limits.

### A8. Encounter profiles and candidate dispatch

**Existing approach.** Stage04 separates TX combat ownership/profile from summon lifecycle; Stage05 registers an encounter dispatcher instead of overwriting the singleton dynamic hook on entry. This is already preferable to species-only activation. A deeper alternative would replace all dynamic AI with a generic plugin/effect registry.

**Integrated alternative and impact.** Introduce a small static candidate-provider interface used by A2: enumerate this owner's additional action descriptors, check availability and evaluate profile utility. Preserve the existing encounter-level dynamic callback as an adapter for Castoria/Razeluxe and ordinary dynamic trainers. Move TX slot count, move-specific legality and profile outcome lists behind consistent queries used by `GetBattlerMoveSlotCount`, `GetBattlerMoveAtSlot`, `IsArcanumAction`, `IsArcanumSlotUsable` and virtual selection. Execution remains `BS_TxRollbackTryWish` and existing spell scripts.

**Advantages/cost.** Supports solo opponent-left and summoned opponent-right without hardcoded Reciprocity targets or a second global hook. Logical action metadata is shared between selection and execution; profile-specific utility remains local. Medium effort and medium regression exposure arise from preserving existing Castoria route/Command Seal behavior and indirect move exclusions.

**Recommendation — adopt small provider, retain encounter dispatch/effects.** Static tables and a few explicit adapters suit the bounded set of profiles. Runtime plugin registration, arbitrary callback chains and a global event bus add indirection and cleanup obligations without a demonstrated consumer. Do not grant encounter capabilities to every matching species, reinterpret every TX virtual slot as Wish, or preserve the current INT_MAX-style scripted priority as generic tactical valuation.

## 5. Original design is already preferable

The following are affirmative retention decisions, not postponed mandatory work. Each includes the broader alternative considered.

### A9. Existing Break Bar resolver over a new phase graph

Stages04/06/09 configure original callbacks and one targeted tagged saved-layer continuation. `TryBreakBar` already distinguishes saved layers, preflights script/target stacks, decrements once and orders refill/restore before continuation. A generic phase graph would affect `BreakBarConfig`, `PartyState`, stack dispatch and every original/saved-bar consumer. It could describe future complex bosses, but would duplicate established ordering and expand debugging/state space now. **Retain the existing resolver**, adding an explicit tagged saved-resolution hook and its stack preflight. High regression exposure to Break Charm, Endurance and Kazura outweighs speculative reuse. A1 supplies facts; it must not become another bar executor.

### A10. Existing ability dispatch and local owner timers

Stage02 uses `ABILITYEFFECT_WISH`, success scripts and battle-only growth; Stage05 keeps cooldown in TX combat-owner state with turn-boundary consumption. A universal effect/timer registry would alter `AbilityBattleEffects`, move-end and end-turn dispatch, adding duration clocks, event subscriptions and cleanup for all mechanics. It offers uniform metadata but has medium-high implementation/maintenance cost and broad ability-order regressions. **Retain current dispatch and local timer state**, fed by A6's one-per-action success facts and A4 identity. Explicitly distinguish successful Wish, delayed Wish healing and Meteo. Determine caster-only/all-holder policy separately; architectural consolidation cannot settle it. Cooldown's next-active-turn and absence rules remain the Stage05 approval, not a new generic duration convention.

### A11. In-place encounter transition and ordinary gimmick/switch engines

Stage01's returning party transition preserves the same battle, field, player and trainer resources; sequence switching and No Order Change serve different purposes. A generic wave engine or synthetic second trainer would touch `ConjureKoankoTeam`, battle outcome/rewards, AI setup, trainer inventory and gimmick ownership. It might ease many future waves but is expensive and especially prone to resetting too much. **Retain the in-place transition and current shared switching/gimmick APIs.** Validate existing opponent-only Tera renewal instead of replacing it. Route auxiliary ownership through A4, and keep any approved anti-Eternamax predicate shared by damage/AI without fabricating Dynamax state. First-six switching, G-Max Alcremie, phazing damage and genuine faint replacements remain ordinary. No new global Tera policy, trainer stock or forced Calyrex activation is justified.

### A12. Explicit Bug Space mode over a field-effect framework

Stage09 separates crushing from threshold decay, Doom consequences and panel visibility while preserving Kazuradrop's mode. A field-effect component registry could represent each behavior independently, but would migrate `BugSpaceData`, ability initialization, end-turn dispatch, move blocking, item/faint/legend gates and UI reshow. That is high regression exposure for a single reused mechanic. **Retain explicit modes/behavior gates and persistent visibility**, with a reusable crushing helper. Threshold0 currently exits before crushing; 254 still blocks some power, so sentinel-only shortcuts fail. The fake `Alter Ego's Bug Space` label uses a temporary presentation override with actual Multitype intact. Keep Kazura's text/flags/game-over authority isolated. The mode must gate every known gameplay consumer, not only the panel or end-turn routine.

### A13. Composed visuals and existing Time Stop/recharge

Stages05/06/09 compose returning animation fragments; Stage08 validates the existing Time Stop volatile and optionally extends a palette exemption mask. A generalized animation graph or global action-pause system would touch animation tasks, palette/BG state, scheduler cancelers and recharge across ordinary battles. It could improve future tooling but imposes high integration/testing cost without fixing an absent required mechanic. **Retain the original composition and narrow palette work.** Parameterize effective-form graphics loading where needed, preflight resource use and restore animation context. Mega animation is visual only; Meteo's visual impacts do not generate extra hits. Existing Roar of Eternity timing and mandatory replacement authority stay unchanged. The optional trainer Aurora Beam still lacks an approved damage source; deeper ownership APIs do not supply one automatically.

### A14. Ordinary tactical AI and bounded search

Stages03/05/11 reuse ordinary scoring, add narrow missing utility and require production-profile scenarios. A full lookahead simulator, exhaustive move ranking or thirteen specialized scorers would require new state cloning, script-effect models, RNG/hidden-information rules and long-term maintenance across `battle_ai_*` and execution changes. Potential tactical gains come with high GBA latency and large parity/regression costs. **Retain ordinary AI plus tested adapters and bounded sampling.** A2 is an evaluator integration, not a new planning algorithm. Adopt no automatic first-team flag changes: CHECK_VIABILITY/SMART_TERA remain proposals. Generic Dynamax scoring defects that affect required Alcremie behavior still need focused fixes; broader architecture does not excuse them. Time Stop value must include applicability/probability and avoid double-penalizing recharge. Production choices and actual effects, rather than injected Wish scores or a fixed rotation, remain the acceptance standard.

## 6. GBA resource and performance implications

### Verified limits versus unmeasured capacity

The local [linker script](../../../../ld_script_modern.ld) defines 256 KiB EWRAM, 32 KiB IWRAM and a 32 MiB ROM region. [sprite.h](../../../../include/sprite.h) defines 64 software sprites; [task.h](../../../../include/task.h) defines 16 tasks. [gba/defines.h](../../../../include/gba/defines.h) defines 96 KiB VRAM and separate 512-byte BG/OBJ palette areas; a 4bpp palette uses 16 colors, giving 16 banks in each area. These are limits, not available headroom. TX's existing free-sprite check is not proof that a different animation/ally resource set fits.

`AllocateBattleResources` in [battle_util2.c](../../../../src/battle_util2.c) heap-allocates battle and AI structures and animation buffers. Enlarging a heap-owned struct still consumes shared EWRAM even if the map lists only its pointer. The existing `pokeemerald.map` predates this review and cannot demonstrate current worst-case heap, stack or animation capacity. No build, allocation instrumentation or timing measurements were performed here; this document makes no claim that the proposed additions already fit.

### Bounded storage choices

- Keep four active battler IDs and the two real six-member parties. A4 adds one auxiliary owner binding/backing party, not a global third six-party matrix for every subsystem. Account separately for its PartyState, AI knowledge and resource permissions.
- Keep normal four-slot caches. The current damage-range payload alone is 4×4×4×6 = **384 bytes** because `SimulatedDamage` contains three `u16`s; this excludes accuracy/effectiveness and all other state. Doubling that grid's move dimension would double its payload everywhere, while virtual candidate scratch only needs the current candidate/targets. A full all-moves grid scales far worse and is unnecessary.
- Smeargle sampling may need a bounded writable ID permutation/list as planned; it does not need 60 full battle snapshots or cached damage for the entire move database. Keep the best candidate, score and tie count while streaming. Pool size, element width and worst-case exhaustion must be measured from the implemented pool.
- A3 can use a small revision/generation record and existing active caches. A6 stores compact committed metadata and a bounded event-context stack; do not put whole AI/BattleStruct copies on the CPU stack. Include the existing eight-entry battle script stack and saved-target capacity in nested-event preflight.
- Store static profile/action descriptors and type→form mappings in ROM. Avoid runtime allocation per candidate and deep callback recursion. Allocate optional auxiliary storage at a clear lifecycle boundary and release it idempotently; mandatory feature allocation failure must have an explicit recovery policy.

### Work budget and measurement gate

Full active-state rebuilding once at a safe changed decision boundary is a reasonable starting point. Rebuilding all pairwise caches for every sampled move would multiply cost by draws, including rejected candidates; worst case is **pool exhaustion**, not merely 60 accepted moves. Compute candidate facts on demand, reuse the stable actor/field snapshot and never repeat a full scoring pass just to commit the winner.

Use existing AI delay/cycle instrumentation as a starting point during implementation. Measure representative ordinary singles/doubles, Smeargle exhaustion, virtual outcomes with two targets, smart Tera comparisons, overlapping bar events and formation entry/departure. Record hardware/emulator configuration and worst observed command-selection delay, heap high-water/free block, stack depth, sprites/tasks/palette use and ROM growth. Set an explicit acceptable latency/headroom budget before accepting a wider rollout; this review does not invent a millisecond threshold. Prefer integer/fixed-point calculations consistent with existing code. Optimize only measured bottlenecks, preserving signed score and damage semantics.

## 7. Scope, regression controls and acceptance evidence

Architectural location and behavioral scope are different. A shared function can retain a Koanko-only rule; a local workaround can still corrupt ordinary state through a shared global.

| Change class | Intended behavior scope | Necessary adjacent controls |
|---|---|---|
| A1 outcome policy | Owned floor/phase policy is opt-in; existing ordinary damage remains equivalent. Layer-aware AI may improve decisions in any battle with layers if explicitly rolled out. | Ordinary direct/passive/instant HP, Substitute, Sash/Sturdy, multihit, saved/original bars, rewards and all Arceus HP sources |
| A2/A3 evaluator/cache | Koanko candidates first, existing Castoria compatibility; wider ordinary scoring migration is separately reviewed. Signed comparison repair is a shared correctness change. | Ordinary singles/doubles ordering/ties, recovery/setup, dynamic types, Tera/Max, simulation restoration and post-mutation fresh-calculation parity |
| A4/A5 owner/formation | One opt-in auxiliary owner and supported live layouts; ordinary default mapping preserved | Real-party canaries, normal partners/multi/loss/revival, AI history/items/gimmicks, TX summon/reshow, simultaneous faint and following ordinary battles |
| A6 commitment | Existing action timing for everyone; additional metadata for special candidates/events | Move callers, recharge/continuations, mid-turn form/kit changes, actor versus positional targets, interrupted spread events and nested script return |
| A7 rebuild | Battle-only overrides opt-in; existing growth/Reduction correctness shared | Ordinary forms, Transform/Polymorph, Power Trick, controller healing/items, canonical saved HP and spent PP |
| A8–A14 local policies | Existing encounter behavior and approved mechanics only | Castoria, Kazuradrop, ordinary Smeargle/Arceus, existing Time Stop, switching and trainer resources |

Stage11's production-profile tactical gate and Stage10's uninterrupted integration acceptance remain required. Suggested architectural evidence supplements them; it does not introduce a new gameplay test substitute:

1. **Projection parity:** controlled real damage versus projected current-layer outcome, including saved reveal, protection and uncertain multihit cases. Assert no evaluation mutates HP, PP, bars, gimmick use, history, inventory or event state beyond explicitly restored scratch fields.
2. **Candidate parity:** equivalent ordinary and virtual/sample contexts, signed negative/zero/>255 ordering, exact conservation ties, legal fallback and no four-slot out-of-bounds access. Include ordinary comparison heuristics, not just a fake scorer.
3. **Freshness/commitment separation:** mutate form, growth, HP/maxHP, status, item, Reduction or topology; the next fresh choice matches a fresh calculation while slower committed actions preserve ordinary identity/timing. Generation reuse must reject stale auxiliary metadata.
4. **Formation/ownership isolation:** controller masks, own PP/status/PartyState/history and resource permissions; no player-right command prompt; no new actor in an old queue; singles restored before Dialga in every faint order. Check low-resource paths and loss during transitions.
5. **Compatibility:** unchanged ordinary/default paths plus Castoria/Kazura controls and real production Koanko flags. Existing generic KNOWN_FAILING cases must be identified precisely and do not waive mandatory encounter behavior.

Implementation validation must record the revision, dirty-worktree context, exact focused filter, nonzero selected count, outcomes and emulator/resource evidence. No fresh passing result is asserted by this documentation review.

## 8. Recommended architectural changes and implementation order

This is a proposed refinement map, not an edit to the earlier chapters or permission to implement.

1. **Agree interfaces before feature-specific copies accumulate.** Define A4 owner/occupancy identity and A6 commitment/target semantics on paper; define A1 outcome facts and A2 candidate inputs/results. Keep encounter policy outside shared arithmetic/execution.
2. **Build shared evaluation foundations in reviewable slices.** A1 read-only outcome policy and A2 signed/candidate context first; then A3 freshness boundary. Preserve legacy adapters and demonstrate ordinary parity before routing sampled/virtual candidates. Stage03 sampling and Stages04–05 tactical work consume these helpers.
3. **Centralize owned effective data.** A7 rebuild/writeback seam consumes A4 identity and preserves Stage02 ordering; Stages04/06/09 supply profile/form policy. Avoid independently adding several reload overrides before this seam is reviewed.
4. **Finish independent actor support before live formation.** A4 accessor/controller/AI/resource isolation is the first high-risk milestone. Only then enable A5 safe entry/retirement and Stage07's mandatory ally. Keep existing TX topology behavior behind adapters until migration proves parity.
5. **Integrate commitment with scripted events.** A6 uses bounded returning contexts, execution revalidation and action-success facts for Stage05 cooldown and Stage09 breaking type. Audit combined bar/Wishful/Reciprocity/form/ally nesting against actual stack capacity.
6. **Retain A9–A14 and complete existing acceptance gates.** Keep ordinary engine dispatch, bar resolution, Time Stop and composed presentation. Complete feature tests → Stage11 production AI gate → Stage10 final encounter acceptance. Wider ordinary-trainer adoption or dependency-graph/scheduler work remains a separate decision.

A1/A2/A3 are shared AI correctness work; A4/A5 are the largest engine integration; A6/A7 prevent repeated local state bridges. A8 is a small support interface, not a prerequisite for a global plugin system. This ordering does not create a dependency cycle between Chapters10/11 or postpone all feature validation to the end.

## 9. Prioritized decisions requiring approval

### Priority 0 — Architectural scope before remaining implementation

1. **Approve bounded shared integration (recommended):** A1 outcome policy, A2 candidate evaluator, A3 fresh-choice boundary, A4 explicit ownership, A5 supported formation service, A6 commitment/context bridge and A7 effective-data seam; A8 remains a small compatible provider. The alternative is to retain the original localized helpers, accepting more manual synchronization and a larger per-feature audit burden. Approval should identify interfaces and migration boundaries, not authorize an unspecified engine rewrite.
2. **Approve ownership/formation boundaries (recommended):** one opt-in auxiliary owner, existing four battler IDs, six-entry compatible temporary storage, no party/save/link size changes, moves-only Zacian permissions, real-party loss authority and safe departure before Dialga. This formalizes Stage07's necessary integration rather than making reinforcement optional.
3. **Approve staged ordinary-path migration (recommended):** shared APIs retain legacy defaults/adapters; ordinary behavior changes require demonstrated correctness evidence and focused controls. Do not globally enable new AI flags or revised tactical weights as a side effect of refactoring.

### Priority 1 — Existing gameplay/tactical approvals remain separate

4. **Resolve Wishful Thinking trigger scope:** caster-only versus current all-living-effective-holder dispatch. Preserve current code during this review; update eventual scoring and tests only to the approved behavior. This is a gameplay footprint decision with direct utility consequences.
5. **Resolve inherited tuning/default choices:** CHECK_VIABILITY/SMART_TERA proposal; Calyrex/Eternatus bar totals; Zacian level/build and anti-Eternamax qualification; Meteo success/next-active-turn semantics; Arceus full-turn/opening/fallback defaults; Grand Battle identity and the other Chapter10 release choices. None is automatically approved by accepting an architectural interface.

The one-time opponent-only Tera reset, optional Calyrex Fire Tera, exact Bug Space presentation and Divine→Stellar preservation are **confirmed constraints**, not questions to reopen.

### Priority 2 — Resource and broader-engine decisions

6. **Approve measurement gates before wider rollout:** explicit command latency and memory/resource headroom budgets, measured on the implementation revision. Select a conservative cache rebuild strategy first; optimize only if evidence requires it.
7. **Defer broader projects (recommended):** arbitrary formations/party counts, one-mon party-view migration, full action scheduler replacement, exact future-phase simulator, fine cache dependency graph, universal effect/modifier/animation registries and global AI lookahead. Revisit only with concrete additional consumers or a measured failure of the bounded approach.

## 10. Review completion record

The deliverable is this separate Chapter12 only. SHA-256 checks confirmed all twelve Chapters00–11 match their review-start contents; recommendations have not been backported. Local document links resolve, and text checks found no replacement characters or trailing whitespace. A separate integrity check of 24 inspected engine/header/linker files detected an outside edit to `battle_util.c` during the review. Its Wishful Thinking full-HP healing guard was re-read; the all-holder dispatch/scope question and architectural conclusions remain unchanged. This review did not edit that file or revert outside work.

Current engine evidence, alternative designs, relative cost, maintenance, GBA constraints, ordinary-versus-encounter scope and regression controls are recorded above. Planning/review completion does not constitute gameplay approval, implementation completion or a passing test/build result.
