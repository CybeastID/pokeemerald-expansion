# Upstream migration checkpoint — 2026-11-11

## Resumed work after checkpoint

### Latest continuation boundary

**Current entry point:** focused Arcanum/Wish, TX, Break Bar, ownership and
Koanko/Kazuradrop unexpected failures documented below are repaired. Latest
Koanko generation is **13/13**, AI **37/37**, diagnostics **4/4**, Kazuradrop
**9/9**, stock smart-Tera **4/4** and Terastal **44/44**. Pre-existing known
failures remain visible. **The first complete migrated suite is running in
`resume-full-01.log`; normal ROM 10 is running in `resume-rom-10.log`.** Do not
edit implementation or run another `make check` against its shared ELF while
validation is active. No full-suite acceptance result exists yet.

The implementation is still on existing `upgrade-branch` at
`f058e5220ce8177d38f99360199db0ad013a0a99`, merge closed, resumed edits
uncommitted; protected `mypatch` remains
`2bfc834d8c5b819b2f120f30de8216f6c35653ba`. No push or final integration.
Read the latest continuation results before the historical stopping boundary.

Execution review became available on continuation (2026-11-11 20:25 UTC).
`resume-rom-08.log` passed: EWRAM 238,608 B, IWRAM 28,608 B, ROM 27,048,672 B.
`resume-tx-entry-validation-02.log` passed 1/1, including both new capability
assertions; `resume-tx-context-01.log` passed 1/1 for full-width four-slot flag
restoration. The first entry validation attempt only exposed a missing fixture
header, now included. `resume-tx-profiles-06.log` passed **12/12**, exit 0,
including all three previously failing Reduction/Reciprocity cases after the PP
port. TX wishes, AFTER_CLEANUP, migration ownership and stock trainer-control
focused groups are now running sequentially on the frozen source; per-group
logs/exits are recorded in `resume-tx-validation-01.json`. The usage-limit
block and uncompiled labels below describe the previous boundary; authorized
autonomous validation has resumed.

Focused matrix 01: TX wishes 22/23 passed (one score-recording failure),
AFTER_CLEANUP 6/6 passed, migration ownership 3/4 passed (new Roar fixture used an
invalid AI-only EXPECT_SEND_OUT directive), stock trainer control 21/21 passed.
The latter validates the production-data/test-mock override. Fixed the failed
fixture directive while retaining its scene and owner-state assertions. Doubles
AI score recording used stale `gBattlerTarget` rather than evaluated target `i`;
this defect also exists in the pinned target. It now records `[battler][i]`, with
no selection-policy change or relaxed tie assertion. Both focused groups are
being retried in `resume-tx-wishes-02.log` / `resume-owner-runtime-02.log`.
TX wishes retry 02 passed **23/23**, exit 0, with the strict ordinary-versus-
virtual tie assertion unchanged. Owner retry 02 found another invalid new Roar
fixture directive (`SEND_OUT` is a WHEN action, not a SCENE matcher); replaced
it with the standard exact drag-out message while retaining animation and
owner-state assertions. The other three ownership regressions passed again.

Virtual-action audit ported remaining baseline guards into target canceler/
move-end functions: Smeargle Choice-lock exemption; ordinary used/printed/last
move and Mirror Move history preserved across Arcanum; Grudge only indexes real
PP slots; virtual casts leave Metronome item tracking untouched. Known-move
learning and defender hit records remain as in the baseline/target integration.
Target consecutive-attack logic and faint-state progression remain intact.
`resume-arcanum-01.log` is validating the complete existing Arcanum file.
Arcanum 01: 57/71 passed; 12 invalid-move errors came from recorded controllers
indexing a four-slot choose buffer with a virtual slot. Recorded player/opponent
default-target selection now uses live virtual lookup, keeping cached real-slot
behavior. Arcanum 02: 68/71 passed. Migration ownership 03 passed **4/4**.

Canonical Grudge and Instruct suites passed **11/11** and **20/20**. Two copied
Arcanum regression scenes were synchronized to those pinned-target cases:
Grudge's exact text/before-faint presentation and Growl's target animation order.
All PP/effect assertions and exact scene checks remain. The third remaining
failure was a cached-damage fixture assuming opponent side implied AI control.
It now explicitly adopts/restores an AI controller while testing own-slot
cache/Risky/Conservative reads; target information restrictions remain intact.

Target introduced maximum-roll defaults for attacking/attacking-partner damage
estimates. Both existing contexts are configured to median to preserve the
baseline shared AI policy; Risky/Conservative overrides remain unchanged. New
target-only setup contexts retain target defaults. This uses target config
interfaces rather than changing the damage-roll algorithm. Arcanum retry 03
is rebuilding/validating these changes.

Arcanum retry 03 passed **71/71**, exit 0 (`resume-arcanum-03.log`). Current
frozen source includes the controller/history fixes and median AI configuration.
The next twelve focused files are running serially via
`resume_custom_matrix_02.py`; `resume-custom-matrix-02.json` records each actual
exit/log. This includes all remaining Arcanum files, TX profiles/wishes repeated
on this source, and Break Bar/grant/exit/trainer/wild cases. Do not run another
`make check` against the shared test ELF while this driver is active.

Matrix 02 completed. Passes: Endurance 10/10, TX profiles 12/12, TX wishes
23/23, exit HP 4/4, wild Break Bars 4/4. Failures: Debilitation 20/22 (Red Card
and Eject Button consumed despite No Order Change); More Wishes 16/18 (entry
Intimidate scene order and 8192/4096-byte animation-buffer leaks in reshow
fixture); Normality 8/9 and Reduction 19/20 (sprite exhaustion); Break Bar
20/21 (two-hit move reported one hit); grants 12/13 (surviving Lunar Dance user
did not request SEND_OUT); trainer bars 10/11 (fixture gender 0 invalid in target).
All logs use `resume-matrix-02-*.log`; exits are in the JSON above.

