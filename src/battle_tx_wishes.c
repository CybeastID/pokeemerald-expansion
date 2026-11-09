#include "global.h"
#include "battle.h"
#include "battle_break_bar.h"
#include "battle_controllers.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "pokemon.h"
#include "random.h"
#include "constants/battle_string_ids.h"

extern const u8 BattleScript_TxWishReversal[];
extern const u8 BattleScript_TxWishReclamation[];
extern const u8 BattleScript_TxWishCalamity[];
extern const u8 BattleScript_TxWishRestoration[];
extern const u8 BattleScript_TxWishReciprocity[];
extern const u8 BattleScript_TxReciprocityHeal[];

// Phase moves and mirrored recovery are subroutines, not additional actions.
struct TxScriptContext
{
    struct BattleScripting scripting;
    u32 hitMarker;
    u16 resultFlags[MAX_BATTLERS_COUNT];
    u8 communication[BATTLE_COMMUNICATION_ENTRIES_COUNT];
    u16 currentMove;
    u16 chosenMove;
    u8 attacker;
    u8 target;
    u8 effectBattler;
    u8 currentMovePos;
    u8 chosenMovePos;
    u8 multiHitCounter;
};

static EWRAM_DATA struct TxScriptContext sPhaseContext = {0};
static EWRAM_DATA struct TxScriptContext sRecoveryContext = {0};
static EWRAM_DATA u16 sRecoveryAmount = 0;

static void SaveContext(struct TxScriptContext *ctx)
{
    ctx->scripting = gBattleScripting;
    ctx->hitMarker = gHitMarker;
    memcpy(ctx->resultFlags, gBattleStruct->moveResultFlags, sizeof(ctx->resultFlags));
    memcpy(ctx->communication, gBattleCommunication, sizeof(ctx->communication));
    ctx->currentMove = gCurrentMove;
    ctx->chosenMove = gChosenMove;
    ctx->attacker = gBattlerAttacker;
    ctx->target = gBattlerTarget;
    ctx->effectBattler = gEffectBattler;
    ctx->currentMovePos = gCurrMovePos;
    ctx->chosenMovePos = gChosenMovePos;
    ctx->multiHitCounter = gMultiHitCounter;
}

static void RestoreContext(const struct TxScriptContext *ctx)
{
    gBattleScripting = ctx->scripting;
    gHitMarker = ctx->hitMarker;
    memcpy(gBattleStruct->moveResultFlags, ctx->resultFlags, sizeof(ctx->resultFlags));
    memcpy(gBattleCommunication, ctx->communication, sizeof(ctx->communication));
    gCurrentMove = ctx->currentMove;
    gChosenMove = ctx->chosenMove;
    gBattlerAttacker = ctx->attacker;
    gBattlerTarget = ctx->target;
    gEffectBattler = ctx->effectBattler;
    gCurrMovePos = ctx->currentMovePos;
    gChosenMovePos = ctx->chosenMovePos;
    gMultiHitCounter = ctx->multiHitCounter;
}

bool32 TxRollbackHasWishes(u32 battler)
{
    return gBattleStruct != NULL && battler < gBattlersCount
        && gBattleStruct->txRollback.stage >= TX_ROLLBACK_AMBUSH_PENDING
        && gBattleStruct->txRollback.stage != TX_ROLLBACK_DEFEATED
        && TxRollbackOwnsPartyMon(GetBattlerSide(battler), gBattlerPartyIndexes[battler]);
}

static bool32 IsOriginalAlly(u32 battler)
{
    if (battler >= gBattlersCount || IsOnPlayerSide(battler) || !IsBattlerAlive(battler))
        return FALSE;
    return gBattlerPartyIndexes[battler] == gBattleStruct->txRollback.casterPartyIndex
        || TxRollbackOwnsPartyMon(GetBattlerSide(battler), gBattlerPartyIndexes[battler]);
}

// One entry is one transferable benefit. Side benefits appear once, even in doubles.
enum TxBenefit
{
    TX_BENEFIT_SUBSTITUTE = NUM_STATS,
    TX_BENEFIT_AQUA_RING,
    TX_BENEFIT_INGRAIN,
    TX_BENEFIT_REFLECT,
    TX_BENEFIT_LIGHT_SCREEN,
    TX_BENEFIT_AURORA_VEIL,
    TX_BENEFIT_TAILWIND,
    TX_BENEFIT_SAFEGUARD,
    TX_BENEFIT_MIST,
};

