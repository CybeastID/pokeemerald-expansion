# Stage 10 — Encounter integration, presentation and regression acceptance

Status: **fully planned**; integration follows all mandatory stages. Snapshot: `mypatch`, 2026-11-09.

Dependency update: 2026-11-10. [Stage11 AI behavior and tactical validation](11_koanko_ai_validation.md) is required before this stage's final acceptance, despite its later document number.

## 1. Objective and scope

Verify the complete intended twelve-mon battle, close cross-stage lifecycle gaps and produce implementation acceptance evidence. Preserve all mandatory mechanics, player item access and existing Castoria/Kazuradrop behavior. This stage is not a license to rebalance levels/stats or add unapproved phases. Optional Aurora Beam is assessed below and recommended for omission.

Apply later confirmed user decisions over conflicting handoff wording: reset only opponent Tera usage once at the team transition, and leave Calyrex Fire Tera optional under ordinary AI, preserving it through transformation only when active.

## 2. Existing code and evidence verified

- `test/battle/koanko.c` supplies the existing full twelve-faint spine and transition edge cases; `TEMPSCRIPTS/koanko-tests.log` records 9/9 historical passes. The test file is already updated to the corrected TX species/kit.
- `test/battle/tx_wishes.c`, `arcanum_more_wishes.c`, `arcanum_reduction.c`, `break_bar.c`, `break_bar_exit_hp.c`, `trainer_break_bars.c`, `ability/wishful_thinking.c` and `move_effect/roar_of_eternity.c` provide focused regression seams inspected in earlier stages. Existing logs are not proof that new features pass.
- `TEMPSCRIPTS/koanko-regression-1/2/3/4.log` record 44/13/15/103 historical passes in adjacent groups. They predate the latest worktree and cannot establish current complete superboss behavior.
- `test/battle/kazuradrop_stack_breaker.c` has a specific KNOWN_FAILING teardown exception: AnimTask_ShakePlatforms remains active after custom game-over. Do not conflate this with a new resource leak or all Kazuradrop failures.
- `data/battle_scripts_2.s` routes HP healing, Full Restore, status cure, PP restoration and revival through ordinary scripts/controller synchronization; revived send-outs call switch-in processing. Independent ally retirement and Reduction overlays must honor these routes.
- `battle_script_commands.c:BS_KazuradropFakeVictorySting` explicitly uses PlayBGM for trainer-victory music; PlayFanfare would select the wrong sound/fallback. `BS_KazuradropSendoutMusic` supplies the existing fade-to-boss-track pattern.
- `battle_tx_rollback.c:TxRollbackTryAmbush` demonstrates returning scripted extra events, saved execution context and pending-bar/faint resumption, but establishes a real TX attacker. It does not provide a trainer-owned damage stat source.
- Asset metadata read with Pillow: Arceus anim_front is indexed P 64×128, back P 64×64; Calyrex Ice/Shadow, Eternamax and Crowned Zacian front/back are indexed P 64×64. No new artwork is required for the planned form/mosaic/Meteo sequences. This verifies metadata/presence, not final appearance or GBA resource headroom.

## 3. Implementation design and order

