#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "battle_setup.h"
#include "battle_gimmick.h"
#include "battle_dynamax.h"
#include "pokemon.h"
#include "battle_util.h"
#include "constants/battle_ai.h"
#include "constants/battle_move_effects.h"

static s32 ClampScore(s32 value, s32 low, s32 high)
{
    return value < low ? low : value > high ? high : value;
}

// Only reads the already-calculated target -> user damage caches. No second
// damage simulation, no RNG and no change to prediction or switching decisions.
static bool32 ThreatensKo(u32 attacker, u32 target, u32 move, bool32 *beforeMove)
{
    bool32 threatened = FALSE;
    *beforeMove = FALSE;
    enum Move *moves = GetMovesArray(target);
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        if (!(gAiLogicData->moveLimitations[target] & (1u << slot))
         && moves[slot] != MOVE_NONE
         && gAiLogicData->simulatedDmg[target][attacker][slot].median >= gBattleMons[attacker].hp
         && !CanEndureHit(target, attacker, moves[slot]))
        {
            threatened = TRUE;
            if (AI_IsSlower(attacker, target, move, moves[slot], CONSIDER_PRIORITY))
                *beforeMove = TRUE;
        }
    }
    return threatened;
}

static s32 RankStatus(u32 attacker, u32 target, u32 move, s32 score, bool32 threatened, bool32 beforeMove)
{
    u32 hpPercent = 100 * gBattleMons[attacker].hp / gBattleMons[attacker].maxHP;
    enum BattleMoveEffects effect = GetMoveEffect(move);
    u32 side = GetBattlerSide(attacker);
    // Ordinary penalties still veto the special utility boosts. A positive
    // ordinary score alone is not evidence that a status move is useful.
    s32 rank = 30 + 2 * ClampScore(score - AI_SCORE_DEFAULT, -10, 10);
    if (effect == EFFECT_RESTORE_HP || effect == EFFECT_REST)
    {
        if (hpPercent == 100 || (effect == EFFECT_REST && gBattleMons[attacker].status1 & STATUS1_SLEEP))
            return 1;
        if (score >= AI_SCORE_DEFAULT)
            rank = hpPercent <= 35 ? 145 : hpPercent < 65 ? 95 : 30;
        if (effect == EFFECT_REST)
            rank -= 10;
    }
    else if (effect == EFFECT_REFLECT || effect == EFFECT_LIGHT_SCREEN || effect == EFFECT_AURORA_VEIL)
    {
        u32 screen = effect == EFFECT_REFLECT ? SIDE_STATUS_REFLECT
                   : effect == EFFECT_LIGHT_SCREEN ? SIDE_STATUS_LIGHTSCREEN : SIDE_STATUS_AURORA_VEIL;
        if (gSideStatuses[side] & screen)
            return 1;
        if (score >= AI_SCORE_DEFAULT)
            rank += 30;
    }
    else if (GetMoveNonVolatileStatus(move) == MOVE_EFFECT_SLEEP)
    {
        if (score >= AI_SCORE_DEFAULT && AI_CanPutToSleep(attacker, target, gAiLogicData->abilities[target], move, MOVE_NONE))
            rank = 120;
    }
    else if (IsNonVolatileStatusMove(move))
    {
        if (gBattleMons[target].status1 != STATUS1_NONE)
            return 1;
        if (score >= AI_SCORE_DEFAULT)
            rank += 40;
    }
    else if (move == MOVE_SWORDS_DANCE || move == MOVE_MEDITATE || move == MOVE_SHARPEN
          || move == MOVE_DRAGON_DANCE || move == MOVE_NASTY_PLOT || move == MOVE_QUIVER_DANCE
          || move == MOVE_SHELL_SMASH || move == MOVE_GEOMANCY)
    {
        if (score >= AI_SCORE_DEFAULT && !threatened)
            rank += 40;
    }
    else if (effect == EFFECT_SPIKES || effect == EFFECT_STEALTH_ROCK || effect == EFFECT_TOXIC_SPIKES || effect == EFFECT_STICKY_WEB)
    {
        if (CountUsablePartyMons(target) == 0)
            return 1;
        if (score >= AI_SCORE_DEFAULT)
            rank += 25;
    }
    u32 accuracy = min(100, gAiLogicData->moveAccuracy[attacker][target][gAiThinkingStruct->movesetIndex]);
    rank = rank * accuracy / 100;
    // Slow utility cannot rescue a user who will be knocked out first. Sash,
    // Sturdy and similar survival checks were already considered by ThreatensKo.
    if (beforeMove)
        rank /= 3;
    return ClampScore(rank, 1, 155);
}

