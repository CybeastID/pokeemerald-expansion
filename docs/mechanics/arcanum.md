# Arcanum reference

1.17.2 migration status: the ROM builds and the complete `arcanum.c` file passes
71/71; Debilitation passes 22/22 and Normality 9/9. Other Wish files and broader
regressions remain under validation; this is not migration acceptance.
Polymorph snapshots now follow trainer-owned
party rows, so equal slots in independent partner/opponent parties do not
overwrite each other's originals. Battle-exit restoration of persistent player
Pokemon uses the player row. See the migration checkpoint and
`test/battle/migration_owner_apis.c` (4/4 passed) for the validation boundary.

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

Endurance shares the once-per-battle Wish use with all implemented outcomes. Selection eligibility
includes implemented outcomes, while the production outcome picker still chooses
only Domination. Endurance and Debilitation tactical rules remain deferred. Tests
nominate them through their own Special AI callback and exercise normal selection
and execution.

Debilitation is implemented for any resolved name route. It applies ordinary
Poison, Burn, Sleep, Freeze, Paralysis, and Frostbite concurrently to the living
opponent opposite the caster (the other opponent is the fallback in doubles).
Initial status prevention, including typing, abilities, Safeguard and terrain,
does not block this Wish. Existing ailments and counters are preserved; a missing
Sleep condition receives the normal generation-dependent random duration.
Subsequent recovery, status damage, abilities and cures retain their normal engine
behavior. In particular, the existing action canceler checks Sleep before Freeze.
Debilitation also applies [No Order Change](no_order_change.md), which remains
after the nonvolatile ailments are cured. A successful application spends the same
shared Wish use; interruption or a missing living target does not.

Normality is implemented for any resolved name route. It ends the living opposing
target's active Mega Evolution, Primal Reversion, Ultra Burst, Dynamax/Gigantamax,
or Terastallization, then changes its battle types to pure Normal. It uses the
same opposite-position target and doubles fallback as Debilitation. A target
without a gimmick can still be made Normal. Like Debilitation, this direct Wish
effect bypasses initial type-change prevention, Substitute and Protect.

Existing form-reversion, Dynamax HP, form-animation, healthbox and primal-weather
helpers handle the reversal. No additional vanilla battle-flow hooks or persistent
flags are introduced. Used gimmick charges remain spent. One-off Z-Moves,
Copied Transform forms and unrelated ability/item forms remain intact. The Normal
typing has ordinary type-change lifetime: switching restores species types,
and later effects can change them. Primal Reversion can activate again on a later
entry with its Orb, following the normal engine rules. Production tactical
selection remains deferred, as for Endurance and Debilitation; tests nominate
Normality through Special AI and execute the ordinary Arcanum action.

Reduction is implemented for the fake-name and backed-out routes. It curses the
player's side for the rest of the battle. The casting turn has no tick; each later
end of turn subtracts 10, then 11, then 12 levels and so on, down to Level 1. The
loss amount is shared across the side and advances once per turn in doubles.
Switching clears the outgoing occupant's accumulated loss, but does not reset the
side's next subtraction. The replacement receives that next subtraction at the
end of its entry turn. Break Bar grants, breaks and reveals do not reset it.

Reduction is a battle-only overlay: party experience, real level and derived
stats stay unreduced, and party screens keep those real values. The battle
healthbox and level-dependent combat checks use the effective level. HP maintains
its percentage as maximum HP changes, with fractional precision retained across
rescaling; fixed healing items heal displayed battle HP. Damage and healing are
converted back to the ordinary HP scale for party synchronization. Level decay
alone cannot faint or revive a Pokemon.

Polymorph only reads the unreduced party representation. Its original species,
experience, nickname, moves and PP backup is never owned or overwritten by
Reduction. A Polymorph cast finishes its canonical species/stats/HP conversion
before the accumulated Reduction is reapplied. Removing Reduction does not undo
Polymorph. Battle cleanup first removes the overlay, then uses existing form and
Break Bar restoration, including converting Polymorph's surviving HP layers to
the original species' HP scale. Production tactical selection remains deferred.

More Wishes is implemented for the fake-name route in the supported local
Razeluxe encounter. It creates the distinct TX Rollback species in a new enemy
position during the ongoing Singles battle, permanently keeping player-right
unavailable. The creation has two extra Break Bars and No Order Change, cannot
withdraw or be revived, and performs one scripted, PP-independent ambush before
the next scheduled action. Both enemies must be defeated. Production tactical
selection remains deferred; tests nominate the outcome through Special AI.
See [TX Rollback](tx_rollback.md) for ownership, initialization and script contracts.

TX Rollback has its own PP-independent virtual Wish slot and independent outcome
state, with reusable Reversal, Reclamation, Calamity, Restoration and Reciprocity.
Its default arrival grants Castoria saved-layer Endurance. Its original breaks
script ordinary Wish, then Reduction plus Doom Desire, without consuming normal
actions or Castoria's Wish allowance. Production selection of the summon and
later TX Wishes remains deferred; deterministic Special AI callbacks test them.

These route rules do not bypass the spell's ordinary selection restrictions.
