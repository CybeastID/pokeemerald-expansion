# Koanko superboss: staged implementation plan

Planning snapshot: 2026-11-09, local `mypatch` worktree. Planning documents only; no implementation is authorized by these documents. The latest user instruction authorizes writing these Markdown files despite the original handoff's read-only wording.

AI planning supplement: 2026-11-10. Stage11 adds encounter-wide tactical validation; its numbering preserves existing filenames. No code implementation or test execution is included.

## Authority and current state

- Design: `TEMPSCRIPTS/KOANKO_CODEX_PLANNING_HANDOFF.md`, as superseded by later confirmed user decisions. In particular, reset only opponent Tera usage once at the team transition and let ordinary AI optionally choose Calyrex Fire Tera. Repository source is authoritative for implemented behavior. Older proposal/progress notes contain superseded claims and are not implementation evidence.
- Existing: in-place second-party replacement and nine Koanko tests; corrected first/conjured party data; levels 100–105; TX species tuning; Wishful Thinking; Roar of Eternity. Inspect and preserve these rather than rebuilding them.
- Latest Stage11 reinspection (2026-11-10): one-time opponent Tera-used reset, conjured sequence switching and the Koanko switch-in No Order Change hook are now present, with unit assertions for the reset. Wishful Thinking now dispatches over all living effective holders; intended trigger scope needs confirmation against the earlier caster-local design. Small source-audit addenda in 01/02 preserve these outside changes and avoid duplicate work; no new test-run claim is made.
- Remaining: encounter presentation/order enforcement, Any Move, solo-TX ownership/Wish policy, Koanko Reduction, TX maxHP deterioration, Meteo, form transitions, independent allied Zacian, Arceus staging/adaptation, integration coverage.
- Preserve every pre-existing dirty file, including existing C/header/script/test changes and MageArmor16 artwork. No resets, generated-source edits, commits, pushes, or full test-suite runs during planning.
- Evidence status: source inspected; existing logs are historical evidence, not tests executed by this planning task. Detailed coverage and limitations belong in each stage.

## Stage index and dependencies

1. [Encounter lifecycle and progression](01_encounter_lifecycle.md) — **fully planned**; depends on existing Koanko/No Order Change systems.
2. [Wishful Thinking validation](02_wishful_thinking_validation.md) — **fully planned**; validate existing implementation and shared stat ownership.
3. [Smeargle Any Move](03_smeargle_any_move.md) — **fully planned**; depends on 01.
4. [TX profiles and Reduction](04_tx_profiles_and_reduction.md) — **fully planned**; depends on 01–02.
5. [Meteo and tactical AI](05_meteo_and_tactical_ai.md) — **fully planned**; depends on 02–04.
6. [Break Bar form transitions](06_break_bar_form_transitions.md) — **fully planned**; depends on 01–02.
7. [Independent Zacian ally](07_independent_zacian_ally.md) — **fully planned**; depends on 01 and 06; integrates 04–05.
8. [Dialga and Time Stop validation](08_dialga_time_stop_validation.md) — **fully planned**; depends on 01 and 07 cleanup.
9. [Arceus Bug Space and Conversion 2](09_arceus_bug_space_and_conversion.md) — **fully planned**; depends on 01, 02 and 06; integrates after 07–08.
10. [Integration and presentation](10_integration_and_presentation.md) — **fully planned**; final acceptance depends on 01–09 and the Stage11 AI gate; recommends cutting optional Aurora Beam.
11. [Koanko AI behavior and tactical validation](11_koanko_ai_validation.md) — **fully planned**; reviews 01/03–09 and the 02 stat-refresh contract, then gates Stage10 final acceptance. Does not depend on completed Stage10.

## Progress and readiness

Planning progress: **11/11 stage documents saved**, plus this master index. All stages are fully planned; none remains only a proposed title. Implementation progress is separate and is not implied by a fully planned label. No battle code changed and no tests run for this planning task.

