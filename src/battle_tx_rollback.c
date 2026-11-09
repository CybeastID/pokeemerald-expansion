#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_anim.h"
#include "battle_break_bar.h"
#include "battle_controllers.h"
#include "battle_gfx_sfx_util.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "castoria_name_theft.h"
#include "pokemon.h"
#include "sprite.h"
#include "string_util.h"
#include "trainer_break_bars.h"
#include "constants/trainers.h"

#define TX_PARTY_INDEX (PARTY_SIZE - 1)

extern const u8 BattleScript_TxRollbackAmbush[];
extern const u8 BattleScript_TxRollbackPrototypeAmbush[];
extern const u8 BattleScript_TxRollbackEndurance[];
extern const u8 BattleScript_TxRollbackBreakWish[];
extern const u8 BattleScript_TxRollbackBreakDoomDesire[];

static const struct TxRollbackProfile sPrototypeProfile =
{
    .ambushScript = BattleScript_TxRollbackEndurance,
};

static const struct BreakBarConfig sTxBreakBars =
{
    .extraBars = 2,
    .phases[1].afterBreakScript = BattleScript_TxRollbackBreakWish,
    .phases[0].afterBreakScript = BattleScript_TxRollbackBreakDoomDesire,
};

// Only transient execution state is restored. Combat effects and faint bits live on.
static EWRAM_DATA struct
{
    struct BattleScripting scripting;
    u32 hitMarker;
    u16 currentMove;
    u16 chosenMove;
    u8 currentMovePos;
    u8 chosenMovePos;
} sAmbushContext = {0};

static EWRAM_DATA const u8 *sAmbushFinishInstr = NULL;

static void ResumeAmbushAfterEffects(void)
{
    gBattlescriptCurrInstr = sAmbushFinishInstr;
    gCurrentActionFuncId = 0;
    gBattleMainFunc = RunBattleScriptCommands;
}

static bool32 FormationActive(void)
{
    return gBattleStruct != NULL && gBattleStruct->txRollback.stage >= TX_ROLLBACK_ENTERING;
}

enum TxRollbackSlotPolicy TxRollbackGetSlotPolicy(u32 battler)
{
    if (!FormationActive() || battler >= gBattlersCount)
        return TX_SLOT_NORMAL;
    if (GetBattlerPosition(battler) == B_POSITION_PLAYER_RIGHT)
        return TX_SLOT_DISABLED;
    if (GetBattlerPosition(battler) == B_POSITION_OPPONENT_RIGHT)
        return gBattleStruct->txRollback.stage == TX_ROLLBACK_DEFEATED ? TX_SLOT_RETIRED : TX_SLOT_RESERVED;
    return TX_SLOT_NORMAL;
}

bool32 TxRollbackCanPopulateSlot(u32 battler)
{
    // Reserved is occupied by the creation only; no ordinary replacement owns it.
    return TxRollbackGetSlotPolicy(battler) == TX_SLOT_NORMAL;
}

bool32 TxRollbackOwnsPartyMon(u32 side, u32 partyIndex)
{
    return FormationActive() && side == B_SIDE_OPPONENT
        && partyIndex == gBattleStruct->txRollback.partyIndex;
}

bool32 TxRollbackCanRevive(u32 side, u32 partyIndex)
{
    return !TxRollbackOwnsPartyMon(side, partyIndex);
}

static bool32 CanPrepareCreation(u32 caster)
{
    if (gBattleStruct == NULL || caster >= gBattlersCount
     || gBattlersCount != 2 || IsDoubleBattle()
     || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
     || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_SAFARI))
     || gBattleStruct->castoria.nameResult != CASTORIA_NAME_FALSE
     || gBattleStruct->castoria.wishUsed
     || GetBattlerPosition(caster) != B_POSITION_OPPONENT_LEFT
     || gBattleMons[caster].species != SPECIES_CASTORIA || !IsBattlerAlive(caster)
     || GetMonData(&gEnemyParty[TX_PARTY_INDEX], MON_DATA_SPECIES) != SPECIES_NONE)
        return FALSE;

    if (gTestRunnerEnabled)
        return gBattleStruct->txRollback.profile != NULL;

    return !(gBattleTypeFlags & BATTLE_TYPE_RECORDED)
        && TRAINER_BATTLE_PARAM.opponentA == TRAINER_RAZELUXE
        && gBattlerPartyIndexes[caster] == 4;
}

