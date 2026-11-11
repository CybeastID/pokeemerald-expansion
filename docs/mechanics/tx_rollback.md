# TX Rollback / Wish for More Wishes

## Current scope

The engine supports Castoria's fake-name-route creation of TX Rollback during an
ongoing local Singles Trainer battle. Production eligibility is confined to
Razeluxe with Castoria in party position five and an empty sixth position.
Production Castoria Wish scoring still selects only Domination. More Wishes
tactics and TX virtual-action scoring remain deferred to Stage 05. Stage 04
implements separate summoned/solo combat profiles, the summoned support kit,
maxHP deterioration and solo Reduction; runtime validation is pending.
Link battles, facilities and production recorded playback are unsupported. Tests
explicitly supply a profile and use the test runner's controller plumbing.

`SPECIES_TX_ROLLBACK` occupies the previously unused `GEN_CUSTOM_START` value.
Existing species IDs and `NUM_SPECIES` are unchanged. Its independently editable
species entry initially copies Jirachi's data. The current front uses
`graphics/pokemon/jirachi/tx_rollback/anim_front.png` (64x128, two indexed frames)
and its matching `normal2.pal`; back and icon assets are temporary Jirachi assets.
The conjured party member does not credit ordinary Jirachi as seen. A standalone
custom Dex entry is deferred.

The prototype creature matches the caster's level, with 31 IVs, zero EVs, Hardy
nature, no item, and Follow Me, Wide Guard, Life Dew and Power Gem. Koanko's
separate authored TX keeps Wish, Doom Desire, Protect and an empty fourth slot,
with its existing held item, EVs and nature. Ordinary Jirachi is unchanged.

## Ownership and position policy

`BattleStruct.txRollback` owns the combat profile, owner side/party index, summon
lifecycle, health overlay and script return context. `combatProfile` is distinct
from the existing `profile` pointer, which supplies an ambush script. Ownership
follows that party entry through changes
to its current species or ability. The species itself grants no bars, trapping,
revival restrictions or extra actions.

`TX_PROFILE_CASTORIA_SUMMON` alone enables formation, ambush, original bars,
sixth-slot reservation and the revival ban. `TX_PROFILE_KOANKO_SOLO` registers
from `KoankoOnSwitchIn` for the conjured Koanko trainer's opponent-side party
index 1. It retains ordinary Singles topology, zero configured bars and the
authored No Order Change rule. Registration does not modify the party kit or
install a new AI callback. Re-entry preserves active local effects; genuine
defeat clears them, and a legitimately revived solo owner may register again.
The species alone and unrelated trainer identities do not register an owner.

The real sixth enemy party member is created only on successful execution.
The engine then expands from two to four positions, preserving the existing two
battlers, choices and action queue. Only `BATTLE_TYPE_DOUBLE` is added; there is
still one enemy trainer. The four-position topology lasts until battle cleanup.

`TxRollbackGetSlotPolicy` distinguishes normal, disabled, reserved and retired
positions. Player-right is permanently disabled. Enemy-right is reserved for the
creation and is retired after its final faint. These positions cannot receive
ordinary replacements, even when reserves are healthy or have just been revived.
Temporary absent flags are not the authority for this policy.

The policy gates replacement availability, controller party prompts, revived
partner entry, sprite presence, and menu-return reconstruction. The sole player
retains Singles coordinates and a Singles healthbox irrespective of reserve count.
Valid, absent-player bookkeeping must not reserve a real bench member or inherit
its pending Break Bar.

## Creation order and bars

Wish first passes the ordinary Arcanum attack canceler and validates the committed
outcome. More Wishes requests creation without spending Wish yet. Following the
Wish animation, creation rechecks its preconditions and sprite headroom before
creating the party member and initializing the new positions. Failed preparation
does not consume Wish or convert the battle.

The creation script then:

1. Requests and loads the real party member through the controller.
2. Finishes `switchindataupdate`, including `SwitchInClearSetData` cleanup.
3. Captures original maxHP, registers two extra Break Bars and applies No Order Change.
4. Loads the missing right-enemy panel/shadow resources, slides Castoria from
   Singles x=176 to doubles x=200 over twelve frames, then presents the creation.
5. Runs ordinary entry abilities/items/reactions, skipping creation hazards.
6. Preserves No Order Change on the surviving creation and restores cast context.
7. Completes Castoria's Wish move-end effects and pending Break Bars.
8. Dispatches the one-time ambush before advancing the original action cursor.

