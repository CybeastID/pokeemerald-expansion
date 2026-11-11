# Stage 04 implementation changes and execution flow

Date: 2026-11-10. Branch: `mypatch`. Base commit: `582c5f5015`.
Status: implementation and source review complete; user-run compilation and
runtime validation pending. Stage 05 has not started: the assignment requires
Stage 04 to pass validation first. No commits, merges or upgrades were performed.

## Changes delivered

- TX combat capabilities now follow explicit side/party ownership and one of
  `NONE`, `CASTORIA_SUMMON`, or `KOANKO_SOLO`. Summon formation remains a separate
  lifecycle. Koanko's registered TX gets Wishes without conversion to doubles,
  a sixth-slot reservation, ambush, configured Break Bars or a summon revival ban.
- Summoned TX gets Follow Me / Wide Guard / Life Dew / Power Gem. Koanko's
  authored Wish / Doom Desire / Protect / empty kit and encounter stats stay as
  authored. Existing all-holder Wishful Thinking dispatch remains authoritative.
- Both profiles share Reversal, Reclamation, Calamity and Reciprocity. Summoned
  TX keeps Restoration; solo TX substitutes Reduction. Reciprocity resolves the
  registered owner rather than assuming the right-hand position.
- The new idempotent Reduction initializer accepts starting loss and side.
  Solo Reduction starts at 1, increases to 2/3/etc., and skips its casting turn.
  Existing Castoria/phase callers retain 10/11/12/etc. Both use the same persistent
  curse, shared turn counter, per-occupant loss and existing canonical stat layer.
- Summoned TX's original bars now have actual maxHP 100% / 60% / 30%. Both reduced
  maxima derive independently from the original maximum, with floor/minimum one.
  The shared refill and saved-layer engine remains responsible for restoring HP.
- A dedicated battle-only health module preserves original party maxHP and HP
  proportions through controller writes, clean stat reloads and battle cleanup.
  Existing non-HP Wishful Thinking growth is reapplied in its existing path.

Meteo, its cooldown, animation and production virtual-action tactics remain
Stage 05 work. No type chart changes were made: native Stellar remains neutral
against player-accessible attack types and the existing boss-only Divine-to-
Stellar 2x interaction is preserved.

## Source map

- `include/battle_tx_rollback.h`: combat profile, owner identity/defeat state,
  explicit Reduction outcome, health state and helper declarations.
- `src/battle_tx_rollback.c`: summon-only formation policy, owner lookup,
  Koanko registration, creation kit, health initialization and genuine-faint
  cleanup. Creation cannot replace an existing combat profile.
- `src/battle_koanko.c:KoankoOnSwitchIn`: invokes registration at the existing
  conjured-entry boundary; retains the existing No Order Change rule.
- `src/battle_tx_wishes.c`: explicit profile lists, eligibility, committed
  recipient resolution, solo Reduction and owner-aware Reciprocity.
- `include/battle_reduction.h`, `src/battle_reduction.c`: affected side and
  `ReductionTryActivate`; compatibility wrapper and persistent engine retained.
- New `src/battle_tx_health.c`: original/applied maxHP, phase transition,
  proportional HP accounting, controller normalization, clean-copy reapplication
  and cleanup. This is a health view, not a second Break Bar resolver.
- `data/battle_scripts_1.s`: pre-refill 60/30 wrappers, Reduction success message
  and existing Wishful Thinking call, plus safe Reciprocity abort-return label.
