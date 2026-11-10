# Alice II and Razeluxe: proposed trainer-scoped tactical AI

**Proposal only — 2026-11-10. Nothing below is installed in game source.**

Reviewed the working tree based on `50ebce4990a90258157cd1b01ddd083ee736926e`, including its existing uncommitted AI, party, Polymorph and Koanko work. This is not a proposal against a clean HEAD. No build, compilation or tests were run. The C below is intended for the current interfaces and still needs compilation and behavioral validation after a separately authorized implementation.

## Scope and architectural fit

Retain `setdynamicaifunc`, `AI_FLAG_DYNAMIC_FUNC`, the existing callbacks and all trainer flags, parties, items, levels, abilities and gimmick policies. The proposed implementation uses shared **trainer-local** helpers and an individual tactical block for every roster member. It does not add a general AI pass, change switching, choose inventory, execute battle scripts, advance phases, alter move slots or spend resources during scoring.

Reference implementation: `src/battle_koanko_ai.c`, especially `ExpectedDamage`, `SecureKo`, `IncomingDamageBefore`, `SetupValue`, `RecoveryValue` and the individual species blocks; see also `docs/mechanics/koanko_trainer_ai.md`. Adopt its actual-damage, accuracy, ownership, legality and Break Bar distinctions, rather than copying its species policies or merely adding fixed bonuses to Alice's existing rules.

Primary inspected sources:

- `src/data/trainers.party`: `TRAINER_ALICE2` and `TRAINER_RAZELUXE`.
- `src/battle_ai_main.c`: both callbacks, `ChooseArcanumForTarget`, ordinary cache population and `Ai_SetMoveAccuracy`.
- `src/battle_ai_util.c`: damage outcomes, reachable damage, speed and status helpers.
- `src/battle_script_commands.c`: Authority, Kazuradrop transformation, Infinite Growth, Trash & Crush and Revivify execution.
- `src/battle_util.c`: Bug Space activation and entry-turn recording.
- `src/castoria_name_theft.c`: name routes, Arcanum pool, Wish eligibility and effects.
- `src/battle_tx_rollback.c`, `src/battle_tx_wishes.c`: formation, actual summoned kit, ownership, reusable Wishes and revival exclusions.
- `data/maps/AliceRoom/scripts.inc`, `data/maps/Test/scripts.inc`: encounter callback registration.

### What the current policies miss

Alice's six blocks are rudimentary. Setup relies on coarse one-/two-hit checks and fixed stage thresholds, not actual attacking payoff or the cost of surrendering a turn. Umbreon can reward redundant Wish, repetitive Protect and ineffective Toxic, while discouraging productive Foul Play. Rest does not budget its sleep turns. Palkia does not compare the actual value of its attacks coherently. Hoopa mostly repeats Espeon's setup heuristic. Kazuradrop has an intentional sequence, but little scoring around survival, secondary-effect eligibility or its replacement attack.

Razeluxe has no individual regular-move policy for Kirlia, Hoopa-Unbound, Mega Alakazam or Castoria. Mewtwo's normal-species-only dispatch can miss its Mega form; its Authority check omits generation-dependent Illuminate. Castoria's production Wish picker currently nominates only Domination. Summoned TX's virtual Wishes currently score zero in production. The last two are **deliberate proposed tactical expansions**, using already implemented effects and existing route eligibility, not claims about current production behavior.

## Intended tactics for every Pokémon

### Alice: Espeon

Compare Psychic, Shadow Ball and Dazzling Gleam by accuracy-weighted damage clipped to the current layer. Prefer a reliable, executable finish over Calm Mind. Setup requires a usable special attack, room to gain relevant stats, no Contrary, a survivable setup turn and enough HP for a conservative two-attack exposure. Reward a boost that can change the next attack's KO threshold and the defensive value of Calm Mind against special pressure. Limit investment at +2 SpA unless relevant defensive payoff remains. Magic Bounce remains engine-owned; it does not make Espeon immune to damaging attacks. Life Orb recoil and already-incurred chip are not ignored: thresholds use current HP, with an extra setup margin for Life Orb.

### Alice: Umbreon

Wish is a two-turn plan. Reject a duplicate pending Wish and Heal Block; favor low-HP self-sustain only if Umbreon can survive the casting turn. A wounded reserve contributes only when order changes are allowed, and is explicitly an opportunity estimate, not an invented switch action. Protect prioritizes a Wish due this turn, avoids repeated use, and can buy poison/burn/Leech Seed recovery turns. It loses value against setup-heavy foes when no recovery/residual objective exists. Toxic requires real poison eligibility and a useful residual payoff; Poison Heal and Magic Guard are not worthwhile targets. Foul Play is a real damage option, using its actual cached damage rather than a flat penalty or Umbreon's own Attack.

### Alice: Gothitelle

Shadow Tag does not justify arbitrary Rest. Rest must act before lethal damage, restore meaningful HP or cure an ailment, and leave enough full HP for a conservative sleep budget. Avoid sleeping in a hopeless damage race. Psychic, Thunderbolt and Shadow Ball use actual damage; a target actually trapped under the engine's current escape rules adds a small sustain payoff, not a fictitious guarantee that Ghosts or escape items are trapped. No new sleep move, trap rule or switching plan is added.

### Alice: Palkia

All four attacks compete on actual expected damage and reliability. Give genuine immediate KO outcomes priority, distinguish breaking a layer from ending a battle, and penalize Draco Meteor's future SpA loss only when another usable immediate attack comes within 85% of its expected damage. Do not impose that penalty when Contrary reverses the drop. Thunder and Hydro Pump inherit actual weather/accuracy/effectiveness caches; no fixed rain or super-effective preference overrides superior damage.

### Alice: Hoopa

Nasty Plot requires a safe window and a real special attacking payoff; it is not the default action merely because Hoopa survives one hit. Hyperspace Hole, Psychic and Thunderbolt compete on actual damage. Prefer Hyperspace Hole when the target has demonstrated Protect and its damage remains competitive, without reading the target's selected action. The ordinary engine retains its immunity, protection-bypass and accuracy behavior.

### Alice: Kazuradrop

Preserve the first two entry-relative Quiver Dance turns, the pre-transformation emergency Quiver Dance slot choice, the OHKO-tier Stomp override, first Infinite Growth, and fresh Melt Virus priority in the established tier range. Respect unusable slots and bad-move vetoes around those priorities. Fix the *proposed AI's* move-swap test to use the actor's bit. Outside mandatory sequence steps, Quiver Dance uses the same survival/payoff analysis as the other setup users. After transformation, inspect the actual slot-2 replacement, credit eligible Melt Virus and Time Stop effects, value Giga Drain only when healing is useful, and distinguish a guaranteed real finish from a layer break. Do not model Trash & Crush's 20%/40% proc as a guaranteed KO, nor treat its 85% accuracy as certain even in its OHKO tier. Mandatory encounter beats intentionally precede generic finish preference.