static s32 RankDamage(u32 attacker, u32 target, u32 move, s32 score, bool32 beforeMove)
{
    u32 slot = gAiThinkingStruct->movesetIndex;
    const struct SimulatedDamage *damage = &gAiLogicData->simulatedDmg[attacker][target][slot];
    u32 hp = max(1, gBattleMons[target].hp);
    u32 accuracy = min(100, gAiLogicData->moveAccuracy[attacker][target][slot]);
    enum BattleMoveEffects effect = GetMoveEffect(move);
    u32 koChance = 0;
    // Interpolate the cached damage range, not an exact enumeration of rolls.
    // Sash/Sturdy/Disguise prevent treating a simulated damage KO as a real KO.
    if (damage->maximum >= hp && !CanEndureHit(attacker, target, move))
        koChance = damage->minimum >= hp ? 100 : 100 * (damage->maximum - hp + 1) / (damage->maximum - damage->minimum + 1);
    bool32 charge = IsTwoTurnNotSemiInvulnerableMove(attacker, move)
                 || (effect == EFFECT_SEMI_INVULNERABLE && gAiLogicData->holdEffects[attacker] != HOLD_EFFECT_POWER_HERB);
    bool32 conditional = effect == EFFECT_FOCUS_PUNCH || effect == EFFECT_REFLECT_DAMAGE || effect == EFFECT_FUTURE_SIGHT;
    u32 reliableKo = koChance * accuracy / 100;
    if (charge || conditional)
        reliableKo = 0; // Damage may happen later or require an opponent action.
    s32 rank = 20 + 100 * AI_GetReachableDamage(target, damage->median) / hp * accuracy / 100;
    // A reliable immediate KO gets its own tier; partial KOs get a smaller bonus.
    rank += reliableKo >= 80 ? 70 + (reliableKo - 80) : reliableKo / 2;
    rank += ClampScore(score - AI_SCORE_DEFAULT, -10, 10);
    if (charge)
        rank /= 2;
    if (conditional)
        rank /= 2;
    if (MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE))
        rank -= 20;

    enum Ability ability = gAiLogicData->abilities[attacker];
    u32 recoil = 0;
    if (ability != ABILITY_MAGIC_GUARD)
    {
        if (effect == EFFECT_RECOIL && ability != ABILITY_ROCK_HEAD)
            recoil = min(damage->median, hp) * GetMoveRecoil(move) / 100;
        else if (effect == EFFECT_MAX_HP_50_RECOIL || effect == EFFECT_CHLOROBLAST)
            recoil = gBattleMons[attacker].maxHP / 2;
    }
    rank -= min(50, 100 * recoil / max(1, gBattleMons[attacker].maxHP));
    if (recoil >= gBattleMons[attacker].hp)
        rank -= 60;
    if (IsExplosionMove(move))
        rank -= 110;
    for (u32 i = 0; i < GetMoveAdditionalEffectCount(move); i++)
    {
        const struct AdditionalEffect *additional = GetMoveAdditionalEffectById(move, i);
        if (additional->self && additional->moveEffect == MOVE_EFFECT_STAT_MINUS
         && ability != ABILITY_CONTRARY)
            rank -= 10;
    }
    if (MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_THRASH))
        rank -= 15;
    // Reward acting before a lethal response, not priority by itself.
    if (beforeMove)
        rank /= 3;
    return ClampScore(rank, 1, 250);
}

// These helpers read current caches/state only. They do not choose actions,
// predict player input, mutate a phase, or draw more RNG during scoring.
static bool32 IsKoankoBattle(void)
{
    return gBattleStruct != NULL && (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        && !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TWO_OPPONENTS))
        && TRAINER_BATTLE_PARAM.opponentA == TRAINER_KOANKO;
}