No occupant initialization runs after No Order Change is applied. Successful
initialization commits Wish use; subsequent combat effects do not refund it.

Bars use the shared refill path: blue, purple, then red, with actual maxima
100%, 60%, then 30% of the original maximum. Both percentages are floored
independently from the original value, with a minimum of one HP. Their normal
damage floors, overkill isolation, move-completion timing and phase processing
apply. A break is not a genuine faint and preserves No Order Change. No additional
phase cures are assigned. The first original break (blue to purple) refills, then
scripts ordinary Wish on TX. The second (purple to red) refills, applies Reduction
if inactive, then scripts Doom Desire against the active player. These extra
actions cost no PP, ignore ordinary turn blockers, and preserve the pending turn
queue. Ordinary Wish's Heal Block/pending-Wish restrictions and Doom Desire's
occupied delayed-attack-slot restriction still apply. Saved layers do not replay
these phase scripts. An existing Reduction curse is never reset; a newly applied
one skips its activation turn. Reduction remains active after TX's defeat.

## Scripted ambush contract

`TxRollbackProfile.ambushScript` must point to a battle-long script subroutine.
The payload must **return** to its wrapper; do not end an ordinary action or jump
to an ordinary move's terminal MoveEnd script. The default payload grants the
original active, living Castoria a full saved Break Bar (Endurance), preserving
the covered HP and respecting the five-layer cap. Tests may override the payload.

The wrapper establishes TX Rollback as attacker and the sole player position as
the initial target. The payload supplies any effect-specific targeting,
validation, accuracy and defense rules. It can yield for text, animation,
controllers or nested scripts. It does not require a move slot or deduct PP, and
ordinary action blockers do not prevent it or tick an extra ordinary status turn.
Wish payloads use their own effect logic rather than resetting or reusing
Castoria's shared Wish consumption state.

The lifecycle marks the ambush running before dispatch, preventing duplicate
starts. Completion explicitly resumes through any pending bar/faint scripts,
then restores transient execution context while retaining real combat effects.
The normal turn cursor is not advanced by the ambush. Previously chosen actions,
their targets and PP bookkeeping survive; normal action restrictions and genuine
faint cancellation still apply. A replacement cannot inherit its predecessor's
pending action. TX Rollback starts ordinary move selection next turn.

A genuine pre-dispatch faint or battle ending cancels the pending event permanently.
A pending bar is resolved before deciding whether the creature survives. An
unexecuted ambush never carries into another turn.

## Reusable Wishes

Each registered combat owner has one virtual, PP-independent `MOVE_ARCANUM_WISH`
slot. It persists through species and ability changes and grants no spells to
ordinary Jirachi. Later actions obey normal virtual-slot restrictions. Special
AI nominates `txRollback.wishCandidate` and `wishCandidateTarget` (a battler);
selection commits the outcome and recipient party identity only when Wish wins.
Ordinary moves win ties. Production scoring declines TX spell candidates for now.
Tests can nominate outcomes with `TxRollbackCanWishFor`. No successful TX Wish
spends Castoria's allowance, and each effect can be reused when eligible.

- **Reversal:** directly maps all positive player stat stages to equivalent
  negative stages, including Accuracy/Evasion, leaving other stages intact.
- **Reclamation:** rolls tagged RNG for 1–3 distinct eligible benefits and steals
  them without replacement. Benefits are individual boosted stats, Substitute,
  Aqua Ring, Ingrain, Reflect, Light Screen, Aurora Veil, Tailwind, Safeguard and
  Mist. Boosts are removed in full and added to TX up to +6. Existing recipient
  benefits are skipped. Side timers retain their remaining durations. Substitute
  HP is scaled proportionally to TX's max HP, rounded down with a minimum of one.
- **Calamity:** applies normal Perish Song state/timer 3 only to living active
  player occupants without an existing count. Switching clears it normally;
  replacements are not automatically affected. Enemy occupants are never affected,
  and TX's defeat does not cancel player countdowns.
- **Restoration:** restores one depleted extra layer on either original active,
  living ally, up to that owner's historical highest layer count and engine cap.
  It creates a saved full layer; breaking it reveals covered HP without phase
  replay. Pending transitions, replacements, reserves and genuine faints fail.
- **Reciprocity:** activates once until TX's genuine defeat, surviving breaks.
  Genuine healing of living active players mirrors the fraction of their current
  max HP actually restored onto TX's current layer. Round down with minimum one
  and cap at missing HP. This bypasses TX's Heal Block. Moves, items, drain,
  abilities and passive recovery qualify; reserve healing, revivals, bar changes,
  HP rescaling and redistribution such as Pain Split do not. It cannot recurse,
  revive TX, or heal a pending-break layer.