Repairs since matrix 02:
- Target item triggers now gate Red Card on the moved attacker and Eject Button/
  Pack on their owner before consumption/queueing. Debilitation retry 02 passes
  **22/22** (`resume-debilitation-02.log`).
- Sprite diagnostics identified Servant Sigil allocation while all 64 slots
  were occupied by doubles intro sprites, not a demonstrated sigil leak. Target
  `CreateSprite` aborts, whereas baseline returned a sentinel. Custom sigil,
  status-icon and Kazuradrop panel/number callers now use target
  `CreateSpriteUnchecked`, retaining their existing failure checks. Temporary
  core sprite diagnostics were removed. Normality retry 02 passes **9/9**.
- More Wishes reshow fixture calls `CloseMainBattleScreen`, matching target
  menu teardown of both windows and animation buffers. Trainer pool entries
  explicitly request random gender, matching baseline's default semantics.
- Multihit continuation restores baseline `IsBattlerAlive` checks (including
  pending layers), replacing target raw HP checks. Further pending hits remain
  damage-suppressed; the phase still resolves after the complete action.
- Lunar Dance resolves its consumed layer before storing the wish; a surviving
  user follows the existing immediate switch script. Genuine faints retain
  target deferred replacement. These last two repairs are being validated in
  `resume-break-bar-02.log` / `resume-break-bar-grants-02.log`.

Recovery snapshot at the 71/71 Arcanum boundary:
`resume-arcanum-03-source.patch`, two named untracked source copies and
`resume-arcanum-03-source.sha256`. Current repairs are newer than this snapshot.
Git remains `upgrade-branch` at `f058e5220ce8177d38f99360199db0ad013a0a99`,
merge closed, resumed edits uncommitted. `mypatch` remains
`2bfc834d8c5b819b2f120f30de8216f6c35653ba`. No push/integration occurred.

Subsequent retries: `resume-break-bar-03.log` **21/21**, grants 03 **13/13**,
Reduction 02 **20/20**, trainer bars 02 **11/11**, all exit 0. The multihit
diagnostic proved two iterations and a pending layer before refill after the
liveness fix; its remaining scene failure was target plural text (`2 times!`
replaces `2 time(s)!`). The exact two-hit assertion remains. Temporary multihit
logging was removed. Lunar Dance preserves the existing withdrawal message
before the immediate switch; genuine faints keep target deferred replacement.

More Wishes retry 02 passes the reconstructed-screen/leak case but still fails
entry ability observation (17/18). Target `switchineffects` marks entry and
resets its state machine; it no longer executes abilities itself. Added the
missing `switchinevents` before `BS_TxRollbackFinishEntry`/ambush. Retry 03
then reached entry processing but detected forbidden hazard HP loss (17/18).
The target hazard stage now skips only the summoned party member during
`TX_ROLLBACK_ENTERING`; abilities/items continue through the shared dispatcher.
The old `switchIn = FALSE` assignment in initialization was ineffective because
target `switchineffects` sets it TRUE, so it is removed. Retry 04 is running.
Do not delete the strict popup, no-HP-bar, White Herb or ambush assertions.

More Wishes retry 04 passes **18/18**, exit 0. All twelve matrix-02 files now
have passing focused runs after the documented repairs. Normal ROM 09 is
building the resulting source. `resume_custom_matrix_03.py` is running the next
22 files serially (ordinary multihit/Lunar Dance/Healing Wish, items, Intimidate,
hazards, Wishful Thinking/Polymorph/status icons, all Koanko and Kazuradrop
files, ownership and cleanup repeats). Its per-file exits are written to
`resume-custom-matrix-03.json`; do not launch a competing `make check`.

Normal ROM 09 **passed**, exit 0 (`resume-rom-09.log`): EWRAM **238,608 B**,
IWRAM **28,608 B**, ROM **27,049,696 B**. The two missing-final-newline map
warnings remain warnings, not link failures. `resume-rom-09-artifacts.sha256`
records ROM/ELF/source-patch hashes. Recovery evidence for this milestone:
`resume-wish-bars-source.patch`, `resume-wish-bars-battle_faerie_anim.c`,
`resume-wish-bars-migration_owner_apis.c`, `resume-wish-bars-source.sha256`.
Matrix 03 ordinary multihit, Lunar Dance and Healing Wish files have passed;
the remaining files are still running on the same implementation source.

Matrix 03 completed; exact per-file counts are preserved in
`resume-custom-matrix-03.summary.json`. Passed: ordinary multihit 13/13,
Healing Wish 4/4, Red Card 26/26, Eject Button 13/13, Eject Pack 16/16,
Intimidate 16/16, hazards 5/5 and starting hazards 12/12, Wishful Thinking
10/10, Polymorph 9/9, status icons 5/5, Smeargle 17/17, Smeargle ranking
10/10, Guts 3/3, ownership 4/4 and cleanup 6/6. Ordinary Lunar Dance has
**3 PASS + 1 pre-existing KNOWN_FAILING / 4**, exit 0. Stack Breaker has
**1 pre-existing KNOWN_FAILING / 1**, exit 0 (AnimTask_ShakePlatforms task
not freed); neither is a full behavioral pass and no suppression was added.

Five unexpected failures across four files:
- Koanko 12/13: untyped reserves acquired random Tera types under target
  SMART_TERA generation (`koanko.c:51`).
- Koanko AI 36/37: optional Fire Tera activation/cache scenario (`:492`).
- Smeargle diagnostics 2/4: final score differed by one from logged pass sums
  (`koanko_smeargle_diagnostics.c:37`).
- Kazuradrop 8/9: Legendary Clause failed after Landorus's Intimidate
  (`kazuradrop.c:135`). Twelve-faint Koanko progression itself passed.

Smeargle diagnostic retry 02 passes **4/4**, and Kazuradrop retry 02 **9/9**.
The target's separate damage-comparison pass is recorded as diagnostic logic
64; scoring is unchanged. Legendary Clause is a return continuation after the
already-queued switch-in ability; target visits that ability stage once, so
the old `effect == 0` gate never ran afterward. The once-only flag, exact scene
and boost-tier assertions remain intact.

