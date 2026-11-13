#include "global.h"
#include "battle_script_native.h"
#include "battle.h"
#include "battle_reduction.h"
#include "battle_util.h"
#include "battle_interface.h"
#include "battle_scripts.h"
#include "battle_controllers.h"
#include "battle_script_commands.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/battle_string_ids.h"

#define HP_FRACTION_ONE (1u << 24)

extern const u8 BattleScript_ReductionTick[];

bool32 ReductionIsActive(void)
{
    return gBattleStruct != NULL && gBattleStruct->reduction.active;
}

static struct ReductionOccupant *GetOccupant(u32 battler)
{
    if (!ReductionIsActive() || battler >= gBattlersCount
     || GetBattlerSide(battler) != gBattleStruct->reduction.affectedSide)
        return NULL;
    return &gBattleStruct->reduction.occupant[battler];
}

bool32 ReductionIsScaled(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    return state != NULL && state->scaled && state->partyIndex == gBattlerPartyIndexes[battler];
}

static u16 FractionHp(u32 fraction, u32 maxHp)
{
    if (fraction == 0 || maxHp == 0)
        return 0;
    return max(1, min(maxHp, ((u64)fraction * maxHp + HP_FRACTION_ONE / 2) >> 24));
}

static u32 HpFraction(u32 hp, u32 maxHp)
{
    return maxHp == 0 ? 0 : min(HP_FRACTION_ONE, ((u64)hp << 24) / maxHp);
}

static void ReadStats(u32 battler, u16 stats[NUM_STATS])
{
    stats[STAT_HP] = gBattleMons[battler].maxHP;
    stats[STAT_ATK] = gBattleMons[battler].attack;
    stats[STAT_DEF] = gBattleMons[battler].defense;
    stats[STAT_SPEED] = gBattleMons[battler].speed;
    stats[STAT_SPATK] = gBattleMons[battler].spAttack;
    stats[STAT_SPDEF] = gBattleMons[battler].spDefense;
}

static void WriteStats(u32 battler, const u16 stats[NUM_STATS])
{
    gBattleMons[battler].maxHP = stats[STAT_HP];
    gBattleMons[battler].attack = stats[STAT_ATK];
    gBattleMons[battler].defense = stats[STAT_DEF];
    gBattleMons[battler].speed = stats[STAT_SPEED];
    gBattleMons[battler].spAttack = stats[STAT_SPATK];
    gBattleMons[battler].spDefense = stats[STAT_SPDEF];
}

// Account for actual healing/damage, retaining the sub-HP remainder across
// rescalings. Merely shrinking or restoring a healthbox cannot add health.
static void ObserveHp(u32 battler, u32 hp)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    u32 maxHp = gBattleMons[battler].maxHP;
    if (maxHp != state->applied[STAT_HP])
    {
        state->base[STAT_HP] = min(65535, (u32)maxHp * state->base[STAT_HP] / max(1, state->applied[STAT_HP]));
        state->applied[STAT_HP] = maxHp;
        state->hpFraction = HpFraction(state->lastHp, maxHp);
    }
    if (hp == 0)
        state->hpFraction = 0;
    else if (hp >= maxHp)
        state->hpFraction = HP_FRACTION_ONE;
    else if (hp != state->lastHp)
    {
        s64 fraction = state->hpFraction + ((s64)hp - state->lastHp) * HP_FRACTION_ONE / max(1, maxHp);
        state->hpFraction = max(1, min(HP_FRACTION_ONE - 1, fraction));
    }
    state->lastHp = hp;
}

u16 ReductionCanonicalHp(u32 battler)
{
    if (!ReductionIsScaled(battler))
        return gBattleMons[battler].hp;
    ObserveHp(battler, gBattleMons[battler].hp);
    struct ReductionOccupant *state = GetOccupant(battler);
    return FractionHp(state->hpFraction, state->base[STAT_HP]);
}

u16 ReductionCanonicalMaxHp(u32 battler)
{
    if (!ReductionIsScaled(battler))
        return gBattleMons[battler].maxHP;
    ObserveHp(battler, gBattleMons[battler].hp);
    return GetOccupant(battler)->base[STAT_HP];
}

