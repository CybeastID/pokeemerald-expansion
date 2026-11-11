# Upstream migration checkpoint — 2026-11-11

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
- At log creation, an uncommitted `--no-commit --no-ff` merge is open, with
  that target as `MERGE_HEAD`. All 107 originally conflicted paths have textual
  resolutions; `git ls-files -u` returned no entries. This does **not** mean
  semantic integration is complete. There are staged and unstaged migration
  edits. The authorized checkpoint commit is named
  `WIP: checkpoint upstream 1.17.2 migration (not buildable)` and closes this
  merge if creation succeeds. Use `git log -1 --format=fuller` to identify its
  hash. Verify actual Git state before proceeding. A WIP merge checkpoint
  records source integration, not successful validation or acceptance.
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
- The custom passive faint script still contains a removed Gulp Missile command;
  reconcile target faint/ability ordering before restoring it. Target ordinary
  faint processing and the Bug Space special presentation must remain compatible.
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
- Koanko encounter generation/reset is incomplete. Preserve 12 opponents,
  fixed progression, final-only victory/rewards, player resources, trainer items
  and the explicit fresh Tera opportunity for the conjured team.
- Custom AI modules are incomplete. Preserve signed scores, tie RNG policy,
  virtual slot legality and commitment, scratch restoration, dynamic callback
  registration, and deliberate information restrictions. Tactical setup ranking
  was mapped to the exact eight baseline move identities; do not broaden it.
- Custom move data still needs a full audit for removed target/effect enums and
  new stat-effect data representation. Other custom C files have not all compiled.
- Earlier presentation/content resolutions need full compilation and visual
  checks: title/menu, player avatar, sprites, status icons, Polymorph, Break Bar
  colors, numerical HP grant, Bag/party/summary reconstruction and full cleanup.
- The two upstream-master terrain/pivot fixes identified in Chapter 14 have
  not been imported or dispositioned. Inspect exact fixes and apply only a
  justified minimal correction if needed. Do not import arbitrary master code.
- Complete numeric-ID, generated-data, partner/multi trainer-party fixtures,
  missing custom INCGFX conversions and old CreateMon call-site audits remain.
- No Stage 05 work, balance redesign, save converter or test suppression is
  authorized by this checkpoint. Saves remain disposable.

## Resume procedure for a fresh session

1. Read this log, Chapter 14 Section 11, the migration request
   `C:/Users/pcrispin/Documents/GithubEmerald/MigrationUpstream.md`, AGENTS.md,
   and the upstream modification ledger. The task-specific autonomous test
   authorization continues for the migration; no push/final integration is
   authorized. Treat the user's checkpoint directive as a stopping boundary
   for this pass, not a declaration that the migration is finished.
2. Inspect `git status`, `git log -1 --format=fuller`, `git ls-files -u`, and
   `git rev-parse mypatch`. Determine whether the checkpoint was committed or
   the merge is still open. Do not rerun resolution scripts: most assert that
   original conflict markers exist and are intentionally single-use.
3. Resume with the exact custom-module errors above and target API definitions.
   Finish ownership/state ports coherently, then repair the script assembly
   failures with explicit continuation/timing review. Do not treat zero textual
   conflicts as a completed subsystem.
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