Stages are reviewable independently, but implementation must honor listed dependencies. Each document records verified code, proposed interfaces, tests, invariants, completion gates, and unresolved decisions. Do not treat a proposed stage title as an inspected plan.

Dependency direction is **01–09 feature implementation/focused tests → 11 AI review and tactical hardening → 10 final integration acceptance**. Stage10 may prepare provisional integration fixtures earlier, but Stage11 never depends on Stage10 completion. Earlier stages retain ownership of their AI features; Stage11 validates them rather than implementing separate per-combatant AI systems.

## Decision tracking

- Preserve configured levels, party tuning, first-team ordinary switching, real player item access and existing Break Charm behavior.
- **Confirmed transition correction (01):** clear only the opponent trainer's Tera-used flag once at ordinary→conjured transition. Preserve player Tera usage and all other trainer-wide gimmick-used flags, including opponent Dynamax. Repeated transition calls, entries and refreshes cannot renew the use again.
- **Confirmed Calyrex correction (06):** Fire Tera is optional through ordinary AI eligibility/scoring, with no forced entry or transformation activation. Preserve active Fire Tera through Ice→Shadow transformation; otherwise preserve the inactive state and normal remaining eligibility. Transformation does not reset Tera usage. These two decisions supersede the original handoff.
- Recommended forced-switch policy: apply existing No Order Change to living conjured opponents, preserving damaging phazing hits and genuine faint replacement. This is a design recommendation, not an existing Koanko implementation.
- Music identity still needs confirmation: `MUS_KAZURADROP_APPEARS` exists, but its identity as the requested Grand Battle track is not proven by the constant.
- Mandatory scope includes genuine Eternamax, independent allied Zacian, and Arceus Command Seal Conversion 2. Optional Aurora Beam may be cut; its damage source is unspecified.
- Preserve 10% Wishful Thinking tuning already implemented; verify missing edge cases rather than recreate it. Roar of Eternity also already exists with repaired targeted tests.
- Recommend one extra original bar each for Calyrex/Eternatus, pending acceptance; do not add unapproved further phases. Zacian tuning/anti-Eternamax qualification and Meteo cooldown definitions are recorded in stages 05–07.
- Arceus first-full-turn semantics and no-resisting-form fallback are explicit recommendations in 09. All 18 ordinary form records/palettes exist and intentionally share sprite pixels. No boss-only Arceus form exists.
- **Confirmed matchup/accessibility rule:** native Stellar TX Rollback has no weaknesses or resistances against player-accessible attack types. The existing TYPE_DIVINE→Stellar 2× interaction is intentional: Divine is boss-exclusive and unavailable to the player through legitimate gameplay. Preserve both the chart interaction and that access restriction; this is not a design discrepancy or an unresolved balance question.
- Optional Aurora Beam is recommended for omission because trainer damage sourcing is unspecified and existing extra-attack infrastructure requires a real battler.
- Stage11 recommends existing CHECK_VIABILITY/SMART_TERA flag tuning because current Koanko flags omit those passes; this remains an approval item, not a silent change to Stage01's first-team flag scope. No new prediction/forced-opening/rotation policy is approved.
- Stage11 also records the current all-holder Wishful Thinking dispatcher versus the earlier caster-only description. Confirm the intended scope before tactical acceptance; no code was changed to resolve this planning question.

## Readiness and review gates

Existing party data, native Stellar TX stats, Wishful Thinking and Roar of Eternity are implementation baselines, not new feature tasks. Stage 02 and 08 are validation/hardening work. Stages 01/03–07/09 add the missing encounter features. Stage11 reviews original/conjured teams, independent Zacian and transition-aware AI; Stage10 proves final cross-stage behavior and presentation only after that AI gate passes.

Review recommendations/open questions in the affected stage before code changes. Music identity, total bars, ally tuning/damage qualification and ambiguous timing/fallback choices remain explicit design decisions; they are not hidden implementer assumptions. Mandatory allied Zacian and Arceus seal adaptation remain in scope regardless of difficulty.

