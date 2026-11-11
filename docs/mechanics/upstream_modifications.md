# Upstream modification ledger

Central record for reviewed custom engine integration points. This initial entry covers
Stage 03 Smeargle integration; it is not an exhaustive inventory of older custom hooks.
Implementation status is separate from runtime validation.

## Pre-migration regression fixtures (2026-11-10)

No production integration points changed in this milestone.
`test/test_runner_battle.c:DeclareSummonedOpponent` now declares the existing
summoned TX profile's four ordinary moves so `EXPECT_MOVE` can resolve Wide
Guard before runtime creation. This is a narrow correction to the existing
test helper; keep it aligned with `BS_TxRollbackCreate` on future moveset edits.
`test/battle/arcanum_more_wishes.c:MoreWishesScore` allows the simultaneous-KO
fixture to choose Follow Me instead of blocking its own trigger with Wide
Guard. Other fixtures retain Wide Guard. Regression assertions are preserved,
including an explicit Follow Me selection in the KO turn. No faint/replacement,
controller, Wish-selection or damage algorithm was changed. Merge maintenance
is confined to test code, with no additional production hook or dependency.
The user reports both More Wishes and TX Wishes focused runs passing after
these repairs; exact counts were not supplied for those two runs. The subsequent
12-file isolation loop also passed (saved logs: 137 PASS, 10 TODO, 147 TOTAL).
The other 13 reported battle failures passed in isolation but recurred together
in the full suite. Subsequent diagnostics identify invocation-stack corruption;
see below and the
[repair record and exact commands](tx_rollback.md#pre-migration-regression-repair-2026-11-10-through-2026-11-11).

### Blocked nonvolatile effect continuation (2026-11-10)

- `src/battle_script_commands.c:SetMoveEffect`: when the shared nonvolatile
  eligibility check rejects an effect, advance to the supplied continuation.
  Previously this branch left the cursor at the same command; diagnostic
  Gulp Missile snapshots confirm paralysis blocked by unintended Misty Terrain
  looping with no outstanding controller request. The repair also applies to
  other nonvolatile effects using the same branch, preserving all immunity
  checks, successful status scripts and existing continuation ownership.
- Regression: the existing faint-trigger Gulp Missile fixture plus a new
  explicit Misty Terrain/faint/replacement case in
  `test/battle/ability/gulp_missile.c`; shared status/ability suites and full
  suite remain required. User-run focused log: 11 PASS/11 TOTAL, including the
  new explicit Misty Terrain case. The user subsequently reports the full
  suite green except the deferred text cases after the stack correction.
- Merge impact: two added lines in the existing upstream switch branch; no
  new hook or module. Recheck upstream status rejection/continuation semantics
  during migration, and drop this local fix if upstream already handles it.

### Test invocation stack capacity (2026-11-11)

- `test/test_runner_battle.c:BATTLE_TEST_STACK_SIZE` provides 4096 bytes.
  Direct healthbox calls from test bodies reach Smol decompression, whose
  symbol-delta decoder alone uses a measured 1740-byte local code buffer in
  the existing ELF. Diagnostic guards caught overwrites in three Break Bar
  fixtures; weather changes and corrupted trial/parameter counters show the
  damage escaping the test stack. The corrupted fog value then correctly
  creates Misty Terrain in later battles, causing misleading status/AI failures.
- The initial embedded-stack increase failed the user's compilation because
  the existing `sBackupMapDataSpace` assertion correctly rejected state larger
  than the 20 KiB map buffer. The corrected design removes the embedded stack
  from `include/test/battle.h:BattleTestData` and adds a separate test-only,
  8-byte-aligned `EWRAM_DATA` stack with two guard words (4104 bytes plus possible
  linker padding). Last linked ELF evidence leaves 19468 EWRAM bytes available;
  the added buffer leaves about 15 KiB if other allocations are unchanged.
  IWRAM, production stack, heap size and fixed map-buffer capacity are unchanged.
- `InvokeTestFunction` retains the guard check on every invocation. Three
  explicit resets mirror the previous state/data memset points, preserving
  test-local lifetime across scene/assertion phases. Static storage has no
  allocation/cleanup callbacks. Existing state-size and result bounds remain.
- Weather/state trace logging and the temporary ELF-inspection script are
  removed. No weather reset or decompressor algorithm modification hides the
  overwrite. Existing offending tests provide regression coverage: Break Bar
  dispatch/redraw/dynamic fill and No Order Change healthbox reconstruction,
  followed by the full suite. The user reports that full stack green except
  text tests. Final linked ELF inspection confirms the stack at `0x0203AA08`
  and EWRAM ending at `0x0203C400`, leaving 15360 bytes free. No additional
  tests were run by Codex.
- Merge impact: test infrastructure only, localized macro/field/invocation
  changes. Review upstream test stack ownership, capacity and result-storage
  layout when merging. Production rendering and compression remain unchanged.

The second diagnostic full run reports 4896 total, 3936 passed, 18 known-failing,
779 TODO and 163 unexpected failures. It exposes stack corruption across more
workers after diagnostic layout/scheduling changes; it is not an accepted
baseline. The subsequent separate-stack full-suite run is user-reported green
except the deferred text tests, with zero unexpected battle failures. Exact
final passed/failed/known-failing/TODO/total counts were not supplied; the
diagnostic numbers above must not be presented as final counts.

## Simplified new game (2026-11-10)

- `src/new_game.c:NewGameInitData` / `InitSimplifiedNewGame`: replaces the
  existing mandatory AliceRoom startup warp with an outdoor Littleroot spawn,
  ordinary starter/item creation and immediate introductory-state setup after
  vanilla resets. This is a narrow fresh-save integration, not a second startup
  route. Continue and save structures are unchanged; Alice maps/scripts/trainers
  and all battle mechanics are preserved.
- `data/scripts/new_game.inc:EventScript_InitSimplifiedNewGame`: new immediate
  initialization script beside existing reset scripts. Mirrors post-Pokedex /
  Running Shoes state, gender-specific household visibility and safe respawn;
  bypasses only the opening through the Route 103 rival handoff.
- `data/layouts/LittlerootTown/map.bin`: two sealed north-exit tiles, (10, 1)
  and (11, 1), changed from `0x044E` to `0x3001` so Route 101 is reachable.
  Review these tiles against upstream map edits rather than replacing layouts.

Merge exposure is localized to the existing new-game function, event-script
file and four map bytes. Semantic compatibility requires rechecking upstream
reset order, intro state values, starter/Pokedex registration and heal-location
setup even after a clean textual merge. No overworld or battle algorithm hook
is added. ROM build `make -j4` passed; fresh-save movement/menu/house/wild-battle/
save-reload/Petalburg/Alice checks remain pending because mGBA computer-use
access was rejected. Exact state and validation instructions:
[Simplified new game](new_game.md).

## Wild Break Bar assignment, capture and overlay (2026-11-10)

Implemented in the working tree. The user reports the wild fixture passing
after its opponent-position setup was corrected. ROM build and emulator color
inspection have not been reported. See [Break Bar reference](break_bars.md).

- `src/scrcmd.c:ScrCmd_dowildbattle` and `src/battle_main.c:CB2_InitBattle`:
  narrow hooks arm and consume a script-owned wild profile before healthbox
  creation. `src/script.c` owns the pending pointer and clears it on script
  stop/reset; battle setup and cleanup clear unused state. Review callback and
  battle initialization order on upstream merges. Trainer party generation is
  unchanged.
- `src/battle_script_commands.c:Cmd_handleballthrow`: narrow rejection before
  catch-rate and guaranteed-capture branches. `src/battle_util.c` skips Bag or
  Safari Ball consumption for that rejection. Review both action handlers and
  any new capture paths when upstream changes Ball transactions; a clean textual
  merge does not establish semantic coverage of new paths.
- `src/battle_break_bar.c:BS_AnimateSavedBreakBar`: changes only the temporary
  covered-layer palette selection. The shared `DrawHealthbarOverlay` and new
  layer fill remain unchanged. Review healthbar palette indices and HP color
  thresholds if upstream graphics change.
- `include/battle.h:PartyState`: one battle-only byte records whether the base
  covered by a dynamic layer was already registered. This preserves its red
  presentation even when another layer is inserted after earlier depletion.
  Review PartyState layout and reset semantics when upstream changes battle
  state; there is no save-format change.

The runtime state and predicate remain in the custom Break Bar module; new
event syntax lives in `asm/macros/event.inc`, and the rejection message in
`data/battle_scripts_2.s`/`src/battle_message.c`. Focused regressions:
`test/battle/wild_break_bars.c`, `Break Bar`, `Trainer Break Bar`, `Break Charm`,
capture and healthbox cases, plus normal ROM build and visual inspection.

## Koanko Stage04 TX profiles, health and Reduction (2026-11-10)

Implemented in the working tree; builds and runtime tests are pending user-run
validation. [Behavior/state/codeflow](tx_rollback.md),
[milestone changes and commands](../extra%20docs/KoankoDev/STAGE04_IMPLEMENTATION_AND_CODEFLOW.md).

- `battle_controllers.c:BtlController_EmitSetMonData`: narrow
  `TxRollbackNormalizeRequest` hook after Reduction normalization, converting
  emitted HP/maxHP/full records to the original party maximum. Existing emission
  buffering is retained. Review request layout, masked party identities and
  inverse overlay order when updating controller serialization.
- `battle_controllers.c:BtlController_HandleHealthBarUpdate` and
  `battle_interface.c:UpdateHealthboxAttribute`: extend existing effective-HP
  predicates to the TX overlay. Party maxima deliberately remain canonical, so
  HUD/preview reads require battle HP/maxHP. These touch upstream display paths;
  a clean merge still needs semantic checks of HP-update timing and reloads.
- `battle_util.c:CopyMonLevelAndBaseStatsToBattleMon`: one clean-copy hook after
  Wishful Thinking growth and before Reduction. `battle_script_commands.c:
  Cmd_switchindataupdate` adds the corresponding hook after growth on its clean
  controller copy, before ordinary switch-in cleanup. Incorrect insertion order
  could lose maxHP deterioration or duplicate growth; check clean-copy semantics
  when either upstream function changes.
- `battle_main.c:HandleEndTurn_FinishBattle`: one cleanup call after Reduction
  restoration; refine the existing Dex exemption to summoned ownership, so solo
  combat ownership does not inherit summon-only behavior. Review cleanup order
  and ownership availability if upstream rearranges teardown/Dex processing.
- `include/battle_tx_rollback.h` extends the existing custom portion of battle
  state; no save-format fields are added. The state is still owned by the
  existing `BattleStruct.txRollback` member, with battle lifetime only.
- `data/battle_scripts_1.s`: add returning-to-shared-refill 60/30 wrappers, a TX
  Reduction success script and a Reciprocity abort-return label. Shared Break
  Bar refill/resolution algorithms are unchanged; assembly label availability,
  native argument layout and controller waits need review on upstream changes.

The substantive implementation stays in custom `battle_tx_health.c`,
`battle_tx_rollback.c`, `battle_tx_wishes.c`, `battle_reduction.c` and the existing
`battle_koanko.c:KoankoOnSwitchIn` entry hook. Original Reduction calls use the
idempotent 10+ wrapper; solo calls request 1+. No generic AI algorithm, type chart
or new Break Bar executor is added. This limits new integration surface, but the
shared controller/stat-copy paths remain semantic maintenance hotspots. Overlap
with later upstream edits predicts review work, not a proven textual conflict.

Required regressions: all of `test/battle/tx_profiles.c`, `TX Wishes`,
`Arcanum Wish More Wishes`, `Arcanum Wish Reduction`, `Wishful Thinking`,
`Break Bar`, `No Order Change`, `Koanko`, and adjacent `Arcanum`; normal ROM
build. New mechanism fixtures inject Wish scores and do not establish production
AI selection quality. Stage05 must add that separate evidence.

## Koanko per-mon dynamic tactics follow-up (2026-11-10)

The user explicitly requested an individual conditional block for all twelve
roster members and accepted retaining the preceding shared AI improvements.
Current behavior and test results: [Koanko trainer AI](koanko_trainer_ai.md).

- `src/battle_ai_main.c:BattleAI_SetupFlags`: one narrow registration hook invokes
  `KoankoSetupTrainerAi` only when no explicit dynamic callback is installed.
  This makes the original six reach their dynamic blocks before conjuring.
  Encounter validation and registration are in the custom module. Script/test
  callback precedence and existing cleanup/reset are preserved.
- `include/battle_koanko.h:KoankoSetupTrainerAi`: declaration for that hook; no
  battle/save state layout or ownership storage is added.
- `src/battle_koanko_ai.c:AI_KoankoTrainer`: explicit roster blocks, real cache
  damage/accuracy and observable conditions. Smeargle retains its existing rank
  path without stacking new ordinary-roster bonuses. Current-form handlers do
  not implement missing form scripts, TX virtual profiles or Zacian ownership.

All new mon-specific tuning stays in the custom file. There is no additional
global per-species scorer or replacement switch/item/execution path. Upstream
maintenance risk is concentrated in initialization order and dynamic callback
lifetime/flag derivation; scoring arithmetic remains shared through the existing
API. The prior shared Break Bar, signed-score, cache and Max-effect work is kept.
The `Koanko Tactics` and full `Koanko` prefixes validate the new route, with
ordinary `AI`, `Arcanum` and `TX Wishes` as adjacent controls.

Zacian analysis does not add a runtime hook: Stage07 still needs an explicit
independent owner, AI recognition, player-partner controller/override gating and
resource isolation. The one per-battle callback may then route that owner to a
separate allied helper before the enemy-side guard. Species alone is insufficient.

## Koanko Stage11 AI validation and shared outcome/cache fixes (2026-11-10)

Validation status and per-combatant evidence are maintained in
[Stage11](../extra%20docs/KoankoDev/planning/11_koanko_ai_validation.md#9-implementation-record-2026-11-10).
The user explicitly approved CHECK_VIABILITY and SMART_TERA in the authored
Koanko trainer profile; these use ordinary AI and do not force an opener or Tera.

- `battle_ai_main.c:ChooseMoveOrAction_Singles/Doubles`: signed maximum comparison,
  authoritative real-slot limitation filtering, and signed-invalid target sentinels.
  Arithmetic/target-selection correction, not a new scoring policy. Boundary fixtures
  include scores above 255, zero, negative scores and INT32_MIN. Target validity
  is separate from its signed score; all-unusable/no-target queries stay bounded.
  Ordinary/virtual exact ties
  continue to favor ordinary moves.
- `battle_ai_util.c:CanEndureHit`, `AI_GetDamageOutcome`, `AI_GetReachableDamage`,
  `CanAIFaintTarget`, `CanTargetMoveFaintAi`: share the existing execution interception
  predicate from `battle_break_bar.c:CanInterceptBreakBar`. Original-layer breaks
  and saved-layer reveals are progress, not KOs, including multi-hit overkill.
  No layer scripts run during valuation; no future-form damage is extrapolated.
  This affects shared lethal reasoning, so upstream KO/survival helpers require
  semantic review even when there is no textual conflict. An unimplemented Arceus
  survival floor is deliberately not fabricated here.
- `battle_ai_main.c:CalcBattlerAiMovesData/SetAiLogicDataForTurn`: clear removed or
  newly unusable slots and refresh only DOUBLE_BATTLE topology flags at the normal
  fresh-turn boundary, preserving explicit zero flags. Preserve encounter/sequence/dynamic flags and committed
  actions. No new mid-turn decision loop or ownership model.
- `battle_ai_util.c:AI_CalcDamage`: restore simulation nesting and transient
  type/category/hit-counter/power scratch state, including its healing-ally early
  return. Existing ordinary damage/type/gimmick engine remains authoritative.
- `battle_ai_util.c:DecideTerastal/ShouldTeraFromCalcs`: keep deferred-Tera
  effectiveness consistent with its already-updated damage row; count intended
  living Tera users through `battle_gimmick.c:ShouldTrainerPartyMonUseGimmick`
  rather than treating every stored Tera type as trainer permission. The existing
  battler API delegates to that shared party-index query. Outcome-based offensive
  KO reasoning cannot mistake a layer break for victory. Actual consumption and
  Stage01's one-time opponent reset are unchanged.
- `battle_ai_util.c:ShouldUseWishAromatherapy`: No Order Change limits Wish's
  reserve-healing assumption to its actual occupant. No new TX profile, Wish
  outcome, ability trigger scope or cooldown is introduced.
- `battle_ai_main.c:AI_CheckViability/AI_CalcAdditionalEffectScore`: real cache
  slots remain unchanged, while viability resolves the planned/active Max move's
  effects through `GetMaxMove`. This fixes the demonstrated Alcremie defect where
  Recover was credited as healing although it would execute Max Guard. Finale
  values reachable healing of living active allies, respecting Heal Block and
  pending layers. The fixture observes actual G-Max Finale damage, healing and
  one-time Dynamax use. This does not claim to finish every generic Max bad-move,
  accuracy or activation-timing edge case; adjacent known failures are tracked
  separately from Koanko's required kit.

Trainer-specific Time Stop expected utility remains in `battle_koanko_ai.c`, gated
by Koanko identity, conjured phase, opponent ownership, Dialga species and party
slot. It does not duplicate ordinary recharge scoring or change the proc chance.
The first six receive no species-specific scorer. These shared edits have more
textual overlap with upstream AI than a Koanko-only workaround, but avoid keeping
duplicate selection, survival and Tera-eligibility algorithms in custom modules.

## Koanko trainer Special AI and Smeargle ranking (2026-11-10)

`src/battle_koanko_ai.c:AI_KoankoTrainer` follows the Alice/Razeluxe dynamic trainer
callback interface. Its Smeargle policy reads existing slot-0 damage/accuracy caches,
adds KO/damage utility and execution/drawback costs, and preserves state-aware utility.
Other regular-move owners keep their incoming score. Unimplemented extra-slot policies
return 0; no generic Arcanum/Wish scoring is accidentally enabled.

- `src/battle_ai_main.c:ScriptSetDynamicAiFunc / SetDynamicAiFunc` and
  `include/battle_ai_main.h`: narrow programmatic setter for the existing callback,
  with the script setter delegating to it. Ordinary scoring algorithms unchanged.
- `src/battle_koanko.c:ConjureKoankoTeam`: register Koanko's callback and add
  AI_FLAG_DYNAMIC_FUNC at the phase transition. `data/scripts/debug.inc:Script_3`
  registers the same callback before launching the dedicated debug encounter.
  Existing battle cleanup resets the pointer. Future first-team tactics can register
  the same callback in their encounter script without duplicating the policy.
- `src/battle_ai_main.c:ScoreMoves_Singles / BattleAI_ScoreSlot0Candidate /
  AI_CheckViability / AI_CheckBadMove`: guarded diagnostic calls observe pass
  boundaries, priority, cached metadata and aggregate effect contributions. No extra
  score-function evaluation is added by these hooks. Instrumentation stays in the
  dedicated diagnostic module under its compile-time feature switch.

Maintenance: ranking remains in the custom Koanko module, uses no new BattleStruct
fields, adds no sampler-specific ranking branch or full damage-calculation pass,
and requires semantic review when upstream changes cache layout, callback lifetime,
scoring order, survival or speed helpers. Detailed records remain in EWRAM.
Verified: Koanko 44/44, TX Wishes 22/22, Arcanum 150/150, ordinary AI 310 pass with four declared known
failures and four TODOs; normal ROM build succeeds. Manual gameplay remains pending.
See [Stage 03 note](../extra%20docs/KoankoDev/PROGRESS_stage03_smeargle_2026-11-10.md)
for formula, before/after examples, timings, memory measurements and exact commands.

## Direct Koanko Smeargle debug encounter (2026-11-10)

- `src/battle_main.c:CreateNPCTrainerParty`: narrow call/early return for the one-shot
  Koanko debug party request. It runs after normal AI flag/resource initialization
  and before opening party/controller reads. The custom module creates only the real
  conjured Smeargle at party index 0 and initializes conjured phase, sequence flags
  and No Order Change. Ordinary calls fall through unchanged. Upstream changes to
  initialization ordering or party-factory dispatch require semantic review.
- `src/debug.c:sDebugMenu_Actions_Scripts` and `data/scripts/debug.inc:Debug_EventScript_Script_3`:
  repurpose unused Script 3 as "Koanko Smeargle" and invoke the dedicated native.
  No debug menu engine or battle selection algorithm changes.

Lifecycle and return handling stay in `src/battle_koanko.c`; the existing debug
transition launcher and normal party constructor are reused without modifying them.
The native deliberately avoids generic debug trainer substitution and follower-party
restoration. No new BattleStruct fields. Normal ROM build and focused Koanko suite
pass; manual entry/exit/repeat with/without a follower remains unverified here.
Details: [Stage 03 implementation note](../extra%20docs/KoankoDev/PROGRESS_stage03_smeargle_2026-11-10.md).

## No Order Change icon placement (2026-11-10)

`src/battle_status_icons.c:sIcons / UpdateIconPosition` now place No Order Change
beside and vertically centered on the healthbox, on the player left/opponent right.
A table-driven singles-player Y adjustment follows the actual healthbox layout.
These changes stay within the custom icon module; no upstream-maintained function
or integration hook changed. Existing movement, visibility and lifetime ownership
are preserved. Upstream healthbox artwork/origin changes can still require a semantic
layout adjustment without a text conflict. Existing icon geometry expectations were
updated. The user reports both focused suites (`Battle status icons` and
`Arcanum No Order Change indicator`) passing; in-game visual confirmation is pending.
See [No Order Change](no_order_change.md).

## Koanko Stage 03: Smeargle Any Move (2026-11-10)

Implemented in the working tree; current build and verification results are recorded above. Details and
test commands: [Stage 03 integration note](../extra%20docs/KoankoDev/PROGRESS_stage03_smeargle_2026-11-10.md).

- `src/battle_ai_main.c:BattleAI_ChooseMoveIndex`: narrow owner-gated hook to sample
  fresh choices after setting the ordinary gimmick assumption; temporarily enable
  CHECK_VIABILITY and restore flags on exit. Depends on action selection still running
  before controllers copy the move list and on the forced-action predicate matching
  `HandleTurnActionSelectionState` (recharge, multipleTurns, Time Stop).
- `src/battle_ai_main.c:ChooseMoveOrAction_Singles / ScoreMoves_Singles`: small shared
  scoring extraction, preserving the existing final picker and TX/Arcanum extra-slot
  logic. `BattleAI_ScoreSlot0Candidate` uses these same passes without selecting an
  attack, and saves/restores ProtectStruct scratch flags. This is more than an inserted
  hook, but avoids maintaining a duplicated singles scoring loop. Future upstream
  scoring-order changes must remain in the shared helper; review prediction and any
  new scoring side effects for repeated hypothetical evaluations.
- `src/battle_ai_main.c:BattleAI_PrepareSlot0Move`, declarations in
  `include/battle_ai_main.h`: candidate/winner API writes real slot 0, refreshes
  limitations and clears damage/effectiveness/accuracy before existing cache calculation.
  Narrow API addition. Future cache fields or calculator prerequisites must be audited
  here. Assumes this encounter keeps slots 1-3 empty; not a general multi-slot API.
- `src/battle_util.c:CancelerPPDeduction`: narrow predicate gate on PP spending only,
  covering Pressure without skipping ordinary post-deduction submove handling or
  Metronome-item bookkeeping. Recheck if upstream moves PP expenditure elsewhere.
- `src/battle_util.c:CancelerChoiceLock`: narrow predicate gate on Choice/Gorilla lock
  assignment. Existing limitation checks remain unchanged; immunity prevents new locks.
- `src/battle_script_commands.c:Cmd_trysetencore`: narrow failure-condition addition
  before Encore acquisition. Recheck if another acquisition path is introduced.
- `include/random.h:RandomTag`: appended DRAW/TIE identifiers; existing numeric tags
  preserved. Calls use shared RNG, not isolated streams.

Regression coverage: `test/battle/koanko_smeargle.c` retains 15 supplied cases and adds
two integration cases; existing Koanko lead expectation updated in `test/battle/koanko.c`.
Required broader checks include Koanko/Loaded Dice, TX Wishes, Arcanum, Encore, Choice
and Gorilla Tactics. Direct Choice/Gorilla immunity, forced continuations beyond recharge,
targeting, runtime cost and memory headroom remain validation gaps.

User build follow-up: the test link failed with 1856 bytes of IWRAM overflow. The
5088-byte full-pool observation arrays now use EWRAM_DATA in the Smeargle test file.
This changes test storage placement only; no linker script or upstream engine hook
was changed. The subsequent user run linked successfully; exact headroom is unmeasured.

User results: Smeargle 16/17, Polymorph 9/9, combined Koanko 28/30 passed, including all
three Loaded Dice cases. The remaining recharge and lead fixture errors were corrected
using explicit SKIP_TURN and a slot-0 expectation, respectively. These
fixture corrections introduce no new upstream integration points.
The recharge assertion was subsequently scoped to per-turn evaluations after the
user observed two pool scans in the lifetime counter: playback can calculate the
next fresh turn before discovering record exhaustion. It still requires one scan on
the Hyper Beam turn and none on recharge. The user subsequently reported all requested
reruns passing. The user's s16 score-observation storage change saves another 1696
bytes, leaving 3392 bytes for both EWRAM observation arrays; fixture scores are 0..53.

Maintenance impact: bespoke eligibility, shuffle, owner predicate and test statistics
live in `src/battle_koanko_smeargle.c`. No Koanko state-layout changes, new transition
hooks, custom attack executor, or modifications to Loaded Dice's damage algorithm.
Textual overlap is concentrated in the AI chooser/cache/scoring area and three small
engine conditions. Semantic overlap is broader: upstream changes to scoring mutation,
gimmicks, restrictions, targeting, PP and forced actions need review even without a
merge conflict. The shared helper reduces duplicated maintenance without replacing
the ordinary scoring algorithm.

## Stage 03.1 test-harness follow-up (2026-11-10)

`src/castoria_name_theft.c:CastoriaSelectPolymorphTargetAndBudget` now accepts a
TESTING-only cyclic species-draw override through `Test_SetPolymorphDrawSpecies`,
declared in `include/castoria_name_theft.h`. This is a custom-module test seam, with
no new upstream-maintained integration point and no production sampling change.
The caller owns the array lifetime and clears the pointer/count after use. The
updated Polymorph regression asserts that the sampled winner belongs to its injected
draw set; it has not been run here. Details: [Polymorph implementation note](../extra%20docs/KoankoDev/PROGRESS_stage03_1_polymorph_2026-11-09.md).