struct TxBenefitEntry
{
    u8 battler;
    u8 benefit;
};

static const u32 sSideBenefits[] =
{
    SIDE_STATUS_REFLECT, SIDE_STATUS_LIGHTSCREEN, SIDE_STATUS_AURORA_VEIL,
    SIDE_STATUS_TAILWIND, SIDE_STATUS_SAFEGUARD, SIDE_STATUS_MIST,
};

static u16 *BenefitTimer(u32 side, u32 benefit)
{
    struct SideTimer *timer = &gSideTimers[side];
    switch (benefit)
    {
    case TX_BENEFIT_REFLECT: return &timer->reflectTimer;
    case TX_BENEFIT_LIGHT_SCREEN: return &timer->lightscreenTimer;
    case TX_BENEFIT_AURORA_VEIL: return &timer->auroraVeilTimer;
    case TX_BENEFIT_TAILWIND: return &timer->tailwindTimer;
    case TX_BENEFIT_SAFEGUARD: return &timer->safeguardTimer;
    default: return &timer->mistTimer;
    }
}

static u32 GetBenefits(u32 tx, struct TxBenefitEntry *entries)
{
    u32 count = 0;
    u32 firstPlayer = MAX_BATTLERS_COUNT;
    for (u32 b = 0; b < gBattlersCount; b++)
    {
        if (!IsOnPlayerSide(b) || !IsBattlerAlive(b))
            continue;
        firstPlayer = min(firstPlayer, b);
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            if (gBattleMons[b].statStages[stat] > DEFAULT_STAT_STAGE
             && gBattleMons[tx].statStages[stat] < MAX_STAT_STAGE)
                entries[count++] = (struct TxBenefitEntry){b, stat};
        if (gBattleMons[b].volatiles.substitute && !gBattleMons[tx].volatiles.substitute)
            entries[count++] = (struct TxBenefitEntry){b, TX_BENEFIT_SUBSTITUTE};
        if (gBattleMons[b].volatiles.aquaRing && !gBattleMons[tx].volatiles.aquaRing)
            entries[count++] = (struct TxBenefitEntry){b, TX_BENEFIT_AQUA_RING};
        if (gBattleMons[b].volatiles.root && !gBattleMons[tx].volatiles.root)
            entries[count++] = (struct TxBenefitEntry){b, TX_BENEFIT_INGRAIN};
    }
    if (firstPlayer < MAX_BATTLERS_COUNT)
        for (u32 i = 0; i < ARRAY_COUNT(sSideBenefits); i++)
            if ((gSideStatuses[B_SIDE_PLAYER] & sSideBenefits[i])
             && !(gSideStatuses[B_SIDE_OPPONENT] & sSideBenefits[i]))
                entries[count++] = (struct TxBenefitEntry){firstPlayer, TX_BENEFIT_REFLECT + i};
    return count;
}

#define TX_MAX_BENEFITS (MAX_BATTLERS_COUNT * (NUM_STATS + 3) + ARRAY_COUNT(sSideBenefits))

bool32 TxRollbackCanWishFor(u32 battler, u32 wish, u32 target)
{
    if (!TxRollbackHasWishes(battler) || !IsBattlerAlive(battler) || IsBreakBarPending(battler))
        return FALSE;
    switch (wish)
    {
    case TX_WISH_REVERSAL:
    case TX_WISH_CALAMITY:
        for (u32 b = 0; b < gBattlersCount; b++)
        {
            if (!IsOnPlayerSide(b) || !IsBattlerAlive(b))
                continue;
            if (wish == TX_WISH_CALAMITY && !gBattleMons[b].volatiles.perishSong)
                return TRUE;
            if (wish == TX_WISH_REVERSAL)
                for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
                    if (gBattleMons[b].statStages[stat] > DEFAULT_STAT_STAGE)
                        return TRUE;
        }
        return FALSE;
    case TX_WISH_RECLAMATION:
    {
        struct TxBenefitEntry entries[TX_MAX_BENEFITS];
        return GetBenefits(battler, entries) != 0;
    }
    case TX_WISH_RESTORATION:
        return IsOriginalAlly(target) && !IsBreakBarPending(target)
            && GetBreakBarCount(target) < GetBattlerPartyState(target)->breakBarsPeak
            && GetBreakBarCount(target) < BREAK_BAR_MAX_EXTRA_BARS;
    case TX_WISH_RECIPROCITY:
        return !gBattleStruct->txRollback.reciprocity;
    default:
        return FALSE;
    }
}