Koanko retry 02 and AI retry 02 still fail at the same fields. The eligibility
edit initially reached only the debug Smeargle generator; it is now also applied
to the actual six-member conjured descriptor. Both temporary descriptors disable
SMART_TERA random-type generation, while the encounter retains its decision flag
and the explicitly authored Fire Calyrex. Retry 03 for both files is running.
Current source is newer than ROM 09 and the Wish/Bar recovery patch.

Koanko retry 03 passes **13/13**. AI retry 03 still failed the activation case.
`ShouldTeraFromCalcs` counted every nonzero type, including target's disabled
`TYPE_MYSTERY`, and excluded Normal. It now excludes the actual disabled
sentinel. Focused Tera retry 04 then passed activation and reached deferral,
where it exposed stale effectiveness (`2048` vs fresh `4096`, `koanko_ai.c:497`).
Target `DecideTerastal` had dropped baseline capture/restoration of effectiveness
when declining Tera. Both damage and effectiveness now use the no-Tera result.
Full Koanko AI retry 05 and stock smart-Tera/Terastal suites are running serially
in `resume-koanko-ai-05.log`, `resume-smart-tera-01.log`, `resume-terastal-01.log`.
No choice assertion or damage/effectiveness equality was weakened.

Koanko AI retry 05 passed **37/37**, stock smart-Tera **4/4** and Terastal
**44/44**, all exit 0. Normal ROM 10 and the full suite are now running on this
frozen implementation; inspect their logs/process state before changing source.

Custom move content audit: all **12** custom move entries are identical to
`mypatch` after only target target-enum spelling and whitespace normalization.
Evidence `resume-custom-move-data-audit.txt`; repeatable read-only script
`audit_custom_move_data.py`. This verifies authored fields, not runtime effects
or the remaining ordinary-move/custom-species data audit.

### Historical usage-limit boundary (superseded above)

The uncompiled/blocked labels in this subsection describe the earlier boundary,
not the current source or execution availability. Continue from the latest
continuation results above; do not replay prior repairs.

- **Uncompiled source correction after the last test run:** target PP canceler
  code directly indexed the four-entry PP array with virtual Arcanum slot 4.
  Restored the baseline bounds check in `CancelerPowerPoints` and bounded PP
  deduction/controller writes in `CancelerPPDeduction`, including conjured
  Smeargle's existing PP/Pressure exemption. Virtual actions retain ordinary
  last-move/Metronome state at this stage; target submove continuation checks
  still run. No MP mechanic was added. This is a statically confirmed out-of-
  bounds integration defect, but it is **not yet established as the sole cause
  of the three Wish failures**. The patch has not compiled or run because
  execution review is unavailable. Current source is beyond ROM 07/test retry 05.
- Further Arcanum history integration still needs audit: attack-string used-
  move recording, move-end history/Mirror Move/Metronome, Grudge PP indexing and
  Smeargle Choice-lock exemptions had baseline guards. Do not assume the PP
  correction alone completes virtual-action migration. Existing `arcanum.c`
  and Smeargle fixtures cover these contracts and must actually run.
- ROM builds through `resume-rom-07.log` passed. Current source additionally
  restores the baseline test observation order in `battle_main.c`; that change
  compiled in the test executable but still needs a new normal ROM build.
- `resume-tx-profiles-05.log`: **9 PASS / 12 total / 3 FAIL**, exit 1. Solo TX
  entry now passes after restoring THEN-before-cleanup. Remaining failures:
  Reduction casting (`tx_profiles.c:303`, unmatched Reduction message), Reduction
  persistence (`:343`, active FALSE versus TRUE), Reciprocity (`:372`, unmatched
  Reciprocity message). Investigate virtual Wish eligibility, selection and
  execution next. Registration/ownership guards were not relaxed.
- Temporary entry diagnostics were removed. Two additional capability assertions
  were just added to the entry fixture: `TxRollbackHasWishes(1)` and virtual-slot
  usability. **They have not run yet.**
- The attempted command `make check -j4 TESTS="TX phase subroutines restore"`
  was **not executed**: automatic approval review failed because the account
  usage limit was reached (retry time reported as 11:50 AM). This was a review
  availability failure, not an unsafe-action determination. Do not bypass it or
  claim `resume-tx-context-01.log` contains a test result.
- No build/test process remains live from the completed attempts above. No new
  commit, push or integration into `mypatch` occurred. Last verified branch
  references remain `upgrade-branch` at `f058e5220ce8177d38f99360199db0ad013a0a99`
  and protected `mypatch` at `2bfc834d8c5b819b2f120f30de8216f6c35653ba`, with
  no open merge. Resumed edits and new animation/ownership fixture files remain
  uncommitted; preserve them and the pre-existing backup file.
- Final read-only Git check confirmed those refs, no MERGE_HEAD and no unmerged
  entries. Resumed tracked edits are unstaged; the two new implementation/test
  files and pre-existing backup are untracked. `git diff --check` completed with
  no whitespace errors (native Git emitted only LF/CRLF advisories for three
  preserved palettes). `latest-resume-source.patch` preserves the resumed tracked
  changes; `latest-resume-source-sha256.json` fingerprints it and the two new
  files. Untracked-file recovery copies also remain in the ROM-05 evidence set.
- Exact next commands, after execution review becomes available, from the
  existing migration worktree in WSL:

```sh
make -j4
make check -j4 TESTS='Koanko TX entry registers'
make check -j4 TESTS='TX phase subroutines restore'
```

  Inspect the newly added capability assertions first; do not rerun unchanged
  failures without investigation. Once corrected, rerun all `tx_profiles.c`,
  then TX wishes, migration ownership and `after_cleanup.c` (the latter verifies
  the restored test observation contract). Continue the full focused matrix,
  broader suites and final normal ROM build. Full regression and emulator gates
  remain outstanding. The goal is active and incomplete.