### Razeluxe: The Face — Raze Kirlia

Psychic and Dazzling Gleam are competitive damage choices. Burn targets with meaningful physical pressure unless they benefit from burn; paralysis earns additional value when it changes this actor's or a living reserve's speed matchup outside Trick Room. Avoid redundant status, reflected status and Substitute. Under pressure, use the disruption only if it can resolve before the dangerous hit. Psychium Z activation remains owned by existing gimmick logic; no callback sets it.

### Razeluxe: The Sadist — Mewtwo / Mega Mewtwo

Authority mirrors **all** the execution-time failure protections, including configured Illuminate, and rejects either side's Contrary. Its value scales with remaining useful stat swing, giving it priority over ordinary offense when it can establish dominance. A real immediate finish still beats unnecessary setup. Spacial Rend is the reliable immediate baseline. Roar of Time loses merit when its recharge can be avoided for comparable damage; Shadow Force receives a tempo penalty when Spacial Rend is competitive. Neither is falsely classified as an immediate KO. Recognize both Mega forms without changing Mega selection.

### Razeluxe: The Ego — Hoopa-Unbound

Use the actual mixed attacking kit, not a generic special-sweeper policy. Hyperspace Fury, Psychic, Drain Punch and Gunk Shot compete on accuracy-weighted damage. Drain Punch gains value for useful, timely healing, but not against Liquid Ooze or Heal Block. Hyperspace Fury's defensive cost matters when comparable coverage avoids it, unless Contrary helps. Known Protect usage can favor its bypass when damage is competitive. Tera Divine and Magician remain existing engine decisions.

### Razeluxe: The Brains — Mega Alakazam

Psychic, Dazzling Gleam and Shadow Ball use real coverage. Calm Mind needs an actual safe investment window. Focus Sash is not a license to set up into an unavoidable follow-up KO: conservatively budget raw incoming damage rather than pretending Sash supplies two safe turns. Read the current traced ability, including Contrary/Unaware interactions; do not presume Trace always gives a useful defensive ability.

### Razeluxe: CASTER — Castoria

Earth Power and Moonblast compete on damage; Calm Mind uses the same threat/payoff model. Regular Revivify requires an eligible fainted ally and time to resolve, rewards adding a full-HP reserve, and loses to an executable finish. Preserve emergency Arcanum Revivify under Perish Song and the existing Perish Song countermeasure through Polymorph. Extend Wish selection locally: replenish exhausted true-name charges; summon More Wishes only when the existing fake-name-route API permits it; use Endurance defensively, Reduction on the backed-out route, and Debilitation against a still-active dangerous opponent. Do not opportunistically spend the shared allowance on Normality: it remains eligible engine behavior but is not nominated by this conservative policy because changing a target's type/form requires a stronger counterfactual damage model. Fireball receives a separately simulated comparison rather than incorrectly indexing the four normal-move cache entries.

### Razeluxe: summoned TX Rollback

Its **current** creation kit is Psychic, Swift, Protect and Celebrate, not Koanko's Wish/Doom Desire kit. Score its real attacks, reject aimless Celebrate, and use Protect for an actual residual/future-hit objective. A separate virtual Wish policy can restore a depleted original ally layer, reverse dangerous enemy boosts, steal existing benefits, establish Reciprocity or start Calamity. All candidates call `TxRollbackCanWishFor`; scoring never restores a layer or applies an effect. Ownership uses the summoned party identity even if its species changes. The current arrival ambush, automatic bar scripts, phase Wish and Doom Desire remain automatic script behavior, not extra chosen actions.

## Copy-and-paste proposal

All blocks below belong together. They are intended for `src/battle_ai_main.c`, where the existing static `Ai_SetMoveAccuracy` is available. Do **not** paste them into a separate translation unit without moving that accuracy interface deliberately. Existing engine functions called below are dependencies, not omitted custom helpers. Every new helper is fully defined here; there are no pseudocode branches.

Add the following explicit includes alongside existing includes if absent:

```c
#include "battle_util.h"
#include "battle_tx_rollback.h"
#include "battle_break_bar.h"
```

Insert this complete helper block immediately before the current `AI_AliceTrainer` definition. `AR_` names are trainer-local proposal helpers, unrelated to Arcanum slot numbering.

