# Stage 03.1 - why `sampleCount` was 0

Your run: `POLY cur=541 target=421 band=168-273 chosen=1523`, `polymorph.c:111 EXPECT_GT(0, 0)`.

## Root cause: test-harness artifact, not the repair and not the band
Under the battle test runner, an untagged `RandomUniform`/`RandomUniformExcept` returns a FIXED value:
`RandomUniformDefaultValue` (test/test_runner.c:946) returns `hi`, walked down past rejected values.
So all 600 sampler draws were the same species (the highest legal id = 1523, your `chosen=1523`), whose hardware score is
outside 168-273 -> every attempt `continue`d -> sampleCount 0 -> random fallback (itself the same fixed species).
Production uses real `Random()`, so this does not occur in the game.

## Is the band itself sane? (measured from species data, my parse of 1117 species, 869 non-high-tier)
Hardware = BST + variance/2 (min 180, median 475, max 796).
target 421 (cur 541): band 168-273 holds 49 species (5.6%) -> E[samples in 600 draws] ~34, P(0 samples) ~ 0.
Band density by target mon threat (cur): 400 -> 1 species (P(0)=50%), 541 -> 49, 600 -> 106, 700 -> 271 ... 1000 -> 527.
=> For genuinely weak player mons (cur threat <= ~450) the band is nearly empty and Polymorph falls back to a random
   species; for typical mons it works. Not a bug, but a tuning caveat (e.g. a fixed floor on the band would avoid it).
(Parse is an approximation: ignores IsSpeciesEnabled config and custom species. Script: poly_band.py.)

## Fixes (new commits, not amends)
d30c3c90: TESTING-only `Test_SetPolymorphDrawSpecies()` injects the sampler draws; the player-side test now uses 5 in-band
species and asserts chosen == bestSampled and chosen is one of the injected draws (not the fallback).
Same harness behavior also broke my Stage 03 tie test (a loop of 40 plain selects can never see both tied moves under a
fixed RNG); it is rewritten with PASSES_RANDOMLY(50,100,RNG_KOANKO_SMEARGLE_TIE).
Other Stage 03 tests are unaffected: Fisher-Yates stays valid when every draw returns `hi`.
Not compiled or run here.
