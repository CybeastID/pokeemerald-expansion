# Stage 04 — TX encounter profiles, deteriorating maxHP and Koanko Reduction

Status: **fully planned**, implementation pending. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Reuse the existing TX Wishes/lifecycle for Castoria and Koanko without giving solo TX Castoria's summon topology, bars, ambush or revival policy. Add 100%→60%→30% actual maxHP for summoned TX and a selectable 1,2,3… Reduction only for Koanko. Meteo and final tactical scoring belong to stage 05.

## 2. Existing code verified

- `include/battle_tx_rollback.h`: `TxRollbackBattleData` holds singleton summon lifecycle, owner party index, caster identity, selected Wish/target and Reciprocity; `TxRollbackProfile` currently contains only an ambush script. Wish enum has Reversal/Reclamation/Calamity/Restoration/Reciprocity, no Reduction choice.
- `src/battle_tx_rollback.c`: eligibility requires the supported Castoria encounter/empty sixth enemy slot; creation uses a real enemy slot, expands singles to four positions, disables player-right and reserves/retires enemy-right. `FormationActive`, `TxRollbackOwnsPartyMon` and `TxRollbackCanRevive` currently couple owner identity to summon formation. Creation still assigns Psychic/Swift/Protect/Celebrate, not the intended support kit.
- `src/battle_tx_wishes.c:TxRollbackHasWishes` requires summon lifecycle/ownership. `TxRollbackCanWishFor` validates five existing effects; `BS_TxRollbackTryWish` consumes the committed outcome and dispatches success scripts. Reciprocity healing and its script setup hardcode opponent-right. Those assumptions fail for Koanko's opponent-left TX.
- `include/battle.h` virtual-slot accessors expose one TX Wish slot only. `src/castoria_name_theft.c:IsArcanumSlotUsable` currently requires a usable Wish for every TX virtual slot; this must be move-specific before adding Meteo.
- `sTxBreakBars` gives two extras, with first-break Wish and second-break Reduction/Doom Desire as **afterBreakScript**. `TryBreakBar` in `battle_break_bar.c` runs onBreak/refill before afterBreak and bypasses both configured scripts for saved layers. `BS_RefillBreakBar` uses current actual maxHP.
- `src/battle_reduction.c`: `ReductionActivate` clears state and starts 10+ with casting-turn grace. `ReductionEndTurn` applies one shared nextLoss per complete turn, stores per-occupant accumulated loss and increments global acceleration. Mutation/normalization helpers preserve canonical party data, HP fractions and copied stats; `ReductionResetOccupant` resets occupant loss, `ReductionFinishBattle` restores on exit. Player-side filtering already exists. `MAX_LEVEL` is 105.
- Existing tests: `tx_wishes.c` covers all five effects, ownership, saved layers, scripted phases, delayed attacks, Reciprocity exclusions and interruption. `arcanum_reduction.c` covers original 10+ progression, switches, doubles, HP/controller normalization, items, Polymorph, Break Bars, faint/revival and eligibility. `arcanum_more_wishes.c` covers creation, ownership/topology and retired positions.

## 3. Implementation design and order