static void RemoveOverlay(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    u16 stats[NUM_STATS];
    if (!ReductionIsScaled(battler))
        return;
    ObserveHp(battler, gBattleMons[battler].hp);
    ReadStats(battler, stats);
    if (state->powerTrick != gBattleMons[battler].volatiles.powerTrick)
    {
        u16 tmp;
        SWAP(state->base[STAT_ATK], state->base[STAT_DEF], tmp);
        SWAP(state->applied[STAT_ATK], state->applied[STAT_DEF], tmp);
    }
    for (u32 i = 0; i < NUM_STATS; i++)
        stats[i] = max(1, min(65535, ((u32)stats[i] * state->base[i] + state->applied[i] / 2) / max(1, state->applied[i])));
    WriteStats(battler, stats);
    gBattleMons[battler].level = GetMonData(GetBattlerMon(battler), MON_DATA_LEVEL);
    gBattleMons[battler].hp = FractionHp(state->hpFraction, stats[STAT_HP]);
    SetMonData(GetBattlerMon(battler), MON_DATA_HP, &gBattleMons[battler].hp);
    state->scaled = FALSE;
}

static void ApplyOverlay(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    struct Pokemon *mon = GetBattlerMon(battler);
    struct Pokemon scratch;
    u16 normal[NUM_STATS], reduced[NUM_STATS];
    u32 level = GetMonData(mon, MON_DATA_LEVEL);
    u32 effective = max(1, (s32)level - state->lostLevels);
    s16 savedLevelUpHp = gBattleScripting.levelUpHP;

    if (state->lostLevels == 0 || state->depth != 0 || state->scaled)
        return;

    // Scratch copies only. Neither Polymorph nor the party can observe these.
    scratch = *mon;
    CalculateMonStatsAtLevel(&scratch, level);
    for (u32 i = 0; i < NUM_STATS; i++)
        normal[i] = GetMonData(&scratch, MON_DATA_MAX_HP + i);
    scratch = *mon;
    CalculateMonStatsAtLevel(&scratch, effective);
    for (u32 i = 0; i < NUM_STATS; i++)
        reduced[i] = GetMonData(&scratch, MON_DATA_MAX_HP + i);
    gBattleScripting.levelUpHP = savedLevelUpHp;

    // Preserve copied Transform stats and other raw stat modifiers by applying
    // the level-derived ratio to the current stat line, not replacing the mon.
    ReadStats(battler, state->base);
    if (FractionHp(state->hpFraction, state->base[STAT_HP]) != gBattleMons[battler].hp)
        state->hpFraction = HpFraction(gBattleMons[battler].hp, state->base[STAT_HP]);
    for (u32 i = 0; i < NUM_STATS; i++)
    {
        u32 stat = i;
        if (gBattleMons[battler].volatiles.powerTrick)
        {
            if (stat == STAT_ATK) stat = STAT_DEF;
            else if (stat == STAT_DEF) stat = STAT_ATK;
        }
        state->applied[i] = max(1, (u32)state->base[i] * reduced[stat] / max(1, normal[stat]));
    }
    WriteStats(battler, state->applied);
    gBattleMons[battler].level = effective;
    gBattleMons[battler].hp = FractionHp(state->hpFraction, gBattleMons[battler].maxHP);
    state->lastHp = gBattleMons[battler].hp;
    state->partyIndex = gBattlerPartyIndexes[battler];
    state->powerTrick = gBattleMons[battler].volatiles.powerTrick;
    state->scaled = TRUE;
}

void ReductionBeginMutation(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    if (state == NULL)
        return;
    if (state->depth == 0)
        RemoveOverlay(battler);
    state->depth++;
}

void ReductionEndMutation(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    if (state == NULL || state->depth == 0)
        return;
    if (--state->depth == 0)
        ApplyOverlay(battler);
}

void ReductionOnStatsCopied(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    if (state == NULL)
        return;
    state->scaled = FALSE;
    ApplyOverlay(battler);
}

void ReductionResetOccupant(u32 battler)
{
    struct ReductionOccupant *state = GetOccupant(battler);
    if (state == NULL)
        return;
    RemoveOverlay(battler);
    memset(state, 0, sizeof(*state));
}

void ReductionFinishBattle(void)
{
    if (!ReductionIsActive())
        return;
    for (u32 battler = 0; battler < gBattlersCount; battler++)
        ReductionResetOccupant(battler);
    gBattleStruct->reduction.active = FALSE;
}

void ReductionActivate(void)
{
    ReductionTryActivate(10, B_SIDE_PLAYER);
}

bool32 ReductionTryActivate(u32 initialLoss, u32 affectedSide)
{
    if (gBattleStruct == NULL || ReductionIsActive() || initialLoss == 0
     || initialLoss > MAX_LEVEL || affectedSide >= NUM_BATTLE_SIDES)
        return FALSE;
    memset(&gBattleStruct->reduction, 0, sizeof(gBattleStruct->reduction));
    gBattleStruct->reduction.active = TRUE;
    gBattleStruct->reduction.grace = TRUE;
    gBattleStruct->reduction.nextLoss = initialLoss;
    gBattleStruct->reduction.affectedSide = affectedSide;
    return TRUE;
}