- **Reduction (solo profile only):** replaces Restoration in the outcome list.
  Starts the existing player-side curse at 1, then 2, then 3 levels per subsequent
  turn, with casting-turn grace. `ReductionTryActivate(initialLoss, affectedSide)`
  is idempotent; an already active curse cannot reset, stack or reward a failed
  recast. `ReductionActivate()` remains the original 10+ player-side wrapper.
  The curse owns its own state and persists after either TX owner's defeat.

Reversal, Reclamation and Calamity bypass initial defensive prevention, including
Protect, Substitute, Soundproof, Contrary, stat-loss protection and reflection.
The summoned profile retains Restoration and excludes selectable Reduction.
The arrival Endurance, phase Reduction and ordinary phase Wish are separate from
the five selectable outcomes of each profile. Reciprocity resolves the current
owner by side/party identity rather than assuming opponent-right.

## Battle-only health layer and execution order

`src/battle_tx_health.c` owns summoned TX's original/applied maxHP, deterioration
percentage, last observed battle HP and a Q24 HP fraction. Original maxHP is
captured once after creation's `switchindataupdate`; solo TX never gets this
overlay. Non-HP Wishful Thinking growth remains in the existing party state.

On an original break, the shared resolver decrements the bar and clears pending
state before dispatching the configured `onBreakScript`. The 60/30 wrapper sets
actual maxHP at zero current HP, synchronizes the controller, waits, then jumps
to `BattleScript_BreakBarRefill`. That shared script heals the new layer before
the unchanged after-break Wish or Reduction/Doom Desire runs. The callback
accepts only 100-to-60 and 60-to-30 transitions. Saved layers use the shared
restore path and bypass both deterioration and original after-break actions.

Clean stat copies reapply Wishful Thinking growth, then the TX health view,
then Reduction where applicable. Controller writes normalize in reverse order:
Reduction, then TX. HP writes retain the fraction against original maxHP;
maxHP writes retain the original canonical maximum, including masked/full
controller records. HUD/healthbar previews use effective battle HP/maxHP.
Unchanged reloads preserve sub-HP remainders, including a rounded one-HP view;
rescaling alone cannot heal. Battle cleanup restores the canonical living-owner
view and clears TX state. Genuine faint leaves canonical zero HP and the
original party maximum, clears local Wish/Reciprocity state, and leaves Reduction
and queued delayed attacks alone.