```c
static bool32 AR_OwnsEncounter(u32 trainer, u32 a, u32 d)
{
    return gBattleStruct != NULL && gAiLogicData != NULL
        && (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER
                              | BATTLE_TYPE_TWO_OPPONENTS))
        && TRAINER_BATTLE_PARAM.opponentA == trainer
        && a < gBattlersCount && d < gBattlersCount
        && !IsOnPlayerSide(a) && IsBattlerAlive(a) && IsBattlerAlive(d)
        && !IsBattlerAlly(a, d);
}

static bool32 AR_Usable(u32 a, u32 slot)
{
    return slot < MAX_MON_MOVES && gBattleMons[a].moves[slot] != MOVE_NONE
        && !(gAiLogicData->moveLimitations[a] & (1u << slot));
}

static bool32 AR_Immediate(u32 a, u32 move)
{
    enum BattleMoveEffects effect = GetMoveEffect(move);
    return move != MOVE_NONE && !IsBattleMoveStatus(move)
        && move != MOVE_DMS_PREROGATIVE
        && !IsTwoTurnNotSemiInvulnerableMove(a, move)
        && effect != EFFECT_SEMI_INVULNERABLE && effect != EFFECT_FUTURE_SIGHT
        && effect != EFFECT_COUNTER && effect != EFFECT_MIRROR_COAT
        && effect != EFFECT_METAL_BURST && effect != EFFECT_FOCUS_PUNCH;
}

static u32 AR_Expected(u32 a, u32 d, u32 slot)
{
    if (!AR_Usable(a, slot) || IsBattleMoveStatus(gBattleMons[a].moves[slot])
     || gBattleMons[a].moves[slot] == MOVE_DMS_PREROGATIVE)
        return 0;
    return AI_GetReachableDamage(d, gAiLogicData->simulatedDmg[a][d][slot].median)
        * min(100, gAiLogicData->moveAccuracy[a][d][slot]) / 100;
}

static u32 AR_Best(u32 a, u32 d)
{
    u32 best = 0;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (AR_Usable(a, i) && AR_Immediate(a, gBattleMons[a].moves[i]))
            best = max(best, AR_Expected(a, d, i));
    return best;
}

/* Raw maximum roll, not accuracy-discounted: a miss is not a survival plan.
 * One strongest usable immediate attack per living opponent. This reads known
 * kits/caches, never the player's selected command. */
static u32 AR_Pressure(u32 a, u32 candidate, bool32 beforeOnly,
                      enum DamageCategory category)
{
    u32 total = 0;
    for (u32 foe = 0; foe < gBattlersCount; foe++)
    {
        if (!IsBattlerAlive(foe) || IsBattlerAlly(a, foe))
            continue;
        u32 best = 0;
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
        {
            u32 m = gBattleMons[foe].moves[i];
            if (!AR_Usable(foe, i) || !AR_Immediate(foe, m)
             || (category != DAMAGE_CATEGORY_STATUS && GetMoveCategory(m) != category))
                continue;
            /* A tie is unsafe for budgeting. AI_IsFaster is strict. */
            if (beforeOnly && AI_IsFaster(a, foe, candidate, m, CONSIDER_PRIORITY))
                continue;
            best = max(best, max(0, gAiLogicData->simulatedDmg[foe][a][i].maximum));
        }
        total += best;
    }
    return total;
}

static u32 AR_Incoming(u32 a)
{
    /* STATUS means all attack categories for this helper. */
    return AR_Pressure(a, MOVE_NONE, FALSE, DAMAGE_CATEGORY_STATUS);
}

static bool32 AR_Ko(u32 a, u32 d, u32 slot)
{
    return AR_Usable(a, slot) && AR_Immediate(a, gBattleMons[a].moves[slot])
        && gAiLogicData->moveAccuracy[a][d][slot] >= 90
        && AI_GetDamageOutcome(a, d, gBattleMons[a].moves[slot],
            gAiLogicData->simulatedDmg[a][d][slot].minimum) == AI_DAMAGE_KO;
}

static bool32 AR_Finish(u32 a, u32 d)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (AR_Ko(a, d, i)
         && AR_Pressure(a, gBattleMons[a].moves[i], TRUE, DAMAGE_CATEGORY_STATUS)
            < gBattleMons[a].hp)
            return TRUE;
    return FALSE;
}

static bool32 AR_Comparable(u32 a, u32 d, u32 slot)
{
    u32 current = AR_Expected(a, d, slot);
    if (current == 0)
        return FALSE;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 m = gBattleMons[a].moves[i];
        if (i != slot && AR_Usable(a, i) && AR_Immediate(a, m)
         && !MoveHasAdditionalEffectSelf(m, MOVE_EFFECT_RECHARGE)
         && !MoveHasAdditionalEffectSelf(m, MOVE_EFFECT_SP_ATK_MINUS_2)
         && AR_Expected(a, d, i) * 100 >= current * 85)
            return TRUE;
    }
    return FALSE;
}

static s32 AR_AttackDelta(u32 a, u32 d, u32 slot, s32 score)
{
    u32 expected = AR_Expected(a, d, slot);
    if (expected == 0 || score < AI_SCORE_DEFAULT)
        return 0; /* Do not undo ordinary immunity/failure penalties. */
    s32 value = expected >= AR_Best(a, d) ? 3 : 0;
    if (AR_Ko(a, d, slot))
        value += 8;
    else if (AR_Immediate(a, gBattleMons[a].moves[slot])
          && gAiLogicData->moveAccuracy[a][d][slot] >= 90)
    {
        enum AiDamageOutcome result = AI_GetDamageOutcome(a, d,
            gBattleMons[a].moves[slot], gAiLogicData->simulatedDmg[a][d][slot].minimum);
        if (result == AI_DAMAGE_BREAK_LAYER || result == AI_DAMAGE_REVEAL_SAVED_LAYER)
            value += 3; /* Progress, not victory; next layer/phase still exists. */
    }
    return value;
}

static bool32 AR_SpeedPayoff(u32 a, u32 d, u32 numerator, u32 denominator)
{
    if (gFieldStatuses & STATUS_FIELD_TRICK_ROOM)
        return FALSE;
    u32 speed = gAiLogicData->speedStats[d];
    u32 ownSpeed = gAiLogicData->speedStats[a];
    if (ownSpeed <= speed && ownSpeed * numerator > speed * denominator)
        return TRUE;
    struct Pokemon *party = GetBattlerParty(a);
    if (HasNoOrderChange(a))
        return FALSE;
    for (u32 i = 0; i < PARTY_SIZE; i++)
        if (i != gBattlerPartyIndexes[a] && GetMonData(&party[i], MON_DATA_HP) != 0
         && GetMonData(&party[i], MON_DATA_SPEED) <= speed
         && GetMonData(&party[i], MON_DATA_SPEED) * numerator > speed * denominator)
            return TRUE; /* Opportunity estimate; not a switch prediction. */
    return FALSE;
}

static s32 AR_Setup(u32 a, u32 d, u32 move, u32 boost)
{
    u32 hp = gBattleMons[a].hp;
    u32 incoming = AR_Incoming(a);
    u32 best = 0;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (AR_Usable(a, i) && AR_Immediate(a, gBattleMons[a].moves[i])
         && GetMoveCategory(gBattleMons[a].moves[i]) == DAMAGE_CATEGORY_SPECIAL)
            best = max(best, AR_Expected(a, d, i));
    u32 stage = gBattleMons[a].statStages[STAT_SPATK];
    bool32 specialFoe = AR_Pressure(a, move, FALSE, DAMAGE_CATEGORY_SPECIAL) != 0;
    bool32 defensePayoff = move != MOVE_NASTY_PLOT && specialFoe
        && gBattleMons[a].statStages[STAT_SPDEF] < DEFAULT_STAT_STAGE + 2;
    if (gAiLogicData->abilities[a] == ABILITY_CONTRARY || best == 0
     || (stage >= DEFAULT_STAT_STAGE + 2 && !defensePayoff)
     || (stage == MAX_STAT_STAGE && !defensePayoff)
     || AR_Finish(a, d) || IsBreakBarPending(a))
        return -12;
    u32 margin = gAiLogicData->holdEffects[a] == HOLD_EFFECT_LIFE_ORB
        ? max(1, gBattleMons[a].maxHP / 10) : 0;
    if (incoming + margin >= hp || incoming * 2 + margin >= hp)
        return -10; /* Do not assume a current extra layer or Sash is free setup. */
    s32 value = 6;
    /* Estimated offensive payoff only, never labelled a proven boosted KO. */
    u32 numerator = stage >= DEFAULT_STAT_STAGE ? stage - DEFAULT_STAT_STAGE + 2 : 2;
    if (stage >= DEFAULT_STAT_STAGE && gAiLogicData->abilities[d] != ABILITY_UNAWARE
     && best < gBattleMons[d].hp
     && best * (numerator + boost) / numerator >= gBattleMons[d].hp)
        value += 3;
    if (defensePayoff && gAiLogicData->abilities[d] != ABILITY_UNAWARE)
        value += 2;
    if (move == MOVE_QUIVER_DANCE && AR_SpeedPayoff(a, d, 3, 2))
        value += 2;
    return value;
}

static bool32 AR_Residual(u32 d)
{
    return (gBattleMons[d].status1 & (STATUS1_POISON | STATUS1_TOXIC_POISON | STATUS1_BURN))
        && gAiLogicData->abilities[d] != ABILITY_MAGIC_GUARD
        && gAiLogicData->abilities[d] != ABILITY_POISON_HEAL;
}

static bool32 AR_WishRecipient(u32 a)
{
    if (gBattleMons[a].hp * 100 < gBattleMons[a].maxHP * 70)
        return TRUE;
    if (HasNoOrderChange(a))
        return FALSE;
    struct Pokemon *party = GetBattlerParty(a);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        u32 hp = GetMonData(&party[i], MON_DATA_HP);
        u32 maxHp = GetMonData(&party[i], MON_DATA_MAX_HP);
        if (i != gBattlerPartyIndexes[a] && hp != 0 && maxHp != 0 && hp * 100 < maxHp * 65)
            return TRUE;
    }
    return FALSE;
}

static s32 AR_Wish(u32 a)
{
    if (gWishFutureKnock.wishCounter[a] || gBattleMons[a].volatiles.healBlock
     || IsBreakBarPending(a))
        return -12;
    u32 incoming = AR_Incoming(a);
    if (incoming >= gBattleMons[a].hp)
        return -12;
    if (!AR_WishRecipient(a))
        return -6;
    /* Protect can bridge the following turn, not the casting turn itself. */
    return gBattleMons[a].hp * 100 <= gBattleMons[a].maxHP * 60 ? 10 : 5;
}

static s32 AR_Protect(u32 a, u32 d)
{
    if (gDisableStructs[a].protectUses)
        return -12;
    if (gWishFutureKnock.wishCounter[a] == 1
     && gBattleMons[a].hp < gBattleMons[a].maxHP && !gBattleMons[a].volatiles.healBlock)
        return 12;
    if (AR_Incoming(a) == 0)
        return -6;
    if (AR_Residual(d) || gBattleMons[d].volatiles.leechSeed)
        return 4;
    if (gWishFutureKnock.futureSightCounter[d] == 1)
        return 3;
    return HasMoveWithCategory(d, DAMAGE_CATEGORY_STATUS) ? -8 : -4;
}

static s32 AR_Rest(u32 a, u32 d)
{
    u32 hp = gBattleMons[a].hp, maximum = gBattleMons[a].maxHP;
    if (gBattleMons[a].volatiles.healBlock || (gBattleMons[a].status1 & STATUS1_SLEEP)
     || IsBreakBarPending(a) || AR_Finish(a, d))
        return -12;
    u32 incoming = AR_Incoming(a);
    if (AR_Pressure(a, MOVE_REST, TRUE, DAMAGE_CATEGORY_STATUS) >= hp
     || incoming * 3 >= maximum)
        return -12;
    bool32 cure = (gBattleMons[a].status1 & STATUS1_ANY) != 0;
    if (hp * 100 > maximum * 70 && !cure)
        return -8;
    s32 value = hp * 100 <= maximum * 50 ? 10 : cure ? 6 : 2;
    if (IsBattlerTrapped(a, d))
        value += 2;
    return value;
}

static bool32 AR_StatusTarget(u32 a, u32 d, u32 move)
{
    return gBattleMons[d].status1 == 0
        && gAiLogicData->abilities[d] != ABILITY_MAGIC_BOUNCE
        && gAiLogicData->abilities[d] != ABILITY_GOOD_AS_GOLD
        && !DoesSubstituteBlockMove(a, d, move);
}

static bool32 AR_Secondary(u32 a, u32 d, u32 move)
{
    return !DoesSubstituteBlockMove(a, d, move)
        && !IsAdditionalEffectBlocked(a, gAiLogicData->abilities[a], d, gAiLogicData->abilities[d])
        && gAiLogicData->abilities[a] != ABILITY_SHEER_FORCE;
}

static s32 AR_Drain(u32 a, u32 d, u32 slot)
{
    if (gBattleMons[a].volatiles.healBlock || gAiLogicData->abilities[d] == ABILITY_LIQUID_OOZE)
        return 0;
    u32 hp = gBattleMons[a].hp, maximum = gBattleMons[a].maxHP;
    u32 heal = min(maximum - hp, AR_Expected(a, d, slot) / 2);
    if (heal == 0 || AR_Pressure(a, gBattleMons[a].moves[slot], TRUE, DAMAGE_CATEGORY_STATUS) >= hp)
        return 0;
    if (AR_Incoming(a) >= hp && AR_Incoming(a) < hp + heal)
        return 7;
    return heal * 100 >= maximum * 15 ? 3 : 1;
}

static s32 AR_Kazuradrop(u32 a, u32 d, u32 move, s32 score, u32 slot)
{
    u32 tier = gBattleStruct->bugSpace.currentTier;
    bool32 swapped = (gBattleStruct->bugSpace.moveSwapActive & (1u << a)) != 0;
    s32 delta = 0;
    if (!swapped)
    {
        /* Retain the intentional opener and late-tier Stomp override. */
        if (gBattleResults.battleTurnCounter - VarGet(VAR_TEMP_0) <= 1)
            delta += move == MOVE_QUIVER_DANCE ? 20 : -20;
        if (tier >= BUGSPACE_TIER_OHKO)
            delta += move == MOVE_STOMP ? 100 : -100;
        if (AR_Incoming(a) >= gBattleMons[a].hp && move == MOVE_QUIVER_DANCE)
            delta += 30; /* Preserve the existing transformation-slot contingency. */
        else if (move == MOVE_QUIVER_DANCE
              && gBattleResults.battleTurnCounter - VarGet(VAR_TEMP_0) > 1
              && tier < BUGSPACE_TIER_OHKO && score >= AI_SCORE_DEFAULT)
            delta += AR_Setup(a, d, move, 1);
        if (move != MOVE_QUIVER_DANCE && tier < BUGSPACE_TIER_OHKO)
            delta += AR_AttackDelta(a, d, slot, score);
    }
    else if (!gBattleMons[a].volatiles.infiniteGrowth)
        delta = move == MOVE_INFINITE_GROWTH ? 50 : -20;
    else if (!gBattleMons[d].volatiles.meltVirus
          && gAiLogicData->abilities[d] != ABILITY_MAGIC_GUARD
          && tier < BUGSPACE_TIER_OHKO && move == MOVE_MELT_VIRUS
          && score >= AI_SCORE_DEFAULT && AR_Expected(a, d, slot) != 0
          && AR_Secondary(a, d, move))
        delta = 50;
    else
    {
        delta += AR_AttackDelta(a, d, slot, score);
        if (score >= AI_SCORE_DEFAULT && AR_Expected(a, d, slot) != 0)
        {
            if (move == MOVE_TRASH_CRUSH && tier >= BUGSPACE_TIER_MINIMIZE)
                delta += 15;
            if (move == MOVE_CRACK_ICE && !gBattleMons[d].volatiles.timestop
             && !(gBattleMons[d].status1 & STATUS1_INCAPACITATED)
             && tier < BUGSPACE_TIER_MINIMIZE && AR_Secondary(a, d, move))
                delta += 4; /* 33% disruption, not guaranteed safety. */
            if (slot == 2 && move != MOVE_INFINITE_GROWTH)
            {
                delta += 3; /* Actual generated attack, not a presumed special move. */
                if (move == MOVE_GIGA_DRAIN)
                    delta += AR_Drain(a, d, slot);
                if (move == MOVE_SPIRIT_BREAK && AR_Secondary(a, d, move)
                 && HasMoveWithCategory(d, DAMAGE_CATEGORY_SPECIAL)
                 && gAiLogicData->abilities[d] != ABILITY_CONTRARY
                 && CanLowerStat(a, d, gAiLogicData, STAT_SPATK))
                    delta += 2;
            }
        }
    }
    /* ADJUST_SCORE's tracing macro requires a local named battlerAtk. */
    u32 battlerAtk = a;
    ADJUST_SCORE(delta);
    return score;
}

static s32 AR_AliceMoves(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    u32 a = battlerAtk, d = battlerDef, slot = gAiThinkingStruct->movesetIndex;
    if (!AR_Usable(a, slot) || gBattleMons[a].moves[slot] != move || score <= 0)
        return score;
    if (gBattleMons[a].species == SPECIES_KAZURADROP)
        return AR_Kazuradrop(a, d, move, score, slot);
    s32 delta = AR_AttackDelta(a, d, slot, score);
    bool32 useful = score >= AI_SCORE_DEFAULT;
    switch (gBattleMons[a].species)
    {
    case SPECIES_ESPEON:
        if (move == MOVE_CALM_MIND && useful)
            delta += AR_Setup(a, d, move, 1);
        break;
    case SPECIES_UMBREON:
        if (move == MOVE_WISH && useful)
            delta += AR_Wish(a);
        if (move == MOVE_PROTECT && useful)
            delta += AR_Protect(a, d);
        if (move == MOVE_TOXIC && useful)
        {
            if (!AR_StatusTarget(a, d, move)
             || !AI_CanPoison(a, d, gAiLogicData->abilities[d], move, MOVE_NONE)
             || gAiLogicData->abilities[d] == ABILITY_MAGIC_GUARD
             || gAiLogicData->abilities[d] == ABILITY_POISON_HEAL)
                delta -= 12;
            else if (AR_Pressure(a, move, TRUE, DAMAGE_CATEGORY_STATUS) >= gBattleMons[a].hp)
                delta -= 10;
            else
                delta += AR_Incoming(a) * 2 < gBattleMons[a].hp ? 9 : 2;
        }
        break;
    case SPECIES_GOTHITELLE:
        if (move == MOVE_REST && useful)
            delta += AR_Rest(a, d);
        break;
    case SPECIES_PALKIA:
        if (move == MOVE_DRACO_METEOR && !AR_Ko(a, d, slot)
         && gAiLogicData->abilities[a] != ABILITY_CONTRARY && AR_Comparable(a, d, slot))
            delta -= 4;
        break;
    case SPECIES_HOOPA:
        if (move == MOVE_NASTY_PLOT && useful)
            delta += AR_Setup(a, d, move, 2);
        if (move == MOVE_HYPERSPACE_HOLE && useful && gDisableStructs[d].protectUses
         && AR_Expected(a, d, slot) != 0
         && AR_Expected(a, d, slot) * 100 >= AR_Best(a, d) * 85)
            delta += 3;
        break;
    default:
        return score;
    }
    if (IsBattleMoveStatus(move) && AR_Finish(a, d))
        delta -= 18;
    ADJUST_SCORE(delta);
    return score;
}

static bool32 AR_AuthorityBlocked(u32 a, u32 d)
{
    enum Ability ability = gAiLogicData->abilities[d];
    if (gAiLogicData->abilities[a] == ABILITY_CONTRARY || ability == ABILITY_CONTRARY
     || ability == ABILITY_CLEAR_BODY || ability == ABILITY_WHITE_SMOKE
     || ability == ABILITY_FULL_METAL_BODY || ability == ABILITY_HYPER_CUTTER
     || ability == ABILITY_BIG_PECKS || ability == ABILITY_KEEN_EYE
     || ability == ABILITY_MINDS_EYE
     || (GetConfig(CONFIG_ILLUMINATE_EFFECT) >= GEN_9 && ability == ABILITY_ILLUMINATE)
     || IsFlowerVeilProtected(d) || gAiLogicData->holdEffects[d] == HOLD_EFFECT_CLEAR_AMULET
     || ((gSideStatuses[GetBattlerSide(d)] & SIDE_STATUS_MIST)
      && gAiLogicData->abilities[a] != ABILITY_INFILTRATOR))
        return TRUE;
    for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
        if (gBattleMons[d].statStages[stat] > MIN_STAT_STAGE)
            return FALSE;
    return TRUE;
}

static s32 AR_RazeMoves(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    u32 a = battlerAtk, d = battlerDef, slot = gAiThinkingStruct->movesetIndex;
    if (!AR_Usable(a, slot) || gBattleMons[a].moves[slot] != move || score <= 0)
        return score;
    bool32 useful = score >= AI_SCORE_DEFAULT;
    bool32 finish = AR_Finish(a, d);
    s32 delta = AR_AttackDelta(a, d, slot, score);
    u32 species = GET_BASE_SPECIES_ID(gBattleMons[a].species);
    if (TxRollbackHasWishes(a))
    {
        if (move == MOVE_PROTECT && useful)
            delta += AR_Protect(a, d);
        if (move == MOVE_CELEBRATE)
            delta -= 20;
    }
    else switch (species)
    {
    case SPECIES_RAZE_KIRLIA:
        if ((move == MOVE_WILL_O_WISP || move == MOVE_THUNDER_WAVE) && useful)
        {
            if (!AR_StatusTarget(a, d, move))
                delta -= 12;
            else if (AR_Pressure(a, move, TRUE, DAMAGE_CATEGORY_STATUS) >= gBattleMons[a].hp)
                delta -= 8;
            else if (move == MOVE_WILL_O_WISP)
            {
                enum Ability ability = gAiLogicData->abilities[d];
                if (!AI_CanBurn(a, d, ability, BATTLE_PARTNER(a), move, MOVE_NONE)
                 || ability == ABILITY_GUTS || ability == ABILITY_FLARE_BOOST
                 || ability == ABILITY_MAGIC_GUARD)
                    delta -= 12;
                else
                    delta += AR_Pressure(a, move, FALSE, DAMAGE_CATEGORY_PHYSICAL) != 0 ? 9 : 1;
            }
            else if (gAiLogicData->abilities[d] == ABILITY_QUICK_FEET)
                delta -= 6;
            else if (AI_CanParalyze(a, d, gAiLogicData->abilities[d], move, MOVE_NONE))
                delta += AR_SpeedPayoff(a, d, B_PARALYSIS_SPEED >= GEN_7 ? 2 : 4, 1) ? 8 : 2;
            else
                delta -= 12;
        }
        break;
    case SPECIES_MEWTWO:
        if (move == MOVE_DMS_PREROGATIVE)
        {
            if (AR_AuthorityBlocked(a, d))
                delta -= 40;
            else if (useful)
            {
                u32 swing = 0;
                const u8 stats[] = {STAT_SPATK, STAT_DEF, STAT_SPDEF, STAT_SPEED};
                for (u32 i = 0; i < ARRAY_COUNT(stats); i++)
                    swing += MAX_STAT_STAGE - gBattleMons[a].statStages[stats[i]]
                           + gBattleMons[d].statStages[stats[i]] - MIN_STAT_STAGE;
                delta += finish ? -18 : min(30, 6 + swing / 2);
            }
        }
        if (move == MOVE_ROAR_OF_TIME && !AR_Ko(a, d, slot) && AR_Comparable(a, d, slot))
            delta -= 5;
        if (move == MOVE_SHADOW_FORCE && AR_Comparable(a, d, slot))
            delta -= 4;
        break;
    case SPECIES_HOOPA: /* GET_BASE_SPECIES_ID also maps Hoopa-Unbound here. */
        if (move == MOVE_DRAIN_PUNCH && useful)
            delta += AR_Drain(a, d, slot);
        if (move == MOVE_HYPERSPACE_FURY && useful)
        {
            if (gAiLogicData->abilities[a] != ABILITY_CONTRARY
             && !AR_Ko(a, d, slot) && AR_Comparable(a, d, slot))
                delta -= 3;
            if (gDisableStructs[d].protectUses && AR_Expected(a, d, slot) != 0
             && AR_Expected(a, d, slot) * 100 >= AR_Best(a, d) * 85)
                delta += 3;
        }
        break;
    case SPECIES_ALAKAZAM:
        if (move == MOVE_CALM_MIND && useful)
            delta += AR_Setup(a, d, move, 1);
        break;
    case SPECIES_CASTORIA:
        if (move == MOVE_CALM_MIND && useful)
            delta += AR_Setup(a, d, move, 1);
        if (move == MOVE_REVIVAL_BLESSING_2 && useful)
        {
            if (GetFirstFaintedPartyIndex(a) == PARTY_SIZE)
                delta -= 20;
            else if (AR_Pressure(a, move, TRUE, DAMAGE_CATEGORY_STATUS) >= gBattleMons[a].hp)
                delta -= 12;
            else
                delta += 10; /* Engine selects/revives an eligible reserve. */
        }
        break;
    default:
        return score;
    }
    if (IsBattleMoveStatus(move) && finish)
        delta -= 18;
    ADJUST_SCORE(delta);
    return score;
}

static s32 AR_TxSpell(u32 a, u32 d, u32 move)
{
    if (move != MOVE_ARCANUM_WISH)
        return 0;
    u32 chosen = TX_WISH_NONE, recipient = MAX_BATTLERS_COUNT;
    s32 best = 0;
    /* Fixed tie order is intentional and deterministic. Restoration first. */
    const u8 wishes[] = {TX_WISH_RESTORATION, TX_WISH_REVERSAL,
        TX_WISH_RECLAMATION, TX_WISH_CALAMITY, TX_WISH_RECIPROCITY};
    for (u32 i = 0; i < ARRAY_COUNT(wishes); i++)
    {
        for (u32 target = 0; target < gBattlersCount; target++)
        {
            if (!TxRollbackCanWishFor(a, wishes[i], target))
                continue;
            s32 value = 0;
            switch (wishes[i])
            {
            case TX_WISH_RESTORATION:
                value = 116 + (AR_Incoming(target) >= gBattleMons[target].hp ? 4 : 0);
                break;
            case TX_WISH_REVERSAL:
            {
                u32 boosts = 0;
                for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
                    if (gBattleMons[d].statStages[stat] > DEFAULT_STAT_STAGE)
                        boosts += gBattleMons[d].statStages[stat] - DEFAULT_STAT_STAGE;
                value = 106 + min(12, boosts * 2);
                break;
            }
            case TX_WISH_RECLAMATION:
                value = 112; /* API proves an actually transferable benefit. */
                break;
            case TX_WISH_CALAMITY:
                value = HasNoOrderChange(d) || IsBattlerTrapped(a, d) ? 114 : 104;
                break;
            case TX_WISH_RECIPROCITY:
                value = HasMoveWithCategory(d, DAMAGE_CATEGORY_STATUS) ? 103 : 101;
                break;
            }
            if (value > best)
            {
                best = value;
                chosen = wishes[i];
                recipient = target;
            }
        }
    }
    if (AR_Finish(a, d))
        best = 0;
    gBattleStruct->txRollback.wishCandidate = best != 0 ? chosen : TX_WISH_NONE;
    gBattleStruct->txRollback.wishCandidateTarget = best != 0 ? recipient : MAX_BATTLERS_COUNT;
    return best;
}

static s32 AR_CastoriaSpell(u32 a, u32 d, u32 move, s32 score)
{
    /* No normal-slot indexing in this function. The caller checked ownership. */
    if (move == MOVE_REVIVAL_BLESSING_2)
    {
        if (GetFirstFaintedPartyIndex(a) == PARTY_SIZE)
            return 0;
        if (gBattleMons[a].volatiles.perishSong)
            return INT32_MAX; /* Preserve the current encounter exception. */
        if (AR_Finish(a, d)
         || AR_Pressure(a, move, TRUE, DAMAGE_CATEGORY_STATUS) >= gBattleMons[a].hp)
            return 0;
        return 110;
    }
    if (move == MOVE_POLYMORPH)
    {
        if (GetMoveSlot(gBattleMons[d].moves, MOVE_PERISH_SONG) < MAX_MON_MOVES)
            return INT32_MAX - 1; /* Preserve existing anti-Perish policy. */
        return 0; /* Do not reroll ordinary opponents without a payoff model. */
    }
    if (move == MOVE_ARCANUM_WISH)
    {
        u32 wish = CASTORIA_WISH_NONE;
        s32 value = 0;
        if (CastoriaCanWishFor(a, CASTORIA_WISH_DOMINATION))
            wish = CASTORIA_WISH_DOMINATION, value = 114;
        else if (CastoriaCanWishFor(a, CASTORIA_WISH_MORE_WISHES))
            wish = CASTORIA_WISH_MORE_WISHES, value = 120;
        else if (CastoriaCanWishFor(a, CASTORIA_WISH_ENDURANCE)
              && AR_Incoming(a) * 2 >= gBattleMons[a].hp)
            wish = CASTORIA_WISH_ENDURANCE, value = 118;
        else if (CastoriaCanWishFor(a, CASTORIA_WISH_REDUCTION)
              && AR_Incoming(a) != 0)
            wish = CASTORIA_WISH_REDUCTION, value = 112;
        else if (CastoriaCanWishFor(a, CASTORIA_WISH_DEBILITATION)
              && !(gBattleMons[d].status1 & STATUS1_INCAPACITATED)
              && AR_Incoming(a) != 0)
            wish = CASTORIA_WISH_DEBILITATION, value = 110;
        if (AR_Finish(a, d))
            wish = CASTORIA_WISH_NONE, value = 0;
        gBattleStruct->castoria.wishCandidate = wish;
        return value;
    }
    if (move == MOVE_FIREBALL)
    {
        /* Virtual spell has no ordinary four-slot cache entry. Use the existing
         * save/restore simulation wrapper, never a temporary installed move. */
        uq4_12_t effectiveness;
        struct SimulatedDamage damage = AI_CalcDamageSaveBattlers(move, a, d,
            &effectiveness, NO_GIMMICK, NO_GIMMICK);
        u32 accuracy = min(100, Ai_SetMoveAccuracy(gAiLogicData, a, d, move));
        u32 expected = AI_GetReachableDamage(d, damage.median) * accuracy / 100;
        if (effectiveness == UQ_4_12(0.0) || expected == 0 || AR_Finish(a, d))
            return 0;
        /* Require a material coverage improvement to spend finite spell uses. */
        return expected * 100 > AR_Best(a, d) * 120 ? score + 8 : 0;
    }
    return 0;
}
```

