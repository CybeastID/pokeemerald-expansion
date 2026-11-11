# Stage 03 pre-implementation finding: what Polymorph gives Smeargle
Evidence: src/castoria_name_theft.c (ScorePolymorphCruelty, CastoriaSelectPolymorphTargetAndBudget, ConstructPolymorphMoveset, EvaluateMoveThreat, sPolyScratch), src/battle_ai_main.c (BattleAI_SetupAIData, CalcBattlerAiMovesData, ChooseMoveOrAction_Singles, BattleAI_DoAIProcessing), include/battle_ai_main.h, src/random.c. Nothing was compiled or run.

## Answer: Stage 03's scoring infrastructure is NOT redundant
1. Polymorph does not construct hypothetical moves. It samples SPECIES and builds a moveset for the chosen species. No scratch move slot, no candidate-move cache.
2. It does not use ordinary AI scoring. Species are scored by a bespoke heuristic (ScorePolymorphCruelty: type exposure to Castoria's moves, speed, ability, EV mirroring). Moves are scored by EvaluateMoveThreat (static power*accuracy*STAB plus status tiers). Neither touches gAiThinkingStruct, sBattleAiFuncTable, AI_CalcDamage or the per-slot caches.
3. So nothing in Polymorph can score "Smeargle uses move X against this target right now".

## What Smeargle CAN reuse from Polymorph (idiom only)
- The bounded Monte-Carlo shape: collect up to 60 samples with an attempt cap.
- The EWRAM scratch-union idiom (sPolyScratch) to keep big buffers out of the battle-script stack.
- A tagged RNG enum entry (new tag for Smeargle).
NOT reusable as-is: RandomUniformExcept. It samples with replacement and loops forever until the predicate accepts (LOOP_RANDOM), so it cannot give distinct draws or a guaranteed bound. Stage 03's partial Fisher-Yates is still required.

## BUG found in Polymorph (not touched; your call)
CastoriaSelectPolymorphTargetAndBudget declares `topCount = 0` and `top[]` but never fills or increments them. After the 60-sample Monte Carlo picks a best species, line `if (topCount == 0) chosen = RandomUniformExcept(...)` always runs and overwrites it. Net effect: the cruelty score and hardware band never influence the species; Polymorph picks a uniformly random legal species every time. The move budget is still derived from that random species. Fix, or keep the random behaviour? This also means Polymorph is not a proven scoring baseline.

## What is missing and must be built (the real Stage 03)
The ordinary AI is slot-indexed: gAiThinkingStruct->score[4], simulatedDmg/effectiveness/moveAccuracy[atk][def][slot], moveLimitations bitmask, moves read from gBattleMons[].moves via movesetIndex. There is no single-move entry point. Needed:
- A one-slot version of CalcBattlerAiMovesData (damage, accuracy, effectiveness for slot 0 vs the one defender in singles).
- A driver that resets the thinking score for slot 0, sets scratch move + nonzero PP, runs the enabled flag passes once, and restores.
- Distinct (without-replacement) draws + positive-score acceptance + best/tie tracking (tagged reservoir ties; the ordinary chooser's Random() % n is untagged).
- PP/Encore/Choice gates and the commit into slot 0 (Stage 03 sections 3.5-3.7). Unchanged by this finding.

## "60 positives, not 60 evaluations": how many evaluations really happen
- Scores start at AI_SCORE_DEFAULT = 100. Penalties are small (NO_DAMAGE_OR_FAILS = -20). A slot only becomes <= 0 when it is unusable (PP 0, MOVE_NONE, limitation, ShouldConsiderMoveForBattler false) or stacks about five penalties.
- So for usable moves "positive" is almost always true; expect roughly 60-70 evaluations, not hundreds. That is a prediction from the macros, NOT a measurement. The worst case is bounded by the eligible pool size N (hundreds of IDs), never unbounded.
- Cost per evaluation is small for Koanko: her flags are only CHECK_BAD_MOVE, ACE_POKEMON, OMNISCIENT, TRY_TO_FAINT, so two scoring passes plus one AI_CalcDamage. No CHECK_VIABILITY, so no cross-move comparison pass to emulate.
- Recommendation: add a TESTING-only counter of evaluations per choice so the real positive rate is measured instead of guessed.

## Quality caveat worth a decision
With only those flags, most non-lethal candidates sit at 100 (or 80 after a bad-move penalty). "Best of 60" therefore means: take a move that can KO if one was drawn, else a uniform random tie among the best scores, often an arbitrary status move. That is exactly what the ordinary AI would do with her flags; it is not smarter. Options: accept (cheap, faithful), or give Smeargle's evaluation a richer flag set (e.g. CHECK_VIABILITY) without changing first-team AI.

## Stage 01 status (your note)
Committed separately (ba3fd15d) and still UNVALIDATED: never compiled or run. Stage 03 will be its own commit. Please run `TESTS=Koanko` before trusting either.

## Decisions I need from you
1. Polymorph topCount bug: leave as is / fix (affects Castoria's fight).
2. Smeargle scoring flags: Koanko's existing four (default) or richer for Smeargle only.