The central engine-hook inventory and maintenance risks are in
[the upstream modification ledger](upstream_modifications.md#koanko-stage04-tx-profiles-health-and-reduction-2026-11-10).

## Genuine defeat and testing

On its final red-bar faint, TX Rollback becomes permanently defeated. Its position
cannot refill and its party entry is excluded from both Revival Blessing variants
and revival-item execution. Other party members retain normal revival behavior.
The other combatant continues independently; victory requires all legitimate
enemy survivors to be defeated. Shared faint handling resolves simultaneous KOs.

Castoria's virtual Arcanum candidate comparison is shared by Singles and doubles
pickers. It remains outside ordinary four-slot caches, preserves ordinary moves
winning ties, and commits Wish outcomes only for the selected action.

`test/battle/arcanum_more_wishes.c` exercises real Singles conversion. Its
`SUMMONED_OPPONENT()` declaration is placed after the creation TURN in an AI
Singles test; it provides subsequent enemy-right expectations without adding a
starting battler or filling the initially empty sixth party entry. Its synthetic
move template never enters the real starting party. New scripts can supply test
payloads through the same `TxRollbackProfile` interface.

### Pre-migration regression repair (2026-11-10 through 2026-11-11)

Current status: the user reports the full suite green except the deferred text
tests after the separate-stack correction (2026-11-11). Zero unexpected battle
failures remain by user report. Exact final total/pass/known-failing/TODO counts
were not supplied, and no final full log exists at the suggested path. Do not
substitute earlier diagnostic counts for that final result.
The original supplied summary contained
20 unexpected failures: 17 battle cases plus three explicitly deferred text
cases. Migration and Stage 05 remain out of scope. The user owns builds/tests.

Implemented repairs and evidence:

- Three Group A move-variety failures: `DeclareSummonedOpponent` in
  `test/test_runner_battle.c` declared the obsolete Psychic/Swift/Protect/Celebrate
  template. Resolving `EXPECT_MOVE(..., MOVE_WIDE_GUARD)` exhausted those four
  slots before battle execution. Its template now matches production's Follow
  Me, Wide Guard, Life Dew and Power Gem. Move limits/assertions are unchanged.
- Simultaneous-KO fixture: its injected AI selected Wide Guard, blocking Misty
  Explosion against both foes. Articuno still fainted, requiring an unrecorded
  replacement instead of ending battle. This fixture now explicitly selects
  legal Follow Me; victory, both enemy faint counts, player faint count and
  defeated-stage assertions remain. Other fixtures still select Wide Guard.
- Gulp Missile's genuine script stall: the diagnostic cursor was inside the
  Gorging primary paralysis command, with no pending controller request
  (`exec=0`). Misty Terrain rejected paralysis, but the nonvolatile branch of
  `src/battle_script_commands.c:SetMoveEffect` left the cursor unchanged.
  The rejected-effect branch now advances to its supplied continuation.
  Successful effects, immunities, controllers and timeouts are unchanged.
  A new fixture explicitly creates Misty Terrain, then checks blocked paralysis,
  genuine fainting and the expected replacement. The user-run file log reports
  11 PASS/11 TOTAL, including this case.
- Shared full-suite contamination: the original other 13 cases passed a 12-file
  isolation loop (saved logs: 137 PASS, 10 TODO, 147 TOTAL), but returned together
  in the full suite. Snapshots showed the same worker entering with weather 6
  (horizontal fog), field 128 (Misty Terrain), zero starting trainer status and
  an infinite terrain timer. Fog was being converted to Misty Terrain normally;
  the tests had not requested it. This blocked actual status/confusion effects,
  skipped G-Max Snooze's eligibility-gated random call, and made Steel Roller
  legitimately usable. Animation, randomness and ordinary AI assertions were
  not relaxed to accommodate contaminated input.
- The next diagnostic run identified invocation-stack overwrites in Break Bar
  dispatch, red-base redraw and dynamic-fill fixtures, and weather changes while
  healthbox helpers ran inside test bodies. Corrupted parameter/trial counters
  corroborate memory overwrite, not ordinary weather persistence. Source and
  existing ELF inspection identify the call path:
  `ActivateBreakBar`/`ReshowBreakBars`/healthbox rebuild ->
  `UpdateHealthboxAttribute` -> `LoadBattleBarGfx` -> Smol decompression.
  `DecodeSymDeltatANS` copies an ARM decoder onto its local stack. In the existing
  ELF, `DecodeSymDeltatANSLoop` is 1708 bytes; `FUNC_BUFFER_SIZE` allocates 1740
  bytes before counting caller frames. The runner supplied only 1024 bytes.
  Its stack grows downward from `sBackupMapData`; nearby `gWeather` was among
  the overwritten globals. Changing diagnostic layout/scheduling changed which
  workers/fixtures were affected, explaining the larger second failure list.

Git provenance: the 1024-byte private stack dates to `f1b9872bf0` (2022-12-24).
Smol support and battle-bar asset conversion date to `6e64f6f5ef` /
`0917af214c` (May 2025). `dd509a377d` (2026-11-03) first adds the demonstrated
direct test-body Break Bar activation/redraw path; `68c13b3582` (2026-11-07)
adds the direct No Order Change healthbox reconstruction path. The staged
dynamic-fill fixture adds another route, but did not introduce the first one.

The first capacity repair embedded 4096 bytes in `BattleTestData`; the user's
compiler rejected it because `BattleTestRunnerState` exceeded the fixed 20 KiB
map buffer. That repair is superseded. `test/test_runner_battle.c` now owns a
separate, 8-byte-aligned `EWRAM_DATA` `sBattleTestStack`: 4096 bytes of usable
stack plus two lower-bound guard words (4104 bytes before linker padding).
`include/test/battle.h:BattleTestData` no longer embeds the invocation stack.
The state-size assertion and parameter-result bounds check remain unchanged;
removing the old embedded stack restores space for state/results in the map
buffer instead of enlarging or weakening its bounds.

The final existing test ELF confirms `sBattleTestStack` at `0x0203AA08`, size
4104, and EWRAM ending at `0x0203C400`: exactly 15360 bytes (15 KiB) remain below
`0x02040000`. This was read from the user's linked ELF without executing it.
The buffer adds no IWRAM allocation, does not shrink `gHeap`, and exists only
in the test executable. It is an actual additional EWRAM allocation, unlike
the rejected embedded-buffer proposal.

`InvokeTestFunction` seeds/checks both guard words on every invocation, including
estimation and assertions. An overwritten guard reports an infrastructure ERROR;
it is not memory protection against every possible overwrite. The separate
buffer is cleared at the three existing state/data reset points (cost estimate,
setup and run), preserving the old reset semantics. It is retained across scene,
assertion and cleanup invocations so captured addresses of test-local variables
remain valid. Static storage needs no heap allocation/free or cleanup callback.
Temporary weather/state tracing and its ELF-inspection helper are removed.
No weather reset, production decompressor change or gameplay hook is added.

Validation record:

- More Wishes and TX Wishes focused runs: user-reported green, counts not supplied.
- Twelve-file isolation loop: 137 PASS, 10 TODO, 147 TOTAL; no unexpected failures.
- First diagnostic full suite: 4082 PASS, 18 KNOWN_FAILING, 779 TODO, 16 FAILED,
  4895 TOTAL; all four Group A failures absent, 13 other battle failures present.
- Gulp Missile repair: saved focused log reports 11 PASS/11 TOTAL. Reduction /
  Polymorph stack probe: 1 PASS/1 TOTAL; not the corrupting fixture.
- Second diagnostic full suite: 3936 PASS, 18 KNOWN_FAILING, 779 TODO, 163 FAILED,
  4896 TOTAL. Stack guard failures and weather mutations locate corruption;
  this is diagnostic evidence, not an accepted baseline.
- Embedded 4096-byte stack proposal: user-run compilation failed the existing
  `sBackupMapDataSpace` assertion; no tests executed from that source snapshot.
- Separate EWRAM stack correction: user reports the full stack green except
  text tests. Existing ELF inspection confirms the separate stack linked and
  leaves 15 KiB EWRAM free. Final detailed counts are unavailable. Codex ran
  no further builds/tests after the user's restriction.

Reproduction commands in native WSL for the offending paths (no further run is
requested after the user's full-suite confirmation):

```bash
make check -j4 TESTS='test/battle/break_bar.c' > TEMPSCRIPTS/pre-migration-stack-break-bars.log 2>&1
make check -j4 TESTS='Arcanum No Order Change indicator reloads' > TEMPSCRIPTS/pre-migration-stack-healthbox.log 2>&1
```

The full validation sequence also includes `make -j4`, the handoff's focused
Arcanum, TX, AI, Break Bar, Break Charm, Reduction, Polymorph and Koanko checks,
then the full suite. To retain the complete log on a future reproduction:

```bash
make check -j4 > TEMPSCRIPTS/pre-migration-full-fixed.log 2>&1
```

The deferred text cases (Fireball description, Roar of Eternity name, battle
message) remain untouched. User-reported full-suite acceptance covers all 17
original battle cases: three summon-template failures, the simultaneous-KO
fixture, Gulp Missile, and the 12 other terrain-contaminated cases (Leaf Guard,
Symbiosis, G-Max Snooze, both cure-status cases, Flatter, Refresh/Sleep Talk,
Rest, Steel Roller, Toxic, Axe Kick and Relic Song). Temporary tracing is gone;
the stack guards remain. No commit, push, migration or Stage 05 work was done.
Merge impact is the narrow upstream `SetMoveEffect` continuation repair
plus test-runner capacity/guard and fixture corrections, documented in the
[upstream ledger](upstream_modifications.md).

Files modified by this repair: `include/test/battle.h`,
`test/test_runner_battle.c`, `src/battle_script_commands.c`,
`test/battle/ability/gulp_missile.c`, `test/battle/arcanum_more_wishes.c`,
`test/battle/arcanum_reduction.c` (stack-size comment only),
`docs/mechanics/tx_rollback.md` and `docs/mechanics/upstream_modifications.md`.

Stage 04 adds `test/battle/tx_profiles.c` unit/mechanism fixtures and updates
the existing TX Wishes, More Wishes and Koanko AI fixtures. Injected Wish
scorers validate mechanisms, not production tactical selection. Stage 05 must
add actual trainer-policy selection coverage.

No Stage 04 build, compiler or battle tests have been run by Codex. Source review
and scoped whitespace checks are separate from runtime verification. Native WSL
commands, the stable validation snapshot and the Stage 05 gate are recorded in
[the Stage 04 implementation/codeflow handoff](../extra%20docs/KoankoDev/STAGE04_IMPLEMENTATION_AND_CODEFLOW.md).
