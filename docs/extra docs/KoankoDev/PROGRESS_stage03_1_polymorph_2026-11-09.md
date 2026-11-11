# Stage 03.1 - Polymorph scoring repair (status: INTEGRATED, FOCUSED TESTS PASS)

Base: ba3fd15d (Stage 01). Independent of Smeargle work; the patch touches only Polymorph files.

## Bug
`CastoriaSelectPolymorphTargetAndBudget` sampled up to 60 species, scored them, picked the best
(`chosen = samples[bestIdx]`), then ran `if (topCount == 0) chosen = RandomUniformExcept(...)`.
`topCount` was declared 0 and never incremented, so the branch always ran and the score never mattered.

## Fix
- Removed `top[]`, `topCount` and the overwrite. The sampled winner is final.
- Tie policy unchanged: strict `>`, earliest sample wins; samples are in random draw order so equal bests are uniform.
- `sampleCount == 0` still falls back to a random legal species. Move-budget math unchanged.
- New `PolymorphPickBestSample()` (pure) and TESTING-only `Test_GetPolymorphSelectionDebug()` {sampleCount, bestSampled, chosen}.
- `struct PolymorphCandidate` / `POLYMORPH_TOP_CANDIDATES` are now unused; left in place (no cleanup).

## Files
src/castoria_name_theft.c, include/castoria_name_theft.h, test/battle/move_effect/polymorph.c

## Tests added (NOT RUN here - no ARM toolchain)
- 3 pure TESTs for the picker (max, earliest-of-equal, single).
- Battle test: Castoria (opponent) polymorphs the player's Wobbuffet; asserts sampleCount > 0, chosen == bestSampled, species applied.
- Battle test: HP/species invariants after player-side selection.
Existing 4 Polymorph tests target the opponent side, which never calls the selection function, so they are not exercised by this change.

## Follow-up integration (2026-11-10)

Ported the three Polymorph hunks from Claude's `d30c3c90` in
`koanko_followups_polymorph_test_and_smeargle_tie.patch` onto the existing working-tree
repair. No cumulative patch was reapplied. Existing scratch-buffer reuse, winner
selection and debug observations are retained.

The reported zero-sample fixture failure was identified as a test-harness artifact:
unconfigured `RandomUniformExcept(RNG_POLYMORPH, ...)` returns the highest non-rejected
species (reported as 1523), repeatedly outside the target's hardware band. It does
not demonstrate that production RNG cannot find candidates.

`Test_SetPolymorphDrawSpecies(species, count)` borrows a caller-owned array under
`#if TESTING`. The sampling loop cycles through that array, bypassing the RNG draw;
the caller supplies legal, in-band species. NULL or zero count restores ordinary
draws. Production sampling, rejection, fallback and budget logic are unchanged.
The fixture uses static Caterpie/Metapod/Weedle/Kakuna/Pidgey draws, installs them in
GIVEN, retains the sampled-winner assertions, additionally verifies membership in the
injected set, then clears the override. Static storage keeps the array alive through
the battle; the override is test state and must be reset by its caller.

User-provided results on 2026-11-10: **9/9 Polymorph tests passed**, including the
injected-draw regression (60 accepted samples in 60 attempts, chosen species 11).
The separate HP/species invariant fixture used the ordinary zero-sample fallback;
that pass does not establish production sampler distribution.

No new upstream engine hooks or state-layout changes. No agent build/test was run.
Keep the current `mypatch` source snapshot (HEAD `50ebce4990` plus
all working-tree integration changes) stable during validation.

## Risks
- The deterministic fixture verifies selection from supplied candidates; it does not validate production RNG distribution or sampling success rates.
- Behavior change by design: Polymorph on player mons now picks the max-cruelty sample instead of a random species.

Focused command from the repository root in PowerShell:
`wsl make check -j4 TESTS=Polymorph`