1. Integrate feature work and focused tests from stages01–09, complete Stage11's cross-stage AI validation/hardening gate, then finish this stage's final encounter acceptance. Provisional integration fixtures may be developed earlier and reused by Stage11; completed Stage10 is not its prerequisite. At every stage retain a runnable focused fixture; do not replace the original transition engine or postpone all tests until the final encounter.
2. Add an end-to-end **Koanko Integration** fixture that traverses the ordinary party and exact conjured sequence, uses the real profiles/events rather than test-only substitutes, and verifies final-only outcome/rewards. Assert the opponent-only one-time Tera-used reset while player Tera and all other trainer gimmick-used flags stay unchanged. Add shorter pairwise fixtures at TX→Calyrex, Calyrex→Eternatus, Eternamax→Dialga and Dialga→Arceus to make failures independently diagnosable; cover Calyrex transforming with active Fire Tera and with Tera still inactive under ordinary AI choice.
3. Ensure all returning event scripts preserve attacker/target/chosen move/slot/action cursor and controller synchronization. Event flags commit before yield, cancel on genuine defeat, and never give replacement occupants their predecessor's actions. Audit combined script-stack depth across saved-bar restore, Wishful Thinking, Reciprocity, phase scripts, form visuals and ally entry/departure.
4. Exercise all allowed player items through ordinary execution: Max Revive on real reserves, Ether PP changes, Full Restore HP/status and held Break Charm saved layers. Never normalize away real recovery or let revivals populate a disabled/retired auxiliary slot. Koanko Reduction remains until final battle cleanup; each new occupant starts fresh accumulated loss.
5. Use a single cleanup ownership checklist: Koanko state/queued events; TX combat/summon state; temporary ally owner/controller/AI caches; form overrides; Arceus Bug Space/panel/floor/seal/type context; Break Bar palettes; Time Stop palettes; animation tasks/BG/blending. Cleanup must work on loss, final victory, simultaneous KOs and exit during a returning event.
6. Emulator-review an uninterrupted encounter using configured levels 100–105 and intended player items. Record fake-victory→reveal→confirmed Grand Battle, the specified TX/Arceus slides, Calyrex's white-flash form change both with AI-activated Fire Tera preserved and with Tera inactive, genuine Eternamax Mega visual and independent Zacian entrance/departure, Dialga recharge/Time Stop, and Arceus's exact `Alter Ego's Bug Space` presentation followed by clock dismissal/saved HP/seal adaptation. Calyrex entry/transformation must not force Tera. Visual impacts must not imply multiple Meteo damage events.
7. Perform resource checks at worst overlapping states: saved Break Charm layers, active screen/status icons, TX doubles/Meteo, Eternamax plus Zacian healthboxes and Arceus mosaic/popup. Count free sprites/tasks/palettes and ensure every allocation failure has a safe non-hanging continuation. After final cleanup start an ordinary singles and an ordinary doubles battle to detect surviving state/resources.
8. Update mechanics documentation after implementation to reflect real supported profiles, cooldown timing, reduced maxHP and fallback policies. Replace stale claims in the older proposal/progress notes only as a separate documentation review; preserve those artifacts during this planning task. Capture exact implementation revision, command/filter, selected test count, pass/fail details and emulator observations. A successful run selecting zero tests is not acceptance evidence.

### Optional Aurora Beam decision

**Recommend cutting it from the mandatory implementation.** Existing trainer slides can supply the timing/presentation and stage 01 can count the third completed conjured turn. However, inspected damage/ambush infrastructure is battler-owned and no trainer raw offensive stat/level source is specified. Reusing the active boss as attacker would silently invent damage sourcing and attribution; a synthetic trainer battler would expand ownership/scheduler work for an optional gag. Neither is justified ahead of mandatory features.

Do not add the third-turn attack, reserve fake battler state or block release for it. If explicitly revived later, first specify damage source/power calculation and prove a returning non-action event using existing infrastructure. Its separate acceptance contract remains exactly once after conjured turn three, ordinary Ice/Aurora Beam 10% Attack secondary and no stolen Pokémon action.

Proposed changes: focused integration tests and small cross-stage fixes in their owning modules; returning scripts/presentation timing where needed; mechanics docs and final validation record. No unrelated combat refactor or new assets.

## 4. Dependencies and safe milestones

- 01 establishes encounter timing/order; 02 validates existing growth; 03 adds Any Move.
- 04 establishes correct TX ownership/curse/phases, then 05 adds Meteo and tactical selection.
- 06 adds genuine boss form transitions; 07 adds independent ally storage/topology and closes it before Dialga.
- 08 validates existing Dialga mechanics; 09 adds final Arceus sequence. These are all mandatory before complete-encounter acceptance.
- [11](11_koanko_ai_validation.md) reviews and validates AI introduced by 01/03–09 plus 02's stat-refresh contract. It must pass before Stage10 final acceptance. The dependency is feature work → 11 AI gate → 10 final acceptance; Stage11 does not depend on completed Stage10 and does not replace earlier feature responsibilities.
- Review 07's ownership-storage change and 09's HP-floor/saved-layer hook as separate high-risk reviews before enabling the full encounter. An unresolved mandatory issue remains a blocker; do not ship a weaker substitute merely because old tests pass.

## 5. Edge cases and invariants

Battle identity/trainer flag/inventory survive the transition; no intermediate win text/payout/cleanup. First team ordinary switching and G-Max Alcremie remain. Conjured order, no living forced switch, twelve genuine enemy faints and real player loss authority remain coherent with extra bars and Zacian. No combat-owned species/maxHP/growth leaks to the real party. Neither UI refresh nor delayed damage resurrects a retired position, repeats a phase, recreates Doom Clock or spends a second seal. Higher levels use existing MAX_LEVEL105 support; do not create another leveling subsystem.

The only trainer-wide gimmick-use reset is the opponent Tera-used flag, once at the team transition. Player Tera usage, opponent Dynamax and every other gimmick-used flag remain intact. Calyrex uses ordinary optional AI Fire Tera; form change preserves active Tera or leaves an inactive state/remaining eligibility intact, never activating it or renewing usage. These confirmed decisions supersede the handoff's forced-Tera wording.

