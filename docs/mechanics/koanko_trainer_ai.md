# Koanko's trainer AI: strategies and conditional blocks

Implemented in `src/battle_koanko_ai.c:AI_KoankoTrainer`. This is the boss's
encounter-specific tactical layer, including an individual block for each of her
twelve roster members. The previously approved shared AI/Break Bar fixes remain.
This document supersedes the earlier Stage11 milestone's statement that most
members merely return their ordinary scores unchanged.

## Dispatch and scoring contract

`BattleAI_SetupFlags` calls `KoankoSetupTrainerAi` when no explicit dynamic
callback is installed. This registers Koanko's callback before the original
team's flags are calculated. An explicitly registered script/test callback keeps
precedence. Conjuring and the Smeargle debug encounter retain their existing
registration. Normal battle cleanup resets the callback.

The engine first performs ordinary bad-move, viability and other enabled passes;
the dynamic pass adds the trainer's conditional adjustments. The ordinary
damaging-move comparison then finishes singles scoring. The normal controllers
commit the result and execute it through the standard battle engine. The
callback never switches, uses inventory, consumes a gimmick, advances a phase or
replaces a committed action.

Ownership is explicit: the battle must identify Koanko as its trainer, the actor
must be on the opponent side, and its phase/roster role must match. Original-team
blocks recognize their roster species, including Alcremie's actual G-Max form.
Conjured blocks additionally require the appropriate party position. Calyrex and
Eternatus recognize both existing form species; Arceus recognizes its form family.
Other trainers, the player, wrong-phase/slot actors and absent/allied targets do
not receive these adjustments. Unimplemented virtual actions still score zero.

Small shared helpers keep the per-mon blocks readable:

- Best attack merit is +2 for highest accuracy-weighted, current-HP-clipped cached
  damage. This rewards actual coverage and reliability rather than move names.
- A reliable immediate KO line receives +6; status moves lose 15 when such a line
  exists. "Reliable" here means minimum cached damage genuinely KOs and cached
  accuracy is at least 90%, not a promise that a 90%-accurate move always hits.
  Charge, delayed and reactive attacks are excluded. A Break Bar is not a KO.
- Utility bonuses generally require ordinary score >=100. A positive but penalized
  status move is not automatically treated as useful. Existing limitations and
  nonpositive scores are respected.
- Incoming pressure sums the strongest known usable cached attack from each
  living enemy. Recovery separately checks attacks that can act before it, using
  ordinary priority/speed/Trick Room handling. These are conservative estimates,
  not knowledge of the player's selected command.

No extra damage simulations, RNG draws, persistent strategy counters, mandatory
openers or fixed rotations are introduced. Smeargle bypasses these new common
bonuses and retains its established rank policy.

## 1. Whimsicott — disruption, team speed and residual pressure

Intended role: create useful openings for the team while keeping Moonblast as a
real attacking option.

Its block gives Taunt +5 only for a fresh, ordinarily viable target with status
moves; an all-damaging target instead loses 6. Tailwind gains +5 with reserves
or +3 alone when doubled speed can change an ordering for Whimsicott or a living
reserve. Reserve raw speed is a payoff estimate, not a promised switch. Active
Tailwind or Trick Room loses 6. Fresh, ordinarily viable Leech Seed gains +4 in
a survivable position or +1 under immediate pressure, with explicit Grass,
Substitute and already-seeded checks. Moonblast receives best-attack merit.

Ordinary Prankster/Dark, Magic Bounce, immunity and legality checks remain in
charge. A reliable KO suppresses further preparation; there is no forced
Taunt/Tailwind/Seed sequence.

## 2. Alcremie — G-Max damage/healing and non-Max sustain

Intended role: pressure through its real coverage, use Finale's healing when
available, and recover normally outside Dynamax.

Damaging moves receive best-attack merit. Dazzling Gleam receives +3 while
planning/using Max and missing healable HP, reflecting its real G-Max Finale
conversion. Recover loses 8 in that context because it will be Max Guard, not a
heal. Otherwise its recovery adjustment ranges from -10 for full/blocked healing
to +6 for useful low-HP sustain or +8 when acting before a survivable lethal hit.
Unsurvivable pre-heal pressure loses 8.