1. Separate TX combat ownership/profile from formation state. Add explicit NONE/CASTORIA_SUMMON/KOANKO_SOLO encounter mode and a side/party owner identity; retain summon stage for creation/ambush/slot policy only. Register solo owner at conjured party creation/entry without setting summon stage, claiming slot six, changing topology or scheduling ambush. Species alone never grants encounter features.
2. Make Wish capability, owner lookup and Reciprocity target use registered identity. Castoria profile allows the original five outcomes; solo profile replaces Restoration with Reduction. Extend the enum and use an explicit profile outcome list rather than assuming contiguous ranges. Keep ordinary move ties and outcome commitment unchanged until stage 05 scoring.
3. Change only summoned creation's kit to Follow Me/Wide Guard/Life Dew/Power Gem. Preserve creation level/IV/nature/item defaults and Endurance ambush; do not retune Koanko's existing Steelium Z/EVs/nature as part of this stage. Solo kit is already Wish/Doom Desire/Protect/empty.
4. Add an idempotent Reduction activation API accepting initial loss and affected side; retain `ReductionActivate()` as the 10+ player-side compatibility wrapper. New solo Wish requests start=1, player side, grace=true, only when inactive. Never call the clearing initializer on recast. The shared global counter and occupant logic remain authoritative after TX faints and through later bosses.
5. Successful Reduction Wish dispatches a new TX success script and calls existing Wishful Thinking once. Active Reduction is ineligible and cannot reset/stack or grant a free success activation. Original Castoria/phase calls retain 10+ behavior.
6. For summoned TX record canonical original maxHP once and its deterioration phase in battle-only owner data. Configure `onBreakScript` for its two **original** bars: set actual maxHP to max(1, floor(original * 60/100)) or max(1, floor(original * 30/100)), synchronize the controller, then call the shared refill script. Leave existing Wish/Reduction+Doom Desire afterBreak scripts in place. Saved layers must not shrink or replay original phases.
7. Add an explicit battle stat overlay/normalization for this owned maxHP so refreshes restore the reduced maxHP without modifying canonical party maxHP. Reuse the normalization approach of Reduction, not its player curse state. Preserve canonical HP fractions at exit and reject dead/pending owners. Apply/rebuild in the stage 02 clean-stat order; non-HP growth is untouched. No duplicate refill engine.
8. On genuine TX faint clear local Wish/Reciprocity/cooldown state, retire only a summoned position, and leave Reduction and queued Doom Desire intact. Delayed Wish stays position-based; a retired summoned slot does not redirect healing to Castoria. Solo faint allows the next ordered boss normally.

Proposed files: TX header/source/Wishes; virtual accessors in `battle.h`; Arcanum usability in `castoria_name_theft.c`; Reduction header/source; controller/stat reload normalization; existing battle scripts and TX/Reduction tests. Update `docs/mechanics/tx_rollback.md` after implementation because it still describes prototype species/kit/scoring.

## 4. Dependencies

Stages 01–02; existing Break Bar/saved-layer scripts, Reduction and controller normalization. Stage 05 extends virtual slots/scoring on this combat ownership model.

## 5. Edge cases and invariants

Summon-only trapping, revival ban, sixth-slot ownership, ambush and dynamic doubles must never leak into solo mode. Original Castoria 10+ curse is unchanged; counter advances once, not once per living player or scripted action. Replacements begin with zero accumulated loss; Level 1, HP fraction, healing items and Break Charm saved HP remain correct. No restore callback clears the global curse after TX's death. A revived creature starts as a fresh occupant under the existing curse.

For deterioration, derive both 60% and 30% from original maxHP, not compounded percentages. Change maxHP before refill, then run the existing after-break action exactly once. Failed phase Wish produces no Wishful Thinking; delayed attacks survive source faint according to existing mechanics.

## 6. Test strategy

Implementation commands, separately: `wsl make check TESTS='TX Wishes'`, `TESTS='Arcanum Wish More Wishes'`, `TESTS='Arcanum Wish Reduction'`, `TESTS='Wishful Thinking'`, `TESTS='Break Bar'`. New solo cases use `Koanko TX` and `Koanko Reduction` prefixes.

Add side-by-side profile eligibility, opposite-slot Reciprocity, no solo summon/bar/ambush/revival leakage; new outcome success/failure/duplicate trigger; loss 1/2/3 after grace, persistence beyond TX through all later bosses, switch reset versus global acceleration and inactive/recast handling. Test 100/60/30 actual maxHP at exact refill timing, rounding/minimum, refresh/menu/controller reload, full visible new bars, Wishful growth preserved, saved layers bypass deterioration, delayed Wish to retired slot unused, Doom Desire after TX faint and unchanged canonical party maxHP.

## 7. Completion criteria

Both encounters share the existing effects while selecting the intended different outcome lists. Castoria retains its original lifecycle with reduced phase maxima and unchanged 10+ Reduction. Solo TX has no summon side effects; its 1+ Reduction persists for the rest of the battle. No stat/HP changes leak into party data, no extra phase engine exists, and focused existing tests remain green.

## 8. Open questions

- **Confirmed:** solo TX has zero configured Break Bars; summoned TX has two. Existing configured boss levels and TX base stats stay unchanged.
- **Recommended default:** side-oriented Reduction affects all active occupants on the player's side, including the independent Zacian added later, while bench members remain canonical; stage 07 must test this explicitly.
- No new product decision blocks the combat-profile split. Overlay implementation must be reviewed with stage 02 before merging because permanent-party writes are a concrete risk.
