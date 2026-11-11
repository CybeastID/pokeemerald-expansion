# Chapter 13 — Upstream merge-minimization and incremental upgrade review

Supplement to [Chapter12](12_engine_architecture_comparison.md). Status: **review complete; recommendations only**. Source snapshot: local `mypatch`, 2026-11-10. Existing Chapters00–12 remain unchanged and authoritative within their stated scope.

## 1. Findings and recommendation

Prefer **bespoke modules behind narrow, explicit engine hooks**. Retain shared engine changes where HP correctness, independent ownership, controller safety or action ordering requires them. Reassessing Chapter12 does not justify copying ordinary damage/scoring code into custom files or hiding every mechanic behind one opaque callback.

The largest recurring maintenance surfaces are battle-script execution, ability/damage utilities, AI selection and caches, battle lifecycle, shared state, and healthbox reconstruction. The number of changed lines alone is misleading: much of the end-turn patch is self-contained new handlers, while smaller HP, targeting and ownership edits sit inside frequently changing execution paths.

The upgrade route materially changes the cost. Against the selected release **1.17.2**, the measured surface contains **80 distinct upstream paths** and **137 identifiable custom function contexts** with later upstream changes. A direct jump visits them in one reconciliation round. Four release-family checkpoints produce **252 file-step** and **320 function-step** attention units; taking all fourteen releases produces **469** and **423**. These are static exposure measures, **not conflict counts or labor multipliers**. Incremental steps can reduce diagnostic complexity and preserve migration inputs even while revisiting integrations.

**Recommended decision:** choose a release target and migration policy before expanding A1–A8. For a future 1.17.2 upgrade, examine a **1.16.4 bridge** first because it exposes trainer-party migrations before the 1.17 transition; add a 1.15.3 checkpoint if separating switch-in/AI changes makes validation tractable. Do not adopt every patch release merely to reduce merge risk. A direct upgrade remains a credible option if migration inputs can be preserved and the team can validate the combined change. Evidence does not establish which route will take fewer engineering hours.

This review performs no upgrade, branch merge, rebase, source implementation, build or battle-test run. Read-only history, diffs and source inspection are evidence; gameplay correctness after a port remains unverified.

## 2. Baselines, provenance and measurement limits

### Actual shared base, rather than the stale remote-tracking ref