### Exact insertion into the existing Alice callback

Insert this at the very start of `AI_AliceTrainer`, before its current Arcanum guard. Leave the remaining existing body in place for other Alice encounters. This is a complete interception for `TRAINER_ALICE2`; none of its old per-species bonuses run afterward.

```c
    if (AR_OwnsEncounter(TRAINER_ALICE2, battlerAtk, battlerDef))
    {
        if (BattleAI_IsScoringArcanum())
            return 0;
        return AR_AliceMoves(battlerAtk, battlerDef, move, score);
    }
```

### Exact insertion into the existing Razeluxe callback

Insert this at the very start of `AI_RazeluxeTrainer`, before its current Arcanum branch. Leave the remaining body as the existing fallback. The ordinary helper does not run on virtual moves; the virtual helper never assumes `movesetIndex` is a valid ordinary slot.

```c
    if (AR_OwnsEncounter(TRAINER_RAZELUXE, battlerAtk, battlerDef))
    {
        if (BattleAI_IsScoringArcanum())
        {
            if (TxRollbackHasWishes(battlerAtk))
                return AR_TxSpell(battlerAtk, battlerDef, move);
            if (gBattleMons[battlerAtk].species == SPECIES_CASTORIA)
                return AR_CastoriaSpell(battlerAtk, battlerDef, move, score);
            return 0;
        }
        return AR_RazeMoves(battlerAtk, battlerDef, move, score);
    }
```