bool32 TxRollbackCanCreate(u32 caster)
{
    return gBattleStruct != NULL
        && gBattleStruct->txRollback.stage == TX_ROLLBACK_NONE
        && CanPrepareCreation(caster);
}

bool32 TxRollbackRequestCreation(u32 caster)
{
    if (!TxRollbackCanCreate(caster))
        return FALSE;
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    if (data->profile == NULL)
        data->profile = &sPrototypeProfile;
    data->caster = caster;
    data->casterPartyIndex = gBattlerPartyIndexes[caster];
    data->partyIndex = TX_PARTY_INDEX;
    data->savedTarget = gBattlerTarget;
    data->savedEffectBattler = gEffectBattler;
    data->savedScriptingBattler = gBattleScripting.battler;
    data->stage = TX_ROLLBACK_REQUESTED;
    return TRUE;
}

void BS_TxRollbackCreate(void)
{
    NATIVE_ARGS(const u8 *failInstr);
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    u32 freeSprites = 0;
    for (u32 i = 0; i < MAX_SPRITES; i++)
        freeSprites += !gSprites[i].inUse;
    if (data->stage != TX_ROLLBACK_REQUESTED || freeSprites < 20
     || !CanPrepareCreation(data->caster))
    {
        data->stage = TX_ROLLBACK_NONE;
        gBattlescriptCurrInstr = cmd->failInstr;
        return;
    }

    // Fixed Hardy personality and non-shiny OT; encounter kit is provisional.
    CreateMon(&gEnemyParty[TX_PARTY_INDEX], SPECIES_TX_ROLLBACK,
        gBattleMons[data->caster].level, 31, TRUE, 0, OT_ID_PRESET, 0x12345678);
    SetMonData(&gEnemyParty[TX_PARTY_INDEX], MON_DATA_NICKNAME, COMPOUND_STRING("TX Rollback"));
    static const u16 moves[MAX_MON_MOVES] = {MOVE_PSYCHIC, MOVE_SWIFT, MOVE_PROTECT, MOVE_CELEBRATE};
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(&gEnemyParty[TX_PARTY_INDEX], moves[i], i);
    gEnemyPartyCount = CalculatePartyCount(gEnemyParty);
    gAiPartyData->count[B_SIDE_OPPONENT] = gEnemyPartyCount;

    // Existing positions, actors, choices and queue entries are untouched.
    gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    gBattlersCount = MAX_BATTLERS_COUNT;
    data->stage = TX_ROLLBACK_ENTERING;
    for (u32 battler = 2; battler < MAX_BATTLERS_COUNT; battler++)
    {
        gBattlerPositions[battler] = battler;
        gBattlerPartyIndexes[battler] = battler == B_POSITION_OPPONENT_RIGHT ? TX_PARTY_INDEX : gBattlerPartyIndexes[0];
        memset(&gBattleMons[battler], 0, sizeof(gBattleMons[battler]));
        memset(&gDisableStructs[battler], 0, sizeof(gDisableStructs[battler]));
        memset(&gProtectStructs[battler], 0, sizeof(gProtectStructs[battler]));
        memset(&gSpecialStatuses[battler], 0, sizeof(gSpecialStatuses[battler]));
        memset(&gBattleStruct->battlerState[battler], 0, sizeof(gBattleStruct->battlerState[battler]));
        gBattleStruct->monToSwitchIntoId[battler] = PARTY_SIZE;
        gBattleStruct->AI_monToSwitchIntoId[battler] = PARTY_SIZE;
        gBattleStruct->chosenMovePositions[battler] = 0;
        gChosenMoveByBattler[battler] = MOVE_NONE;
        gChosenActionByBattler[battler] = B_ACTION_NOTHING_FAINTED;
        gBattlerByTurnOrder[battler] = battler;
        gActionsByTurnOrder[battler] = B_ACTION_NOTHING_FAINTED;
        gBattlerSpriteIds[battler] = MAX_SPRITES;
        gHealthboxSpriteIds[battler] = MAX_SPRITES;
        memcpy(gBattleStruct->battlerPartyOrders[battler], gBattleStruct->battlerPartyOrders[battler & BIT_SIDE], 3);
        gAiThinkingStruct->aiFlags[battler] = gAiThinkingStruct->aiFlags[battler & BIT_SIDE];
        SetControllerToOpponent(battler); // Disabled player position never emits commands.
    }
    gAbsentBattlerFlags |= 1u << B_POSITION_PLAYER_RIGHT;
    gAbsentBattlerFlags &= ~(1u << B_POSITION_OPPONENT_RIGHT);
    gBattleScripting.battler = B_POSITION_OPPONENT_RIGHT;
    BtlController_EmitGetMonData(B_POSITION_OPPONENT_RIGHT, B_COMM_TO_CONTROLLER,
        REQUEST_ALL_BATTLE, 1u << TX_PARTY_INDEX);
    MarkBattlerForControllerExec(B_POSITION_OPPONENT_RIGHT);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackInitialize(void)
{
    NATIVE_ARGS();
    u32 battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    // switchindataupdate has completed SwitchInClearSetData before this call.
    ActivateBreakBar(battler, &sTxBreakBars);
    gBattleStruct->battlerState[battler].noOrderChange = TRUE;
    gDisableStructs[battler].hazardsDone = TRUE;
    BattleLoadTxRollbackHealthboxGfx();
    gHealthboxSpriteIds[battler] = CreateBattlerHealthboxSprites(battler);
    InitBattlerHealthboxCoords(battler);
    UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], GetBattlerMon(battler), HEALTHBOX_ALL);
    CreateEnemyShadowSprite(battler);
    // The player retains singles coordinates; move only the existing enemy.
    u32 caster = gBattleStruct->txRollback.caster;
    if (IsBattlerSpritePresent(caster))
    {
        gSprites[gBattlerSpriteIds[caster]].y = GetBattlerSpriteDefault_Y(caster);
    }
    InitBattlerHealthboxCoords(caster);
    UpdateHealthboxAttribute(gHealthboxSpriteIds[caster], GetBattlerMon(caster), HEALTHBOX_ALL);
    gBattleStruct->castoria.wishUsed = TRUE;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

