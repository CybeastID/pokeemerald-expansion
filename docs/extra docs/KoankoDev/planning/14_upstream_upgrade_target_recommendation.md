# Chapter 14 — Upstream upgrade target recommendation

Date: **2026-11-10**. Status: **research complete; upgrade and migration implementation not authorized**.

Implementation update, **2026-11-11**: the user has now authorized the migration,
including builds and tests. Implementation is underway on `upgrade-branch` in
an isolated worktree. The historical assessment below retains its research
provenance; the current execution record is in Section 11. Save compatibility
has been waived for this migration, so old numeric IDs are not preserved.

## 1. Decision

Recommend **rh-hideout/pokeemerald-expansion, tag `refs/tags/expansion/1.17.2`, commit `79dd436920845ebc145577649de624f65298c209`** as the upgrade destination. Complete the upgrade and its compatibility gates **before Stage 05**. Provisionally use **1.16.4 as one migration/validation bridge**, subject to the conversion-input audit below. Do not stop at every release.

This target supplies trainer-owned party infrastructure, controller-based AI recognition, shared calculation improvements, targeting fixes and two rounds of 1.17 patch fixes. Those are useful foundations for virtual Meteo and the later independent Zacian. It also entails substantial manual HP, AI, ownership and script reconciliation. This is a high-risk engine migration, not a routine dependency update.

The recommendation selects the released tree exactly. It does **not** silently include later master fixes, enable Champions rules, approve outstanding encounter defaults, or authorize any repository operation. Known post-release fixes are identified in Section 4 and must be dispositioned before upgraded gameplay is accepted.

## 2. Evidence, scope and what changed since the heat map

This report builds on [Chapter 12](12_engine_architecture_comparison.md), [Chapter 13](13_upstream_merge_minimization_review.md), [the centralized ledger](../../../mechanics/upstream_modifications.md), repository `AGENTS.md`, [Stage 05](05_meteo_and_tactical_ai.md), and [the Stage 04 handoff](../STAGE04_IMPLEMENTATION_AND_CODEFLOW.md). Their architectural analysis is reused rather than repeated. Current source and exact candidate trees were inspected to check target-specific consequences.

The research snapshot is branch **`mypatch`**, HEAD **`582c5f50155753295f0f5c5b81a1eddab0690a1e`**, plus its pre-existing working-tree edits and untracked implementation files. HEAD alone does not reproduce Stage 04. In particular, `src/battle_tx_health.c`, `test/battle/tx_profiles.c` and `test/battle/wild_break_bars.c` already existed untracked at research start and belong to the migration input.

Chapter 13 used HEAD `e48e6c57735a3dba567629644e6b1a6a6079f762`. Six later fork commits and existing uncommitted work materially update that snapshot:

- Stage 03 sampling and its shared slot-0 evaluator are implemented. `BattleAI_ScoreSlot0Candidate`, signed selection, limitation/cache cleanup and layer-aware KO reasoning are existing behavior to port, not merely future A1/A2 work.
- `battle_koanko_ai.c` provides encounter-scoped roster tactics. `BattleAI_SetupFlags` has a registration hook; CHECK_VIABILITY/SMART_TERA approval is now recorded in the ledger. Older statements that these flags are absent are outdated.
- `battle_gimmick.c` now contains the party-index eligibility helper used by Tera reasoning. Chapter 13's assertion that this file is unchanged no longer holds.
- Stage 04 adds explicit summoned/solo TX profiles, owner-aware Wish effects, 100/60/30% summoned maxHP and controller normalization. Its handoff still records pending user-run compilation/runtime validation.
- Wild Break Bar assignment, capture rejection and persistent covered-layer presentation add script, initialization, capture and PartyState responsibilities.
- Polymorph has newer candidate/normalization and test-harness work. Preserve its current implementation rather than reconstructing the older planning snapshot.

No builds, tests, migration scripts, merge simulations or upgrade operations were executed. Static inspection cannot prove upgraded correctness or actual conflict counts. Historical passing results in the ledger remain historical; this report does not certify the dirty source snapshot.

Network evidence was available: live `git ls-remote` verified upstream branches/tags, and GitHub's [latest release endpoint](https://github.com/rh-hideout/pokeemerald-expansion/releases/latest) resolved to [1.17.2](https://github.com/rh-hideout/pokeemerald-expansion/releases/tag/expansion/1.17.2). The initial sandboxed remote read failed; the authorized escalated read succeeded. Existing release objects were already local. The two current development tips were downloaded using `git fetch --no-write-fetch-head --no-tags upstream <SHA> <SHA>` for inspection, without updating branches, tracking refs, tags or FETCH_HEAD. Git object metadata is the only fetched material; the only authored deliverable is this Markdown file.

## 3. Actual origin and current synchronization point

### Original divergence

The engine upstream is **[rh-hideout/pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion)**, configured as both `upstream` and `RHH`. `origin` is **CybeastID/pokeemerald-expansion**. The historical ghoulslash remote is an additional overworld source, not the engine release upstream.