This insertion approach preserves other trainers' existing behavior, including the separately registered Alice callback in `LittlerootTown` and `TRAINER_ALICE3`. It also keeps callback registration/cleanup unchanged. A later implementation may extract the helpers into a dedicated trainer module, but would then need an explicit public accuracy helper or a local equivalent; that extraction is not required by this proposal.

## Ordering, ownership and invariants

1. Ordinary bad-move and enabled scoring passes run first; these trainers do **not** currently enable Koanko's `AI_FLAG_CHECK_VIABILITY`. The proposal does not assume that flag is present. Positive tactical rewards generally require ordinary score at least `AI_SCORE_DEFAULT`, and ordinary slot limitation checks always precede cache access.
2. The registered Dynamic Function adds the proposed regular-move deltas. The engine's subsequent damaging-move comparison still runs; a +8 delta is a preference, not an unconditional command.
3. Virtual Arcanum candidates receive complete scores from their own branch. Their normal-slot caches are never indexed using the virtual move. Existing `IsArcanumSlotUsable` checks still own slot legality, resources and restrictions before callback evaluation.
4. Wish nominations write only the existing scratch candidate fields. The normal chooser commits outcome/recipient identity only if the virtual move wins; ordinary moves still win ties. Actual scripts revalidate and apply effects later.
5. Threat calculations use strongest known usable immediate attacks, maximum rolls and strict ordering, never `gChosenMoveByBattler` or extra prediction RNG. They are conservative local budgets. They exclude delayed/charge/reactive attacks and cannot promise survival against every status move, residual event or virtual spell.
6. Break Bars are not extra ordinary HP in setup/recovery arithmetic. A `AI_DAMAGE_BREAK_LAYER`/saved-layer reveal receives progress merit, not KO merit. Pending breaks suppress investment/healing. The existing scripts retain all refill, transformation and cleanup ownership.
7. Kazuradrop's battle-entry turn comes from the existing `VAR_TEMP_0` writes, and its phase comes from its own move-swap bit. The callback does not initialize/reset either. The Quiver Dance selection contingency preserves the existing slot-resolution behavior when transformation happens before the selected slot executes; it does not predict or perform a transformation itself.
8. TX is owned by the summoned party entry, not merely by species. Its virtual ability survives its existing species-change mechanics. It never becomes eligible for ordinary revival through this policy, and the disabled player position is filtered through `IsBattlerAlive`.
9. No new persistent state, counters or cleanup are required. Ordinary caches retain their existing per-choice refresh lifetime. Fireball alone uses an additional existing save/restore damage simulation because it lacks a real four-slot entry; no custom sampling or RNG is added by these helpers.