Non-Max Mystical Fire gets +3 against a known special attacking kit while the
target's Special Attack can still fall. It gets no fake Sp. Atk-drop credit while
converted into Max Flare. G-Max activation and its one-use resource remain ordinary
engine decisions. Existing shared Max-effect valuation is retained.

## 3. Glaceon — matchup-aware Ice coverage and useful Yawn

Intended role: choose its best actual damage, with Yawn as a useful disruption
option rather than a repeated idle action.

All damage receives best-attack merit. Freeze-Dry receives another +2 against
Water only when the actual cached, accuracy-weighted damage is best. It does not
blindly beat a stronger Blizzard or Shadow Ball. Fresh, eligible Yawn gains +5
when current pressure is survivable or +1 otherwise. Existing drowsiness,
status/sleep restrictions, abilities, Substitute and sleep clause suppress that
bonus. A reliable KO takes precedence over Yawn.

## 4. Vanilluxe — weather-supported offense with targeted Taunt

Intended role: exploit Snow Warning's accurate Blizzard and choose coverage when
the real matchup is better.

Blizzard receives +2 beyond best-attack merit only in icy weather, with cached
accuracy >=100 and best expected damage. Replacing the weather removes that
credit on the next rebuild. Best Water coverage can give Freeze-Dry +2. Fresh
viable Taunt gains +3 against status-capable targets and loses 6 against an
all-damaging kit. No static-power preference overrides the actual damage cache.

## 5. Audino — reachable Wish, useful restriction and item removal

Intended role: sustain a reachable occupant/reserve, disrupt meaningful options,
and make progress through Knock Off.

Wish receives +6 when current HP is below 65%, or an injured living reserve can
actually occupy its position. A pending Wish loses 10; no reachable injured
recipient loses 4. No Order Change prevents reserve-healing credit. These values
do not force a switch; ordinary switching/Regenerator behavior remains separate.

Encore gets +4 for a valid, ordinarily viable last status move when the target
also has attacks and is not already Encored. Thunder Wave gets +4 against a
faster/equal-speed eligible target or +2 against a slower one. Knock Off gets
best-attack merit and +3 only for an ordinarily viable removable item. Ordinary
mental prevention, status/type immunity and item-removal prevention still apply.

## 6. Blissey — sustainable attrition without redundant setup

Intended role: recover when it improves survival, establish useful hazards/status,
and cash in fixed damage when it wins.

Soft-Boiled uses the shared trainer recovery adjustment against current actual
HP/maxHP and incoming timing. Seismic Toss receives +2 pressure plus best-attack
merit when it can damage the target. Toxic gets +4 in a survivable eligible matchup
or +1 under pressure. Existing status/immunity checks prevent false Toxic credit.
Stealth Rock gains +4 only with a surviving reserve to punish and a survivable
setup window; existing rocks or no reserves lose 6. A reliable KO suppresses
additional healing/hazard/status preparation.

## 7. Smeargle — retain the established Any Move policy

The explicit Smeargle block retains `KoankoSmeargleAnyMoveActive`, fresh-choice
sampling, and the previous `RankStatus`/`RankDamage` functions. None of the new
generic +2/+6/-15 trainer adjustments are stacked onto its ranks.

It still samples at most the existing eligible pool, stops at 60 positive accepted
candidates or exhaustion, applies its established damaging-move eligibility
filter, resolves tied ranks through its tagged policy, and executes one winner
through real slot 0. The established damage/reliability/recoil/recharge/priority
and utility policy remains. Break Bar progress retains value without receiving
the victory tier. PP/lock/forced-turn behavior is unchanged.

## 8. Solo TX Rollback — coordinate its currently implemented real kit

Intended current role: apply Doom Desire pressure, sustain itself through Wish and
Wishful Thinking, and bridge a useful delay with Protect.

A fresh damaging Doom Desire gets +4; an already occupied delayed-damage position
loses 10. Wish gains +6 for reachable missing HP. Its own effective Wishful
Thinking can add +1 for remaining raw-stat growth and +1 for immediate missing-HP
healing, reflecting the current ability without changing who triggers it. A
pending Wish loses 10. Pressure exceeding current HP plus the immediate 1/16 heal
reduces Wish by 8 rather than treating delayed healing as instant rescue.