void KoankoSetupTrainerAi(void)
{
    if (IsKoankoBattle())
        SetDynamicAiFunc(AI_KoankoTrainer);
}

static bool32 IsImmediateAttack(u32 attacker, u32 move)
{
    enum BattleMoveEffects effect = GetMoveEffect(move);
    return !IsBattleMoveStatus(move) && !IsTwoTurnNotSemiInvulnerableMove(attacker, move)
        && effect != EFFECT_SEMI_INVULNERABLE && effect != EFFECT_FUTURE_SIGHT
        && effect != EFFECT_REFLECT_DAMAGE && effect != EFFECT_FOCUS_PUNCH;
}

static u32 ExpectedDamage(u32 attacker, u32 target, u32 slot)
{
    return AI_GetReachableDamage(target, gAiLogicData->simulatedDmg[attacker][target][slot].median)
        * min(100, gAiLogicData->moveAccuracy[attacker][target][slot]) / 100;
}

static bool32 SecureKo(u32 attacker, u32 target, u32 slot)
{
    u32 move = gBattleMons[attacker].moves[slot];
    return !(gAiLogicData->moveLimitations[attacker] & (1u << slot))
        && move != MOVE_NONE && IsImmediateAttack(attacker, move)
        && gAiLogicData->moveAccuracy[attacker][target][slot] >= 90
        && AI_GetDamageOutcome(attacker, target, move,
            gAiLogicData->simulatedDmg[attacker][target][slot].minimum) == AI_DAMAGE_KO;
}

static bool32 HasSecureKo(u32 attacker, u32 target)
{
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (SecureKo(attacker, target, slot))
            return TRUE;
    return FALSE;
}

static u32 BestExpectedDamage(u32 attacker, u32 target)
{
    u32 damage = 0;
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (!(gAiLogicData->moveLimitations[attacker] & (1u << slot)))
            damage = max(damage, ExpectedDamage(attacker, target, slot));
    return damage;
}

// One strongest known usable attack per living opponent. This is a conservative
// pressure estimate, not knowledge of either opponent's committed command.
static u32 IncomingDamageBefore(u32 attacker, u32 candidate, bool32 beforeOnly)
{
    u32 total = 0;
    for (u32 foe = 0; foe < gBattlersCount; foe++)
    {
        if (!IsBattlerAlive(foe) || IsBattlerAlly(attacker, foe))
            continue;
        u32 best = 0;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            if (!(gAiLogicData->moveLimitations[foe] & (1u << slot))
             && (!beforeOnly || AI_IsSlower(attacker, foe, candidate, gBattleMons[foe].moves[slot], CONSIDER_PRIORITY)))
                best = max(best, gAiLogicData->simulatedDmg[foe][attacker][slot].median);
        total += best;
    }
    return total;
}

static u32 IncomingDamage(u32 attacker)
{
    return IncomingDamageBefore(attacker, MOVE_NONE, FALSE);
}

static bool32 PlanningMax(u32 attacker)
{
    return GetActiveGimmick(attacker) == GIMMICK_DYNAMAX
        || ((gAiBattleData->aiUsingGimmick & (1u << attacker)) && gBattleStruct->gimmick.usableGimmick[attacker] == GIMMICK_DYNAMAX);
}

static bool32 TailwindChangesOrder(u32 attacker, u32 target)
{
    u32 foeSpeed = gAiLogicData->speedStats[target];
    u32 speed = gAiLogicData->speedStats[attacker];
    if (speed <= foeSpeed && speed * 2 > foeSpeed)
        return TRUE;
    // Bench raw speed is a payoff estimate, not a prediction of a switch.
    struct Pokemon *party = GetBattlerParty(attacker);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        if (i == gBattlerPartyIndexes[attacker] || GetMonData(&party[i], MON_DATA_HP) == 0)
            continue;
        speed = GetMonData(&party[i], MON_DATA_SPEED);
        if (speed != 0 && speed <= foeSpeed && speed * 2 > foeSpeed)
            return TRUE;
    }
    return FALSE;
}

