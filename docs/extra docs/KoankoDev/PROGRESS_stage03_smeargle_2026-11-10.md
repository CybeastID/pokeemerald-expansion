# Stage 03 - Smeargle Any Move integration (2026-11-10)

Status: **ranking implemented; normal ROM build and focused autonomous checks pass**. Authoritative checkout:
`mypatch`, based on `50ebce4990`. No integration commit or push. The supplied patch
passed `git apply --check` and applied without text conflicts. Existing uncommitted
Polymorph work and unrelated files were retained. Committed Koanko, TX Wishes and
Stage 03.2 Loaded Dice remain intact.

## Behavior and state

### Koanko Special AI ranking (2026-11-10)

`src/battle_koanko_ai.c:AI_KoankoTrainer` is Koanko's trainer callback, following
the Alice/Razeluxe `AI_FLAG_DYNAMIC_FUNC` route. The debug encounter installs it
with `setdynamicaifunc`; the ordinary encounter installs it at the conjuring
transition and adds the flag to the opponent's AI. Other regular-move owners
return their incoming scores unchanged in this Stage03 milestone. Stage11 later adds
the narrow Dialga Time Stop adapter and approved ordinary trainer flags; see the
[Stage11 results](planning/11_koanko_ai_validation.md#9-implementation-record-2026-11-10).
Unimplemented Arcanum policies return 0,
so registering the callback does not implicitly enable extra-slot moves. Future
policies for Koanko's other party members belong in this callback.

Only `KoankoSmeargleAnyMoveActive()` in singles receives the new ranking. No
ranking was added to the sampler itself: its unchanged loop compares the final
AI score, now including Koanko's Special AI pass. Slot-0 limitations and ordinary
nonpositive scores remain vetoes; positive candidates still count toward the same
60-positive stopping rule. The current user-supplied 409-move eligibility pool,
100-BP damaging-move filter, Foul Play exception, 339 offenses, PP/Encore behavior,
forced continuation guards and tagged reservoir tie algorithm are preserved.

**Why the old behavior failed:** ordinary singles scores are small tactical
increments, not a cross-move damage utility. Each isolated powered candidate can
receive the same +1 best-damaging-move bonus because the other three slots are
empty. High damage below a KO threshold adds little to the ordinary score; status
effects can earn larger bonuses. The supplied evidence therefore requires more
than an extra tie-break. This update changes the score in the trainer callback
before the ordinary comparison pass, which still contributes its usual +1.

**Ranking formula:** damage starts at 20 plus accuracy-weighted median damage as
a percentage of current target HP, capped at 100%. Add a capped +/-10 contribution
from the ordinary AI score relative to 100. Estimate KO chance by interpolating
the cached min/max range, multiply by cached accuracy, and award a separate bonus
of 70..90 for reliability >=80%; weaker KO chances receive half their percentage.
Do not award the immediate-KO bonus for charging, delayed or conditional attacks.
Sash/Sturdy/Disguise checks use the existing survival helper. Damaging ranks clamp
to 1..250. Stage11 subsequently fixes the picker's signed comparison, so this
existing range remains a scoring policy rather than an overflow workaround.
No overkill bonus is added after damage reaches current target HP; Stage11 also
removes the victory bonus when a real Break Bar intercepts the attack.

Costs: charge/conditional attacks halve their rank; recharge costs 20; recoil costs
up to 50 based on user max HP, with another 60 for self-lethal recoil. Self-KO costs
110, self-stat-drop effects cost 10 each unless Contrary reverses them, and rampage
lock-in costs 15. Magic Guard/Rock Head are respected for applicable recoil.

Status utility starts at 30 plus twice the ordinary score adjustment (capped +/-10).
Ordinary penalties prevent special boosts. Useful recovery gets 145 at <=35% HP,
95 below 65%, and 30 otherwise (Rest costs another 10); full-HP recovery scores 1.
Eligible sleep gets 120, other status +40, fresh screens +30, safe setup +40,
and hazards +25 only with opposing reserves. Existing screens score 1. Utility
ranks clamp to 1..155. Consequently useful healing/sleep can beat poor damage,
while low-impact utility cannot trump an accurate immediate KO or ~80% damage.

Incoming threats use existing target-to-user damage caches and `CanEndureHit`.
The existing `AI_IsSlower` helper handles priority, speed, Trick Room, Stall and
Lagging Tail. A lethal response that can precede the candidate divides its rank
by three. This is a risk heuristic, not a prediction that the player must choose
that attack. No additional full damage simulation or RNG call is introduced.

**Lifecycle/maintenance:** `SetDynamicAiFunc` is a small programmatic setter for
the existing callback; `ScriptSetDynamicAiFunc` delegates to it. Existing normal
and recorded battle cleanup reset the callback. Ranking uses local scalars only,
with no new battle-state fields, per-pool score arrays or upstream AI algorithm
replacement. Changes to damage caches, scoring-pass order or the callback interface
need semantic review on upstream updates. Detailed instrumentation remains behind
`TESTING || DEBUG_KOANKO_SMEARGLE_DIAGNOSTICS`, with all buffers in EWRAM and zero
static initialization. The user's enabled diagnostic switch is retained.

**Reproducible measured examples:** `test/battle/koanko_smeargle_ranking.c` compares
the real scorer with and without the Special AI flag on explicitly evaluated
alternatives; it does not pretend those alternatives were all drawn by a live seed.

- Weavile, 281 HP: Light of Ruin ordinary 105 -> rank 145 (362 median, 90% accuracy);
  Hyper Beam 105 -> 125 (291 median, 90%); Close Combat 105 -> 205 (768 median, 100%).
  Boomburst is 101 -> 130 (271 median): its cached minimum is below target HP, so it
  is correctly not classified as a guaranteed KO despite its perfect accuracy.
- Serperior, 224 current HP: Flatter 102 -> 34; Eruption 105 -> 215 (354 median,
  100% accuracy). On Dragapult, lowering Smeargle HP drops Eruption's rank 54 -> 22;
  Foul Play stays rank 106 / 272 median when Smeargle's own Attack changes.
- Alomomola, 473 HP: Lucky Chant 100 -> 30; Electro Drift 101 -> 104 (395 median,
  363..427 range, 100% accuracy). This is substantial non-KO damage, not a KO-tier win.
- Low-HP KO fixture: Fusion Bolt rank 217 versus Zap Cannon 102, Meteor Assault 197
  and Steel Beam 158. Adding a full-HP target Sash drops Fusion Bolt to 121; a
  charge-turn Solar Beam scores 64. Critical recovery scores 145 versus weak damage
  21; Spore scores 120. Redundant screens and full-HP recovery score 1.

**Validation:** the updated-filter baseline had five pre-existing fixture failures:
Tackle and the old low-ID/tie candidates were no longer eligible. All 17 original
cases remain; injected execution fixtures now use eligible Boomburst, and small
sample/tie fixtures use eligible high-power moves. Ten new ranking cases run the
actual AI pipeline and verify damage, accuracy, costs, status value, Foul Play,
variable power, ownership, Sash/charge ordering, Trick Room and fixed-tag replay.
Koanko: 44/44 pass, including all original Smeargle cases, four diagnostics cases
and Loaded Dice. TX Wishes: 22/22 pass. Arcanum: 150/150 pass, including the existing
Alice/Razeluxe callback routing. Ordinary AI: 310 pass, four explicitly
KNOWN_FAILING and four TO_DO, no unexpected failures. Normal `make -j4` succeeds.
The first directory-shaped AI filter selected no tests; it was replaced by `TESTS=AI`.

Same-fixture headless mGBA timing: 60 evaluations without ranking = 19 VBlank frames;
60 with ranking = 21. Diagnostic printing occurs after the measurement. This is
one controlled measurement with frame-boundary granularity, not a hardware benchmark
or an exact replay of the user's earlier 14..15-frame encounters. No extra full-pool
pass or arbitrary candidate cap was introduced.

Memory: diagnostic object has 3280 bytes of .sbss and zero .bss. Test ELF EWRAM uses
242668 bytes (19476 free); IWRAM reaches offset 29292, leaving 3220 bytes before the
0x7F00 persistent region. Normal ROM EWRAM uses 239756 bytes and IWRAM 28796 bytes.
No nonzero-initialized EWRAM variable was added. Existing unrelated unused-variable
and map-file missing-newline build warnings remain.

Commands/logs (repository root, Ubuntu WSL Bash):

```sh
make check -j4 TESTS=Koanko
make check -j4 TESTS='TX Wishes'
make check -j4 TESTS=AI
make check -j4 TESTS=Arcanum
make -j4
```

Results are in `TEMPSCRIPTS/koanko-smeargle-ranking-regression.log`,
`koanko-smeargle-ranking-tx.log`, `koanko-smeargle-ranking-general-ai.log`,
`koanko-smeargle-ranking-arcanum.log` and `koanko-smeargle-ranking-rom.log` in the same
directory. To inspect actual gameplay,
open the rebuilt `pokeemerald.gba` in mGBA, choose Debug Menu -> Scripts -> Koanko
Smeargle, and inspect `SMEAR` logging. Pass 60 is Koanko's ordinary-score-to-rank
conversion; pass 64 is the separate damage-comparison bonus. The diagnostics keep
the top 12 and eight watched moves, plus evaluated membership bits. NOT_EVALUATED
means no score was observed, not rejection. Set the diagnostic switch FALSE and
rebuild for quiet gameplay; it was not silently disabled here.

Limits: KO probability is a cached-range approximation, not exact roll enumeration.
Future Protect/switch choices, secondary effects, speed ties and some conditional
mechanics remain heuristic. Utility weights are hand-tuned and not a multi-turn
search. Foe-move caches reflect the existing engine's knowledge, so tests initialize
the real Koanko flags before turn caches are built. Only sampled candidates compete.
Future work should first review live ranked diagnostics, then refine demonstrated
effect-specific mistakes; expanding simulation/search has a much higher performance
and maintenance cost. Manual post-change gameplay remains unverified. No commit/push.

### Direct Debug Menu encounter (2026-11-10; builds, gameplay not manually reverified)

Choose **Debug Menu -> Scripts -> Koanko Smeargle** (formerly the unused Script 3).
Bring a usable player party. The entry leaves the player's roster intact and starts
a singles trainer battle directly against the level-100 conjured Smeargle, with
its actual EVs/IVs, Own Tempo, Focus Sash and provisional slot-0 move from
`src/data/koanko_party.h`. Other conjured opponents are omitted.

`data/scripts/debug.inc` calls `Script_StartKoankoSmeargleDebugBattle` in
`src/battle_koanko.c`, then resumes/releases the script after battle. The native
resets trainer parameters, sets `TRAINER_KOANKO` and `BATTLE_TYPE_TRAINER`, and uses
the existing debug transition launcher to keep singles even with a follower.
`gIsDebugBattle` stays FALSE deliberately: TRUE would replace the trainer/AI with
generic debug data and skip normal party initialization.

The one-shot pending flag is consumed by `KoankoTryCreateDebugSmeargleParty`, called
from `src/battle_main.c:CreateNPCTrainerParty` after resources and ordinary trainer
AI flags exist but before opening controller requests. It reuses the normal party
constructor with the real conjured party's first entry only, sets CONJURED_TEAM,
adds SEQUENCE_SWITCHING and sets the opponent's No Order Change state. Initial
battler loading therefore puts Smeargle in actual opponent party slot 0; all Any
Move ownership conditions hold on its first fresh decision. The ordinary chooser
temporarily enables CHECK_VIABILITY and uses the production scorer/RNG, without
test injection. The phase prevents a second team from being conjured on defeat.

Pending state is module-owned EWRAM, consumed before party creation, and cleared
again by the dedicated debug return callback. The normal battle allocation/free
owns encounter phase and battler state. The return callback resumes the field
script without marking Koanko defeated or attempting a follower-party restore
for this singles battle. Player battle consequences such as HP/PP and EXP are
ordinary; the entry does not heal or replace the player's team. Recorded replay
of this debug-only setup is not implemented.

Only one narrow upstream party-factory hook is added. Encounter data, initialization
and return handling stay in the custom Koanko module; the menu label and script slot
are the other upstream-facing edits. Review factory ordering if upstream moves AI
setup or opening party reads. No normal encounter behavior changes while the pending
flag is false.

No builds/tests were run for this addition. Keep the current checkout stable for:

```sh
make -j4
make check -j4 TESTS=Koanko
```

Manual verification: open the entry, confirm a single Smeargle under Koanko's name,
No Order Change badge and sampled ordinary slot-0 attacks; confirm Pressure does not
spend its PP, recharge turns keep the move, defeating it ends the battle directly,
and a subsequent ordinary Koanko encounter still starts with the original team.
Check return to field and repeat entry, including with a follower. Prior focused
passes recorded below do not validate this new debug launch path.

`src/battle_koanko_smeargle.c` owns selection. Its predicate requires the Koanko trainer
battle, CONJURED_TEAM phase, opponent side, party index 0 and current Smeargle species.
Ordinary Smeargle and another party member copying Smeargle receive no exemptions.
There are no new battle-struct fields or transition/switch-in reset hooks.

On a fresh choice, enumerate ordinary IDs 1..MOVES_COUNT_GEN9-1, excluding Struggle,
placeholders, Z/Max/custom moves, indirect move callers and self move-slot rewrites.
Draw distinct IDs using partial Fisher-Yates; stop at 60 **positive** scores or pool
exhaustion. Highest observed score wins, with reservoir ties; zero positives commits
Struggle. Rebuild the pool in ID order each decision so previous shuffles cannot
affect replay with identical RNG inputs.

A reusable 848-entry u16 EWRAM pool uses 1696 bytes, plus a 2-byte size. Each selection
overwrites this scratch state, so no battle-end cleanup is needed. TESTING-only stats
describe the last selection; injected scorers are test-only and fixtures restore NULL.

## AI and execution order

1. ComputeBattlerDecisions invokes BattleAI_ChooseMoveIndex before controllers copy
   moves. Existing TX slot-policy and Koanko party-order logic are untouched.
2. Set the ordinary attacker gimmick assumption, temporarily add CHECK_VIABILITY,
   then sample only outside multipleTurns, recharge and Time Stop > 1. Forced
   continuations keep their committed move.
3. BattleAI_PrepareSlot0Move writes the candidate and nonzero PP into real slot 0,
   recalculates limitations, clears its damage/effectiveness/accuracy caches and uses
   the existing move-data calculator to refresh them. Slots 1-3 remain empty.
   Unusable candidates cannot inherit previous damage.
4. BattleAI_ScoreSlot0Candidate resets thinking data and calls ScoreMoves_Singles,
   shared with ordinary singles selection. It restores ProtectStruct scratch flags
   per candidate. It does not execute moves, spend PP or rebuild party data.
5. Re-prepare the winner and reset thinking data, then use the ordinary chooser.
   It scores the winner once more, chooses index 0, and retains ordinary targeting,
   gimmick reconsideration and execution. Flags are restored at chooser exit.
   Controller requests copy the updated battle moves/PP; the enemy party's
   provisional move is deliberately not rewritten.

The shared extraction preserves ordinary TX/Arcanum wish reset, extra-slot comparison
and commitment. Candidates do not invoke the final picker or extra-slot chooser.
Current Koanko flags enable neither predicted-switch nor dynamic trainer AI.
A powered, effective lone damaging move can receive the ordinary BEST_DAMAGE_MOVE
bonus; this does not compare damage across sampled candidates.

## Exemptions and interactions

CancelerPPDeduction skips PP spending, including Pressure, for owned Smeargle while
preserving Metronome-item tracking and remaining submove handling. CancelerChoiceLock
suppresses Choice/Gorilla lock acquisition. Cmd_trysetencore rejects Encore acquisition.
Other ordinary restrictions remain, including Taunt, Disable, Torment, Imprison,
Gravity, Heal Block, sleep, paralysis, flinch and species/move requirements.
Existing locks are not cleared retroactively; the conjured lead starts without them.

Loaded Dice's existing Koanko-wide 85..102 damage-roll behavior and AI damage semantics
are inherited through the existing calculator. Its algorithm and tests are unchanged.

## RNG and cost audit

DRAW and TIE tags are appended to RandomTag, preserving existing numeric tag values.
They identify calls to the **shared RNG**, not independent streams. RandomUniform's
multiply-and-shift mapping has small finite-resolution bias for ranges that do not
divide 65536. Reservoir ties use the standard uniform algorithm subject to that engine
mapping. AI scoring can consume ordinary RNG, so order affects subsequent scores/draws.

For N eligible moves: at most N candidate evaluations, one winner cache refresh and
one ordinary winner scoring pass. N <= 847 for the current range. Sixty positives is
not a 60-evaluation cap. Pool construction is O(847), sampling is O(N), each evaluation
uses existing enabled scoring passes/damage calculation, and there is no second full
pool evaluation. Supported singles has one live opposing target. Future prediction
flags could increase per-candidate cost. Latency and EWRAM headroom are unmeasured.

The final ordinary winner pass can consume fresh RNG and report a different score.
The contract selects the best score observed while sampling, without guaranteeing
the same final score. Generic AI SET_SCORE/ADJUST_SCORE test hooks run per candidate;
score-assertion fixtures may require special handling.

## Tests and integration corrections

All **15 supplied cases** are retained (the patch summary incorrectly says 13).
Scorer setup/reset is inside GIVEN: the runner re-enters the test body for THEN,
so top-level resets would erase the recorded call count and flag observations.
The tie test now uses PASSES_RANDOMLY to enumerate its RNG tag: the harness otherwise
always returns the upper bound, so the original 40 calls could not cover both ties.
Claude's follow-up `01436942` was manually merged into the already corrected tie test:
`PASSES_RANDOMLY(50, 100, RNG_KOANKO_SMEARGLE_TIE)` and an assertion against the second
tied move observed in draw order. The equivalent prior 1/2 expectation is replaced;
the integration's pool rebuilding, ownership, cache and fixture corrections remain.
The real-score fixture injects eligible Boomburst only for encounter setup, then directly calls
the real scorer; it also checks Disable/Taunt and all three cleared slot caches.
Two added cases cover shuffle-history independence with fixed tagged RNG and party-slot
ownership. The existing Koanko lead test expects deterministic sampled Boomburst instead
of its obsolete four-move expectation.

User validation reached the test linker but failed before test execution: IWRAM
overflowed by 1856 bytes. The two full-pool observation arrays in
`test/battle/koanko_smeargle.c` used ordinary static .bss, placing 5088 bytes in IWRAM.
Both now use `EWRAM_DATA` (.sbss), which the existing test linker maps to EWRAM.
Assertions and production code are unchanged; there is no linker-script modification.
The user's next run linked successfully after this correction. Exact remaining EWRAM
headroom is unmeasured. No agent build/test was run.

Initial user results: Smeargle 16/17 passed; Polymorph 9/9 passed; combined Koanko 28/30 passed,
including all three Loaded Dice cases. The two distinct remaining failures were fixture
bookkeeping, corrected after reviewing the runner:

- Recharge: CloseTurn queued a default AI-choice expectation for the forced recharge
  turn. No choice callback consumes it until the next fresh turn, causing the reported
  3-vs-4-turn mismatch. The fixture now marks that turn with SKIP_TURN(opponent), while
  retaining the recharge message. A subsequent user run reached THEN but reported
  1670 lifetime evaluations versus the 835-move pool. AI can run for fresh turn 4
  before the playback controller notices the exhausted record. The scorer now records
  evaluations by battle turn and requires zero on turn 1, exactly one pool scan on
  turn 2, and zero on recharge turn 3. This distinguishes forced-turn sampling from
  valid post-recharge sampling during playback shutdown.
- Lead: named EXPECT_MOVE(Tackle) tried to find/add Tackle in the original full four-move
  fixture before runtime replacement. The expectation now names slot 0; the scene and
  final assertions verify Tackle execution, slot 0 commitment and three empty slots.

The user subsequently reported all requested reruns passing, including the corrected
recharge and combined Koanko fixtures. Production code and upstream integration points
are unchanged. No agent test execution. Read-only whitespace checks pass.

The user narrowed the test observation array sSeenScores from s32 to s16. Its current
injected scores range from 0 to 53, so the narrower storage preserves all recorded
values and saves 1696 bytes. Combined observation arrays now occupy 3392 bytes in
EWRAM. The scorer callback remains s32 to match the ordinary AI API.
Validation snapshot: this integrated working tree, including headers, engine changes,
new Smeargle module, both Koanko test files, and pre-existing Polymorph/TX changes.
Keep the entire source checkout stable during validation.

From the repository root in PowerShell:

```powershell
wsl make check -j4 TESTS='Koanko Smeargle'
wsl make check -j4 TESTS=Polymorph
wsl make check -j4 TESTS=Koanko
wsl make check -j4 TESTS='TX Wishes'
wsl make check -j4 TESTS=Arcanum
wsl make check -j4 TESTS=Encore
wsl make check -j4 TESTS='*Choice*'
wsl make check -j4 TESTS='*Gorilla Tactics*'
```

Remaining gaps: Choice/Gorilla immunity after Trick/Skill Swap; charging/rampage and
Time Stop continuation; end-to-end real sampler variety; targeting/Tera; Torment/
Imprison; controller/party PP after Spite and unusual effects; production-seed replay
and distribution; scratch-state effects under future prediction/dynamic AI; exhaustion
latency and EWRAM headroom. Injected scorers do not establish real AI performance.

Upstream integration details: [central ledger](../../mechanics/upstream_modifications.md).