The first Protect gains +6 with pending Wish and incoming damage, or +2 while
delayed damage is pending. Repeated Protect loses 8; an otherwise idle Protect
loses 4. Failed/blocked ordinary actions are not rescued by these bonuses.

Stage04 implements the solo virtual Wish profile and Reduction, awaiting user-run
validation. Stage05 still owns production virtual-action ranking, Meteo and
cooldown. See [the Stage04 codeflow handoff](../extra%20docs/KoankoDev/STAGE04_IMPLEMENTATION_AND_CODEFLOW.md).
The block does not grant summon-owned Wishes or invent missing outcomes. The
current all-holder Wishful Thinking trigger footprint remains unchanged; this
block only values benefits to the casting TX itself.

## 9. Calyrex Ice / Shadow — useful setup for the actual offensive form

Intended role: invest in a relevant offense when safe, then exploit that offense
through the actual loaded kit and current Tera assumption.

Attacks receive best-attack merit. Swords Dance considers Attack/physical payoff;
Nasty Plot considers Special Attack/special payoff. Setup gets +5 only with a
usable damaging move of that category and survivable pressure. No payoff, lethal
pressure, or an already +4-or-higher relevant stage loses 8. A reliable KO reduces
status preparation by another 15.

The block reads the actual Ice or Shadow species, stats, moves and caches; it
does not transform Calyrex or force Fire Tera. Ordinary SMART_TERA, availability,
consumption and the approved one-time opponent reset remain authoritative.
Stage06 still owns the actual form/kit reload and committed-move/PP preservation.
The Shadow test constructs a mechanism fixture, not a new encounter transition.

## 10. Eternatus / Eternamax — coverage and form-sized sustain

Intended role: use productive coverage, maintain its actual current HP pool, and
reserve recharge attacks for a worthwhile payoff.

Attacks receive best-attack merit; Recover uses actual current maxHP, both enemies'
known pressure and the timing of pre-heal attacks. It does not use covered layer
HP or an assumed Dynamax multiplier. A nonwinning recharge move loses 3 when a
usable immediate non-recharge alternative reaches at least 80% of its expected
damage. A reliable KO retains its immediate payoff.

Both species are supported when actually loaded. The test loads genuine Eternamax
species stats, but Stage06/07 still own its actual entry/kit transition, independent
ally and resulting encounter topology. No artificial Dynamax state is added.

## 11. Dialga Origin — pressure with conditional Time Stop value

Intended role: use real coverage and weigh Roar's special payoff against recharge.

Attacks receive best-attack merit. The same comparable-alternative recharge
adjustment applies. Roar's Time Stop utility remains 4 times actual proc
probability: +2 at 50%, never a guaranteed extra turn. It adds no new secondary
credit for immunity, blocked secondaries, Substitute, Sheer Force, an existing
stop/cooldown, or an already reliable KO. Ordinary recharge/canceler behavior is
unchanged. Time Stop's incremental test is compared against the blocked score so
ordinary pressure merit is not confused with secondary utility.

## 12. Arceus — actual Judgment coverage and deliberate recovery

Intended current role: pressure through Judgment or coverage as the actual type
matchup warrants, and recover when sustain beats further preparation.

Attacks receive best-attack merit. Judgment gets +1 only when its actual cached
damage is best and nonzero; no Normal-type assumption or unconditional signature
priority is used. Recover uses actual current-layer HP/maxHP and timing. A useful
KO beats further recovery.

Stage09 still owns opening protection, seal/saved-layer transitions, crushing,
converted Judgment typing and the exact Bug Space presentation with real Multitype.
The block can consume corrected caches when those mechanics exist; it does not
fabricate their execution or change Arceus's ability.

## Zacian: where its AI belongs

**The single encounter callback can dispatch Zacian's tactics, but the current
opponent-only route is not yet a safe Zacian controller.** Recommended Stage07
integration: recognize the explicit temporary ally owner before the opponent-side
guard, then delegate to a separate `AI_KoankoZacian` helper in this module. Keep
Koanko's enemy roster switch separate. This proposed helper is not implemented or
registered by species alone in this change.

The missing requirements are concrete:

- `battle_ai_util.c:BattlerHasAi` recognizes player-right for INGAME_PARTNER or
  AI-vs-AI, not the proposed DOUBLE-only independent formation. Installing a
  player-partner controller alone does not make `ComputeBattlerDecisions` score it.
