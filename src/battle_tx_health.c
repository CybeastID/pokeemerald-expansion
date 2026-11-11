#include "global.h"
#include "battle_script_native.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_script_commands.h"
#include "pokemon.h"

#define TX_HP_FRACTION_ONE (1u << 24)

static struct TxRollbackHealth *GetHealth(u32 battler)
{
    if (gBattleStruct == NULL || battler >= gBattlersCount
     || gBattleStruct->txRollback.combatProfile != TX_PROFILE_CASTORIA_SUMMON
     || gBattleStruct->txRollback.ownerDefeated
     || !TxRollbackOwnsPartyMon(GetBattlerTrainer(battler), gBattlerPartyIndexes[battler]))
        return NULL;
    return &gBattleStruct->txRollback.health;
}

static u32 HpFraction(u32 hp, u32 maxHp)
{
    return maxHp == 0 ? 0 : min(TX_HP_FRACTION_ONE, ((u64)hp << 24) / maxHp);
}

static u16 FractionHp(u32 fraction, u32 maxHp)
{
    if (fraction == 0 || maxHp == 0)
        return 0;
    return max(1, min(maxHp, ((u64)fraction * maxHp + TX_HP_FRACTION_ONE / 2) >> 24));
}

static void ObserveHp(struct TxRollbackHealth *health, u32 hp)
{
    // A rounded one-HP view can look full while retaining a lower canonical
    // fraction. Re-emitting that unchanged view is not genuine healing.
    if (hp == health->lastHp)
        return;
    if (hp == 0)
        health->hpFraction = 0;
    else if (hp >= health->appliedMaxHp)
        health->hpFraction = TX_HP_FRACTION_ONE;
    else if (hp != health->lastHp)
    {
        s64 fraction = health->hpFraction
            + ((s64)hp - health->lastHp) * TX_HP_FRACTION_ONE / max(1, health->appliedMaxHp);
        health->hpFraction = max(1, min(TX_HP_FRACTION_ONE - 1, fraction));
    }
    health->lastHp = hp;
}

void TxRollbackInitializeHealth(u32 battler)
{
    struct TxRollbackHealth *health = GetHealth(battler);
    if (health == NULL || health->originalMaxHp != 0 || !IsBattlerAlive(battler))
        return;
    health->originalMaxHp = GetMonData(GetBattlerMon(battler), MON_DATA_MAX_HP);
    health->appliedMaxHp = health->originalMaxHp;
    health->percent = 100;
    health->hpFraction = HpFraction(gBattleMons[battler].hp, health->originalMaxHp);
    health->lastHp = gBattleMons[battler].hp;
}

bool32 TxRollbackHasHealthOverlay(u32 battler)
{
    struct TxRollbackHealth *health = GetHealth(battler);
    return health != NULL && health->originalMaxHp != 0 && health->percent != 100;
}

// Only after a clean party/controller copy. Raw non-HP growth has already been
// reapplied; Reduction, if present, must be applied after this owned maxHP view.
void TxRollbackOnStatsCopied(u32 battler)
{
    struct TxRollbackHealth *health = GetHealth(battler);
    if (health == NULL || health->originalMaxHp == 0)
        return;
    if (gBattleMons[battler].hp != FractionHp(health->hpFraction, health->originalMaxHp))
        health->hpFraction = HpFraction(gBattleMons[battler].hp, health->originalMaxHp);
    health->appliedMaxHp = max(1, (u32)health->originalMaxHp * health->percent / 100);
    gBattleMons[battler].maxHP = health->appliedMaxHp;
    gBattleMons[battler].hp = FractionHp(health->hpFraction, health->appliedMaxHp);
    health->lastHp = gBattleMons[battler].hp;
}

// The emission buffer is a canonical party write, never the effective battle
// view. This runs after Reduction's inverse normalization, in reverse overlay order.
void TxRollbackNormalizeRequest(u32 battler, u32 request, u32 monMask, void *data)
{
    for (u32 target = 0; target < gBattlersCount; target++)
    {
        struct TxRollbackHealth *health = GetHealth(target);
        if (health == NULL || health->originalMaxHp == 0
         || GetBattlerTrainer(target) != GetBattlerTrainer(battler)
         || (monMask == 0 ? target != battler : !(monMask & (1u << gBattlerPartyIndexes[target]))))
            continue;
        u16 value;
        switch (request)
        {
        case REQUEST_HP_BATTLE:
            memcpy(&value, data, sizeof(value));
            ObserveHp(health, value);
            value = FractionHp(health->hpFraction, health->originalMaxHp);
            memcpy(data, &value, sizeof(value));
            break;
        case REQUEST_MAX_HP_BATTLE:
            value = health->originalMaxHp;
            memcpy(data, &value, sizeof(value));
            break;
        case REQUEST_ALL_BATTLE:
        {
            struct BattlePokemon copy;
            memcpy(&copy, data, sizeof(copy));
            ObserveHp(health, copy.hp);
            copy.hp = FractionHp(health->hpFraction, health->originalMaxHp);
            copy.maxHP = health->originalMaxHp;
            memcpy(data, &copy, sizeof(copy));
            break;
        }
        }
        break;
    }
}

void BS_TxRollbackDeteriorate(void)
{
    NATIVE_ARGS(u8 percent);
    if (gBattleControllerExecFlags)
        return;
    u32 battler = gBattlerTarget;
    struct TxRollbackHealth *health = GetHealth(battler);
    if (health != NULL && health->originalMaxHp != 0 && gBattleMons[battler].hp == 0
     && !IsBreakBarPending(battler)
     && ((cmd->percent == 60 && health->percent == 100)
      || (cmd->percent == 30 && health->percent == 60)))
    {
        health->percent = cmd->percent;
        health->appliedMaxHp = max(1, (u32)health->originalMaxHp * health->percent / 100);
        // The original layer is already empty. The existing refill script is
        // solely responsible for healing the new layer, then afterBreak runs.
        health->hpFraction = 0;
        health->lastHp = 0;
        gBattleMons[battler].maxHP = health->appliedMaxHp;
        BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_MAX_HP_BATTLE,
            1u << gBattlerPartyIndexes[battler], sizeof(gBattleMons[battler].maxHP), &gBattleMons[battler].maxHP);
        MarkBattlerForControllerExec(battler);
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void TxRollbackFinishBattle(void)
{
    if (gBattleStruct == NULL)
        return;
    u32 battler = TxRollbackGetOwnerBattler();
    struct TxRollbackHealth *health = GetHealth(battler);
    if (health != NULL && health->originalMaxHp != 0)
    {
        ObserveHp(health, gBattleMons[battler].hp);
        gBattleMons[battler].maxHP = health->originalMaxHp;
        gBattleMons[battler].hp = FractionHp(health->hpFraction, health->originalMaxHp);
        SetMonData(GetBattlerMon(battler), MON_DATA_MAX_HP, &gBattleMons[battler].maxHP);
        SetMonData(GetBattlerMon(battler), MON_DATA_HP, &gBattleMons[battler].hp);
    }
    memset(&gBattleStruct->txRollback, 0, sizeof(gBattleStruct->txRollback));
}
