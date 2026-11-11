# Alice / Razeluxe tactical AI - proposal applied + fixtures (branch claude-ai-tactics-fixtures, base 582c5f50)

Commit 1 cfbca1ea: proposal code applied to src/battle_ai_main.c. ONE deviation: Castoria Polymorph outside the Perish
Song countermeasure returns the caller's default score (existing behaviour) instead of 0.
Commit 2: test/battle/alice_tactics.c (6 tests), test/battle/razeluxe_tactics.c (9 tests; 2 parametrized).
Status: host-clang -fsyntax-only clean for all three; NOT built for ARM, NOT run. Run:
  make -j10 check TESTS='Alice Tactics'    make -j10 check TESTS='Razeluxe Tactics'

## Expected score arithmetic (for triaging a failure)
base 100 | best expected damage +3 | secure KO +8 (min roll, acc >= 90) | layer break/reveal +3 | status while AR_Finish -18
setup (Calm Mind etc.) +6 (+3 boosted-KO estimate, +2 defensive payoff) or -10/-12 when unsafe | Authority 6+swing/2 (<=30), -40 if it fails
Most fragile assertions (depend on unmeasured ordinary-pass contributions):
 - Espeon Calm Mind (106) > Psychic (103) when safe
 - Raze Kirlia Will-O-Wisp (109) > Psychic (103)  (also Psychium Z / gimmick selection)
 - Castoria Revivify (+10) > Earth Power (103)
If these fail by a point or two, inspect ordinary passes before changing the policy.

## Not covered
Kazuradrop (opener, emergency slot, actor-bit swap, post-transform), Umbreon pending-Wish/Protect timing, Gothitelle Rest budget,
Palkia Draco Meteor comparison, Hoopa Nasty Plot / Protect bypass, Alakazam Trace/Unaware, Hoopa-Unbound Drain Punch/Fury,
Castoria Wish ranking, Perish Revivify, Polymorph selection, Fireball vs ordinary score scale (flagged concern), summoned-TX
virtual Wishes, callback isolation (a Mega-Mewtwo "legacy path" check was dropped: ordinary scores tie, so it was not robust).