**Current milestone: ROM build PASS; first focused runtime 8/12 passed.**
TX profiles attempt 03 ran 12 tests: eight unit cases passed; four Koanko battle
cases were INVALID at `battle_message.c:3033` (opponent name lacks terminator),
before their mechanics assertions. The target test-only mock trainer table
omitted production Koanko data. The test build now retains the authored
production table and resolves target mock entries through a TESTING-only
override. No assertion/name check was bypassed, and boss data is not duplicated.
Retry 04 is building from `resume-tx-profiles-04-source.patch`.
Retry 04 finished (exit 1): 8 passed / 12 total, now four genuine FAIL results
rather than INVALID. Solo TX profile remains NONE after entry, Reduction never
activates/persists, and Reciprocity's expected message is absent. A one-case
entry diagnostic confirms the injected scorer ran and Koanko phase is CONJURED;
profile registration is still missing. Investigation is active; do not treat
these as accepted baseline failures or bypass the profile's ownership guards.
Normal ROM rebuild 07 passed with current production trainer-data handling.
Entry diagnostics confirmed the injected callback, conjured phase, trainer,
species, slot and side guards. The registration assertion was instead observing
post-cleanup state: target recorded exit invoked end-turn cleanup before THEN.
Restored the baseline THEN-before-cleanup / AFTER_CLEANUP-after-restoration
contract in `battle_main.c` for recorded truncation and natural finishes.
Temporary entry diagnostics were removed. TX retry 05 is running; registration
guards and original mechanics assertions are unchanged.

Latest corrected ROM: `resume-rom-06.log`, exit 0, EWRAM 238,608 B,
IWRAM 28,608 B, ROM 27,048,488 B. Source/artifact hashes are in
`resume-rom-06-snapshot.sha256`. The production trainer-data/test-only override
change above follows ROM 06; ordinary-build behavior is unchanged by that hook,
but a final normal rebuild is still required.

### Test compilation history

Attempt 02 found two failed fixture objects (Break Charm grant typed
battler arguments and an appended Sleep Clause Uproar assumption). Their source
was corrected while the failed attempt continued independent targets; that
attempt did not run tests.
The same attempt later found Wishful Thinking fixture accesses to removed Wish
storage and old position-to-battler arguments; these are ported too. The first
two repaired fixture objects passed (`resume-final-fixture-objects-01.log`).
Wishful Thinking also passed its focused object check
(`resume-wishful-fixture-01.log`). A later Polymorph fixture failure used removed
`SpeciesInfo.isLegendary`; its exclusion assertion now checks both target
`isSubLegendary` and `isRestrictedLegendary`, retaining Mythical/Ultra Beast/
Paradox/Shedinja checks. Failed units are being repaired after their own compile
failures while attempt 02 continued independent stock test targets.
Attempt 02 finished (exit 1), with the additional Kazuradrop Guts fixture still
using removed `STARTING_STATUS_NONE`/`B_VAR_STARTING_STATUS`. Its same six
species/terrain combinations now use target Reset/SetStartingStatuses APIs.
Attempt 03 starts from `resume-tx-profiles-03-source.patch`; engine code still
matches ROM 06. No test behavior was weakened or marked known-failing.

`resume-rom-05.log` exited 0 and produced the migrated ELF/GBA. Link usage:
EWRAM 233 KiB (91.02%), IWRAM 28,608 B, ROM 27,048,488 B. The stable source
patch, new source/fixture copies, artifact hashes and section sizes are saved
beside the worktree as `resume-rom-05-*`. Focused TX profiles are starting in
`resume-tx-profiles-01.log`; no migrated runtime pass is claimed yet.

ROM SHA-256: `9855dd7ba69210664ac8d91d8efa3a1dab42c8f136ba7ad6ef4d55d37a029353`.
The source patch hash and both newly added source files are captured in
`resume-rom-05-snapshot.sha256`. Current test compilation has the same engine
source; documentation-only updates after the snapshot do not change that code.

The user explicitly resumed migration. The stopping boundary below describes
the preceding pass. Verified starting state: checkpoint merge commit
`f058e5220ce8177d38f99360199db0ad013a0a99`, no open merge/unmerged entries,
and protected `mypatch` still `2bfc834d8c5b819b2f120f30de8216f6c35653ba`.
Only the documented backup file was untracked before resumed edits.

- `resume-custom-c-01.log`: PASS for TX creation, Koanko encounter/tactical AI,
  Castoria and trainer Break Bar generation. Trainer-local resources are mapped
  to target owners; fresh generated Pokemon replace removed gimmick eligibility
  masks; trainer gimmick consumption is retained except the explicit fresh Tera
  opportunity. Generation buffer counting no longer passes a pointer to the
  target trainer-ID API. Castoria's Sky Drop release uses the target script
  command before canceling the holder's action. Setup scoring preserves the
  exact baseline moves rather than scoring every generic stat-effect move alike.
- `resume-script-apis-01.log`: PASS for the changed move resolver, script-command,
  TX-wish and custom animation objects and all three battle/animation assembly
  targets. `SetFutureSightAttack` is shared between the target canceler and a TX
  phase native at the original scheduling point. The custom passive faint flow
  uses target Gulp Missile dispatch, Dynamax cleanup and Sky Drop release.
  Eleven particle preloads use a custom visual task with target-managed assets
  and preserve the original one-frame wait at each loading point.
- These are compilation/assembly results. No migrated runtime behavior is
  verified yet. The next complete ROM build is logged as `resume-rom-01.log`.
  Earlier failures below are retained as historical checkpoint diagnostics.
- ROM attempts 01/02 exposed a removed task-count helper and end-turn callback/
  Heal Block field changes; these were adapted. Attempt 03 continues independent
  targets and graphics generation. Its Break Charm activation/header failures
  were repaired separately: opening and later entries now share the target
  switch-in call/return path. `resume-battle-main-01.log` compiled held-item
  dispatch and exposed a first-turn state spelling mismatch, subsequently fixed.