- Local HEAD: `e48e6c57735a3dba567629644e6b1a6a6079f762`, 2026-11-09, with existing uncommitted changes. Its parent is the published fork tip `b0bbd28e4a4e43406e4a5e58f35eda3fc239965c`.
- Local `upstream/master` still points to `396f2c9565082757361bb26e5c63a40fdb3c9fd8`, dated 2024-01-06. Comparing against it would include intervening upstream work in the supposed custom footprint.
- GitHub's fork comparison identified the actual shared base with the verified later upstream head. Local ancestry analysis then confirmed **`64d40d3c1464011d8642d280b622f3e7fa27c5d3`**, the **expansion/1.14.2** release of 2026-01-02.
- Verified upstream master snapshot **M**: [`be3b089fc121143b7ccf6503ea1385b8abe05b8c`](https://github.com/rh-hideout/pokeemerald-expansion/commit/be3b089fc121143b7ccf6503ea1385b8abe05b8c), dated 2026-11-09. Local HEAD and M have 141 and 1,754 commits respectively outside their common ancestry. These are graph counts, not counts of custom mechanics or conflicts.
- Selected later release **R**: **expansion/1.17.2**, [`79dd436920845ebc145577649de624f65298c209`](https://github.com/rh-hideout/pokeemerald-expansion/commit/79dd436920845ebc145577649de624f65298c209), dated 2026-10-31. M contains additional post-release fixes and is a separate observation, not a substitute for R.

The verified upstream objects were downloaded without moving branches or editing the worktree; release tag hashes were read remotely. The repository is not shallow. The stale tracking ref was not used as the custom-patch baseline.

### What each number means

**Current footprint:** `git diff --unified=0 BASE -- path` and `--numstat`, including tracked uncommitted changes, with normal Git line-ending conversion. A hunk is a U0 diff segment, not a function, hook or independently mergeable unit. Added-only blocks, declarations, dead code and ordinary behavior changes are included in line totals; they are distinguished in the analysis.

**Historical churn:** distinct non-merge commits in `BASE..M` affecting a path. Function counts use Git's hunk context, normalized across signature changes. A context indicates the function or nearby code named by Git, not proof that both patches edit the same statement. Some commits have dates earlier than BASE because ancestry ranges include changes developed on another branch before incorporation. No date filter substitutes for ancestry.

**Release file exposure:** intersection of the 81 inventoried modified upstream paths with paths whose trees differ across each release interval. Counts include interfaces/data as well as C implementations. They omit new upstream destination paths as additional unique custom files, while the migration analysis explicitly tracks destinations.

**Release function exposure:** a fixed set of baseline custom-modified functions was identified from C definitions and changed ranges. Matching function/nearby contexts in non-merge upstream history were then associated with each interval. Historical contexts, rather than only endpoint diffs, retain changes later revised within an interval. Aliases track `battle_ai_switch_items.c` into the switch/item split and `SetMoveEffect` into `battle_set_effect.c`. The resulting 137 contexts are a **measured subset**, not a complete census of semantic integration points: renamed/split functions, shared fields, script labels and new custom handlers need separate inspection.

**Repeated exposure:** the same file or function context appears at more than one boundary. **Cumulative burden proxy:** sum of file-step or function-step occurrences. One integration appearing at ten boundaries contributes ten attention units, even if some boundaries merge cleanly. Every-release and checkpoint routes have the same union of 137 measured contexts; their repetition differs.

**Direct textual-overlap candidates:** compare custom and upstream U0 ranges in the common base. Count a custom hunk if both edit a common base line, or both insert at the same anchor. Adjacent edits and semantic dependencies may be missed. Overlap is observed; a Git textual conflict is **not inferred automatically**. No actual branch merge was performed, and these counts are not an upgrade conflict forecast.

## 3. Current modification footprint

There are **474 tracked paths** differing from the actual base across the project. This includes authored data, graphics, maps, documentation and unrelated systems. The explicitly inventoried battle/runtime, supporting interface and combat-data surface in AppendixA contains **107 paths: 81 modified upstream paths and 26 custom additions**. This is a bounded battle-maintenance inventory, not a claim that all 474 differences implement these encounters.

The primary engine/interface subset contains 73 paths: 54 modified upstream paths and 19 custom additions. Its modified upstream paths account for **4,471 added lines, 238 removed lines and 449 U0 hunks**. Supplemental support/data paths are listed separately in the same appendix so these totals are not confused with the entire project patch.

### Existing custom modules already provide useful isolation

`battle_break_bar.c`, `battle_reduction.c`, `battle_tx_rollback.c`, `battle_tx_wishes.c`, `battle_koanko.c`, `castoria_name_theft.c`, `kazuradrop_battle_ui.c`, `kazuradrop_entrance.c`, `kazuradrop_gameover.c`, `battle_status_icons.c`, `servant_sigil.c` and `trainer_break_bars.c`, with their custom headers/data, are additions relative to BASE. None of the tracked added paths collides by name with a path in M. This lowers direct add/add exposure; it does **not** remove dependencies on upstream globals, layouts, ownership, callbacks or graphics APIs.

By contrast, `src/battle_ai_script_commands.c` exists at BASE, has a four-line local placeholder, and is removed upstream. It is a modified upstream path despite its custom-looking content. `src/battle_ai_switch_items.c` is also removed/replaced upstream. Both need explicit deletion/migration review rather than assuming low line count means low exposure.

### Implemented versus still planned

Already implemented: Kazuradrop's Bug Space, transformation/moves and custom loss paths; Castoria's virtual actions/Polymorph; summoned TX creation, Wishes/ambush; Reduction normalization; original/saved Break Bars; existing Koanko party transition, opponent-only one-time Tera reset, sequence flag and No Order Change entry hook; Wishful Thinking and Roar of Eternity.

Still planned: Any Move sampling, solo TX profiles/Reduction, Meteo and production tactics, Koanko form phases, independent Zacian and return to singles, Arceus's staged sequence, and Chapter12's broader interfaces. A1–A8 must not be counted as existing patches. In particular, `battle_gimmick.c` is currently unchanged relative to BASE even though A4 could create a future integration there.

Authoring/registration exposure also remains in move/type/species/item constants, effect tables, trainer data and graphics declarations. For example, `src/data/trainer_parties.h` contributes 11,851 added lines in two hunks; that is not 11,851 lines of engine surgery. Generated/authoritative input relationships must be identified before extracting registrations. No generator or migration script was run here.

## 4. Ranked current hotspot map

Ranking considers observed recurring changes, execution centrality, migration history and cost of preserving behavior. It is **not** a probability calculation or an ordering by lines changed. `+/-; H` means current additions/removals and U0 hunks versus BASE. `O` is the count of custom hunks with direct common-base overlap against R. Churn is through M; release-specific attention is in Section5.

| Rank | Upstream file and exact integration locations | Mechanics / patch distribution | Footprint | Historical churn / direct overlap | Exposure and recommended action |
|---|---|---|---|---|---|
| 1 | `battle_script_commands.c`: `Cmd_adjustdamage`, `Cmd_healthbarupdate`, `PassiveDataHpUpdate`, `MoveDamageDataHpUpdate`, `Cmd_tryfaintmon`, `Cmd_moveend`, `Cmd_checkteamslost`, `Cmd_switchindataupdate`, `SetMoveEffect` | Bars, Guts, Bug Space, custom move effects, TX, Reduction, Koanko, virtual slot guards. Dispersed semantic edits plus sizeable bespoke native handlers | +1000/-84; H104 | 284 file commits; `SetMoveEffect` context52, moveend24, tryfaint10; O43 | **Very high textual and semantic exposure. Extract bespoke handlers; retain precise HP/faint/execution gates.** Port effect registration to the destination dispatcher when upgrading. |
| 2 | `battle_util.c`: `AbilityBattleEffects`, action completion/use, cancelers, damage/type modifiers, clean stat copy/recalc and form reversion | Kazura ability/legend policy, Wishful, virtual PP/selection, sigils, Melt Virus, bars, TX/Reduction. New helper bodies mixed with numerous small alterations | +359/-30; H49 | 302 file commits; ability91, type-effectiveness15, accuracy15; O15 | **Very high semantic exposure. Extract custom ability/stat/presentation policy, consolidate phase-specific adapters; preserve shared calculation ordering.** |
| 3 | `battle_ai_main.c`: singles/doubles selection, `ChooseArcanumForTarget`, `ClearWishSelection`, `CommitWishSelection`, `SetAiLogicDataForTurn`, `AI_AliceTrainer`, `AI_RazeluxeTrainer` | Large encounter scorer bodies plus normal/virtual comparison, context and cache edits | +587/-3; H20 | 148 file commits; bad-move63, effect-score55, cache14, singles7/doubles10; O6 | **High textual and very high semantic exposure. Extract encounter scorers and virtual-policy loop; expose only the ordinary evaluation bridge that correctness requires.** |
| 4 | `battle_main.c`: `SwitchInClearSetData`, `FaintClearSetData`, turn selection, party creation, intro/finish | TX slot policy, Koanko transition/entry, Reduction cleanup, bars and Time Stop; edits distributed across lifecycle states | +243/-14; H37 | 136 file commits; faint23, switch20, selection18, finish14; O11 | **High exposure. Consolidate owned lifecycle operations at existing phases; accept required queue/owner gates.** Never collapse differently timed phases into one callback. |
| 5 | `battle_ai_util.c`: `AI_GetDamage`, `GetAIChosenMove`, escape/trapping and KO helpers | Sigil estimates, virtual-aware access, No Order Change; small dispersed edits in shared utilities | +60/-12; H17 | 136 file commits; `AI_GetDamage`5, `CanIndexMoveFaintTarget`5; O5. Shared callee `AI_CalcDamage` has21 contexts | **High semantic exposure despite small patch. Retain shared parity adapters; avoid copying the calculator/scoring library into custom code.** |
| 6 | `include/battle.h`: `PartyState`, `BattlerState`, `EventStates`, `BattleStruct`, move accessors and HP setters | Custom stored state, virtual moves, HP/Reduction integration; cross-cutting layout/API surface | +127/-6; H19 | 103 file commits; BattleStruct48, PartyState7; O5 | **High migration exposure. Keep owned records in custom headers where sensible, but retain required authoritative fields/accessors.** Do not invent a competing trainer-party framework. |
| 7 | `battle_interface.c`: coordinate selection, healthbox creation/update, level/status/name rendering and bar drawing | TX asymmetric layout, bar overlays, numeric display and effective stats. Several new helpers and many view changes | +125/-24; H26 | 35 file commits; level5, HP text4; O11 | **High text/semantic exposure. Keep overlay bodies custom; consolidate view policy.** Upstream percentage HP changes require actual rendering review. |
| 8 | `data/battle_scripts_1.s`, related macros and script declarations | Custom appended sequences plus inserts into faint/replacement/loss/Wish and HP-related scripts | scripts +665/-21; H24; macros +96/-0; H3 | scripts135, macros55 file commits; O5 for scripts | **High semantic exposure. Extract custom-only script blocks into included custom files, preserve ordinary insertion points and return semantics.** |
| 9 | `battle_end_turn.c`: custom `HandleEndTurnBugSpaceDecay`, passive OHKO, Infinite Growth, Melt Virus; `HandleEndTurnVarious`, `HandleEndTurnDynamax`, handler table and `DoEndTurnEffects` | Most of 241 lines are bespoke new handler bodies; few actual scheduler boundaries | +241/-0; H6 | 49 file commits; Various5, DoEndTurn3; O1 | **Moderate textual, high timing risk. Extract bodies while retaining explicit handler order/cursor ownership and bar resolution barrier.** |
| 10 | `battle_controllers.c`: mon-data normalization, healthbar update, send-out policy; `battle_util2.c`: allocation/free; `reshow_battle_screen.c`: both reshow flows, sprites/healthboxes | Existing narrow adapters across a graphics/state lifetime boundary | controllers +54/-2 H11; util2 +10/-0 H4; reshow +29/-3 H8 | file churn47/8/18; O0/0/0 | **Low observed direct range overlap does not imply safety.** Accept narrow hooks, review graphics teardown/rebuild separately from battle-state teardown. |
| 11 | `battle_anim_effects_2.c`: `AnimTask_TrashCrush` and steps; `battle_anim_effects_3.c`: `AnimTask_PolymorphMon` | Added task bodies, not a wholesale animation engine rewrite; Polymorph is inserted after upstream `AnimTask_TransformMon` | +55/-0 H2; +64/-0 H2 | effects3 has11 file commits, nearby Transform3 | **Good extraction candidates.** Keep public task symbols and their runtime context; avoid forking the upstream Transform task. |
| 12 | `sprite.c`: `BeginAnim`, `ContinueAnim`, frame/jump handlers, `SetSpriteOamFlipBits`, affine matrix application and subsprites | Global affine/flip changes supporting visuals; substantially broader than one encounter | +83/-13; H12 | 20 file commits | **High semantic reach. Defer extraction.** Establish original rationale and ordinary sprite compatibility; moving shared sprite semantics into a boss hook could break other users. |

The rank10 zero-overlap cases are especially useful counterexamples to treating co-change as textual conflict: graphics lifetime and owner meaning can change even when source insertions do not share base lines. Conversely, the custom native/AI/task bodies can occupy many lines with only a few direct insertion seams.

### Concrete upstream changes explaining the ranking

- [AI/switch-item split, `fee619075e`](https://github.com/rh-hideout/pokeemerald-expansion/commit/fee619075eac6cd61862d642849eaba5603d512d): old file is replaced; orchestration also moves into AI main. [Controller-based AI recognition, `2e6f941fdf`](https://github.com/rh-hideout/pokeemerald-expansion/commit/2e6f941fdf1605592766fe09943ded9d6be86da4) and [switch-in events, `f222bcaff5`](https://github.com/rh-hideout/pokeemerald-expansion/commit/f222bcaff530f85346c781c9829a007d291b5ca9) first occur in release1.15.0. Porting the old file unchanged would preserve an obsolete engine boundary.
- [Trainer-owned parties/12v12, `70340c1135`](https://github.com/rh-hideout/pokeemerald-expansion/commit/70340c11355a8c9b5c467b4e5467a1faa720e880) first occurs in1.16.0. M's `GetBattlerParty` uses `gParties[GetBattlerTrainer(battler)]`; `GetBattlerPartyState` and active gimmicks also use trainer ownership. Chapter12's description of side/index-only ownership is correct for this branch, but not the eventual upgrade target.
- [Graphics heap lifetime, `63fbcce1cd`](https://github.com/rh-hideout/pokeemerald-expansion/commit/63fbcce1cdb6ccbb09b78e39155e2d5efdb61ee3) first occurs in1.16.0 and separates graphics allocation/free from full battle allocation/free. Boss state must survive menu reconstruction while graphics resources may not.
- [Target selection versus resolution, `49ad9f9c1b`](https://github.com/rh-hideout/pokeemerald-expansion/commit/49ad9f9c1bcfb292f485ec654baa71a4d2cf6ab2), [opponent HP percentages, `9450f2c031`](https://github.com/rh-hideout/pokeemerald-expansion/commit/9450f2c031), and [SetMoveEffect table extraction, `1242c88a10`](https://github.com/rh-hideout/pokeemerald-expansion/commit/1242c88a103ea1fc342eb80a6df4166a7e5df43e) first occur in1.17.0. Virtual actions, converted Judgment, HP layers and Time Stop effects must follow these new seams.
- [Partner/opposite helper migration, `a242cd18ac`](https://github.com/rh-hideout/pokeemerald-expansion/commit/a242cd18acb39cca28d17c3f5df294e5ebfe2ca6), [recorded AI RNG ordering, `2cfbd9a891`](https://github.com/rh-hideout/pokeemerald-expansion/commit/2cfbd9a89167687b9562f7a534a0753c870c27e3), and [2v1 healthbox correctness, `6057b187f9`](https://github.com/rh-hideout/pokeemerald-expansion/commit/6057b187f946094d614e9c34bc12731ea6a5b7bb) show why small targeting/cache/layout hooks remain semantic audit points after a clean text merge.

## 5. Direct and incremental upgrade cost

### Release paths and interval identifiers

The remotely verified tag sequence is linear by ancestry for these fourteen intervals. Release commits, in order after BASE: 1.14.3 `2d17ad27f921`, 1.14.4 `d6f59caeafd2`, 1.15.0 `150649546e90`, 1.15.1 `fcecfadda60d`, 1.15.2 `6798f72e037d`, 1.15.3 `b669d9ccf9c1`, 1.16.0 `a9d9c29acc53`, 1.16.1 `983a884ab887`, 1.16.2 `ad0fd4d17f54`, 1.16.3 `c828d127217b`, 1.16.4 `3796c72237d2`, 1.17.0 `e8bd1cd7b03f`, 1.17.1 `bb1093a7c72d`, 1.17.2 `79dd43692084`. Their commit dates span 2026-02-01 to 2026-10-31. The same-day stable patch/feature-release pairs still have distinct trees; they are not interchangeable checkpoints.

| Boundary | Release interval | Selected upstream paths with net changes | Custom function contexts touched in interval history |
|---|---|---:|---:|
| B1 | 1.14.2 → 1.14.3 | 30 | 18 |
| B2 | 1.14.3 → 1.14.4 | 21 | 32 |
| B3 | 1.14.4 → 1.15.0 | 76 | 124 |
| B4 | 1.15.0 → 1.15.1 | 23 | 14 |
| B5 | 1.15.1 → 1.15.2 | 23 | 11 |
| B6 | 1.15.2 → 1.15.3 | 20 | 5 |
| B7 | 1.15.3 → 1.16.0 | 68 | 79 |
| B8 | 1.16.0 → 1.16.1 | 18 | 6 |
| B9 | 1.16.1 → 1.16.2 | 25 | 11 |
| B10 | 1.16.2 → 1.16.3 | 25 | 10 |
| B11 | 1.16.3 → 1.16.4 | 40 | 36 |
| B12 | 1.16.4 → 1.17.0 | 65 | 63 |
| B13 | 1.17.0 → 1.17.1 | 22 | 11 |
| B14 | 1.17.1 → 1.17.2 | 13 | 3 |

The largest function-context sets occur at feature-release boundaries. Patch releases also revisit critical logic. This rules out both assumptions that every patch step is cheap and that one endpoint comparison captures cumulative attention.

### Per-release hotspot matrix

`X` = upstream non-merge history touches the named baseline custom function or its Git-adjacent context in that interval. `F` = the named shared-state file changes between release trees; no function-level claim is made. `—` = no observation by this measurement, **not proof of no migration/semantic work**. SetMoveEffect includes its new destination file; named contexts cease to be sufficient when functions are replaced or split.

| Custom integration point | B1 | B2 | B3 | B4 | B5 | B6 | B7 |
|---|---|---|---|---|---|---|---|
| `AbilityBattleEffects` | X | X | X | X | X | X | X |
| `SetMoveEffect` | X | X | X | X | X | — | X |
| `AI_CheckBadMove` | X | X | X | X | — | — | X |
| `AI_CalcMoveEffectScore` | X | X | X | X | — | — | X |
| `SetAiLogicDataForTurn` | — | — | X | X | X | — | X |
| `FaintClearSetData` | X | X | X | X | — | — | X |
| `SwitchInClearSetData` | — | X | X | X | X | — | X |
| `HandleTurnActionSelectionState` | — | X | X | — | — | — | X |
| `Cmd_moveend` | X | X | X | — | X | — | X |
| `Cmd_tryfaintmon` | — | X | X | — | — | — | X |
| `Cmd_getexp` | X | — | X | — | — | — | X |
| `DoEndTurnEffects` | — | — | X | — | — | — | X |
| `UpdateHealthboxAttribute` | — | — | X | — | — | — | X |
| `BtlController_HandleHealthBarUpdate` | — | — | X | — | — | — | — |
| `include/battle.h` state/layout (file) | F | F | F | F | F | F | F |


| Custom integration point | B8 | B9 | B10 | B11 | B12 | B13 | B14 |
|---|---|---|---|---|---|---|---|
| `AbilityBattleEffects` | — | X | X | X | X | X | X |
| `SetMoveEffect` | — | X | X | X | X | X | — |
| `AI_CheckBadMove` | — | X | X | X | X | X | — |
| `AI_CalcMoveEffectScore` | — | — | — | X | X | X | — |
| `SetAiLogicDataForTurn` | — | — | — | — | X | X | — |
| `FaintClearSetData` | — | X | X | X | X | — | — |
| `SwitchInClearSetData` | — | X | — | X | X | X | — |
| `HandleTurnActionSelectionState` | — | X | — | X | X | X | — |
| `Cmd_moveend` | — | — | — | — | — | — | — |
| `Cmd_tryfaintmon` | — | X | — | X | — | — | — |
| `Cmd_getexp` | — | — | X | X | X | — | X |
| `DoEndTurnEffects` | — | — | — | — | — | — | — |
| `UpdateHealthboxAttribute` | — | — | — | — | X | — | — |
| `BtlController_HandleHealthBarUpdate` | — | — | — | — | — | — | — |
| `include/battle.h` state/layout (file) | — | F | F | F | F | — | F |


These are representative **custom integration points**, not all137 measured contexts. Ability dispatch appears in13 boundaries, SetMoveEffect in11, bad-move scoring in10, and both switch-in and faint clearing in9. Stable keys track a maintenance responsibility; they do not assert that its implementation or signature stays constant.

### Cumulative comparison

| Route to 1.17.2 | Steps | Unique files | File-step exposures | Unique measured function contexts | Function-step exposures | Files / functions revisited |
|---|---:|---:|---:|---:|---:|---:|
| Direct | 1 | 80 | 80 | 137 | 137 | 0 / 0 |
| Bridge at 1.16.4 | 2 | 80 | 147 | 137 | 200 | 67 / 63 |
| Checkpoints 1.15.3 / 1.16.4 | 3 | 80 | 215 | 137 | 283 | 74 / 97 |
| Four family checkpoints | 4 | 80 | 252 | 137 | 320 | 75 / 104 |
| Every release | 14 | 80 | 469 | 137 | 423 | 75 / 107 |

The bridge route is BASE→1.16.4→1.17.2. The three-step route adds1.15.3. The four-step route additionally stops at1.14.4. Every-release means all B1–B14. Direct means BASE→1.17.2 once. File unions remain80, but interval counts sum differently; context unions remain137 because history observations use fixed identities. Repeated files/functions count distinct hotspots appearing in at least two steps, not the number of repeats.

**What the evidence supports:** a shorter route has fewer reconciliation/validation rounds and fewer repeated exposure units. For example, the1.16.4 bridge adds67 file-step and63 function-step units to the direct route while isolating the last major transition. Taking all releases adds389 file-step and286 function-step units. This is potential revisit work, not proof that each unit needs manual editing.

**What it does not support:** direct is not proven cheaper overall, and incremental is not proven safer. Actual cost combines per-round validation/setup, per-integration adaptation, migration work, and difficulty of understanding interacting refactors. One large reconciliation may exceed available diagnostic capacity; several smaller ports may force the same adapter to be rewritten multiple times. No person-hour estimate is justified without a reviewed port and validation evidence.

### Migration dependencies and useful checkpoints

1. **1.15:** switch-in events, controller-based AI recognition, AI switch/item separation, move target rules and damage/canceler changes. These overlap custom ability entry, virtual actions and bar/faint timing. Follow the [1.15.0 refactor notes](https://github.com/rh-hideout/pokeemerald-expansion/blob/150649546e909fe96591be707c0fc1810b86480b/docs/changelogs/1.15.x/1.15.0.md), including relevant script/text/trainer generation migrations. A checkpoint can isolate this first large semantic port; it is not established as a mandatory intermediate merge.
2. **1.16:** trainer-owned parties, PartyState/gimmick ownership and graphics lifetime. [1.16.0 notes](https://github.com/rh-hideout/pokeemerald-expansion/blob/a9d9c29acc53c6f80c39729dfbb995001ff2d2f4/docs/changelogs/1.16.x/1.16.0.md) describe temporary deprecated party aliases and the later removal of enemy-party aliases. M's trainer-picture migration script reads an older trainer graphics table shape; migration inputs must be preserved or reconstructed if upgrading directly. Script applicability/idempotence and the repository's custom trainer entries still need a dedicated audit.
3. **1.17:** removal/replacement of deprecated interfaces, effect dispatch extraction, targeting separation and additional graphics/AI changes. The [1.17.0 notes](https://github.com/rh-hideout/pokeemerald-expansion/blob/e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7/docs/changelogs/1.17.x/1.17.0.md) document move-effect/signature and targeting work. Some changes already appear in late1.16 patches; tree/history comparisons determine inclusion, rather than assuming every item in a feature changelog first lands at that tag.

A bridge can make intermediate data/schema inspection and warnings useful before APIs disappear. Direct upgrading can still apply migrations in their required logical order using preserved inputs. Documentation inspection does not prove the scripts require every release to be checked out, nor that they can all safely run against the final tree. No migration was executed. Save/recorded-battle compatibility, custom trainer generation and project-wide nonbattle migrations remain outside the measured battle-only proxy and could decide the route.

### Strategy recommendation and unknowns

Use the **fewest checkpoints that provide a concrete migration or validation benefit**. Provisionally evaluate the1.16.4 bridge because there is a demonstrated party/graphics migration boundary; add1.15.3 if the switch-in/AI port is too entangled to diagnose together. Do not prescribe fourteen upgrades. If a migration audit confirms final-tree conversion is safe and adequate integration validation is available, a direct1.17.2 port may minimize repeated maintenance.

This is a conditional recommendation, not an instruction to upgrade now. Missing inputs are the selected target/support horizon, migration-script compatibility with custom data, save/recording requirements, and the team's validation budget. M's post1.17.2 fixes should be selected deliberately; they are not silently included in the release model.

## 6. Relocation candidates preserving established mechanics

The classifications below refer to proposed future work. **Decrease / unchanged / increase** describes expected recurring merge burden qualitatively; no post-extraction patch exists from which to claim exact saved hunks.

| Candidate / current location | Proposed home or seam | Classification / burden | Execution and ownership conditions |
|---|---|---|---|
| `AI_AliceTrainer`, `AI_RazeluxeTrainer`, encounter virtual utility in `battle_ai_main.c` | Dedicated encounter AI modules; retain existing `AiScoreFunc` dispatch and narrow candidate bridge | **Extract bespoke handler; decrease, high confidence** | Preserve current weights, information policy, tagged RNG, macros/helper dependencies and Castoria priorities. Extraction is not permission to retune. |
| `ChooseArcanumForTarget`, Wish candidate/commit policy | Custom action/AI module behind singles/doubles comparison hooks | **Extract + narrow hook; decrease expected** | One winning outcome/recipient, ordinary exact ties, per-target handling; no slot4/5 access into four-slot arrays. Ordinary evaluator remains shared. |
| `GetMoveBasePowerForBugSpace`, `IsMoveBlockedByBugSpace`; Kazura transformation/native dialogue/music/loss helpers in `battle_script_commands.c` | Dedicated Kazura mechanics/script modules, distinct from existing UI/entrance/gameover responsibilities | **Extract bespoke handler; decrease expected** | Preserve NATIVE_ARGS decoding, next/fail script pointers, controller yields, flag/variable ownership and the original transformation script order. Dead `#if 0` alternatives are not live requirements. |
| Bug Space ability-entry and legend policy inside `AbilityBattleEffects`; `GetKazuraLegendTier` | Kazura module, invoked at the existing effective-ability/switch-in phase | **Extract + reuse narrow hook; decrease expected** | Call only after ordinary suppression/eligibility setup; preserve effect counts and script scheduling. Future upstream switch-in refactor changes the right caller. |
| Wishful growth helpers and ABILITYEFFECT_WISH body in `battle_util.c` | Owned Wishful/stat helper module, ordinary ability dispatch case retained | **Extract bespoke handler; decrease expected** | Preserve current holder dispatch pending its separate approval, full-HP guard, raw-stat rounding, one random stat, heal semantics and Reduction brackets. No trigger-scope decision is made here. |
| Bug Space decay/passive OHKO, Infinite Growth, Melt Virus bodies in `battle_end_turn.c` | Kazura end-turn handlers exported from a custom module | **Extract bespoke handlers; decrease expected** | Keep individual scheduler positions, `endTurnBattler` increments, returns and selected attacker/target exactly. Do not replace several stages with one once-per-turn callback. |
| Castoria max-move expiration inside `HandleEndTurnDynamax` | Existing Castoria module, timed call retained at the same phase | **Extract small handler; small decrease** | Do not silently change expiry relative to Dynamax cleanup, turn pauses or Command Seal behavior. |
| `AnimTask_PolymorphMon`, Trash Crush task family, Time Stop grayscale bodies | Custom battle-animation/presentation module with current exported symbols | **Extract bespoke handlers; decrease expected** | Preserve task data layout, target/attacker selection, mosaic/window/palette cleanup and effective graphic loading. Do not copy upstream Transform gameplay. |
| Appended custom battle/animation scripts and declaration blocks | Custom included script/data files at a small number of physical inclusion seams | **Narrow include seam; decrease expected** | Retain global labels, ordinary script insertions, call/return/end behavior and build preprocessing conventions. Includes do not remove required registrations or schema changes. |
| TX, Reduction and Break Bar modules already custom | Existing modules; only consolidate duplicate calls at identical semantic phases | **Retain existing implementation; unchanged or small decrease** | Keep saved/original layer ordering, reciprocal heals, delayed attacks, mutation normalization, ambush context and cursor restoration. Re-inlining them would increase exposure. |
| HP preview/commit, faint/reward gates, no-order switch checks, auxiliary owner routing | Core semantic points invoking custom facts/policy | **Accept justified core modification; residual burden unavoidable** | A late callback cannot repair a previewed zero HP, already-issued reward, aliased party write or advanced action cursor. |
| Global affine flips in `sprite.c` | Existing shared sprite implementation pending rationale/compatibility audit | **Defer; burden unchanged** | No evidence establishes these are safely replaceable with a boss-specific task. Preserve existing visuals and ordinary sprites. |

### Critical compatibility constraint: Kazuradrop's chosen slot

The current `AI_AliceTrainer` comments explicitly describe Kazuradrop's Quiver Dance slot becoming Infinite Growth before a slower chosen action resolves. `HandleAction_UseMove` retains ordinary slot-based rereading for real moves while treating virtual moves specially. A global A6 change that freezes every chosen move ID would alter this established behavior.

Therefore scope Koanko's new form/action preservation to its intended owners or make the identity policy explicit. Do not silently apply Koanko's commitment rule to existing Kazura transformation. This is a concrete example where a seemingly cleaner global abstraction creates semantic reconciliation work even if Git finds no conflict.

## 7. A1–A8 reassessed under merge minimization

The localized column means the existing Chapters00–11 design, not necessarily fully implemented behavior. The deeper column is Chapter12's recommendation. The preferred middle approach retains correctness contracts while minimizing the recurring upstream surface.

| ID | Existing localized approach | Minimal-hook alternative | Deeper integration / comparative cost | Revised recommendation / recurring burden |
|---|---|---|---|---|
| A1 Outcome policy | Small shared AI outcome query plus owned clamps | Custom read-only HP/layer policy shared by execution and AI; typed calls at every necessary HP/faint point | General HP commit/damage framework touches rank1/2 and future effect dispatch repeatedly; high correctness/review cost | **Narrow shared policy + justified core gates.** Decrease versus a broad rewrite; do not remove multiple source-specific hooks merely to reduce lines. |
| A2 Candidate evaluation | Factored ordinary single-candidate evaluator, scratch slot0 and virtual damage support | Custom candidate enumeration/utility and scratch storage; a narrow ordinary-scoring/cache bridge; core signed comparison repair | Migrating all ordinary AI to a new descriptor/cache API increases exposure in148/136-commit files and repeated release intervals | **Retain bounded factoring; defer universal migration.** Decrease expected. M already exposes `CalcBattlerAiMovesData` and factors score processing; reuse appropriate upstream APIs after a port. Never fork the ordinary scorer. |
| A3 Cache freshness | Next-fresh-choice contract and candidate-only rebuild | Custom dirty/decision context for owned transitions; ensure/rebuild at an existing safe choice boundary using ordinary builders | Global epochs stamped by all stat/item/field mutators multiply upstream touchpoints; dependencies and RNG become recurring migration obligations | **Narrow boundary; defer global invalidation instrumentation.** Decrease expected. Correct effective type/weather/terrain, knowledge and limitations remain required; do not reroll committed actions. |
| A4 Independent ownership | Dedicated compatible backing party, state/history and routed accessors | Custom auxiliary storage/permissions plus the smallest authoritative owner-routing bridge | A new general registry would compete with upstream's trainer-owned `gParties`/PartyState/gimmick model | **Accept necessary owner integration, reuse upstream abstraction when available.** New required core surface increases versus no feature; reduces future burden versus a competing framework. Controller-based AI recognition upstream may eliminate a branch-local workaround. |
| A5 Live topology | Safe local Zacian entry/departure and common slot policy | Custom bounded formation coordinator; explicit entry/departure requests at queue/controller-safe barriers; reuse TX policy through adapters | General topology service in core would migrate state/loops across rank4/7/10 and recurrent absent-slot changes | **Custom coordinator + justified core barriers.** Decrease versus generic core migration. Do not unnecessarily rewrite the existing TX formation to obtain API uniformity. |
| A6 Commitment/context | Preserve Koanko move identity and committed Wish metadata; returning events save context | Owned metadata/recipient generation and bounded context helper; bridge to existing choice/loading/result points | Universal per-action records or scheduler changes affect ordinary callers, recordings, multi-turn moves and Kazura slot behavior | **Owned bridge; defer global action-record semantics.** Decrease expected. M's `ChosenAction` only pairs moveIndex/target; reuse that envelope where suitable, but it does not replace durable special-action metadata. |
| A7 Effective data | Narrow battle-form override, ordered growth/maxHP/Reduction normalization | Custom effective-form/stat provider called from authoritative clean-copy and writeback seams; retain existing Reduction brackets | A global modifier/rebuild pipeline must preserve all ordinary forms/copies/mutations and adapt repeatedly to upstream stat/owner changes | **Consolidate supported owned overlays; defer universal pipeline.** Decrease expected. Keep one agreed ordering and canonical HP writeback; do not add a second calculation engine. |
| A8 Profiles/dispatch | TX profile split and encounter dynamic dispatch | Static custom profile/provider tables behind existing move accessors/usability/AI dispatch | Generic runtime capability/event registries add core state, indirection and cleanup without a demonstrated requirement | **Retain encounter dispatch with a small custom provider.** Unchanged to lower expected burden; preserve Castoria/Razeluxe and real execution revalidation. |

### Current upstream interfaces change the alternatives

M already has controller-based AI recognition, trainer-party accessors outside the inline header, `ChosenAction` for moveIndex/target, a public cache-calculation entry with weather/terrain context, split switch/item logic and extracted effect dispatch. These do not automatically implement Koanko. They do reduce the justification for creating parallel generic systems now if an upstream port is imminent.

Conversely, targeting independence, player loss authority, no inventory/gimmick access for Zacian, no extra action on live entry, and all-source survival protection remain encounter requirements not established by those upstream features. Reuse does not mean toggling MULTI/INGAME_PARTNER and accepting unrelated party/loss rules.

## 8. Proposed upstream footprint after isolation

This is the required **proposed-footprint table**. Remaining items are semantic integration families, not predicted exact hunk counts. Exact post-extraction counts and conflict reduction cannot be measured without an implementation, which is not authorized.

| Upstream surface | Current footprint | What should remain after extraction / consolidation | What moves out | Expected merge burden |
|---|---|---|---|---|
| Script execution | `battle_script_commands.c` H104 | HP preview/commit policy calls, layer/faint/reward gates, legality/virtual slot protection, native/effect dispatch adapters, authoritative type capture and safe continuations | Boss-specific native bodies, transformation policy, messages/music/loss steps | **Decrease**, substantial residual semantic risk |
| Ability/damage/stat utilities | `battle_util.c` H49 | Correct-phase ability adapters; shared type/damage modifiers; clean-stat and canonical-writeback hooks; action/bar boundary and restriction guards | Kazura/Wishful policy, grayscale bodies, local overlay rebuilding | **Decrease**; calculation/order changes remain explicit |
| AI | main H20, util H17, old switch/items H1 | Ordinary evaluator/cache bridge, signed maxima, normal/virtual comparison seam, ownership/action permission and layer-outcome adapters | Encounter scorers, profiles and candidate utility | **Decrease**; map old switch/item responsibilities to their release-specific destinations |
| Lifecycle and topology | main H37; controllers H11 | Safe phase notifications, retired-position/owner gates, controller-compatible data routing and necessary queue handling | Encounter transition/formation orchestration and auxiliary storage | **Decrease expected**; mandatory entry/departure correctness still requires core work |
| State/header | `battle.h` H19 | Required fields or compact owned records/accessor declarations; normal default behavior | Profile tables and implementation bodies that need not be inline | **Unchanged to modest decrease**; pointer indirection is not automatically preferable on GBA |
| End turn | H6 | Explicit ordered handler entries and same-phase thin calls; saved-bar resolution barrier and scheduler cursor contract | Four sizeable bespoke handlers and local timers | **Decrease**, mainly extraction rather than scheduler redesign |
| UI/resources | interface H26; util2 H4; reshow H8; gfx H2 | Overlay/slot/effective-form queries at authoritative drawing/load/reshow points; separate graphics versus encounter cleanup hooks | Overlay drawing bodies and owned graphics lifecycle | **Decrease expected**, maintain percentage/display and menu semantics |
| Animations/scripts | effects2 H2, effects3 H2; scripts1 H24; anim scripts H5 | Existing animation/task declarations, ordinary sequence insertions and custom include seams | Self-contained tasks and custom-only script blocks | **Decrease**; native ABI and script order still require audit |
| IDs, effect/species/move/item/type registrations | Various appendix paths | Explicit stable registration points or supported extension includes; preserve numeric/schema contracts | Only genuinely custom tables that can be included without changing format/order | **Mostly unchanged**; extraction cannot avoid ABI/data reconciliation |
| Shared sprite affine engine | sprite H12 | Existing shared behavior until a supported alternative is proven | Nothing assumed safe to extract | **Unchanged / defer** |

Consolidate multiple custom calls within the **same** upstream function/phase when one typed adapter can preserve order and return semantics. Do not combine HP preview with commit, per-target results with per-action success, graphics close with full battle cleanup, or separate end-turn events just to shrink this table. Avoid a generic hook that receives an enum for every engine operation and becomes a second hidden scheduler.

## 9. Minimal integration contracts and necessary deeper changes

1. **HP/layer facts:** read-only projection and preview/commit constraints in a custom module; core retains actual HP, Substitute/prevention ordering, pending bars, typed source capture and faint/reward authority. All instant/residual/direct-zero paths require coverage. This is justified deeper work, not replaceable by a final heal after faint processing.
2. **AI evaluation boundary:** ordinary damage/scoring remains authoritative; custom enumerators supply sampled/virtual candidates and profile utility. One candidate scratch view and signed result comparison; ordinary arrays stay four-wide. Refresh at the next fresh decision using the release's actual weather/terrain/gimmick/knowledge context. No player-action foresight or free reselection.
3. **Actor owner/permissions:** separate combat side, backing party owner and controller. Default mappings stay ordinary. Auxiliary PP/items/PartyState/history/gimmicks and loss checks must resolve correctly before any live reinforcement is enabled. Upstream trainer-party APIs are the preferred eventual foundation; auxiliary action restrictions and true-player loss still need review.
4. **Formation barrier:** prepare resources without exposing a half-created actor, then commit only at an explicit safe boundary; retire actions/targets/graphics before reducing active iteration bounds. Preserve ordinary left actors and their chosen queue. No mandatory Zacian omission as a resource-failure success case.
5. **Returning event context:** custom bounded context saves declared transient script/action fields and selected special metadata; costs/success are recorded at the existing authoritative phase. Actor recipients and positional delayed targets have different semantics. Preserve Kazura's established slot-reading rule while enforcing the Koanko-specific contract.
6. **Effective data/writeback:** clean form → approved growth → owned maxHP → Reduction in the existing plan's order, with canonical party data protected. Copy/mutation exceptions, active Tera, spent PP and exact saved HP remain explicit. No global rebuild that erases legitimate Transform/Polymorph/Power Trick changes.
7. **Resource lifecycle:** full battle creation/destruction, graphics close, menu reshow, owner retirement and per-turn execution are separate events. A custom state owner must not be freed by a graphics-only close; graphics must not be retained merely because encounter data persists.

The remaining footprint cannot truthfully be promised as a handful of total engine lines. Correctness requires several well-placed hooks across independent paths. GBA constraints also favor fixed bounded storage, static tables and direct calls over large snapshots, per-candidate allocation or dynamic plugin dispatch. Measure future heap/stack/sprite/task/latency impact on the implementation revision; no new timing or free-memory claim is made here.

## 10. Priorities by maintenance benefit and regression risk

| Priority | Action / classification | Maintenance benefit | Regression risk and evidence needed |
|---|---|---|---|
| P0 | Record a target release, route and migration checklist; **defer upgrade execution** | Prevents implementing interfaces superseded by an imminent port | No code risk now; actual target, save policy and validation budget remain missing |
| P1 | Extract existing Alice/Razeluxe AI and self-contained native/task/script bodies; **extract bespoke handlers** | High recurring reduction at top hotspots, independent of route | Medium; preserve dependencies, symbols, weights, native arguments, task data and script continuations |
| P1 | Extract end-turn and ability policy at unchanged phases; **introduce/reuse narrow hooks** | High benefit for ability's13-interval and effect dispatch's11-interval revisit burden | High timing risk; effect count, effective ability, cursors and callback return order must remain equivalent |
| P1 | Retain bars/TX/Reduction modules and consolidate only identical-phase adapters; **retain implementation** | Protects useful isolation already achieved | Verify saved/original bars, after-break events, delayed effects, ambush and normalization during any future port |
| P2 | Define auxiliary owner and formation contracts using the intended release APIs; **accept justified core modification** | Avoids a second ownership model and repeated routing exceptions | Highest integration risk; real-party canaries, true loss authority, permissions, queue and resource lifecycle |
| P2 | Keep A1/A2/A3/A6/A7 shared facts with custom policy and minimal bridges; **narrow hooks** | Lower expected burden than global engine migrations | Shared parity, signed scores, dynamic type, no stale caches, no committed-action rerolls and Kazura compatibility |
| P3 | Isolate graphics reconstruction and custom view policy; **extract/consolidate** | Moderate reduction; menu/healthbox migrations remain explicit | Resource exhaustion/cleanup and ordinary singles/doubles rendering need runtime evidence later |
| P3 | Universal descriptor, epoch graph, modifier framework, scheduler or arbitrary formations; **defer** | Avoids new recurring core surface without established consumers | Reconsider only if a measured correctness/performance gap cannot be solved by the bounded approach |
| Separate audit | Global affine/sprite changes and unrelated support edits; **defer** | No claimed benefit until rationale is established | Broad ordinary-engine effects cannot be safely relabeled as encounter-only |

Do not batch all extractions and an upstream port into one unreviewable change. A future implementation can isolate behavior-preserving relocation from release-specific adaptation so reviewers can see which changes merely move code and which change engine contracts. Validation is required at that future stage, not run as part of this review.

## 11. Preserved mechanics and acceptance limits

This supplement changes architecture recommendations only. Chapters00–11 keep gameplay authority; Chapter12 remains a comparison. Preserve Kazuradrop, Castoria, TX Rollback and Break Bars as well as the mandatory Koanko mechanics. Keep the opponent-only one-time Tera reset, optional AI Fire Tera with active-state preservation through Calyrex transformation, genuine Eternamax, independent Zacian and singles before Dialga, and Arceus's exact `Alter Ego's Bug Space` presentation with Multitype. Native Stellar TX remains neutral against player-accessible attack types; Divine→Stellar2× remains intentional, boss-exclusive and unavailable through legitimate player gameplay.

No changes to levels, kits, stats, items, bars, trigger scopes or unresolved defaults are approved by preferring a smaller engine patch. Current all-holder Wishful dispatch is preserved pending its existing scope decision. A clean text merge, lower hunk count or absence of an upstream filename collision cannot establish combat correctness.

Historical churn and static overlap are verified observations; future conflict probability, post-extraction hunk savings, person-hours and successful upgraded behavior remain unmeasured. Required future acceptance includes production AI, every HP source, canonical data/owner isolation, script/turn timing, menu resource reconstruction and ordinary adjacent controls. Existing historical passing logs do not establish that an upgraded branch passes.

## Appendix A. Explicit current battle-maintenance inventory

Provenance is relative to BASE, not a guess from filename. Line/hunk counts include tracked worktree changes at the review snapshot. Custom added files contribute one whole-file hunk except empty placeholders. The selected107 paths define the scope of the81-path release exposure model; other authored graphics/trainer registries, maps, tests and unrelated project changes remain outside that numeric model and may add migration work.

| Path | Provenance versus merge base | Added / removed | U0 hunks |
|---|---|---:|---:|
| [asm/macros/battle_script.inc](../../../../asm/macros/battle_script.inc) | Upstream path modified | 96 / 0 | 3 |
| [data/battle_anim_scripts.s](../../../../data/battle_anim_scripts.s) | Upstream path modified | 183 / 0 | 5 |
| [data/battle_scripts_1.s](../../../../data/battle_scripts_1.s) | Upstream path modified | 665 / 21 | 24 |
| [data/battle_scripts_2.s](../../../../data/battle_scripts_2.s) | Upstream path modified | 11 / 0 | 1 |
| [include/battle_ai_main.h](../../../../include/battle_ai_main.h) | Upstream path modified | 3 / 0 | 1 |
| [include/battle_ai_util.h](../../../../include/battle_ai_util.h) | Upstream path modified | 2 / 0 | 2 |
| [include/battle_anim_scripts.h](../../../../include/battle_anim_scripts.h) | Upstream path modified | 7 / 0 | 2 |
| [include/battle_anim.h](../../../../include/battle_anim.h) | Upstream path modified | 3 / 0 | 1 |
| [include/battle_break_bar.h](../../../../include/battle_break_bar.h) | Custom added | 62 / 0 | 1 |
| [include/battle_controllers.h](../../../../include/battle_controllers.h) | Upstream path modified | 2 / 0 | 1 |
| [include/battle_dynamax.h](../../../../include/battle_dynamax.h) | Upstream path modified | 1 / 0 | 1 |
| [include/battle_gfx_sfx_util.h](../../../../include/battle_gfx_sfx_util.h) | Upstream path modified | 1 / 0 | 1 |
| [include/battle_interface.h](../../../../include/battle_interface.h) | Upstream path modified | 27 / 1 | 4 |
| [include/battle_koanko.h](../../../../include/battle_koanko.h) | Custom added | 16 / 0 | 1 |
| [include/battle_main.h](../../../../include/battle_main.h) | Upstream path modified | 3 / 0 | 2 |
| [include/battle_reduction.h](../../../../include/battle_reduction.h) | Custom added | 45 / 0 | 1 |
| [include/battle_scripts.h](../../../../include/battle_scripts.h) | Upstream path modified | 46 / 0 | 5 |
| [include/battle_status_icons.h](../../../../include/battle_status_icons.h) | Custom added | 32 / 0 | 1 |
| [include/battle_tx_rollback.h](../../../../include/battle_tx_rollback.h) | Custom added | 88 / 0 | 1 |
| [include/battle_util.h](../../../../include/battle_util.h) | Upstream path modified | 9 / 0 | 7 |
| [include/battle.h](../../../../include/battle.h) | Upstream path modified | 127 / 6 | 19 |
| [include/castoria_name_theft.h](../../../../include/castoria_name_theft.h) | Custom added | 114 / 0 | 1 |
| [include/config/battle.h](../../../../include/config/battle.h) | Upstream path modified | 3 / 3 | 3 |
| [include/config/general.h](../../../../include/config/general.h) | Upstream path modified | 1 / 1 | 1 |
| [include/config/item.h](../../../../include/config/item.h) | Upstream path modified | 3 / 3 | 3 |
| [include/constants/abilities.h](../../../../include/constants/abilities.h) | Upstream path modified | 4 / 1 | 1 |
| [include/constants/battle_end_turn.h](../../../../include/constants/battle_end_turn.h) | Upstream path modified | 5 / 0 | 1 |
| [include/constants/battle_frontier.h](../../../../include/constants/battle_frontier.h) | Upstream path modified | 1 / 1 | 1 |
| [include/constants/battle_move_effects.h](../../../../include/constants/battle_move_effects.h) | Upstream path modified | 6 / 0 | 1 |
| [include/constants/battle_script_commands.h](../../../../include/constants/battle_script_commands.h) | Upstream path modified | 2 / 0 | 2 |
| [include/constants/battle_string_ids.h](../../../../include/constants/battle_string_ids.h) | Upstream path modified | 68 / 0 | 3 |
| [include/constants/battle.h](../../../../include/constants/battle.h) | Upstream path modified | 11 / 2 | 2 |
| [include/constants/flags.h](../../../../include/constants/flags.h) | Upstream path modified | 7 / 7 | 2 |
| [include/constants/hold_effects.h](../../../../include/constants/hold_effects.h) | Upstream path modified | 1 / 0 | 1 |
| [include/constants/items.h](../../../../include/constants/items.h) | Upstream path modified | 3 / 1 | 1 |
| [include/constants/moves.h](../../../../include/constants/moves.h) | Upstream path modified | 18 / 2 | 2 |
| [include/constants/pokemon.h](../../../../include/constants/pokemon.h) | Upstream path modified | 6 / 2 | 2 |
| [include/constants/species.h](../../../../include/constants/species.h) | Upstream path modified | 8 / 1 | 1 |
| [include/constants/trainer_slide.h](../../../../include/constants/trainer_slide.h) | Upstream path modified | 1 / 0 | 1 |
| [include/constants/vars.h](../../../../include/constants/vars.h) | Upstream path modified | 3 / 2 | 3 |
| [include/item_use.h](../../../../include/item_use.h) | Upstream path modified | 1 / 0 | 1 |
| [include/kazuradrop_battle_ui.h](../../../../include/kazuradrop_battle_ui.h) | Custom added | 60 / 0 | 1 |
| [include/kazuradrop_entrance.h](../../../../include/kazuradrop_entrance.h) | Custom added | 13 / 0 | 1 |
| [include/kazuradrop_gameover.h](../../../../include/kazuradrop_gameover.h) | Custom added | 8 / 0 | 1 |
| [include/pokemon.h](../../../../include/pokemon.h) | Upstream path modified | 3 / 0 | 2 |
| [include/random.h](../../../../include/random.h) | Upstream path modified | 5 / 0 | 2 |
| [include/servant_sigil.h](../../../../include/servant_sigil.h) | Custom added | 16 / 0 | 1 |
| [include/sprite.h](../../../../include/sprite.h) | Upstream path modified | 1 / 0 | 1 |
| [include/trainer_break_bars.h](../../../../include/trainer_break_bars.h) | Custom added | 48 / 0 | 1 |
| [src/battle_ai_main.c](../../../../src/battle_ai_main.c) | Upstream path modified | 587 / 3 | 20 |
| [src/battle_ai_script_commands.c](../../../../src/battle_ai_script_commands.c) | Upstream path modified | 4 / 0 | 1 |
| [src/battle_ai_switch_items.c](../../../../src/battle_ai_switch_items.c) | Upstream path modified | 2 / 0 | 1 |
| [src/battle_ai_util.c](../../../../src/battle_ai_util.c) | Upstream path modified | 60 / 12 | 17 |
| [src/battle_anim_effects_2.c](../../../../src/battle_anim_effects_2.c) | Upstream path modified | 55 / 0 | 2 |
| [src/battle_anim_effects_3.c](../../../../src/battle_anim_effects_3.c) | Upstream path modified | 64 / 0 | 2 |
| [src/battle_anim_mons.c](../../../../src/battle_anim_mons.c) | Upstream path modified | 3 / 0 | 1 |
| [src/battle_anim.c](../../../../src/battle_anim.c) | Upstream path modified | 22 / 1 | 4 |
| [src/battle_bg.c](../../../../src/battle_bg.c) | Upstream path modified | 19 / 4 | 5 |
| [src/battle_break_bar.c](../../../../src/battle_break_bar.c) | Custom added | 635 / 0 | 1 |
| [src/battle_controller_opponent.c](../../../../src/battle_controller_opponent.c) | Upstream path modified | 14 / 3 | 3 |
| [src/battle_controller_player_partner.c](../../../../src/battle_controller_player_partner.c) | Upstream path modified | 1 / 2 | 2 |
| [src/battle_controller_player.c](../../../../src/battle_controller_player.c) | Upstream path modified | 51 / 1 | 9 |
| [src/battle_controllers.c](../../../../src/battle_controllers.c) | Upstream path modified | 54 / 2 | 11 |
| [src/battle_dynamax.c](../../../../src/battle_dynamax.c) | Upstream path modified | 20 / 0 | 4 |
| [src/battle_end_turn.c](../../../../src/battle_end_turn.c) | Upstream path modified | 241 / 0 | 6 |
| [src/battle_gfx_sfx_util.c](../../../../src/battle_gfx_sfx_util.c) | Upstream path modified | 12 / 0 | 2 |
| [src/battle_hold_effects.c](../../../../src/battle_hold_effects.c) | Upstream path modified | 12 / 0 | 1 |
| [src/battle_interface.c](../../../../src/battle_interface.c) | Upstream path modified | 125 / 24 | 26 |
| [src/battle_koanko.c](../../../../src/battle_koanko.c) | Custom added | 144 / 0 | 1 |
| [src/battle_main.c](../../../../src/battle_main.c) | Upstream path modified | 243 / 14 | 37 |
| [src/battle_message.c](../../../../src/battle_message.c) | Upstream path modified | 65 / 1 | 5 |
| [src/battle_reduction.c](../../../../src/battle_reduction.c) | Custom added | 340 / 0 | 1 |
| [src/battle_script_commands.c](../../../../src/battle_script_commands.c) | Upstream path modified | 1000 / 84 | 104 |
| [src/battle_setup.c](../../../../src/battle_setup.c) | Upstream path modified | 12 / 6 | 9 |
| [src/battle_status_icons.c](../../../../src/battle_status_icons.c) | Custom added | 291 / 0 | 1 |
| [src/battle_tx_rollback.c](../../../../src/battle_tx_rollback.c) | Custom added | 341 / 0 | 1 |
| [src/battle_tx_wishes.c](../../../../src/battle_tx_wishes.c) | Custom added | 412 / 0 | 1 |
| [src/battle_util.c](../../../../src/battle_util.c) | Upstream path modified | 359 / 30 | 49 |
| [src/battle_util2.c](../../../../src/battle_util2.c) | Upstream path modified | 10 / 0 | 4 |
| [src/castoria_name_theft.c](../../../../src/castoria_name_theft.c) | Custom added | 1500 / 0 | 1 |
| [src/data/abilities.h](../../../../src/data/abilities.h) | Upstream path modified | 24 / 0 | 1 |
| [src/data/battle_move_effects.h](../../../../src/data/battle_move_effects.h) | Upstream path modified | 37 / 1 | 3 |
| [src/data/hold_effects.h](../../../../src/data/hold_effects.h) | Upstream path modified | 5 / 0 | 1 |
| [src/data/items.h](../../../../src/data/items.h) | Upstream path modified | 15 / 1 | 2 |
| [src/data/koanko_party.h](../../../../src/data/koanko_party.h) | Custom added | 104 / 0 | 1 |
| [src/data/moves_info.h](../../../../src/data/moves_info.h) | Upstream path modified | 311 / 0 | 3 |
| [src/data/pokemon/Castoria.h](../../../../src/data/pokemon/Castoria.h) | Custom added | 0 / 0 | 0 |
| [src/data/pokemon/experience_tables.h](../../../../src/data/pokemon/experience_tables.h) | Upstream path modified | 49 / 1 | 10 |
| [src/data/pokemon/species_info.h](../../../../src/data/pokemon/species_info.h) | Upstream path modified | 180 / 0 | 2 |
| [src/data/pokemon/species_info/gen_3_families.h](../../../../src/data/pokemon/species_info/gen_3_families.h) | Upstream path modified | 89 / 1 | 2 |
| [src/data/trainer_break_bars.h](../../../../src/data/trainer_break_bars.h) | Custom added | 41 / 0 | 1 |
| [src/data/trainer_parties.h](../../../../src/data/trainer_parties.h) | Upstream path modified | 11851 / 0 | 2 |
| [src/data/types_info.h](../../../../src/data/types_info.h) | Upstream path modified | 57 / 22 | 3 |
| [src/item_use.c](../../../../src/item_use.c) | Upstream path modified | 14 / 0 | 2 |
| [src/kazuradrop_battle_ui.c](../../../../src/kazuradrop_battle_ui.c) | Custom added | 616 / 0 | 1 |
| [src/kazuradrop_entrance.c](../../../../src/kazuradrop_entrance.c) | Custom added | 211 / 0 | 1 |
| [src/kazuradrop_gameover.c](../../../../src/kazuradrop_gameover.c) | Custom added | 545 / 0 | 1 |
| [src/party_menu.c](../../../../src/party_menu.c) | Upstream path modified | 46 / 7 | 4 |
| [src/pokeball.c](../../../../src/pokeball.c) | Upstream path modified | 6 / 0 | 2 |
| [src/pokemon_summary_screen.c](../../../../src/pokemon_summary_screen.c) | Upstream path modified | 6 / 1 | 3 |
| [src/pokemon.c](../../../../src/pokemon.c) | Upstream path modified | 45 / 4 | 13 |
| [src/post_battle_event_funcs.c](../../../../src/post_battle_event_funcs.c) | Upstream path modified | 3 / 2 | 3 |
| [src/reshow_battle_screen.c](../../../../src/reshow_battle_screen.c) | Upstream path modified | 29 / 3 | 8 |
| [src/servant_sigil.c](../../../../src/servant_sigil.c) | Custom added | 178 / 0 | 1 |
| [src/sprite.c](../../../../src/sprite.c) | Upstream path modified | 83 / 13 | 12 |
| [src/trainer_break_bars.c](../../../../src/trainer_break_bars.c) | Custom added | 68 / 0 | 1 |
| [src/trainer_slide.c](../../../../src/trainer_slide.c) | Upstream path modified | 24 / 0 | 4 |

## Appendix B. Reproducible read-only evidence

Use the recorded SHA values rather than moving refs when repeating the review. Relevant operations were ancestry queries (`merge-base`, `rev-list`, `--is-ancestor`), committed source/history reads (`show`, `log --no-merges --unified=0`), current/custom and release-tree diffs, remote reference reads and code search. The exposure calculations use U0 hunk ranges and named contexts as defined in Section2. They do not replay the unported custom patch independently against each later release and call those results cumulative conflicts.

Incremental predictions deliberately do **not** construct hypothetical post-resolution branches. Actual later-step conflicts depend on how earlier custom integrations are adapted. Reported repeated exposure is therefore an observed need to reassess integrations, not a simulated conflict total. Module extraction benefit is qualitative until an approved implementation exists.

Delivery checks confirmed that all13 existing Chapters00–12 and the aggregate fingerprint of2,521 tracked source/script/header files match their review-start snapshots. The new document's local links resolve; it has no unfilled table markers, replacement characters or trailing whitespace. HEAD and the stale upstream tracking ref retain their original values. No existing plan is backported, and no implementation, migration, upgrade, commit or push is part of this deliverable.

