# Arcanum reference

Arcanum currently gives opponent Castoria four virtual move slots, outside the
ordinary four-slot move and PP arrays. The pool is Fireball, Revivify, Wish, and
Polymorph. Mage Armor is a separate scripted opener on the backed-out name route.
MP costs are not implemented.

## Special AI policy

In singles, ordinary moves reach the registered callback through the existing
scoring pass. Usable Arcanum slots are evaluated afterward through that callback,
starting at `AI_SCORE_DEFAULT` (100). A positive spell score must strictly exceed
the best ordinary score to win; ordinary moves win ties. Zero or negative spell
scores decline that candidate. Four-slot score and simulation arrays are unchanged.

Inside a callback, `BattleAI_IsScoringArcanum()` distinguishes a spell candidate
from an ordinary move, including when both have the same move ID. Use the supplied
move ID and safe damage helpers for spell rules. `movesetIndex` is not the spell
slot, and ordinary cached effectiveness/accuracy entries do not describe it. Use
plain score arithmetic in the spell branch: ordinary `ADJUST_SCORE` test logs are
indexed by `movesetIndex`. The context is restored after each callback.

The fight registers `AI_RazeluxeTrainer`. Castoria's spell branch runs before its
ordinary-slot helpers. While Castoria is affected by Perish Song, Revivify receives
`INT32_MAX` priority if she has an eligible fainted party member. With nobody to
revive, Revivify receives zero; otherwise it retains the supplied baseline.

If the active opponent has Perish Song in its actual moveset, Polymorph receives
`INT32_MAX - 1` priority, including before Perish Song is revealed. An eligible
Revivify therefore outranks Polymorph independently of pool order. Existing slot
restrictions filter spells before scoring. Other spells retain the baseline.
Alice's callback declines spell candidates.

## Wish

`MOVE_ARCANUM_WISH` is separate from ordinary Pokemon Wish. Domination requires
the true-name route, zero Command Seals, and an unused Wish. Its score remains the
supplied baseline; tactical priorities are not final.

Special AI stores a candidate outcome during scoring and commits it only if Wish
wins selection. Execution rechecks the committed outcome after the ordinary attack
canceler. Successful Domination restores three seals and consumes Castoria's
once-per-battle Wish. Interrupted or failed actions do not consume it. Switching
and slot changes do not reset the use flag. Wish cannot be copied or called by
other moves.

Endurance is also implemented. It requires the true-name or fake-name route, an
unused Wish, and no currently active extra bar. At execution it saves the covered
bar's current/max HP in its owner's party state, then adds a full purple bar with the current
max HP. After the move animation, purple fills from left to right over the saved
red fill. The fill progress is visual only. Breaking purple reveals the saved HP
pair immediately, without healing the underlying bar or carrying over excess
damage. The snapshot and active bar survive switching. All hits of the breaking
move finish before the saved layer is revealed; only a later move can damage it.

Endurance shares the once-per-battle Wish use with Domination. Selection eligibility
includes implemented outcomes, while the production outcome picker still chooses
only Domination. Endurance tactical rules remain deferred. Tests nominate Endurance
through their own Special AI callback and exercise normal selection and execution.

Reserved outcomes are Reduction, More Wishes, Debilitation, and
Normality. They remain unavailable until their effects are implemented. The
agreed name-route rules for those future outcomes are:

- Reduction: fake name or backed out.
- More Wishes: fake name only.
- Debilitation and Normality: any resolved name route.

These route rules do not bypass the spell's ordinary selection restrictions.