Native Stellar TX has no weaknesses or resistances against player-accessible attack types. Preserve the existing Divine→Stellar 2× interaction: Divine is boss-exclusive and unavailable through legitimate player gameplay. Do not change the chart or expose Divine to the player to resolve a nonexistent design conflict.

## 6. Test strategy

No tests are run for this planning stage. During implementation first use `wsl make check TESTS='Koanko Integration'` plus all new stage-specific prefixes, then separately run `Koanko`, `TX Wishes`, `Wishful Thinking`, `Roar of Eternity`, `Arcanum Wish More Wishes`, `Arcanum Wish Reduction`, `Arcanum No Order Change`, `Break Bar`, `Trainer Break Bar`, `Kazuradrop`, `Conversion 2`, `Multitype`, `Intrepid Sword`, and the affected ordinary doubles/form/recharge groups. File filters such as `TESTS=test/battle/break_bar_exit_hp.c` are supported when name prefixes are ambiguous.

Before treating those runs as final acceptance, require Stage11's production-profile `wsl make check TESTS='Koanko AI'` gate and its targeted ordinary AI controls. Cover all thirteen encounter combatants, active versus inactive Calyrex Tera, layer-break versus genuine-KO valuation, virtual/ordinary scoring and commitment, correct converted Judgment/approved Behemoth damage, and cache/ownership changes across temporary doubles. Existing TX tests that inject a chosen Wish scorer remain mechanism evidence; they cannot substitute for tactical cases using the production dispatcher. Reuse Stage11's allowed alternative sets and observable-effect assertions rather than require a hardcoded move rotation.

Coverage must include direct/residual/recoil/instant/deferred/simultaneous KOs, pending original and saved bars, hazards chaining replacements, player resource exhaustion, item-only turns, paused actions, forced switch attempts, allocation failure, menu reconstruction and loss during scripted events. Run a normal `wsl make` ROM build only when implementation changes warrant it. Broader suites are a later implementation decision supported by actual changes/failures; never initiate a full suite merely to prepare this plan.

Update `Koanko`, `Koanko Calyrex` and `Koanko Integration` expectations for the later Tera decisions: seed opponent/player Tera-used and opponent Dynamax/other gimmick-used states, verify only opponent Tera usage clears at transition, then consume it and prove repeated transition/entry/controller paths do not clear it again. Cover ordinary AI choosing and deferring Fire Tera, transformations with active versus inactive Tera, unused eligibility surviving transformation, and already-consumed usage preventing a later activation. Remove expectations of forced entry Tera. Add an exact-text presentation assertion for `Alter Ego's Bug Space` with Arceus's ability still Multitype.

Include neutral native-Stellar matchups for player-accessible attack types and a separate chart regression preserving Divine→Stellar at 2×. Verify boss-only move exclusions remain intact for player-accessible move acquisition/call paths; synthetic test use of Divine must not alter legitimate gameplay access.

Text-width failures require the exact test, string and current baseline evidence. The handoff mentions legacy failures, but no current text-window failure was demonstrated in this planning pass; none is pre-excused. Keep the exact known Stack Breaker exception separate from new failures.

## 7. Completion criteria

All mandatory stage completion gates pass and the uninterrupted encounter visibly matches the handoff as superseded by later confirmed user decisions and recorded approved defaults. The opponent-only one-time Tera-used reset and ordinary optional Calyrex Fire Tera, including preservation through transformation when active, are proven. Focused test selections have nonzero coverage, current successful logs and no unexplained new failures. Runtime presentation/cleanup/resource evidence complements tests. Player-party canaries remain intact except for legitimate battle effects; a following ordinary encounter is normal. Optional Aurora Beam is explicitly omitted, not silently half-implemented.

**Required AI acceptance gate:** Stage11's completion criteria and production-profile tactical scenarios pass, with approved flag/tactical choices recorded and a validation record linked from the implementation results. No unresolved mandatory AI architecture defect affecting this encounter may be waived by generic suite results, injected test scores or a new KNOWN_FAILING marker. Stage10 remains incomplete until this gate is satisfied.

## 8. Open questions and release readiness

Resolve before the affected implementation: Grand Battle track identity; acceptance of No Order Change forced-switch policy; Calyrex/Eternatus total bars; Zacian tuning and anti-Eternamax qualification; Meteo success/next-active-turn semantics; Arceus complete-turn timing/opening threshold/fallback for no resisting form. Each recommendation is stated in its owning stage; no new precise boss levels are invented.

Ready now for review: existing code audit, interfaces, integration order, concrete test scenarios and asset reuse. Highest implementation risks are independent ally ownership/collapse, HP/stat overlay normalization, and Arceus's cross-source HP floor and saved-layer event. Those risks justify focused stage gates, not dropping mandatory mechanics. Planning completion does not authorize implementation.