bool32 ReductionEndTurn(enum BattlerId battler)
{
    struct ReductionData *data = &gBattleStruct->reduction;
    bool32 effect = FALSE;
    gBattleStruct->eventState.endTurnBattler++;
    if (data->active && !data->grace && GetBattlerSide(battler) == data->affectedSide && IsBattlerAlive(battler))
    {
        struct ReductionOccupant *state = GetOccupant(battler);
        u32 oldLevel = gBattleMons[battler].level;
        ReductionBeginMutation(battler);
        state->lostLevels = min(MAX_LEVEL, state->lostLevels + data->nextLoss);
        ReductionEndMutation(battler);
        if (gBattleMons[battler].level != oldLevel)
        {
            PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 3, gBattleMons[battler].level);
            gBattleScripting.battler = battler;
            BattleScriptCall(BattleScript_ReductionTick);
            effect = TRUE;
        }
    }
    if (data->active && gBattleStruct->eventState.endTurnBattler == gBattlersCount)
    {
        if (data->grace)
            data->grace = FALSE;
        else
            data->nextLoss = min(MAX_LEVEL, data->nextLoss + 1);
    }
    return effect;
}

void ReductionRefreshDisplay(u32 battler)
{
    UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], GetBattlerMon(battler), HEALTHBOX_ALL);
}

void BS_ReductionRefreshDisplay(void)
{
    NATIVE_ARGS();
    ReductionRefreshDisplay(gBattleScripting.battler);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void ReductionNormalizeRequest(u32 battler, u32 request, u32 monMask, void *data)
{
    if (!ReductionIsActive())
        return;
    // A controller can update an explicitly selected partner/bench member.
    for (u32 target = 0; target < gBattlersCount; target++)
    {
        if (GetBattlerTrainer(target) != GetBattlerTrainer(battler)
         || (monMask == 0 ? target != battler : !(monMask & (1u << gBattlerPartyIndexes[target])))
         || !ReductionIsScaled(target))
            continue;
        struct ReductionOccupant *state = GetOccupant(target);
        struct Pokemon *mon = GetBattlerMon(target);
        u16 value;
        switch (request)
        {
        case REQUEST_HP_BATTLE:
            memcpy(&value, data, sizeof(value));
            ObserveHp(target, value);
            value = FractionHp(state->hpFraction, state->base[STAT_HP]);
            memcpy(data, &value, sizeof(value));
            break;
        case REQUEST_LEVEL_BATTLE:
            *(u8 *)data = GetMonData(mon, MON_DATA_LEVEL);
            break;
        case REQUEST_EXP_BATTLE:
        {
            u32 exp = GetMonData(mon, MON_DATA_EXP);
            memcpy(data, &exp, sizeof(exp));
            break;
        }
        case REQUEST_MAX_HP_BATTLE:
        case REQUEST_ATK_BATTLE:
        case REQUEST_DEF_BATTLE:
        case REQUEST_SPEED_BATTLE:
        case REQUEST_SPATK_BATTLE:
        case REQUEST_SPDEF_BATTLE:
        {
            u32 stat = request - REQUEST_MAX_HP_BATTLE;
            memcpy(&value, data, sizeof(value));
            value = min(65535, (u32)value * state->base[stat] / max(1, state->applied[stat]));
            memcpy(data, &value, sizeof(value));
            break;
        }
        case REQUEST_ALL_BATTLE:
        {
            struct BattlePokemon copy;
            memcpy(&copy, data, sizeof(copy));
            struct BattlePokemon *out = &copy;
            ObserveHp(target, out->hp);
            out->hp = FractionHp(state->hpFraction, state->base[STAT_HP]);
            out->level = GetMonData(mon, MON_DATA_LEVEL);
            out->experience = GetMonData(mon, MON_DATA_EXP);
            out->maxHP = GetMonData(mon, MON_DATA_MAX_HP);
            out->attack = GetMonData(mon, MON_DATA_ATK);
            out->defense = GetMonData(mon, MON_DATA_DEF);
            out->speed = GetMonData(mon, MON_DATA_SPEED);
            out->spAttack = GetMonData(mon, MON_DATA_SPATK);
            out->spDefense = GetMonData(mon, MON_DATA_SPDEF);
            memcpy(data, out, sizeof(*out));
            break;
        }
        }
        break;
    }
}