bool32 TxRollbackHasUsableWish(u32 battler)
{
    for (u32 wish = TX_WISH_REVERSAL; wish <= TX_WISH_RECIPROCITY; wish++)
        for (u32 target = 0; target < gBattlersCount; target++)
            if (TxRollbackCanWishFor(battler, wish, target))
                return TRUE;
    return FALSE;
}

static void StealBenefit(u32 tx, struct TxBenefitEntry entry)
{
    u32 b = entry.battler;
    u32 benefit = entry.benefit;
    if (benefit < NUM_STATS)
    {
        u32 stages = gBattleMons[b].statStages[benefit] - DEFAULT_STAT_STAGE;
        gBattleMons[b].statStages[benefit] = DEFAULT_STAT_STAGE;
        gBattleMons[tx].statStages[benefit] = min(MAX_STAT_STAGE, gBattleMons[tx].statStages[benefit] + stages);
    }
    else if (benefit == TX_BENEFIT_SUBSTITUTE)
    {
        gDisableStructs[tx].substituteHP = max(1, (u32)gDisableStructs[b].substituteHP
            * gBattleMons[tx].maxHP / max(1, gBattleMons[b].maxHP));
        gDisableStructs[b].substituteHP = 0;
        gBattleMons[b].volatiles.substitute = FALSE;
        gBattleMons[tx].volatiles.substitute = TRUE;
        gEffectBattler = b;
    }
    else if (benefit == TX_BENEFIT_AQUA_RING)
    {
        gBattleMons[b].volatiles.aquaRing = FALSE;
        gBattleMons[tx].volatiles.aquaRing = TRUE;
    }
    else if (benefit == TX_BENEFIT_INGRAIN)
    {
        gBattleMons[b].volatiles.root = FALSE;
        gBattleMons[tx].volatiles.root = TRUE;
    }
    else
    {
        u32 flag = sSideBenefits[benefit - TX_BENEFIT_REFLECT];
        *BenefitTimer(B_SIDE_OPPONENT, benefit) = *BenefitTimer(B_SIDE_PLAYER, benefit);
        *BenefitTimer(B_SIDE_PLAYER, benefit) = 0;
        gSideStatuses[B_SIDE_PLAYER] &= ~flag;
        gSideStatuses[B_SIDE_OPPONENT] |= flag;
    }
}

void BS_TxRollbackTryWish(void)
{
    NATIVE_ARGS(const u8 *failInstr);
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    u32 tx = gBattlerAttacker;
    u32 wish = data->wishOutcome;
    u32 target = MAX_BATTLERS_COUNT;
    data->wishOutcome = TX_WISH_NONE;
    for (u32 b = 0; b < gBattlersCount; b++)
        if (!IsOnPlayerSide(b) && gBattlerPartyIndexes[b] == data->wishTargetPartyIndex)
            target = b;
    if (!IsArcanumAction(tx) || gChosenMove != MOVE_ARCANUM_WISH
     || gCurrentMove != MOVE_ARCANUM_WISH || !TxRollbackCanWishFor(tx, wish, target))
    {
        gBattlescriptCurrInstr = cmd->failInstr;
        return;
    }
    switch (wish)
    {
    case TX_WISH_REVERSAL:
        for (u32 b = 0; b < gBattlersCount; b++)
            if (IsOnPlayerSide(b) && IsBattlerAlive(b))
                for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
                    if (gBattleMons[b].statStages[stat] > DEFAULT_STAT_STAGE)
                        gBattleMons[b].statStages[stat] = 2 * DEFAULT_STAT_STAGE - gBattleMons[b].statStages[stat];
        gBattlescriptCurrInstr = BattleScript_TxWishReversal;
        break;
    case TX_WISH_CALAMITY:
        for (u32 b = 0; b < gBattlersCount; b++)
            if (IsOnPlayerSide(b) && IsBattlerAlive(b) && !gBattleMons[b].volatiles.perishSong)
            {
                gBattleMons[b].volatiles.perishSong = TRUE;
                gDisableStructs[b].perishSongTimer = 3;
            }
        gBattlescriptCurrInstr = BattleScript_TxWishCalamity;
        break;
    case TX_WISH_RECLAMATION:
    {
        struct TxBenefitEntry entries[TX_MAX_BENEFITS];
        u32 count = RandomUniform(RNG_TX_RECLAMATION_COUNT, 1, 3);
        gEffectBattler = MAX_BATTLERS_COUNT; // No Substitute stolen yet.
        while (count--)
        {
            // Rebuild after each transfer: receiving a benefit can make another ineligible.
            u32 available = GetBenefits(tx, entries);
            if (available == 0)
                break;
            StealBenefit(tx, entries[RandomUniform(RNG_TX_RECLAMATION_BENEFIT, 0, available - 1)]);
        }
        gBattlescriptCurrInstr = BattleScript_TxWishReclamation;
        break;
    }
    case TX_WISH_RESTORATION:
        if (!GrantSavedBreakBar(target))
        {
            gBattlescriptCurrInstr = cmd->failInstr;
            return;
        }
        gBattleScripting.battler = target;
        gBattlescriptCurrInstr = BattleScript_TxWishRestoration;
        break;
    case TX_WISH_RECIPROCITY:
        data->reciprocity = TRUE;
        gBattlescriptCurrInstr = BattleScript_TxWishReciprocity;
        break;
    }
}