- Existing ownership fixtures were adapted without dropping their behavioral
  assertions: trainer pool entries use independent A/B parties; Koanko tests
  inspect fresh Pokemon gimmick eligibility and per-slot Knock Off state, and
  retain an independent opponent-B EXP-state canary. These tests have not run.
- `resume-failed-units-01.log`: PASS for battle main and the two custom dialogue
  modules after updating the first-turn state name and target text-printer API.
  `resume-owner-fixture-syntax-02.log`: syntax/type check PASS for Koanko and
  trainer Break Bar fixtures with `__INTELLISENSE__` graphics stubs; this is
  neither a real test-object build nor runtime validation.
- Known post-release fixes were inspected locally. Applied the exact terrain
  correction/test from `be3b089fc121143b7ccf6503ea1385b8abe05b8c` (terrain enum
  passed to AI helpers; grounded-target test for Grassy Terrain damage). Adapted
  `8b10030b4c42f2b481567bcf43eb46eb64679d93` for trainer-owned pivot/phazing:
  side counts include both owners and pending Break Bar layers; forced reserves
  come from the target owner's whole party, excluding active slots only when
  parties are shared. No Order Change remains before forced-switch processing.
  These are the two specifically requested defect dispositions, not a new
  upstream target. Runtime validation is pending.
- Ownership audit follow-ups: Polymorph's dedicated snapshot array now has
  one row per target trainer, indexed with `GetBattlerTrainer`, preventing
  partner/opponent-B collisions at identical party slots. Persistent player
  battle-exit restoration remains player-owned. Final AI target selection
  reads Smeargle's committed sampled move from live slot 0; ordinary target
  controller-buffer behavior is retained. New `migration_owner_apis.c` covers
  those regressions, independent pending-layer side survival and owner-local
  Roar reserves. Syntax check 03 passed for all four fixtures; no runtime
  tests have run. `resume-owner-compile-01.log` passed actual object compilation
  for controllers, script commands, AI main, battle util, Castoria and move data
  (normal compiler pipeline with `NODEP=1` for already-built independent units).
  Full dependency-driven ROM compilation/link validation is still pending.
- `resume-custom-fixture-syntax-04.log`: syntax/type check PASS for selected
  custom fixture groups (`arcanum*.c`, `break_bar*.c`, `tx_*.c`, `koanko*.c`,
  trainer Break Bars and migration ownership). Ports retain assertions while
  using target volatiles, Wish/Future Sight records, configuration keys, fixed-IV
  Pokemon creation and explicit full-stat copying. Koanko AI fixtures construct
  target `AiCalcValues`, retaining controlled weather/gimmicks and scratch-state
  assertions. Two executable fixture scripts now use target opcode constants
  instead of stale numeric bytes. These are preparation checks, not runtime
  passes. Actual TEST=1 object/harness compilation is in progress.
- ROM attempt 03 subsequently exposed the target League lighting reference to
  `FLAG_TEMP_5`. Restored standard temporary aliases 5/6 while keeping the
  custom Caster/Kazuradrop names and numeric values unchanged. The map-local
  flags retain their original lifecycle. `resume-field-specials-01.log`: actual
  object compilation PASS. The ongoing attempt retains historical errors;
  a subsequent normal dependency-driven build is still necessary.
- Attempt 03 finished (exit 1); all its exact C failures have been repaired in
  focused compilation. Normal dependency-driven attempt 04 is running. Attempt
  03's first asset generation completed; do not repeat or discard those outputs.
  `resume-test-objects-01.log` compiled TX profiles and migration ownership
  fixtures, but found two target enum arguments and one controller callback
  mismatch in wish fixtures, now repaired. Its two missing `src/test_runner*.s`
  failures were incorrect requested object paths, not source defects; actual
  harness sources are `test/test_runner*.c`. Attempt 02 uses those paths.
- `resume-test-objects-03.log`: PASS for both real TEST=1 harness objects and
  TX profiles, TX wishes, More Wishes and migration ownership fixture objects.
  The harness's summoned-opponent declaration now uses fixed-IV creation and
  its actual fixed test battler ID; it must work before the runtime summon is
  present in the position map. Other custom test-object compilation is ongoing.
- `resume-custom-test-objects-02.log`: PASS for 17 selected custom fixture
  objects (Arcanum/Wish, Reduction, Break Bars/exit HP, TX profiles/wishes,
  Koanko AI/sampling/diagnostics/ranking, trainer/wild bars). Target typed battler
  lookups, gimmick enums, callback cleanup and task-exhaustion setup replace
  retired APIs. These are actual compiler-pipeline checks, not runtime passes.
- ROM attempt 04 reached linking and failed on exactly three unresolved symbols:
  `UnsetPsychicBg`, `mus_bb_channel_fixed`, `mus_bb_channelGO`. No compilation
  errors remained. Restored the baseline standalone assembly-song object
  registrations in Makefile, using the retained `audio_rules.mk` recipe; tracks
  are not concatenated into sound_data (their `.end` directives forbid that).
  The Hurricane throw-out tail now calls target `UnsetBackground`, whose body
  matches the old restore/fade/sentinel/wait/return sequence. Attempt 05 is
  running. Attempt 04's incomplete link reported EWRAM 233 KiB, IWRAM 28,608 B,
  ROM 27,040,416 B; these are not successful-link acceptance measurements.
- Read-only decimal-ID scan during test compilation: old custom species
  1550–1554 occur only in target species enum declarations, trainer `#line`
  directives and one weight value. No hardcoded old custom species reference
  was found across `src/include/data/asm/test`. A narrower context search for
  old ability IDs 311/312 and Break Charm 855 also produced no candidates.
  Evidence: `resume-old-species-id-audit.txt` and
  `resume-old-other-id-candidates.txt`. This is not a complete binary/generated
  data or all-identifier audit; symbolic IDs remain authoritative.