// Assembly-only native. Advance by two pixels per frame before presenting the
// creation, keeping the existing sprite and all of its animation/affine state.
void BS_TxRollbackSlideCaster(void)
{
    NATIVE_ARGS();
    u32 caster = gBattleStruct->txRollback.caster;
    u32 spriteId = gBattlerSpriteIds[caster];
    if (spriteId < MAX_SPRITES && gSprites[spriteId].inUse)
    {
        struct Sprite *sprite = &gSprites[spriteId];
        s16 targetX = GetBattlerSpriteCoord(caster, BATTLER_COORD_X_2);
        if (sprite->x < targetX)
        {
            sprite->x = min(sprite->x + 2, targetX);
            return;
        }
        if (sprite->x > targetX)
        {
            sprite->x = max(sprite->x - 2, targetX);
            return;
        }
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TxRollbackFinishEntry(void)
{
    NATIVE_ARGS();
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    u32 battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    if (data->stage != TX_ROLLBACK_DEFEATED && IsBattlerAlive(battler))
    {
        gBattleStruct->battlerState[battler].noOrderChange = TRUE;
        data->stage = TX_ROLLBACK_AMBUSH_PENDING;
    }
    gBattlerAttacker = data->caster;
    gBattlerTarget = data->savedTarget;
    gEffectBattler = data->savedEffectBattler;
    gBattleScripting.battler = data->savedScriptingBattler;
    gCurrentMove = gChosenMove = MOVE_ARCANUM_WISH;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void TxRollbackOnFaint(u32 battler)
{
    if (TxRollbackOwnsPartyMon(GetBattlerSide(battler), gBattlerPartyIndexes[battler])
     && !IsBreakBarPending(battler) && gBattleMons[battler].hp == 0)
    {
        gBattleStruct->txRollback.stage = TX_ROLLBACK_DEFEATED;
        gBattleStruct->txRollback.reciprocity = FALSE;
    }
}

bool32 TxRollbackTryAmbush(void)
{
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    if (data->stage != TX_ROLLBACK_AMBUSH_PENDING)
        return FALSE;
    u32 battler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    data->stage = TX_ROLLBACK_ACTIVE;
    if (gBattleOutcome != 0 || !IsBattlerAlive(battler))
        return FALSE;
    data->stage = TX_ROLLBACK_AMBUSH_RUNNING;
    data->savedAttacker = gBattlerAttacker;
    data->savedTarget = gBattlerTarget;
    data->savedEffectBattler = gEffectBattler;
    data->savedScriptingBattler = gBattleScripting.battler;
    data->savedActionFunc = gCurrentActionFuncId;
    sAmbushContext.scripting = gBattleScripting;
    sAmbushContext.hitMarker = gHitMarker;
    sAmbushContext.currentMove = gCurrentMove;
    sAmbushContext.chosenMove = gChosenMove;
    sAmbushContext.currentMovePos = gCurrMovePos;
    sAmbushContext.chosenMovePos = gChosenMovePos;
    gBattlerAttacker = gBattleScripting.battler = battler;
    gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    gCurrentMove = gChosenMove = MOVE_NONE;
    BattleScriptPushCursorAndCallback(BattleScript_TxRollbackAmbush);
    return TRUE;
}

void BS_TxRollbackCallAmbush(void)
{
    NATIVE_ARGS();
    gBattlescriptCurrInstr = cmd->nextInstr;
    if (gBattleStruct->txRollback.profile->ambushScript != NULL)
        BattleScriptCall(gBattleStruct->txRollback.profile->ambushScript);
}

void BS_TxRollbackFinishAmbush(void)
{
    NATIVE_ARGS();
    // Execute-style phase/faint scripts restore their callback, not the bytecode
    // cursor. Give those scripts an explicit continuation into this native.
    sAmbushFinishInstr = gBattlescriptCurrInstr;
    gBattleMainFunc = ResumeAmbushAfterEffects;
    if (TryResolveBreakBarsAfterMove())
        return;
    if (HandleFaintedMonActions())
        return;
    gBattleMainFunc = RunBattleScriptCommands;
    gBattleStruct->eventState.faintedAction = 0;
    struct TxRollbackBattleData *data = &gBattleStruct->txRollback;
    if (data->stage == TX_ROLLBACK_AMBUSH_RUNNING)
        data->stage = TX_ROLLBACK_ACTIVE;
    gBattlerAttacker = data->savedAttacker;
    gBattlerTarget = data->savedTarget;
    gEffectBattler = data->savedEffectBattler;
    gBattleScripting = sAmbushContext.scripting;
    gCurrentMove = sAmbushContext.currentMove;
    gChosenMove = sAmbushContext.chosenMove;
    gCurrMovePos = sAmbushContext.currentMovePos;
    gChosenMovePos = sAmbushContext.chosenMovePos;
    const u32 transientHitMarkers = HITMARKER_DISABLE_ANIMATION | HITMARKER_UNABLE_TO_USE_MOVE | HITMARKER_OBEYS;
    gHitMarker = (gHitMarker & ~transientHitMarkers) | (sAmbushContext.hitMarker & transientHitMarkers);
    gCurrentActionFuncId = data->savedActionFunc;
    gBattlescriptCurrInstr = cmd->nextInstr;
}