## Maintenance impact and deliberate limits

**No engine integration is changed by this document.** Consequently the centralized upstream-modification ledger should not be edited to claim these hooks have been implemented. If adopted, record the two callback interceptions and their dependencies there.

The future source footprint would be one local helper block plus two narrow entry interceptions in the existing trainer Dynamic Functions. Existing script registration, controller selection, shared AI algorithms and battle-script execution need no changes. This minimizes new integration points while leaving the tactical policy readable. Keeping a large helper block in an upstream-maintained file does carry textual merge cost; a later dedicated module is preferable if these policies grow, but exposing a single stable accuracy interface should be evaluated before moving it. Fewer lines are not automatically safer than explicit species blocks.

Semantic compatibility risks remain in `AiLogicData` cache dimensions/meaning, simulated Mega/Max/Tera state, strict speed ordering, move-effect constants, Authority's execution protections, and the Arcanum scratch/commit contract. Upstream updates to those interfaces require rechecking this proposal even if there is no textual conflict.

The policy deliberately avoids a search tree, repeated full damage simulations for setup, counterfactual Normality type/form scoring, selecting the revived Pokémon itself, predicting an actual switch, and changing gimmick activation. Setup's projected damage is an estimate; no calculated post-boost KO is asserted. Rest's three-hit budget is conservative. Drain healing uses accuracy-weighted damage and half-drain approximation; Liquid Ooze and Heal Block earn no healing bonus. Protect does not promise safety against bypassing moves, Max damage or status setup. Threat assessment does not include weather/status chip, recoil, multiaction combinations, future impacts or opposing virtual spells, so positive survival judgments remain heuristic. These limitations need scenario validation, not hidden assumptions of tactical perfection.