static s32 RecoveryValue(u32 attacker, u32 move)
{
    u32 hp = gBattleMons[attacker].hp;
    u32 maximum = gBattleMons[attacker].maxHP;
    if (hp == maximum || gBattleMons[attacker].volatiles.healBlockTimer || IsBreakBarPending(attacker))
        return -10;
    u32 before = IncomingDamageBefore(attacker, move, TRUE);
    u32 incoming = IncomingDamage(attacker);
    if (before >= hp || (incoming >= hp && incoming >= hp + maximum / 2))
        return -8;
    if (incoming >= hp && before < hp)
        return 8; // Can heal before a potentially lethal attack and survive it.
    return hp * 100 / maximum <= 50 ? 6 : hp * 100 / maximum < 70 ? 2 : -2;
}

static s32 SetupValue(u32 attacker, u32 target, enum Stat stat, enum DamageCategory category)
{
    if (gBattleMons[attacker].statStages[stat] >= DEFAULT_STAT_STAGE + 4)
        return -8; // Stop investing after two boosts, even before the hard cap.
    bool32 payoff = FALSE;
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (!(gAiLogicData->moveLimitations[attacker] & (1u << slot))
         && GetMoveCategory(gBattleMons[attacker].moves[slot]) == category
         && ExpectedDamage(attacker, target, slot) != 0)
            payoff = TRUE;
    if (!payoff || IncomingDamage(attacker) >= gBattleMons[attacker].hp)
        return -8;
    return 5;
}

static bool32 ReachableWishRecipient(u32 attacker)
{
    if (gBattleMons[attacker].hp * 100 / gBattleMons[attacker].maxHP < 65)
        return TRUE;
    if (HasNoOrderChange(attacker))
        return FALSE;
    struct Pokemon *party = GetBattlerParty(attacker);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        u32 hp = GetMonData(&party[i], MON_DATA_HP);
        u32 maximum = GetMonData(&party[i], MON_DATA_MAX_HP);
        if (i != gBattlerPartyIndexes[attacker] && hp != 0 && maximum != 0 && hp * 100 / maximum < 65)
            return TRUE;
    }
    return FALSE;
}

static bool32 ComparableAttack(u32 attacker, u32 target, u32 slot)
{
    u32 current = ExpectedDamage(attacker, target, slot);
    if (current == 0)
        return FALSE; // Ordinary immunity/failure scoring already owns this case.
    for (u32 other = 0; other < MAX_MON_MOVES; other++)
    {
        u32 move = gBattleMons[attacker].moves[other];
        if (other != slot && move != MOVE_NONE && !(gAiLogicData->moveLimitations[attacker] & (1u << other))
         && IsImmediateAttack(attacker, move) && !MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE)
         && ExpectedDamage(attacker, target, other) * 100 >= current * 80)
            return TRUE;
    }
    return FALSE;
}