- `battle_controllers.c`, `battle_interface.c`, `battle_util.c`,
  `battle_script_commands.c`, `battle_main.c`: narrow serialization, HUD, clean-
  copy and cleanup hooks. Exact symbols and maintenance concerns are in the
  [central ledger](../../mechanics/upstream_modifications.md#koanko-stage04-tx-profiles-health-and-reduction-2026-11-10).

## State ownership and lifetime

`BattleStruct.txRollback.combatProfile` plus `ownerSide/partyIndex` owns combat
capabilities. `stage` controls summoned formation only; `profile` remains the
existing ambush-script pointer. `health` is initialized only for summoned TX.
Owner identity survives changes of species/ability. It does not grant ownership
to another creature with the same species or a replacement in the same position.

`BattleStruct.reduction` owns the curse independently of TX. Initial activation
sets the starting counter, affected side and casting-turn grace. Repeated
activation returns false before touching accumulated state. Existing replacement
cleanup clears only that occupant's accumulated loss; global acceleration stays.
Battle cleanup restores Reduction before clearing TX's health/profile state.

On a genuine owner faint, local Reciprocity and pending Wish candidate/outcome/
recipient state clear. Only a summoned owner retires its special slot. Neither
global Reduction nor ordinary queued Doom Desire is cancelled. A pending Break
Bar is not a genuine faint. Solo revival is not prohibited by the summon policy;
a living, legitimately revived solo re-entry re-enables its registered owner with
fresh local effects, while any global Reduction remains active.

## New execution flow

### Koanko solo entry and Reduction

1. The existing switch-in path reaches `KoankoOnSwitchIn` after switch-in cleanup.
   The conjured phase retains No Order Change and calls `TxRollbackRegisterKoanko`.
2. Registration checks Koanko trainer identity, conjured phase, opponent ownership,
   party index 1 and the initial TX species. It creates `KOANKO_SOLO`, leaving
   summon stage `NONE` and all topology/ambush/bar machinery idle. Repeated entry
   does not reset an active owner's effects.
3. Existing virtual-slot access recognizes `TxRollbackHasWishes`. The explicit
   profile list supplies Reduction rather than Restoration. Production Koanko
   virtual scoring still declines these candidates until Stage 05; mechanism
   fixtures deliberately inject a nominated outcome to exercise execution.
4. Normal virtual-action selection commits a Wish outcome/recipient only when
   Wish wins. `BS_TxRollbackTryWish` consumes that commitment before revalidating
   the action, current owner, current profile, pending-break state and effect.
5. Reduction calls `ReductionTryActivate(1, B_SIDE_PLAYER)`. Success enters the
   TX Reduction success script and the existing Wishful Thinking dispatcher once.
   An already active/stale request fails and grants no success reward.
6. Existing end-turn Reduction processing consumes casting-turn grace, then
   applies one shared next-loss value to eligible living affected-side occupants.
   The counter advances once per completed turn, independently of TX's survival.

### Summoned TX original bar transition

1. Existing More Wishes creates the real party entry, expands formation and loads
   the controller. `BS_TxRollbackInitialize` captures original maxHP after
   `switchindataupdate`, then installs the existing two-extra-bar configuration.
2. Ordinary damage empties a layer. The existing resolver clears pending state,
   decrements the bar and dispatches its configured original `onBreakScript`.
3. `BattleScript_TxRollbackRefill60/30` calls `BS_TxRollbackDeteriorate`.
   Only the expected 100-to-60 / 60-to-30 zero-HP transitions change phase.
   Actual maxHP becomes `max(1, originalMaxHp * percent / 100)`.
4. A normal controller MAX_HP write is emitted; TX normalization keeps the party
   maximum original. After `waitstate`, execution jumps to the shared refill
   script, which heals the full new effective layer and updates its healthbar.
5. Existing after-break scripts run: ordinary Wish after the first original
   break, then idempotent 10+ Reduction and Doom Desire after the second. Pending
   Wish/delayed-slot restrictions and existing Wishful Thinking behavior remain.
6. Saved layers use the existing restore script, bypassing original deterioration
   and after-break scripts. Revealing a saved layer restores its covered HP/maxHP
   within the same deterioration phase.

### Clean copies, controller writes and recovery

- Clean copy: canonical party/controller stats -> Wishful Thinking growth -> TX
  maxHP/HP view -> Reduction where applicable. Switch-in and ordinary stat-copy
  integration use their existing cleanup/mutation boundaries.
- Controller write: copy emission buffer -> Reduction inverse normalization ->
  TX inverse normalization -> ordinary controller. Battle values stay effective;
  canonical party maxHP stays original. Masked/full records preserve other fields.
- HP accounting tracks a Q24 fraction and genuine HP changes. An unchanged reload
  or emission preserves the remainder even if a one-HP view rounds up to full.
  Changing only the displayed maximum cannot repeatedly heal the canonical mon.
- Reciprocity: actual player healing -> calculate actual restored fraction ->
  find current registered TX -> reject dead/pending/full owner -> save execution
  context -> existing heal script -> restore context. It retains existing
  exclusions for redistribution, reserve healing, revival and rescaling.
- Battle exit: existing Reduction restoration -> restore living summoned TX's
  original-maxHP view/proportional HP -> clear TX state. Fainted party HP is zero;
  normal controller normalization has preserved the original maximum already.

## Fixtures and validation handoff

New `test/battle/tx_profiles.c` contains eight unit fixtures and five battle
mechanism fixtures. They cover profile/formation isolation, Wish lists, idempotent
1+/10+ initialization, repeated clean reloads with growth, 60/30 original-relative
rounding, one-HP minimum, masked/full controller records, saved-layer coverage,
pending versus genuine faint, real Koanko entry routing, 1/2/3 progression,
persistence for four further turns after TX faints and the sole AI-selected
reserve enters, controlled `SINGLE_BATTLE_TEST` replacements through four later
party members, and left-owner Reciprocity. The controlled fixture seeds solo
owner identity and the same 1+ curse directly after TX enters; the AI fixtures
cover actual registration and Wish casting. Existing `Arcanum Wish Reduction`
switching fixtures also
cover per-occupant loss reset while the side counter accelerates.
Injected scores are explicitly labeled mechanism coverage, not production tactics.

`tx_wishes.c` now asserts 240/120 maxima from a 400 baseline at the real original
breaks, canonical maxHP retention and no deterioration on a saved layer. Its
synthetic HP seed also initializes the matching canonical health baseline.
`arcanum_more_wishes.c` checks the support kit and PP behavior. `koanko_ai.c`
expects the newly registered solo capability while retaining ordinary-kit tests.

Run these commands in native WSL from this repository, in order; a filter must
select a nonzero number of tests. Stop and report any failure with its log.

```sh
make check -j8 TESTS='test/battle/tx_profiles.c'
make check -j8 TESTS='TX Wishes'
make check -j8 TESTS='Arcanum Wish More Wishes'
make check -j8 TESTS='Arcanum Wish Reduction'
make check -j8 TESTS='Wishful Thinking'
make check -j8 TESTS='Break Bar'
make check -j8 TESTS='No Order Change'
make check -j8 TESTS='Koanko'
make check -j8 TESTS='Arcanum'
make -j8
```

Validation snapshot: this checkout based on `582c5f5015`, including the new
untracked health module/test file. The milestone file hashes are recorded in
`TEMPSCRIPTS/koanko-stage04-snapshot.json`. Preserve the entire working tree's
source, headers, data/scripts, tests and build configuration during validation;
the manifest fingerprints milestone files rather than every build input.
Do not apply the proposal patch while validating this snapshot.

Actual status: user-run compilation failed in `tx_profiles.c` because its new
controller-normalization fixtures omitted `battle_controllers.h`, which declares
`REQUEST_HP_BATTLE`, `REQUEST_MAX_HP_BATTLE` and `REQUEST_ALL_BATTLE`. The missing
include was added on 2026-11-10; compilation and runtime verification must be
repeated against the refreshed snapshot. The subsequent user run reached the
battle fixtures and reported `Speed required` in all four: the player declared
an explicit Speed, but opponents did not. Each opponent, including reserves,
now declares `Speed(1)` against the player's `Speed(1000)` to satisfy the harness
and retain the intended player-first ordering for ordinary-priority actions.
The user then corrected all four AI fixtures to use `EXPECT_SEND_OUT`. The next
run reported one failure in the persistence fixture: its requested second reserve
was party index 2, but the AI selected the healthier index 5. That fixture now
has only one legal reserve and checks four further turns of Reduction after TX's
defeat. The updated fixture awaits another user run. No production code or
execution flow changed in these corrections.

A separate `SINGLE_BATTLE_TEST` now controls all four later opponent party
replacements with `SEND_OUT`, checks the registered solo owner's genuine defeat,
and checks that Reduction reaches the fifth loss and leaves the player's party
level canonical. Its first user run rejected the TX faint turn because the
non-AI harness defaulted to Celebrate, which is absent from the explicit TX
kit. That turn now assigns Doom Desire; the faster player knocks TX out before
the assigned action executes. The next user run completed the controlled
replacements but found `TX_PROFILE_NONE` at its final assertion: the non-AI
fixture did not register a combat owner. This fixture now seeds the explicit
solo owner identity before the curse, so it tests owner defeat and curse lifetime
without depending on that fixture's encounter-registration path. The AI entry
fixture remains responsible for real registration. This correction awaits a
fresh run.

Codex performed source/diff review and scoped whitespace checks. No compiler,
ROM build, test executable, test suite or emulator was run by Codex for this
milestone. Prior green logs are historical and do not validate these changes.
HUD/controller timing, full script integration and canonical cleanup therefore
remain unverified until these runs. Stage 05 stays gated on passing results.

## Alice/Razeluxe Proposal Review

Reference: the corrected `ai_tactics_proposal_plus_fixtures.patch` and accompanying
notes dated 2026-11-10. The patch is reviewed material, not applied source. The
fixtures-only patch is superseded. These are preliminary Stage 05 constraints;
none is claimed to be verified by the new Stage 04 mechanism fixtures.

1. **Trainer ownership/coverage — partially addressed in the supplied patch;
   production registration coverage still missing.** Added fixtures set actual
   `TRAINER_ALICE2`/`TRAINER_RAZELUXE` identity, so they can enter `AR_OwnsEncounter`.
   They still install `sDynamicAiFunc` and explicit flags themselves. Older
   injected Arcanum fixtures and the wrong-trainer direct call do not validate
   the gate. Stage 05 needs correctly owned TX/Castoria production-policy cases
   and negative ownership/registration controls.
2. **Score scale — unresolved, requires scenario calibration.** Ordinary policy
   adds deltas to the caller's accumulated score, but several virtual outcomes
   return fixed scores near 100–120; Fireball returns caller score +8 only after
   an expected-damage improvement threshold. These are not measured equivalent
   utilities. Compare real ordinary versus virtual selection, exact ties,
   immediate KO/layer progress and survival before adopting the arithmetic.
3. **Alice/Kazuradrop replacement — unresolved and outside this implementation.**
   The new Alice early return replaces legacy handling wholesale. The supplied
   tests omit Kazuradrop opener/emergency actor-bit/form routing. Preserve and
   explicitly regress those contracts before a separate bonus-boss integration;
   Stages 04/05 do not authorize wholesale installation of this policy.
4. **Two-maximum-hit setup budget — unresolved conservative heuristic.**
   Rejecting setup when `incoming * 2 + margin >= hp`, using maximum incoming
   damage, can reject viable faster, inaccurate-attack or defensive-payoff lines.
   Reuse current observable damage/speed/layer helpers and compare representative
   scenarios rather than importing that rule as a universal setup gate.
5. **Broader Polymorph behavior — corrected by the replacement patch, unverified.**
   The patch returns the caller's score outside anti-Perish behavior instead of
   zero, preserving the broader existing selection opportunity. Anti-Perish
   exceptions remain. Neither new proposal fixtures nor Stage 04 tests verify
   the complete Polymorph policy; retain dedicated regression coverage.
6. **Placement/private accuracy helper — unresolved architectural follow-up.**
   The patch adds roughly 650 policy lines inside upstream `battle_ai_main.c`,
   where it can call static `Ai_SetMoveAccuracy`. Moving it into a custom module
   requires a deliberate shared candidate/accuracy interface. Stage 05 should
   reuse the recent signed selection, layer-aware outcomes, safe damage simulation
   and `BattleAI_ScoreSlot0Candidate` infrastructure, exposing only the bounded
   functionality it needs. This avoids duplicating ordinary AI/cache algorithms
   or indexing four-slot caches with virtual positions. Any new engine interface
   must be recorded in the ledger with semantic and merge-maintenance costs.

The supplied notes report host syntax checking on a different branch only, with
ARM compilation and runtime tests not run. That is not validation of this
working tree, and its acknowledged missing tactical cases remain open.