void BS_TxRollbackEndurance(void)
{
    NATIVE_ARGS(const u8 *failInstr);
    u32 caster = gBattleStruct->txRollback.caster;
    if (!IsOriginalAlly(caster)
     || gBattlerPartyIndexes[caster] != gBattleStruct->txRollback.casterPartyIndex
     || !GrantSavedBreakBar(caster))
    {
        gBattlescriptCurrInstr = cmd->failInstr;
        return;
    }
    gBattleScripting.battler = caster;
    gBattlerAttacker = caster;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackBeginPhase(void)
{
    NATIVE_ARGS(u16 move);
    SaveContext(&sPhaseContext);
    gBattlerAttacker = gBattleScripting.battler = gBattlerTarget;
    gCurrentMove = gChosenMove = cmd->move;
    gCurrMovePos = gChosenMovePos = 0xFF;
    gMultiHitCounter = 0;
    gBattleScripting.animTurn = gBattleScripting.animTargetsHit = 0;
    memset(gBattleStruct->moveResultFlags, 0, sizeof(gBattleStruct->moveResultFlags));
    gHitMarker &= ~(HITMARKER_DISABLE_ANIMATION | HITMARKER_UNABLE_TO_USE_MOVE);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackEndPhase(void)
{
    NATIVE_ARGS();
    RestoreContext(&sPhaseContext);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackReduction(void)
{
    NATIVE_ARGS(const u8 *alreadyActiveInstr);
    if (ReductionIsActive())
        gBattlescriptCurrInstr = cmd->alreadyActiveInstr;
    else
    {
        ReductionActivate();
        gBattlescriptCurrInstr = cmd->nextInstr;
    }
}

void BS_TxRollbackPrepareDoomDesire(void)
{
    NATIVE_ARGS(const u8 *failInstr);
    gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    if (!IsBattlerAlive(gBattlerTarget) || gWishFutureKnock.futureSightCounter[gBattlerTarget] != 0)
        gBattlescriptCurrInstr = cmd->failInstr;
    else
        gBattlescriptCurrInstr = cmd->nextInstr;
}

void TxRollbackMirrorHealing(u32 battler, u32 oldHp, const u8 *returnInstr)
{
    if (!gBattleStruct->txRollback.reciprocity || !IsOnPlayerSide(battler)
     || !IsBattlerAlive(battler) || oldHp == 0 || gBattleMons[battler].hp <= oldHp)
        return;
    u32 tx = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    if (!TxRollbackHasWishes(tx) || !IsBattlerAlive(tx) || IsBreakBarPending(tx)
     || gBattleMons[tx].hp == gBattleMons[tx].maxHP)
        return;
    sRecoveryAmount = min(gBattleMons[tx].maxHP - gBattleMons[tx].hp,
        max(1, (u32)(gBattleMons[battler].hp - oldHp) * gBattleMons[tx].maxHP / gBattleMons[battler].maxHP));
    SaveContext(&sRecoveryContext);
    BattleScriptPush(returnInstr);
    gBattlescriptCurrInstr = BattleScript_TxReciprocityHeal;
}

void BS_TxRollbackBeginReciprocity(void)
{
    NATIVE_ARGS();
    gBattleScripting.battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    SetHealAmount(gBattleScripting.battler, sRecoveryAmount);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackEndReciprocity(void)
{
    NATIVE_ARGS();
    RestoreContext(&sRecoveryContext);
    gBattlescriptCurrInstr = cmd->nextInstr;
}