- `PlayerPartnerHandleChooseMove` can consume the resulting cached move/target, so
  it is a useful execution path once ownership is correctly routed.
- `PlayerPartnerHandleChooseAction` calls `AI_TrySwitchOrUseItem`; those paths,
  ChoosePokemon, and gimmick commitment need independent-ally gating outside a
  move-scoring callback. Do not enable INGAME_PARTNER/MULTI as a shortcut.
- Current party/history/gimmick storage indexes real player side/party slots.
  Stage07 must supply independent owner storage and route these accesses, retire
  the actor safely, and preserve player-left commands/resources/loss authority.
- Attack/setup policy can then prefer real useful pressure, suppress redundant
  Swords Dance, and use the approved Behemoth-versus-Eternamax rule identically in
  scoring and execution. The tuning/damage rule and owner lifecycle are not
  invented here.

The ownership regression explicitly proves that an ordinary player-side Zacian
does not acquire this boss policy just by sharing its species.

## Integration, maintenance and validation

All new tactical conditions are in the dedicated Koanko module. The only new
core integration is the small registration hook in `BattleAI_SetupFlags`; the
existing dynamic pass, selection, switching and executor remain. This avoids
duplicating the core scoring pipeline and concentrates future boss tuning in
one file. Upstream AI flag initialization/dynamic callback lifetime still require
semantic review during merges. The approved earlier shared fixes are preserved.

No new per-battle storage or cleanup path is needed. Scores and context are local
to evaluation; current caches rebuild normally at fresh choices. No phase script
or gameplay resource is changed during a policy comparison. Original-team
switching remains ordinary; conjured sequencing/No Order Change remain their
existing engine policy. See the upstream modification ledger for the registration
hook and earlier shared changes.

Tests are in `test/battle/koanko_ai.c`, with existing sampler/ranking/diagnostic
tests retained in their respective files. `Koanko Tactics` compares conditional
deltas using real roster kits/caches; existing `Koanko AI` fixtures also observe
production selection and real damage/status/healing/item effects. Mechanism-only
future-form fixtures are labeled as such.

```bash
make check -j8 TESTS='Koanko Tactics'
make check -j8 TESTS='Koanko'
make check -j8 TESTS='AI'
make check -j8 TESTS='Arcanum'
make check -j8 TESTS='TX Wishes'
make -j8
```

Validation on the fingerprinted dirty `mypatch` snapshot (base `50ebce4990`):

- `Koanko Tactics`: **15/15 passed** (parameterized trials include all original
  members, both Eternatus species, and Shadow-kit mechanism readiness).
- `Koanko`: **81/81 passed twice** on the final snapshot, retaining all Smeargle
  sampler/ranking/diagnostic, Loaded Dice and earlier Stage11 execution/cache/resource
  cases. The same timing fixture remains 60 evaluations / 21 frames with ranking
  versus 60 / 19 without; diagnostic printing is excluded.
- `AI`: **310 passed**, **4 pre-existing KNOWN_FAILING**, **4 pre-existing TODO**;
  no new unexpected failure.
- `Arcanum`: **150/150 passed**; `TX Wishes`: **22/22 passed**.
- `Break Bar`: **37/37 passed**; `Wishful Thinking`: **10/10 passed**.
- Normal `make -j8`: **passed**. Fingerprinted source remained unchanged through
  those runs and the production build.

Logs and source hashes are in `TEMPSCRIPTS/koanko-tactics-*.log` and
`TEMPSCRIPTS/koanko-tactics-snapshot.json`. The old Time Stop absolute-score
assertion was updated to isolate its incremental secondary benefit. The new
Blissey fixture clears opening-turn rocks before comparing fresh/redundant
hazards. A zero-damage recharge-comparison edge case was corrected in the policy.
One initial cache-fixture invalid-party-size report did not reproduce in its
isolated run or either final full run; no gameplay correction was inferred from
that unconfirmed report.

Long endurance playthroughs, fine balance tuning against diverse player teams,
and missing encounter-stage transitions remain separate acceptance work. Current
weights are tested conditional heuristics, not an exhaustive turn search. No
commit or push was performed. For repeatable validation keep the fingerprinted
AI/Koanko source, header, trainer/party data and test file stable; regenerate the
fingerprint if changing that snapshot.