// The per-battle callback is the encounter dispatcher. Every roster member has
// its own block; shared helpers keep cache/legality arithmetic in one place.
s32 AI_KoankoTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (BattleAI_IsScoringArcanum())
        return 0; // Stage04/05 must supply the real solo profile/virtual policy.
    if (!IsKoankoBattle() || GetBattlerSide(battlerAtk) != B_SIDE_OPPONENT
     || !IsBattlerAlive(battlerDef) || IsBattlerAlly(battlerAtk, battlerDef))
        return score;

    u32 species = gBattleMons[battlerAtk].species;
    u32 slot = gAiThinkingStruct->movesetIndex;
    u32 partySlot = gBattlerPartyIndexes[battlerAtk];
    bool32 conjured = gBattleStruct->koankoPhase == KOANKO_CONJURED_TEAM;
    if (gBattleStruct->koankoPhase != KOANKO_ORDINARY_TEAM && !conjured)
        return score;
    if (GET_BASE_SPECIES_ID(species) == SPECIES_ALCREMIE)
        species = SPECIES_ALCREMIE;
    if (GET_BASE_SPECIES_ID(species) == SPECIES_ARCEUS)
        species = SPECIES_ARCEUS;

    // Preserve the established sampled-move rank policy exactly.
    if (species == SPECIES_SMEARGLE)
    {
        if (!KoankoSmeargleAnyMoveActive(battlerAtk) || IsDoubleBattle())
            return score;
        if (score <= 0 || (gAiLogicData->moveLimitations[battlerAtk] & (1u << slot)))
            return 0;
        bool32 before;
        bool32 threatened = ThreatensKo(battlerAtk, battlerDef, move, &before);
        s32 rank = IsBattleMoveStatus(move) ? RankStatus(battlerAtk, battlerDef, move, score, threatened, before)
                                          : RankDamage(battlerAtk, battlerDef, move, score, before);
        ADJUST_SCORE(rank - score);
        return score;
    }
    if (slot >= MAX_MON_MOVES || score <= 0 || (gAiLogicData->moveLimitations[battlerAtk] & (1u << slot)))
        return score;

    bool32 original = !conjured && (species == SPECIES_WHIMSICOTT || species == SPECIES_ALCREMIE
        || species == SPECIES_GLACEON || species == SPECIES_VANILLUXE || species == SPECIES_AUDINO || species == SPECIES_BLISSEY);
    bool32 ownedConjured = conjured && ((species == SPECIES_TX_ROLLBACK && partySlot == 1)
        || ((species == SPECIES_CALYREX_ICE || species == SPECIES_CALYREX_SHADOW) && partySlot == 2)
        || ((species == SPECIES_ETERNATUS || species == SPECIES_ETERNATUS_ETERNAMAX) && partySlot == 3)
        || (species == SPECIES_DIALGA_ORIGIN && partySlot == 4) || (species == SPECIES_ARCEUS && partySlot == 5));
    if (!original && !ownedConjured)
        return score;

    bool32 status = IsBattleMoveStatus(move);
    bool32 useful = score >= AI_SCORE_DEFAULT;
    bool32 ko = SecureKo(battlerAtk, battlerDef, slot);
    bool32 canKo = HasSecureKo(battlerAtk, battlerDef);
    u32 expected = ExpectedDamage(battlerAtk, battlerDef, slot);
    u32 bestDamage = BestExpectedDamage(battlerAtk, battlerDef);
    s32 attack = !status && expected != 0 && expected == bestDamage ? 2 : 0;
    s32 delta = 0;

    switch (species)
    {
    case SPECIES_WHIMSICOTT:
        // Disrupt a useful target, establish speed for the team, then pressure.
        if (move == MOVE_MOONBLAST)
            delta += attack;
        else if (move == MOVE_TAUNT && useful && gBattleMons[battlerDef].volatiles.tauntTimer == 0)
            delta += HasMoveWithCategory(battlerDef, DAMAGE_CATEGORY_STATUS) ? 5 : -6;
        else if (move == MOVE_TAILWIND)
        {
            if (gSideTimers[GetBattlerSide(battlerAtk)].tailwindTimer || (gFieldStatuses & STATUS_FIELD_TRICK_ROOM))
                delta -= 6;
            else if (useful && TailwindChangesOrder(battlerAtk, battlerDef))
                delta += CountUsablePartyMons(battlerAtk) != 0 ? 5 : 3;
        }
        else if (move == MOVE_LEECH_SEED && useful && !gBattleMons[battlerDef].volatiles.leechSeed
              && !IS_BATTLER_OF_TYPE(battlerDef, TYPE_GRASS) && !DoesSubstituteBlockMove(battlerAtk, battlerDef, move))
            delta += IncomingDamage(battlerAtk) < gBattleMons[battlerAtk].hp ? 4 : 1;
        break;

    case SPECIES_ALCREMIE:
        // Use real G-Max healing while Maxed; otherwise sustain and blunt special attackers.
        if (status && move == MOVE_RECOVER)
            delta += PlanningMax(battlerAtk) ? -8 : useful ? RecoveryValue(battlerAtk, move) : 0;
        else if (!status)
        {
            delta += attack;
            if (move == MOVE_DAZZLING_GLEAM && PlanningMax(battlerAtk)
             && gBattleMons[battlerAtk].hp < gBattleMons[battlerAtk].maxHP && !gBattleMons[battlerAtk].volatiles.healBlockTimer)
                delta += 3;
            if (move == MOVE_MYSTICAL_FIRE && useful && !PlanningMax(battlerAtk)
             && HasMoveWithCategory(battlerDef, DAMAGE_CATEGORY_SPECIAL)
             && gBattleMons[battlerDef].statStages[STAT_SPATK] > MIN_STAT_STAGE)
                delta += 3;
        }
        break;

    case SPECIES_GLACEON:
        delta += attack;
        if (move == MOVE_FREEZE_DRY && expected != 0 && expected == bestDamage && IS_BATTLER_OF_TYPE(battlerDef, TYPE_WATER))
            delta += 2;
        if (move == MOVE_YAWN && useful && !gBattleMons[battlerDef].volatiles.yawn
         && AI_CanPutToSleep(battlerAtk, battlerDef, gAiLogicData->abilities[battlerDef], move, MOVE_NONE))
            delta += IncomingDamage(battlerAtk) < gBattleMons[battlerAtk].hp ? 5 : 1;
        break;

    case SPECIES_VANILLUXE:
        delta += attack;
        if (move == MOVE_BLIZZARD && expected != 0 && expected == bestDamage
         && (AI_GetWeather() & B_WEATHER_ICY_ANY) && gAiLogicData->moveAccuracy[battlerAtk][battlerDef][slot] >= 100)
            delta += 2;
        if (move == MOVE_FREEZE_DRY && expected != 0 && expected == bestDamage && IS_BATTLER_OF_TYPE(battlerDef, TYPE_WATER))
            delta += 2;
        if (move == MOVE_TAUNT && useful && gBattleMons[battlerDef].volatiles.tauntTimer == 0)
            delta += HasMoveWithCategory(battlerDef, DAMAGE_CATEGORY_STATUS) ? 3 : -6;
        break;

    case SPECIES_AUDINO:
        if (move == MOVE_WISH)
            delta += gBattleStruct->wish[battlerAtk].counter ? -10
                : useful ? (ReachableWishRecipient(battlerAtk) ? 6 : -4) : 0;
        else if (move == MOVE_ENCORE && useful && !gBattleMons[battlerDef].volatiles.encoreTimer
              && gAiLogicData->lastUsedMove[battlerDef] != MOVE_NONE
              && IsBattleMoveStatus(gAiLogicData->lastUsedMove[battlerDef]) && HasDamagingMove(battlerDef))
            delta += 4;
        else if (move == MOVE_THUNDER_WAVE && useful
              && AI_CanParalyze(battlerAtk, battlerDef, gAiLogicData->abilities[battlerDef], move, MOVE_NONE))
            delta += gAiLogicData->speedStats[battlerDef] >= gAiLogicData->speedStats[battlerAtk] ? 4 : 2;
        else if (move == MOVE_KNOCK_OFF && expected != 0)
            delta += attack + (useful && CanKnockOffItem(battlerAtk, battlerDef, gAiLogicData->items[battlerDef]) ? 3 : 0);
        break;

    case SPECIES_BLISSEY:
        if (move == MOVE_SOFT_BOILED && useful)
            delta += RecoveryValue(battlerAtk, move);
        else if (move == MOVE_SEISMIC_TOSS && expected != 0)
            delta += attack + 2;
        else if (move == MOVE_TOXIC && useful
              && AI_CanPoison(battlerAtk, battlerDef, gAiLogicData->abilities[battlerDef], move, MOVE_NONE))
            delta += IncomingDamage(battlerAtk) < gBattleMons[battlerAtk].hp ? 4 : 1;
        else if (move == MOVE_STEALTH_ROCK)
            delta += IsHazardOnSide(GetBattlerSide(battlerDef), HAZARDS_STEALTH_ROCK) || CountUsablePartyMons(battlerDef) == 0 ? -6
                : useful && IncomingDamage(battlerAtk) < gBattleMons[battlerAtk].hp ? 4 : 0;
        break;

    case SPECIES_TX_ROLLBACK:
        // Current solo real kit only. Summon Wishes/Meteo need their owning stages.
        if (move == MOVE_WISH)
        {
            if (gBattleStruct->wish[battlerAtk].counter)
                delta -= 10;
            else if (useful)
            {
                delta += ReachableWishRecipient(battlerAtk) ? 6 : 0;
                if (gAiLogicData->abilities[battlerAtk] == ABILITY_WISHFUL_THINKING)
                {
                    const struct BattlePokemon *mon = &gBattleMons[battlerAtk];
                    if (mon->attack < 65535 || mon->defense < 65535 || mon->speed < 65535 || mon->spAttack < 65535 || mon->spDefense < 65535)
                        delta += 1; // A successful own Wish can still grow a raw stat.
                    if (mon->hp < mon->maxHP)
                        delta += 1; // Current ability's immediate 1/16 heal.
                }
                if (IncomingDamage(battlerAtk) >= gBattleMons[battlerAtk].hp + gBattleMons[battlerAtk].maxHP / 16)
                    delta -= 8;
            }
        }
        else if (move == MOVE_DOOM_DESIRE)
            delta += gBattleStruct->futureSight[battlerDef].counter ? -10 : useful && expected != 0 ? 4 : 0;
        else if (move == MOVE_PROTECT)
            delta += gBattleMons[battlerAtk].volatiles.consecutiveMoveUses ? -8
                : gBattleStruct->wish[battlerAtk].counter && IncomingDamage(battlerAtk) != 0 ? 6
                : gBattleStruct->futureSight[battlerDef].counter ? 2 : -4;
        break;

    case SPECIES_CALYREX_ICE:
    case SPECIES_CALYREX_SHADOW:
        // Read the actual form/kit/category; never cause a transformation or Tera.
        delta += attack;
        if (move == MOVE_SWORDS_DANCE && useful)
            delta += SetupValue(battlerAtk, battlerDef, STAT_ATK, DAMAGE_CATEGORY_PHYSICAL);
        if (move == MOVE_NASTY_PLOT && useful)
            delta += SetupValue(battlerAtk, battlerDef, STAT_SPATK, DAMAGE_CATEGORY_SPECIAL);
        break;

    case SPECIES_ETERNATUS:
    case SPECIES_ETERNATUS_ETERNAMAX:
        delta += attack;
        if (move == MOVE_RECOVER && useful)
            delta += RecoveryValue(battlerAtk, move);
        if (MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE) && !ko && ComparableAttack(battlerAtk, battlerDef, slot))
            delta -= 3;
        break;

    case SPECIES_DIALGA_ORIGIN:
        delta += attack;
        if (MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE) && !ko && ComparableAttack(battlerAtk, battlerDef, slot))
            delta -= 3;
        if (move == MOVE_ROAR_OF_ETERNITY && useful && !gBattleMons[battlerDef].volatiles.timestop
         && !DoesSubstituteBlockMove(battlerAtk, battlerDef, move)
         && !IsAdditionalEffectBlocked(battlerAtk, gAiLogicData->abilities[battlerAtk], battlerDef, gAiLogicData->abilities[battlerDef], move)
         && gAiLogicData->abilities[battlerAtk] != ABILITY_SHEER_FORCE && expected != 0 && !ko)
            for (u32 i = 0; i < GetMoveAdditionalEffectCount(move); i++)
            {
                const struct AdditionalEffect *additional = GetMoveAdditionalEffectById(move, i);
                if (!additional->self && additional->moveEffect == MOVE_EFFECT_TIMESTOP)
                    delta += 4 * min(100, CalcSecondaryEffectChance(battlerAtk, gAiLogicData->abilities[battlerAtk], additional)) / 100;
            }
        break;

    case SPECIES_ARCEUS:
        delta += attack;
        if (move == MOVE_RECOVER && useful)
            delta += RecoveryValue(battlerAtk, move);
        if (move == MOVE_JUDGMENT && expected != 0 && expected == bestDamage)
            delta += 1; // Its actual cached type/damage earns this, not a presumed Normal type.
        break;
    }
    if (ko)
        delta += 6;
    if (status && canKo)
        delta -= 15; // A useful immediate win takes precedence over more preparation.
    ADJUST_SCORE(delta);
    return score;
}