The principal engineering risks are temporary ally ownership and topology collapse, stat/maxHP normalization across refreshes, bounded AI sampling cost, and Arceus's HP-floor/saved-layer/type capture. Each has concrete tests and completion gates in the corresponding stage. No migrations or persistent save-format changes are planned; new encounter state is battle-only and existing IDs must remain stable.

Current AI audit gaps are recorded with concrete symbols in Stage11: absent production TX/virtual scoring policy; four-slot caches and dynamic spell score parity; layer breaks mistaken for genuine KOs; independent player-right AI recognition/ownership; topology/form cache refresh; actual form-based Judgment/approved Behemoth damage parity; and signed-score comparison range safety. Ordinary recovery/status/coverage logic already exists and should be reused. Existing effect/plumbing tests with injected scorers do not establish production tactical quality.

## Evidence and planning-delivery acceptance

- Source/data inspection, symbol references and historical-log limitations are recorded per stage. No full suite or focused battle tests were run by this planning task.
- Historical Koanko log: 9/9; adjacent historical Koanko regression groups: 44/13/15/103. User reports the newer targeted Wishful Thinking/Roar tests passed; this plan inspected their repaired current sources without claiming a fresh run.
- Sprite presence/metadata verified: indexed P assets for Calyrex Ice/Shadow, Eternamax and Crowned Zacian (64×64 fronts/backs); Arceus indexed 64×128 animated front/64×64 back with all ordinary form palettes. Runtime visual/resource verification remains implementation work.
- Delivery gate: one master plus eleven separate stage Markdown documents, every stage containing all eight requested sections, relative stage links valid, no code/test/asset edits by this task. Stage11 includes the per-combatant behavior matrix and targeted scenario assertions.

Original planning validation (2026-11-09): **passed** for the original master plus stages01–10. The Stage11 supplement and updated Stage10 dependency are revalidated separately below. No battle tests, builds, commits or pushes were performed. Open design choices are explicitly recorded, not reported as implemented or resolved.

Stage11 planning validation (2026-11-10): **passed**. The master plus eleven stage documents exist; every stage has sections1–8 and all relative Markdown links resolve. Stage11's matrix covers all thirteen encounter combatants plus a Castoria/summoned-TX regression row. Dependency review confirms 01–09 → 11 → Stage10 final acceptance with no completion cycle. Only Markdown was written by this task; no compilation/tests/commits/pushes were performed. An integrity check of 32 inspected files found 24 unchanged, six outside source/test changes re-read and preserved, and the two explained Stage01/02 documentation addenda; no unexpected changes appeared in that checked set. Source presence and historical test results are not reported as new passing AI validation.

## Verification policy

Do not run the full suite to plan. At implementation time use the specific `TESTS=` test-name prefixes in stage documents, collect fresh logs, then run only justified adjacent regressions. Historical known failures must be identified by exact test and signature rather than assumed unrelated. Final acceptance includes source/data checks, focused battle tests, and emulator presentation checks; automated tests alone do not establish animation quality or music identity.

The corrected Tera contract must be covered by `Koanko` transition fixtures, `Koanko Calyrex` focused cases and `Koanko Integration`: opponent-only one-time Tera-used reset; player/other gimmick usage unchanged; no reset on re-entry/repeated transition; ordinary AI activation and deferral; transformed Calyrex preserving both active Fire Tera and inactive/unused eligibility. Stage 09 must assert the exact presentation text `Alter Ego's Bug Space` while Arceus retains genuine Multitype. These are planned test updates, not test-source changes or new run results.

Stage10 final acceptance additionally requires Stage11's production-profile `Koanko AI` results: all original/conjured combatants and independent Zacian, legality/target/commitment parity, layer-aware damage, virtual outcome/cooldown policy, stat/form/type/topology refresh and resource ownership. Record nonzero selected tests and observable effects; ordinary randomized/equivalent choices are allowed under documented scenario expectations. A broad green suite or a test-only forced scorer is insufficient.