- First focused TX attempt stopped in test-executable compilation (exit 1),
  before selecting/running tests. Exact failures: `test/test_test_runner.c`
  typeof on `grassyTerrain` bitfield and removed `TESTING_FLAG_UNUSED_2` (now
  trainer slides); `test/battle/after_cleanup.c` old u8 RNG weights callback
  versus target u16 weights, plus a typed battler argument. Fixed by promotion,
  an actually unused testing flag, matching u16 callback and position lookup.
- A source audit confirmed a migration regression in TX phase/recovery context:
  baseline move-result flags were u16, target flags are u32. The snapshot array
  now uses u32 for all four battlers, preserving high bits and all slots during
  nested scripted phase/recovery work. Added a full-width, four-slot restoration
  regression in `tx_wishes.c`. This correction was made after the failed test
  build stopped, preserving the earlier build snapshot. ROM attempt 06 is
  rebuilding this correction before focused validation resumes.

**Incomplete migration. Not an acceptance build. No migrated battle behavior has been verified.**
The user requested this checkpoint because the current usage window is nearly
exhausted. Stop implementation at this boundary; resume from the working tree,
not from older plans or an assumed clean upstream checkout.

## Location, target, and recovery

- Migration worktree: `TEMPSCRIPTS/upstream-1.17.2/worktree`, inside the original
  `pokeemerald-expansion` checkout. Branch: `upgrade-branch`.
- Protected original branch and pre-migration HEAD: `mypatch`,
  `2bfc834d8c5b819b2f120f30de8216f6c35653ba`.
- Exact target: `expansion/1.17.2`,
  `79dd436920845ebc145577649de624f65298c209`.
- The initial merge was closed by checkpoint commit
  `f058e5220ce8177d38f99360199db0ad013a0a99`, named
  `WIP: checkpoint upstream 1.17.2 migration (not buildable)`. Its parents are
  the protected baseline and exact target above. All 107 original textual
  conflicts were resolved; no `MERGE_HEAD` or unmerged entries remain.
  Resumed migration edits are uncommitted. This records source integration,
  not successful validation or acceptance; verify actual state on resumption.
- No push or merge into `mypatch` has occurred. Do not reset, clean, rebase,
  discard the original state, or treat a checkpoint commit as acceptance.
- Full preservation archive:
  `TEMPSCRIPTS/upstream-1.17.2/pre-migration.tar.gz`; SHA-256:
  `3aa906fafc2ec73ca43906ca83ca996f4ba16ce44a11ed17585a2356192cc629`.
  It includes relevant tracked/untracked/ignored source and assets, including
  the simplified fresh-game startup and numerical Break Charm HP fix.
  `snapshot-sha256.json` records 57,607 paths. The archive excludes `.git`, build
  objects, the code index, and its own evidence directory.
- `conversion-input/` preserves the baseline conversion inputs; `target/`
  preserves an archive of the pinned target. The evidence directory also holds
  conversion scripts, resolution scripts, manifests, and build logs. These are
  local recovery evidence, not all tracked deliverables.
- Worktree links are intentionally relative for Windows/MSYS Git and WSL:
  worktree `.git` is `gitdir: ../../../.git/worktrees/worktree`; its metadata
  `gitdir` points to `../../../TEMPSCRIPTS/upstream-1.17.2/worktree/.git`.
  Do not replace these with platform-specific absolute paths.
- `graphics_file_rules.mk.bak` is pre-existing preservation material. Leave it
  intact; the migration copy is untracked. Do not stage it as a feature change.

## Integration accomplished, with limits

The direct 1.14.2 → 1.17.2 route was used; no intermediate bridge was necessary
for conversion diagnostics. Chapter 14, Section 11 records the earlier work.

1. Target data/interfaces are merged while retaining custom species, moves,
   abilities, items, trainers, maps, title assets and the shortened fresh-game
   flow. Custom species are 1573–1576, Egg/NUM_SPECIES 1577; Break Charm 874;
   abilities 320/321; custom moves 848–859. ARM identifier assertions passed.
   GEN_LATEST remains GEN_9, player level cap 100 and engine maximum 105.
   These checks do not establish complete hardcoded-ID or content consistency.
2. Trainer parties use the target generator in `battle_setup.c`. The old custom
   generator in `battle_main.c` is retired. `trainer_break_bars.c` records
   selected source-entry configurations before battle allocation, then applies
   them after allocation. PartyState uses MAX_BATTLE_TRAINERS. Owner/selection,
   partner/multi, preview generation and runtime replacement still need tests.
3. Custom graphics init/free follows `AllocateBattleGfxResources` and
   `FreeBattleGfxResources`; full teardown separately clears pending profiles.
   Menu reconstruction is intended to retain combat state. The separate aligned
   guarded 4096-byte EWRAM test invocation stack is retained. Integrated memory
   and emulator checks have not run.
4. Battle-command conflicts use the target opcode table and target algorithms.
   Old attackcanceler, Cmd_adjustdamage, SetMoveEffect switch and monolithic
   move-end code were not reinstated. Custom native callbacks were retained.
5. `battle_move_resolution.c` now carries Break Bar checks in direct HP updates,
   damage skipping and the new faint block. Eligible zero HP queues a pending
   layer before true-faint effects; phase scripts remain deferred through the
   existing after-action resolver. `Cmd_tryfaintmon` retains its independent
   gate for scripted/passive paths. Reward/EXP and multihit behavior are untested.
6. `Cmd_healthbarupdate` and `Cmd_datahpupdate` apply the passive-damage floor,
   suppress mutation of pending layers, preserve healing bookkeeping and invoke
   TX mirrored healing after actual recovery. New `instantDeathHpUpdate` is a
   per-battler bit mask: explicit native marking bypasses the floor, ordinary
   damage/heal setters clear it, and the HP commit consumes it. Existing custom
   instant-death scripts use the native rather than restoring old HP-mode args.
7. `battle_faerie_move_resolution.inc` holds Timestop, Bug Space blocking and
   Trash & Crush policy adjacent to narrow target hooks. `trashCrushOhko` resets
   per action and records target rolls. Its OHKO override remains after survival
   adjustment; Bug Space's separate 9999-damage crushing tier remains before
   survival adjustment. Kazuradrop's Guts snap and controller update are ported.
   Neither execution order nor graphics have runtime verification.