## Required validation after implementation, not now

There are no new tests in this proposal, and no executable behavior changed. Existing suites can only protect their existing contracts; they cannot validate the proposed tactics until the code and meaningful new fixtures are implemented.

Required new scenario groups:

- Every roster member chooses useful damage/coverage, avoids immunity and unusable slots, and distinguishes a real KO, Sash/Sturdy survival, extra-layer break and saved-layer reveal.
- Espeon, Hoopa, Alakazam and Castoria: safe setup, no damaging payoff, Contrary, opposing Unaware, useful SpD payoff, stage caps, low HP, Life Orb margin, speed ties, priority, Trick Room and inability to survive the next exchange.
- Umbreon: pending Wish, Wish due at counter 1, full HP/Heal Block, no reachable reserve under No Order Change, repeated Protect, Toxic prevention/Poison Heal/Magic Guard, boosted physical enemy making Foul Play preferable.
- Gothitelle: early/later Rest, ailment cure, full HP, insufficient sleep budget, recovery before/after an incoming hit, and a legally escapable target versus one actually trapped.
- Palkia: Draco Meteor needed to cross a damage threshold versus comparable Spacial Rend; reduced SpA, Contrary, rain/sun Thunder/Hydro Pump accuracy and actual type immunity.
- Kazuradrop: the exact two-turn opener, emergency slot selection, entry-turn accounting after earlier faints, actor-bit move-swap ownership, first Growth, fresh/repeated/blocked Virus, replacement move variants, Time Stop suppression, tiers including PASSIVE_OHKO, and Trash & Crush's proc/accuracy remaining probabilistic.
- Kirlia: physical versus special threats, status benefits, reflected/blocked/redundant status, paralysis speed payoff and existing Z choice.
- Mewtwo in base/Mega X/Mega Y: every Authority protection, Illuminate before/after configured generation 9, Contrary on either side, target at stat floor, remaining swing, immediate finish, recharge and Shadow Force timing.
- Hoopa-Unbound: physical/special coverage, Fury cost, Contrary, Protect history, timely Drain Punch, Liquid Ooze/Heal Block and unchanged Tera policy.
- Castoria: name true/false/backed-out routes; Domination eligibility; shared one-use Wish; eligible More Wishes creation; Endurance/Reduction/Debilitation ranking; no opportunistic Normality; emergency Perish Revivify and Polymorph; regular/virtual Revivify parity; Fireball coverage, immunity and simulation restoration.
- TX: actual Psychic/Swift/Protect/Celebrate kit; all five Wish outcomes, Restoration recipient identity and caps, no repeated already-ineligible benefit, Calamity on changed occupants, species change ownership, post-defeat retirement and no revival.
- Callback isolation: Alice2 versus Alice3/other Alice encounters, unrelated trainer, player-side scoring, ally/absent target, no stale nomination after a losing virtual candidate, unchanged committed actions and RNG state, repeated evaluations returning stable scores.

After a later implementation, label new tactical tests `Alice Tactics` and `Razeluxe Tactics`. Exact WSL commands from repository root (these proposed filters do not select new tests until those tests exist):

```sh
make check -j8 TESTS='Alice Tactics'
make check -j8 TESTS='Razeluxe Tactics'
make check -j8 TESTS='Kazuradrop'
make check -j8 TESTS='Arcanum'
make check -j8 TESTS='TX Wishes'
make check -j8 TESTS='Break Bar'
make check -j8 TESTS='Koanko'
make check -j8 TESTS='AI'
```

During that future validation, keep the implementation checkout's AI source/headers, battle mechanics, trainer party inputs, generated party output and tests stable together. This document's source reference is the inspected working tree, not a claim that HEAD alone reproduces it. **Current validation status: source-reviewed proposal only; uncompiled and untested. Stop here until implementation is requested.**