The first identifiable fork-authored commit on the initial custom lineage is **`26b9f02b848f7a5b9eb7453baec9373c1dc730a4`**, 2024-01-09, “Added map and single trainer.” Its sole parent is **`396f2c9565082757361bb26e5c63a40fdb3c9fd8`**, 2024-01-06, “some pledge combo fixes (#3934).” This parent is the verified original divergence revision. It is a development commit; no release number is inferred for it. The next fork commit adds Divine and Authority of Anri. The later `a8fbbd0a42d1a7385837c34caf71fd72dde7071e` merge imports ghoulslash's overworld-expansion history, explaining why all commits unique to the fork are not custom battle commits.

Local `master`, `upstream/master` and `origin/master` still point to that 2024 revision. They are stale and unsuitable as the current customization base.

### Latest incorporated upstream release

The current common ancestor with **both** live upstream master and upcoming is **`64d40d3c1464011d8642d280b622f3e7fa27c5d3`**, tag **`expansion/1.14.2`**, 2026-01-02. It is also the merge base with the selected target.

The explicit synchronization commit is **`fe8cdd1aaec623bc4c69efed9d3daa63b7a0c419`**, 2026-01-02, whose second parent is exactly that release commit. Earlier first-parent merges show 1.9.2, 1.10.0, 1.11.x, 1.12.3, 1.13.4 and 1.14.1 integrations. Thus original divergence and latest synchronization are distinct facts, not competing guesses.

No later full upstream synchronization is visible in ancestry. Isolated copied fixes cannot be ruled out by ancestry alone. The repository is not shallow. HEAD and live master have **147 / 1,756** commits respectively outside their common ancestry; these are graph counts, including imported side history, not estimates of porting effort.

## 4. Candidate comparison and release selection

All SHA values below were checked against live remote refs and local objects. Counts are commits reachable from the candidate but not from the 1.14.2 baseline, including merges. They indicate scale and inclusion, not feature quality or conflict totals.

### 1.15.3 — useful diagnostic checkpoint, insufficient destination

`refs/tags/expansion/1.15.3` → **`b669d9ccf9c148070b99eba11cd65f6071be0741`**, 2026-05-29; **892** commits after baseline.

The 1.15 family supplies switch-in event refactoring, controller-based AI recognition, split switch/item AI, improved survival/KO and Wish-recipient logic, damage rounding fixes and richer test controls. It already requires substantial migration of the existing battle hooks. It lacks trainer-owned 12v12 infrastructure and later calculation/targeting work; building Stage 05 here postpones another AI port and builds Stage 07 against the older owner model. Use only if separating the first large switch-in/AI transition has a concrete diagnostic benefit. See [1.15.0 notes at the selected tree](https://github.com/rh-hideout/pokeemerald-expansion/blob/79dd436920845ebc145577649de624f65298c209/docs/changelogs/1.15.x/1.15.0.md).

### 1.16.4 — compelling bridge and fallback, not the preferred final target

`refs/tags/expansion/1.16.4` → **`3796c72237d2ebf0361c6f4a1b4be551d8858904`**, 2026-08-31; **1,476** commits after baseline.

This is the strongest intermediate candidate: trainer-owned parties/PartyState/gimmicks and separate graphics lifetime are already available. AI prediction, damage-context bridging and speed work benefit sampling and future virtual-action evaluation. It includes late fixes to multi-party collisions, replacement handling and battle graphics. It provides a place to inspect the owner migration before remaining deprecated interfaces disappear.

It is not a cheap escape from the port: **80 of Chapter 13's 81 inventoried upstream paths change by this target**, the same union size as 1.17.2, though the actual edits differ. Stopping here forgoes the final targeting/effect interfaces and subsequent patch fixes; Stage 05 would still face another integration round. Several fixes also appear in the later feature changelog, so do not double-count them as unique 1.17 benefits. See [1.16.0](https://github.com/rh-hideout/pokeemerald-expansion/blob/79dd436920845ebc145577649de624f65298c209/docs/changelogs/1.16.x/1.16.0.md) and [1.16.4](https://github.com/rh-hideout/pokeemerald-expansion/blob/79dd436920845ebc145577649de624f65298c209/docs/changelogs/1.16.x/1.16.4.md).

### 1.17.2 — selected released destination

`refs/tags/expansion/1.17.2` → **`79dd436920845ebc145577649de624f65298c209`**, 2026-10-31; **1,737** commits after baseline.

Prefer this over initial 1.17.0 (`e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7`) and 1.17.1 (`bb1093a7c72d3961d9a37d8e02ad2abc2c866682`): it retains their new interfaces and adds patch fixes rather than requiring an immediate second upgrade. The 1.17 patches address multi-battle return/outcome and bounds problems, recorded-battle behavior, AI terrain access, and 2v1 healthboxes. These matter more to this fork than merely obtaining more content. Released status gives a reproducible review boundary; it does not guarantee this fork or every upstream mechanic passes.

### Current master — credible fix-bearing alternative, not selected automatically

Live `refs/heads/master` → **`31df832cc7fc3aa890bf3cd44f2df4baea369c23`**, 2026-11-10; **1,756** commits after baseline, **19** after 1.17.2. The new cycle begins at `ad8dc98b7b` (“Start of 1.17.3 cycle”). Chapter 13's `be3b089fc1` master snapshot is two commits behind this live tip.

This alternative deserves consideration: the release-to-master diff is only 30 paths (+520/−268), but includes meaningful behavior changes:

- **`be3b089fc121143b7ccf6503ea1385b8abe05b8c`** fixes terrain scoring and Grassy Terrain damage applicability. Inspection shows corrected terrain-enum calls in AI and `IsGrassyTerrainAffected` in the Earthquake/Magnitude power modifier, with a new terrain regression fixture.
- **`8b10030b4c42f2b481567bcf43eb46eb64679d93`** fixes pivot/phazing activation and uses owner-aware party sharing in forced-switch selection. This is directly relevant to ordinary switching, No Order Change guards and later auxiliary ownership.
- Other changes fix controller checks, move-selection messages, multi-battle level-up ownership, Mimicry, Cud Chew and Master Ball capture of Ultra Beasts (`11533539eb`).

The restricted battle/header/script portion alone changes ten paths (+176/−207), including the hottest custom interfaces. This is not a zero-risk “stable plus harmless cleanup” target. **Prefer the released tag as the initial migration baseline**, then explicitly triage these known fixes in a separately authorized correction milestone. At minimum reproduce/disposition the terrain and pivot/phazing defects before Stage 05 tactical acceptance; do not describe the unpatched tag as free of those defects. If they block acceptance and upstream publishes a new patch release before implementation, re-evaluate that release then. If a master-based target is approved instead, pin the full SHA above and revise the target-specific plan; do not track moving master implicitly.

### Upcoming — useful preview, excessive additional scope

Live `refs/heads/upcoming` → **`83dc0468d65c6cef958eeca223b1ff4ea4a8c3f8`**, 2026-11-10; **1,894** commits after baseline. Compared with master, the tips have **4 master-only / 142 upcoming-only** commits: upcoming is not simply “master plus features” and does not contain every newest master fix.

Interesting changes include deeper partner prediction, dynamic mon-selection AI, more owner/party cleanup, status-effect refactors and gimmick checks. They add another unsettled integration surface across AI, end-turn and effect dispatch, plus unrelated features such as mining. They are not prerequisites for Meteo or independent Zacian. Reject as the current destination.

## 5. Worthwhile capabilities and fixes

These are verified in candidate source/history and release-family notes. Feature notes sometimes repeat work already backported to late patch releases; inclusion must be checked by ancestry/tree, not heading alone.

1. **Ownership foundation:** 1.16's trainer-party model maps `GetBattlerParty` to `gParties[GetBattlerTrainer(battler)]`, and `GetBattlerPartyState` to trainer/local-slot state. Controller-based `BattlerHasAi` recognizes the player-partner controller. This substantially improves the foundation for Stage 07 without adding a competing general owner framework. It does not implement dynamic ally entry, moves-only permissions, player-loss authority or safe return to singles.
2. **Shared AI/calculation work:** 1.15 improves damage rounding, endure reasoning, switch-in simulation and recovery utility; 1.16 adds engine/AI context bridging, contextual damage rolls and speed improvements. At 1.17.2, `AI_CalcDamage(struct AiCalcValues *, ...)` carries weather/terrain/gimmick context, and `CalcBattlerAiMovesData` is public with explicit weather/terrain arguments. The 1.17 family improves calculation cost, accuracy, recoil decisions, stat-effect scoring and crit consideration. These are valuable for a 60-candidate sampler, but no latency improvement for this fork has been measured.
3. **Execution correctness and reusable seams:** target selection is separated from execution resolution; effect dispatch moves to `src/battle_set_effect.c`; damage and HP execution move into cancellation/resolution phases. This supplies the actual framework in which Stage 05 spread targets and once-per-action success must be implemented. Reusing it is preferable to implementing against soon-obsolete script command timing.
4. **Data/mechanics:** later species/forms and items include Legends Z-A: Mega Dimension content; new species metadata, learnsets, cries and animation corrections are available. The 1.17 family adds Champions data and abilities such as Fire Mane/Eelevate, with further regulation data in 1.17.1. **Retain this fork's `GEN_LATEST GEN_9` behavior policy** unless separately changed: data availability is not permission to adopt Champions balance or messages indiscriminately. Preserve boss kits, custom Divine/Stellar interactions and access restrictions.
5. **Harness/build maintenance:** richer tie-break, expected-failure and capture-odds assertions; multi-party fixtures and AI internal configs; thinking-time isolation; item-popup assertions; callback/inventory isolation fixes and safer runner output. Makefile, preprocessing, scaninc, trainerproc and graphics rules also change. 1.17.1 fixes generational-data compilation issues. Accept supported toolchain changes deliberately, preserving this fork's graphics/audio and trainer generation inputs. No build portability, ROM capacity or memory headroom claim is established here.

## 6. Updated conflict assessment using the existing heat map

**Overall: high textual reconciliation risk; very high semantic risk until validated.** Primary drivers are relocated HP authority, changed owner identity, changed AI context and reused numeric IDs. The difficulty is not explained by file counts alone.

### Measured continuity and new exposure

Rechecking Chapter 13's fixed 81-path upstream inventory against baseline-to-release trees gives **76 exposed paths at 1.15.3, 80 at 1.16.4, and 80 at 1.17.2**. These comparisons explicitly disable rename detection so deleted old paths remain visible. Default rename presentation can report 75/79/79 by replacing an old path with its destination. Neither rendering is a different gameplay risk.

The inherited 137 function-context exposure model and route counts remain useful **historical lower-bound planning evidence**, not current counts. New sampling/scoring/Tera/Stage04/wild-bar hooks were not all in that model. No full heat-map census was repeated.

A focused current-worktree U0 overlap refresh against 1.17.2 uses Chapter 13's strict rule: shared removed/base lines, or two insertions at the same anchor. Count each custom hunk once if any upstream hunk qualifies. Adjacent edits and insertion-versus-replacement semantics are excluded. Results, expressed as current custom hunks / overlap candidates:

- `battle_script_commands.c`: **106 / 43**; `battle_util.c`: **52 / 15**.
- `battle_ai_main.c`: **54 / 17**; `battle_ai_util.c`: **38 / 11**. Chapter 13 had 20/6 and 17/5 respectively: recent shared AI work noticeably increases this port surface.
- `battle_main.c`: **38 / 11**; `include/battle.h`: **19 / 5**; `battle_interface.c`: **26 / 11**.
- `battle_controllers.c`: **11 / 0**; `battle_end_turn.c`: **6 / 1**; newly modified `battle_gimmick.c`: **4 / 3**.

These are **overlap candidates, not Git conflict counts**, and omit important new destination files. No merge was performed to manufacture a conflict total. In particular, zero controller overlap does not protect serialization identity or effective HP correctness.

### Highest-risk manual reconciliation

**HP, bars, saved layers and Break Charms — very high semantic / high textual.** The old `Cmd_adjustdamage`, `MoveDamageDataHpUpdate` and `PassiveDataHpUpdate` responsibilities cannot simply be restored wholesale. The target has `CancelerDamageCalc`, `TryMoveDamageUpdate` and `CancelerMoveDamageUpdate` in **`src/battle_move_resolution.c`**, while `Cmd_healthbarupdate`/`Cmd_datahpupdate` operate through `passiveHpUpdate`. Inspection finds direct battle-HP zeroing and controller emission in the new move path. Port bar interception before authoritative HP/faint/reward decisions and preview, preserve original versus saved restoration and multihit overkill isolation, and audit residual/instant/direct-zero paths independently. The new file is a crucial semantic collision absent from a same-filename inventory. Capture rejection must precede guaranteed-capture handling and preserve Ball consumption policy.

**AI and virtual actions — very high semantic / high textual.** Remap the shared slot-0 evaluator and scratch restoration onto the target's `DoAIScoreProcessing`, `AiCalcValues`, explicit terrain and changed return envelope (`ChosenAction` contains moveIndex/target). Preserve signed scores, ordinary-over-virtual ties, limitations, forced-action behavior, production dynamic callbacks and no four-slot array indexing by virtual slot 4/5. The target still sets/clears `aiCalcInProgress` directly, including an early return; existing nested simulation restoration must not be lost. Recheck all KO/survival callers for bars rather than porting one old helper and assuming coverage. RNG order changes affect recordings, sampling and deterministic fixtures. Upstream information/prediction flags must not silently grant Koanko extra knowledge.

**TX/Castoria ownership and lifecycle — very high semantic / moderate-to-high textual.** Stage04's `ownerSide/partyIndex` identity must be translated to trainer/local slot wherever state is actor-owned. Audit combat profile, saved layers, Reciprocity recipients, Wish commitments, AI history, active gimmicks and canonical party HP; side-wide Reduction/field effects retain side semantics. Trainer resources remain separate from positions. Keep summon formation and solo combat capability separate. Rebind only at legitimate initialization, survive reshow, clear owner-local effects on genuine defeat and full battle cleanup, and preserve global Reduction/ordinary delayed attacks as documented. No clean compile proves these identities are correct.

**Reduction, Polymorph and stat normalization — very high semantic / high shared-path exposure.** `CopyMonLevelAndBaseStatsToBattleMon` gains an `updateSpeedStat` argument; switch-in cleanup uses a volatile snapshot. Port clean form → Wishful non-HP growth → TX maxHP overlay → Reduction in that order and inverse controller normalization in the matching order. Preserve legal copied/mutated stats, HP proportions, spent PP and canonical party data. Generic form refresh is not equivalent to battle-only effective state. Castoria's copied identity and Reduction baselines must use the correct trainer owner after the party refactor.

**Kazuradrop, Bug Space and transformations — high semantic / high script/ability exposure.** Reattach ability entry after ordinary suppression and switch-in event setup; preserve separate end-turn decay/crushing cursors and bar barriers. End-turn scripts migrate toward call/return, while 1.17 removes `end2`; native continuations, effect tables and animation loading change. Keep Kazura's established real-slot reread behavior when Quiver Dance becomes Infinite Growth before a slower action executes. A global committed-move freeze introduced during the port would change gameplay despite apparently cleaner action metadata. Polymorph/Trash Crush/grayscale tasks and global affine sprite modifications need actual resource/visual checks, not automatic upstream replacement.

**Healthboxes/controllers/resources — high semantic / mixed textual.** Reconcile percentage HP and 2v1 fixes with bar overlays, numeric effective maxHP, TX asymmetric layout, status icons and transformed names/graphics. The target separates graphics allocation/free from full battle allocation/free. Custom combat state must survive Bag/party/summary reconstruction; graphics resources must be recreated and freed at their own boundaries. Keep controller request/mask/record layouts consistent with trainer ownership and TX canonical writeback. Stable numeric display alone does not establish correct persistent HP.

**Koanko encounter and future Zacian — high semantic.** Keep one uninterrupted twelve-opponent battle and in-place party replacement, ordinary first-six switching, conjured fixed-order/No Order Change, real-party item/revival/loss rules, per-KO EXP and final-only rewards. Clear only opponent Tera usage once; preserve other trainer resources. Upstream 12v12 is infrastructure for multiple owners, not permission to convert this encounter into two trainers or reset resources. Later Zacian still needs explicit action permissions, no player inventory/gimmick access, no extra action in an already chosen queue, and departure before Dialga. Optional Calyrex Tera and active-state preservation remain mandatory contracts.

### Additional blocking data compatibility issue

Current `include/constants/species.h` places custom species immediately after Falinks Mega (1549): **TX 1550, Castoria 1551, Raze Kirlia 1552, Kazuradrop 1553**. In the selected upstream tree, **1550–1553 mean Heatran Mega, Darkrai Mega, Zeraora Mega and Raichu Mega X**. This is a verified numeric collision, not hypothetical churn.

Moving the custom block to the new `SPECIES_CUSTOM_START` area without preserving values would silently reinterpret existing saved mons. A future port must record a complete species/move/item/ability/type ID manifest, protect current persisted custom IDs or implement an explicitly approved conversion, and reconcile upstream additions around that policy. Existing docs' “battle-only state, no save-format changes” cannot waive this broader upgrade issue. Config inspection finds DexNav search levels disabled and first-time item descriptions off, reducing two documented array-growth hazards; other save-size/ID risks remain unaudited. Trainer graphics enums, trainer IDs, recorded-battle structure changes and generated party inputs need equivalent review. No save or recording compatibility is promised.

## 7. What can be adopted with comparatively little customization

- Keep supported upstream controller-based AI recognition, trainer-party accessors, split `battle_ai_switch.c`/`battle_ai_items.c`, effect table dispatch and graphics lifecycle infrastructure. Put custom policy behind their authoritative seams instead of resurrecting deleted files (`battle_ai_switch_items.c`, `battle_ai_script_commands.c`).
- Adopt independent upstream fixes to ordinary abilities/items, bounds checking and runner diagnostics once corresponding custom overrides and configuration are reviewed. They need regressions, but usually do not need new bespoke algorithms.
- Reuse upstream weather/terrain/damage APIs and cache builders for sampled and virtual valuation. Keep encounter utility and candidate enumeration in custom modules; do not copy an older damage or scoring engine into them.
- Retain custom Break Bar, Reduction, TX, Koanko and presentation modules as implementation owners. Their lack of a filename collision lowers add/add exposure, but all external API/state assumptions still need adaptation.

“Minimal modification” describes the intended resulting upstream footprint, not proof these changes merge cleanly. Text relocations should be distinguishable from behavioral adaptations. Avoid an unrelated general callback bus, scheduler, global epoch framework or universal owner registry during the release port. This follows Chapter 13's merge-maintenance recommendation and limits future repeated adaptation without sacrificing HP or owner correctness.

## 8. Recommended migration route and ordering

### Why before Stage 05

Stage 05 adds offensive virtual actions, spread-target execution, ordinary-versus-Wish/Meteo valuation, committed recipients and once-per-action cooldown accounting. The selected release changes precisely those scoring, target, damage and move-end seams. Implementing now would increase the patch inside the hottest files and require proving tactical behavior again after the port. Later Zacian would additionally create a branch-local ownership model that upstream has already superseded.

First preserve and obtain the outstanding Stage04 baseline evidence. Then upgrade, establish compatibility, disposition known upstream defects, and implement Stage05 using the selected interfaces. Documentation and migration-input inventory can proceed before those runtime results; editing the tested source snapshot cannot.

### Fewest useful checkpoints

Prefer **1.14.2-based fork → 1.16.4 bridge → 1.17.2 destination**, if the bridge actually enables conversion inspection, deprecation diagnostics or focused ownership validation. It isolates the final targeting/HP/effect transition from earlier owner and graphics work; it does not eliminate either.

Chapter 13's fixed historical model gives direct **80 file-step / 137 function-step** exposures versus **147 / 200** through 1.16.4, **215 / 283** with an added 1.15.3 stop, and **469 / 423** for every release. These are inherited attention proxies, not current conflict totals, time estimates or measured safety improvements. Recent shared AI edits increase the need to revisit this model during actual implementation.

Incremental migration meaningfully reduces diagnostic scope **only when each checkpoint has a defined validation result or required input conversion**. If the input audit proves conversions can be applied in order from preserved old files and combined validation is tractable, prefer a direct port to reduce duplicate integration rounds. Add 1.15.3 only if switch-in/AI migration cannot be diagnosed alongside ownership. Do not require fourteen intermediate upgrades or claim direct is proven cheaper.

### Future implementation milestones — no actions authorized here

1. **Freeze a complete baseline.** Preserve HEAD plus dirty/untracked source, data, scripts, tests and configuration in an authorized immutable snapshot or separate worktree. Record exact ID manifest, config values, known test failures, save/recording policy and resource baseline. Resolve Stage04's pending verification before treating it as the control. Do not port only the committed tree and lose the health module.
2. **Audit conversion inputs before replacement.** Read release notes and each applicable script at the selected SHA. Preserve old trainer graphics tables, `.party` authoring inputs, custom learnsets, scripts and animation assets. `migration_scripts/1.16/trainer_pic_migration/main.py` parses the old `gTrainerSprites[]` table from `src/data/graphics/trainers.h` and emits snippets requiring manual placement; running it after that shape is replaced is not equivalent. Do not regenerate over hand-authored trainer data blindly.
3. **Plan schema conversions in dependency order.** Review 1.15 trainer ball/givemon changes, INC graphics and text-printer migrations, learnset JSON generation and implicit waitstate removal; then 1.16 trainer pictures/regional dex and animation loading; then 1.17 money/coinbox arguments. Use the actual paths under the target's `migration_scripts/`, not stale paths in prose. For example, the 1.16 notes mention an older remove-loadspritegfx location, while the selected tree contains `migration_scripts/1.16/remove_loadspritegfx.py`. Removing `loadspritegfx` can remove an implicit delay; preserve custom animation timing explicitly. Review each script's scope/idempotence; none was executed here.
4. **Reconcile owners and lifecycle.** Migrate core party/PartyState/history/gimmick consumers and custom state identities against the upstream owner model, keeping side policy distinct. Validate ordinary multi/partner and Castoria summon/solo behavior before enabling any new ally feature. Keep graphics-only close separate from combat teardown.
5. **Reconcile HP/execution and stat overlays.** Port bar interception into the new resolution file and passive path, then faint/reward/queue boundaries, canonical writes and clean-copy ordering. Move custom effects to the new dispatcher and review call/return scripts. This milestone needs all-source HP evidence, not just a successful damaging move.
6. **Reconcile AI and existing sampling.** Adapt the current shared evaluator, restoration and encounter registration to target APIs; retain signed comparisons, bar-outcome logic, production flags and exact tie policy. Validate ordinary AI, sampling, Castoria and TX execution before adding Meteo. Keep upstream bug fixes distinct from tuning changes and fixture repairs.
7. **Complete presentation/data/toolchain gates.** Reconcile IDs and generated registries, build from clean authorized outputs, inspect original/copy forms and overlays through menu reconstruction, and measure memory/latency. Identify baseline versus introduced failures by exact names/signatures. The existing low IWRAM margin and new upstream data/party storage make old resource results insufficient.
8. **Accept the pinned destination before Stage05.** Record conversion decisions, known master-fix dispositions, test evidence and remaining limitations. Update existing implementation documentation and the central ledger in that future session, particularly the new `battle_move_resolution.c` and effect/owner seams. Resume Stage05 only after compatibility review. No such ledger changes are made by this report.

## 9. Validation and handoff

**This reconnaissance needs no build or test run.** Source, scripts, configuration and existing documents remain unchanged by it. The stable research reference is HEAD `582c5f50155753295f0f5c5b81a1eddab0690a1e` plus the recorded pre-existing working tree; preserve that complete snapshot for any user-run baseline check. During validation, do not modify its source/data/scripts/tests. Future migration checkpoints require their own pinned revision and dirty-tree manifest.

After a separate implementation authorization, run focused WSL checks from the repository root, without a `wsl` prefix. Example commands for the current named suites, to repeat against each applicable frozen migration milestone:

```bash
make -j4
make check -j4 TESTS='Koanko'
make check -j4 TESTS='AI'
make check -j4 TESTS='Arcanum'
make check -j4 TESTS='TX'
make check -j4 TESTS='Wishful Thinking'
make check -j4 TESTS='Reduction'
make check -j4 TESTS='Polymorph'
make check -j4 TESTS='Break Bar'
make check -j4 TESTS='Trainer Break Bar'
make check -j4 TESTS='Wild Break Bar'
make check -j4 TESTS='Break Charm'
make check -j4 TESTS='No Order Change'
make check -j4 TESTS='Battle status icons'
```

The TX prefix intentionally includes profiles and Wishes. Confirm nonzero selected counts after harness migration; adapt a filter only to actual retained test names. Add targeted capture, terrain, pivot/phazing, partner/multi, transform, damage/protection and saved-layer fixtures for changed contracts rather than treating this command list as complete coverage. Planned `Meteo`/`TX Arcanum cooldown` suites are Stage05 work and must not be presented as existing passing coverage.

Acceptance must demonstrate:

- Bars/saved layers/Charms: attack, passive, instant and multihit damage; no false faint/EXP/reward; correct refill/reveal; capture rejection and consumption.
- Stat/owner integrity: real-party canaries, canonical HP/maxHP after healing/revival/switch/menu/cleanup, Reduction/growth/Polymorph ordering, no owner aliasing or spent-resource renewal.
- AI parity: real production callbacks and flags, sampled/virtual legality, signed negative/zero/>255 boundaries, layer progress versus KO, no stale cache/commitment and no simulation state leakage. Historical injected-score TX tests establish effect plumbing, not tactical selection quality.
- Timeline/formation: ordinary singles/doubles/multi, Castoria ambush, saved-bar nested returns, Time Stop/recharge, No Order Change, Koanko transition and trainer resource preservation. Future Zacian must still satisfy its independent owner/queue/loss/retirement contracts.
- Emulator/resource evidence: exact Kazura/Bug Space/Polymorph/healthbox behavior, repeated Bag/summary reshow, original/copy sprites, no leaks/corruption, and measured ROM/IWRAM/EWRAM/heap/task/sprite/decision-time headroom. Automated tests do not establish presentation quality or capacity.
- Persisted compatibility: approved ID/save policy and explicit recorded-battle disposition. A clean textual merge and successful ROM link do not establish either.

Validation performed for this report is limited to Git/history/source/document inspection and lightweight delivery checks. No game source, tests, generated assets or existing documentation were intentionally edited; no branch was switched, merged, rebased, cherry-picked, reset, committed or pushed. No new engine integration point is introduced. The recommendation is ready for review; upgrade execution remains a separate decision.

## 10. Reproducing the core evidence

These commands are read-only and use pinned objects available after the metadata research. Moving upstream refs must be checked again when implementation is authorized.

```bash
git rev-parse HEAD
git remote -v
git show --no-patch --format=raw 26b9f02b848f7a5b9eb7453baec9373c1dc730a4
git show --no-patch --format=raw fe8cdd1aaec623bc4c69efed9d3daa63b7a0c419
git merge-base HEAD 79dd436920845ebc145577649de624f65298c209
git merge-base HEAD 31df832cc7fc3aa890bf3cd44f2df4baea369c23
git ls-remote upstream HEAD refs/heads/master refs/heads/upcoming 'refs/tags/expansion/1.1[5678]*'
git diff --no-renames --name-status 64d40d3c1464011d8642d280b622f3e7fa27c5d3 79dd436920845ebc145577649de624f65298c209 -- src include data
git diff --unified=0 64d40d3c1464011d8642d280b622f3e7fa27c5d3 -- src/battle_ai_main.c
git diff --unified=0 64d40d3c1464011d8642d280b622f3e7fa27c5d3 79dd436920845ebc145577649de624f65298c209 -- src/battle_ai_main.c
git log --oneline 79dd436920845ebc145577649de624f65298c209..31df832cc7fc3aa890bf3cd44f2df4baea369c23
git show 79dd436920845ebc145577649de624f65298c209:src/battle_move_resolution.c
git show 79dd436920845ebc145577649de624f65298c209:migration_scripts/1.16/trainer_pic_migration/main.py
```

The common-base U0 inspection compares the actual custom patch and upstream changes against the same ancestor. It is not a dry-run upgrade, and hunk overlap cannot reveal all semantic dependencies. Chapters12–13 retain their original snapshot provenance; this report supplies the target decision and current-snapshot corrections without rewriting them.

Delivery verification: local Markdown links resolve, with no trailing whitespace or replacement characters. An aggregate SHA-256 fingerprint of 5,820 existing files under source/header/data/assembly/test/docs plus inspected root instructions/build configuration matched before and after report creation (`50D781E20B5F0DD3217A6B716436C799C4681088A609B3686BF2D1046B29F36F`), excluding this new report. HEAD and the stale upstream tracking ref retained their recorded values. This is file-integrity evidence, not runtime validation.

## 11. Authorized migration execution record — 2026-11-11

### Baseline preservation and working locations

- Protected baseline: `mypatch`, **`2bfc834d8c5b819b2f120f30de8216f6c35653ba`**.
  This newer user commit contains the TX health module, TX/wild fixtures,
  simplified new game, Break Charm numerical animation and separate guarded
  4096-byte test stack. The older research HEAD is not the migration input.
- Original checkout remains on `mypatch`; its additional
  `graphics_file_rules.mk.bak` contents are preserved. No baseline commit was
  changed, reset, cleaned, rebased or merged.
- Implementation worktree: `TEMPSCRIPTS/upstream-1.17.2/worktree`, branch
  **`upgrade-branch`**. Worktree metadata uses relative links so Windows/MSYS
  Git and WSL Git can both access the same files.
- Target: **`79dd436920845ebc145577649de624f65298c209`**, exact 1.17.2 release.
  A `--no-commit --no-ff` merge is open in the implementation worktree.
  **The merge is unfinished and the worktree is not an acceptance build.**
- Local recovery archive:
  `TEMPSCRIPTS/upstream-1.17.2/pre-migration.tar.gz`, SHA-256
  **`3aa906fafc2ec73ca43906ca83ca996f4ba16ce44a11ed17585a2356192cc629`**.
  It includes working, untracked and ignored source/assets, generated assets,
  tools, and existing ROM/ELF artifacts; excludes Git metadata, the build
  object directory, the search index and the archive's own evidence directory.
  The committed baseline and copied index/patch evidence preserve Git state.
  `snapshot-sha256.json` fingerprints 57,607 archived input/artifact files.
  Verification found changes only in rebuilt ROM/ELF/map outputs.

### Route and conversion decisions

Use the **direct 1.14.2-based fork → pinned 1.17.2** route. Clean copies of the
original input and target trees are retained beside the worktree. The reviewed
scripts can consume original data without compiling an intermediate release;
no demonstrated ownership or conversion diagnostic currently justifies paying
for a second reconciliation at 1.16.4. This is the conditional direct route
allowed by Section 8, not a claim that the combined semantic port is low risk.

All 1.15/1.16/1.17 migration scripts were inspected before execution. Applied or
prepared conversions are recorded in `conversions.json` and `conversions.log`:

- Trainer-picture migration ran against original `gTrainerSprites`, producing
  review snippets. Razeluxe/Alice were manually registered in the target
  `gTrainerPicInfo`; target back-picture and FRLG registrations remain intact.
- The clean-input INCGFX converter prepared 21 modified graphics-bearing files.
  Converted data registrations have been transferred; remaining custom runtime
  declarations still require review. Original rules and assets remain available.
- Five trainer Ball fields were converted to target names. The supplied
  converter cannot parse this fork's existing `ITEM_LUXURY_BALL` field, so that
  case and the four textual names were converted explicitly, preserving party
  content. No trainer-party regeneration was run over authoring inputs.
- Givemon, text-printer, text-color and money/coin scripts ran only in the
  disposable conversion copy. Their remaining live custom call sites must be
  reconciled; do not infer completion from prepared output.
- Easy-chat and decoration inputs have no custom delta requiring a second
  conversion. Regional dex changes were reconciled into the target macros,
  retaining Castoria and Kazuradrop entries.
- Animation load removal and implicit-wait deletion were deliberately **not
  executed**. Custom timing and new assembler diagnostics must be reviewed
  before removing waits. Teachable JSON regeneration has not run; original
  helper inputs and generated header are preserved.
- Map layouts, map groups, heal locations and encounter tables were merged by
  symbolic identity. Custom layout versions are `emerald`; target FRLG content
  is retained. Old saves/recordings are unsupported under the waived policy.

### Identifier and interface checkpoint

Custom bosses now use species **1573–1576**, with Egg/`NUM_SPECIES` **1577**.
`GEN_CUSTOM_START` follows the complete upstream range. Break Charm is **874**,
with `ITEMS_COUNT`/field arrow **875**. Bug Space/Wishful Thinking are **320/321**,
with ability count **322**. Custom moves retain **848–859**, move count **860**,
and Z/Max boundaries derive from the expanded count. Trainer pictures/classes
and overworld objects/palette tags follow target ranges. Original and integrated
ID expressions/values are in `id-manifest.json` and
`integrated-id-manifest.json`; a full hardcoded-ID audit remains required.

ARM header compilation and boundary assertions passed using
`identifier_compile.c`. These checks cover actual C enum interpretation and
sentinel separation; they do **not** validate battle behavior or every data
reference. `GEN_LATEST GEN_9`, level 105/player cap 100, custom type values and
the fork's explicit badge/obedience/item policies are retained.

Battle PartyState adopts `MAX_BATTLE_TRAINERS`; pending-layer lookup now uses
`GetBattlerPartyState`. Custom native effect IDs move to
`constants/battle_set_effect.h`, Time Stop's canceler ID to
`constants/battle_move_resolution.h`, and Wish's ability event to
`constants/battle_util.h`. Melt Virus and Timestop now use target SetEffect
handlers; the remaining custom module API ports are unfinished.

Trainer generation uses the new target generator, with configuration recording
in `trainer_break_bars.c` and narrow `battle_setup.c` hooks. Pending static
configuration pointers bridge pre-battle generation to allocated PartyState;
runtime Koanko generation can bind to existing state. No old generator was
restored. This integration still requires owner, selection-source, override,
partner/multi and consecutive-battle regressions.

Graphics init/free now follows `AllocateBattleGfxResources` and
`FreeBattleGfxResources`. Full teardown clears script overrides and pending
generation configurations. Persistent battle records are not freed by menu
closure. The test harness retains a separate aligned 4096-byte stack/guard,
adopts target trainer bindings and preserves summoned-opponent/cleanup helpers.
The original deleted AI switch module's No Order Change hook moves to
`battle_ai_switch.c`; its empty Alice placeholder is retired.

### Actual validation and remaining gates

Frozen original baseline, executed in WSL before source integration:

```sh
make -j4
make check -j4 TESTS='test/battle/tx_profiles.c'
make check -j4
```

ROM build passed: EWRAM **239,764 B**, IWRAM **28,812 B**, ROM **25,184,640 B**.
Focused TX profiles: **12 passed / 12 total**. Full suite: **4,896 total,
4,096 passed, 18 known-failing, 779 TODO, 3 failures**. The three failures are
exactly the accepted text-width cases: Roar of Eternity on the Contest screen,
the Fireball description on the summary screen, and Castoria's real-name battle
message. There were **zero unexpected battle failures**. Full-suite exit code
is 1 because accepted text failures remain failures; `KNOWN_FAILING` was not
expanded. A separate `TESTS='Break Charm'` invocation selected zero tests and is
not coverage evidence. The full baseline suite includes the existing fixtures.
Logs are in `TEMPSCRIPTS/upstream-1.17.2/baseline-*.log`.

Migration checkpoint: numeric-manifest assertions and ARM identifier-header
compilation passed. **No integrated ROM build or runtime suite has passed.**
The initial merge had **107 conflicted paths**; all textual conflicts are now
resolved. Seven core battle objects compile, but the migrated ROM has not
linked and no migrated runtime tests have run. Custom-module compilation and
battle-script assembly still fail. TX/Castoria/Koanko/Reduction ownership and
normalization ports remain incomplete.

The user requested a stopping checkpoint due to the usage window. Read
[the durable migration checkpoint](15_upstream_migration_checkpoint.md) for
exact failures, partial ports, semantic risks, Git state and resume commands.
It supersedes earlier unresolved-path counts. Do not mistake a WIP source
checkpoint for an accepted migration.

Next: finish those semantic ports, compile a coherent target, run each focused
suite from Section 9 with nonzero selections, repair unexpected failures, then
run the full suite. Preserve the source snapshot for each test run. The terrain
and pivot/phazing master fixes still need disposition; neither was imported.
No Stage 05 implementation, push or merge into `mypatch` has occurred.
All fresh-game, animation, healthbox/menu reconstruction and resource checks
remain required in the emulator after automated integration succeeds.