8. Melt Virus and Timestop have target SetEffect handlers. Infinite Growth's
   native retains activation and moveset replacement. No Order Change was added
   to target move-failure and Emergency Exit paths as well as retained command
   and AI-switch guards. Complete pivot/phazing coverage remains outstanding.
9. AI now returns target `ChosenAction`, retains signed candidate scores,
   legality/tie handling, Arcanum choices and Koanko's slot-0 hook. Ordinary and
   hypothetical slot-0 scoring share target scoring passes. Damage calculation
   uses AiCalcValues and restores relevant scratch fields. Custom dynamic AI
   and Break Bar-aware outcomes remain, but tactical modules are only partially
   migrated and simulations are not behaviorally validated.
10. TX ownerSide was changed to ownerTrainer using GetBattlerTrainer; masked
    Reduction/TX controller normalization now compares trainer owners. Side-wide
    effects remain side-wide. Summoned TX still uses opponent A's local slot 5.
    Name Theft and pre-hazard Break Charm have target switch-in stages; Lunar
    Dance still grants a layer to a healthy recipient. These ports remain
    incomplete and untested.
11. Target packed `callnative` parsing is shared by custom modules through new
    `include/battle_script_native.h`. Custom field references have begun moving
    from removed DisableStructs to target Volatiles. This is a partial API port,
    not a general compatibility layer restoring old state ownership.

Upstream merge impact: the new party generator, effect dispatcher, move resolver
and graphics lifetime replace obsolete integration locations. The move/faint
changes are substantive semantic integrations despite their small hooks. Keep
custom policy separate, but do not equate a smaller diff with correctness.

## Actual validation

Frozen original baseline, before migration:

- `make -j4`: PASS. EWRAM 239,764 B; IWRAM 28,812 B; ROM 25,184,640 B.
- `make check -j4 TESTS='test/battle/tx_profiles.c'`: 12/12 PASS.
- `make check -j4`: 4,896 total; 4,096 passed; 18 known-failing; 779 TODO;
  3 failed, exactly the accepted text-width cases (Roar of Eternity Contest
  name, Fireball summary description, Castoria real-name battle message).
  Zero unexpected battle failures. Exit 1 is retained; KNOWN_FAILING was not
  expanded. A separate `TESTS='Break Charm'` selected zero and is not coverage.

Migrated source:

- Identifier manifest/boundary assertions and ARM header compilation: PASS.
- First complete-build attempt (`migration-build-01.log`): FAIL at AI API
  mismatches. Those particular core errors were subsequently repaired.
- Six core units compiled successfully in `core-build-02.log`.
- Seven core units passed in `checkpoint-core-build.log`: move resolution,
  script commands, battle utility, AI main, AI utility, effect dispatch and
  switch-in. A final same-target check is recorded separately as
  `checkpoint-core-build-final.log`: PASS, exit 0. That check reused unchanged
  objects where appropriate and rebuilt the final move-resolution edit.
  The core log includes unused-local and deprecated-player-party warnings.
- Focused custom-module compilation (`custom-build-04.log`): FAIL. TX wishes,
  TX health, Reduction, Smeargle and switch-in compiled; TX creation, Koanko
  encounter/tactical AI and Castoria still failed. Exact diagnostics follow.
- Script assembly (`checkpoint-script-build.log`): FAIL for battle_scripts_1
  and battle_anim_scripts; battle_scripts_2 assembled successfully.
- **No migrated ROM has linked successfully. No migrated automated runtime
  tests or full suite have run. No manual emulator validation has occurred.**
  Some standalone syntax probes used mocked graphics declarations; those are
  not substitutes for the actual successful object-compilation checks above.

## Exact current focused failures

`custom-build-04.log` records these source diagnostics (line numbers refer to
the checkpoint source before subsequent edits):

```text
battle_tx_rollback.c:253: enum BattlerPosition -> enum BattlerId conversion
battle_koanko.c:47: TrainerBattleParameter.mode and TRAINER_BATTLE_SINGLE_NO_INTRO_TEXT removed
battle_koanko.c:106,113: enum BattlerPosition -> enum BattlerId conversion
battle_koanko.c:130: gWishFutureKnock undeclared
battle_koanko.c:131: assignment to givenExpMons array
battle_koanko.c:132,133: opponentMonCanTera/opponentMonCanDynamax removed
battle_koanko.c:158: TryFormChange argument 1 needs pointer; argument 2/3 enum mismatches
battle_koanko.c:168-170: ClearBattlerMoveHistory/AbilityHistory/ItemEffectHistory undeclared
battle_koanko.c:181: BattleStruct.changedItems removed
battle_koanko_ai.c:495: Volatiles.protectUses removed
castoria_name_theft.c:199: BATTLE_OPPOSITE undeclared
castoria_name_theft.c:698: TryBattleFormChange missing argument
castoria_name_theft.c:725: SKY_DROP_CANCEL_MULTI_TURN_MOVES removed; CancelMultiTurnMoves takes fewer arguments and returns void
castoria_name_theft.c:881: SpeciesInfo.isLegendary replaced by category fields
castoria_name_theft.c:965: SpeciesCanLearnEggMove undeclared
castoria_name_theft.c:1059-1063: EFFECT_ATTACK_UP_2, EFFECT_SPECIAL_ATTACK_UP_2, EFFECT_DRAGON_DANCE, EFFECT_QUIVER_DANCE, EFFECT_SHELL_SMASH removed
```

These are migration errors, not accepted baseline test failures. Castoria's
Sky Drop continuation needs an intentional target-engine port, not deletion of
the behavior or a cast to hide the error. Koanko's resource/gimmick preservation
needs owner-aware field replacements, not removal of its reset policy.

Script diagnostics:

```text
data/battle_scripts_1.s:2784: bad instruction tryactivategulpmissile
data/battle_scripts_1.s:6820: bad instruction setfutureattack
data/battle_anim_scripts.s:5706,5732,35113,35114,35139-35142,35190-35192:
    bad instruction loadspritegfx (11 occurrences)
```

The old explicit sprite loading command must be adapted with deliberate wait
timing. Do not blindly run the upstream wait-removal converter over custom
animations. Full logs remain under `TEMPSCRIPTS/upstream-1.17.2/`.

## Known semantic risks and incomplete work

- All current ports are implementation-only. Specifically audit Break Bar
  pending state across multihit/spread, simultaneous KOs, recoil, faint-block
  state counters, controller HP sync, true-faint rewards and after-action phase
  continuations. A passing object build proves none of these contracts.
- The custom passive faint script now uses a narrow target Gulp Missile dispatch
  native and assembles. Target ordinary faint processing and the Bug Space
  special presentation still need ordering/continuation regression validation.
- Trash & Crush's old script-side damage pipeline was removed because target
  attackcanceler already applies HP. Verify its chance, Minimize/Bug Space
  exceptions, survival bypass, messages and secondary-effect ordering.
- End2 call sites were converted according to entry context; custom end-turn
  scripts use return under BattleScriptCall, execute wrappers use end, callback
  wrappers retain end3. Audit every custom native continuation and script stack,
  especially TX ambush, forced phase moves, Name Theft and Koanko fake victory.
- TX/Reduction owner and normalization changes need partner/multi tests. Do not
  replace side-wide policy with trainer ownership indiscriminately. Wish
  recipient identity, revival bans and canonical HP fraction restoration are
  acceptance requirements. New target state also needs complete scratch-context
  restoration during TX phase/ambush execution.
- Koanko encounter generation/reset now compiles but is unverified. Preserve 12 opponents,
  fixed progression, final-only victory/rewards, player resources, trainer items
  and the explicit fresh Tera opportunity for the conjured team.
- Custom AI modules compile but remain semantically unverified. Preserve signed scores, tie RNG policy,
  virtual slot legality and commitment, scratch restoration, dynamic callback
  registration, and deliberate information restrictions. Tactical setup ranking
  was mapped to the exact eight baseline move identities; do not broaden it.
- Custom move data compiles after target-enum conversion but still needs an
  audit of new stat-effect representation and preserved content/balance. The
  dependency-driven full build has not completed. Custom test fixtures still
  contain old volatile/field APIs and need an assertion-preserving target port.
- Earlier presentation/content resolutions need full compilation and visual
  checks: title/menu, player avatar, sprites, status icons, Polymorph, Break Bar
  colors, numerical HP grant, Bag/party/summary reconstruction and full cleanup.
- The two specifically identified upstream-master terrain/pivot fixes are
  applied/adapted as recorded above and compile; regression validation remains.
  No arbitrary master code was imported.
- Complete numeric-ID, generated-data, partner/multi trainer-party fixtures,
  missing custom INCGFX conversions and old CreateMon call-site audits remain.
- No Stage 05 work, balance redesign, save converter or test suppression is
  authorized by this checkpoint. Saves remain disposable.

## Resume procedure for a fresh session

1. Read this log, Chapter 14 Section 11, the migration request
   `C:/Users/pcrispin/Documents/GithubEmerald/MigrationUpstream.md`, AGENTS.md,
   and the upstream modification ledger. The task-specific autonomous test
   authorization continues for the migration; no push/final integration is
   authorized. The user subsequently resumed autonomous work; the earlier
   stopping boundary is historical, and the migration is still incomplete.
2. Inspect `git status`, `git log -1 --format=fuller`, `git ls-files -u`, and
   `git rev-parse mypatch`. Determine whether the checkpoint was committed or
   the merge is still open. Do not rerun resolution scripts: most assert that
   original conflict markers exist and are intentionally single-use.
3. The initial custom C/script errors below have been repaired. Inspect the
   current `resume-rom-*.log` and process state first; attempt 03 performs the
   first complete asset generation and retains historical failures corrected
   by focused object builds. Let active generation finish before a new normal
   dependency-driven build. Do not replay any recorded one-use edit scripts.
   Continue from new compiler/linker failures, then port custom fixture APIs
   without weakening their assertions and run focused behavior tests.
4. From WSL, without a `wsl` prefix:

```sh
cd TEMPSCRIPTS/upstream-1.17.2/worktree
make -j4 -k build/emerald/src/battle_tx_rollback.o build/emerald/src/battle_tx_wishes.o build/emerald/src/battle_tx_health.o build/emerald/src/battle_reduction.o build/emerald/src/battle_koanko.o build/emerald/src/battle_koanko_ai.o build/emerald/src/battle_koanko_smeargle.o build/emerald/src/castoria_name_theft.o build/emerald/src/battle_switch_in.o
make -j3 -k build/emerald/data/battle_scripts_1.o build/emerald/data/battle_scripts_2.o build/emerald/data/battle_anim_scripts.o
make -j4
```

5. Once a coherent ROM/test build exists, run focused existing tests with
   **nonzero selection counts**, repair regressions, then run the full suite.
   Locate exact fixtures with `rg --files test/battle`; an initial useful check:

```sh
make check -j4 TESTS='test/battle/tx_profiles.c'
```

   Then run Break Bars/Charms, wild capture, Arcanum Wish, TX wishes/maxHP,
   Reduction/Polymorph/Wishful Thinking, Koanko AI/progression, Kazuradrop,
   partner/multi and ordinary engine suites listed in Chapter 14. Finally:

```sh
make check -j4
```

6. Freeze source while validating a snapshot; retain hashes/logs and actual
   selected/pass/fail counts. Keep the three accepted baseline text failures
   visible and investigate every additional failure. Perform manual emulator
   checks for fresh-game startup, all boss phases, animations/healthboxes and
   repeated menu reconstruction. Update implementation docs and ledger at each
   coherent milestone. Report acceptance readiness separately from compilation.
