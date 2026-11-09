# TX Rollback / Wish for More Wishes

## Current scope

The engine supports Castoria's fake-name-route creation of TX Rollback during an
ongoing local Singles Trainer battle. Production eligibility is confined to
Razeluxe with Castoria in party position five and an empty sixth position.
Production Castoria Wish scoring still selects only Domination. More Wishes
tactics, TX's later Wish scoring, and the final ordinary combat kit remain deferred.
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
nature, no item, and Psychic, Swift, Protect and Celebrate. These are provisional
encounter inputs. Ordinary Jirachi is unchanged.

## Ownership and position policy

`BattleStruct.txRollback` owns the profile, lifecycle, enemy party index, and
script return context. Ownership follows that enemy party entry through changes
to its current species or ability. The species itself grants no bars, trapping,
revival restrictions or extra actions.

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
3. Registers two extra full Break Bars and applies No Order Change.
4. Loads the missing right-enemy panel/shadow resources, slides Castoria from
   Singles x=176 to doubles x=200 over twelve frames, then presents the creation.
5. Runs ordinary entry abilities/items/reactions, skipping creation hazards.
6. Preserves No Order Change on the surviving creation and restores cast context.
7. Completes Castoria's Wish move-end effects and pending Break Bars.
8. Dispatches the one-time ambush before advancing the original action cursor.

No occupant initialization runs after No Order Change is applied. Successful
initialization commits Wish use; subsequent combat effects do not refund it.

Bars use the shared full-refill profile: blue, purple, then red. Their normal
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

The summoned party entry has one virtual, PP-independent `MOVE_ARCANUM_WISH`
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

Reversal, Reclamation and Calamity bypass initial defensive prevention, including
Protect, Substitute, Soundproof, Contrary, stat-loss protection and reflection.
The arrival Endurance, phase Reduction and ordinary phase Wish are separate from
these five selectable outcomes.

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

Build with `wsl make`. Run focused coverage with
`wsl make check TESTS="Arcanum Wish More Wishes"`; use the Arcanum, Break Bar,
No Order Change and full battle groups for regressions. Use headless testing.
