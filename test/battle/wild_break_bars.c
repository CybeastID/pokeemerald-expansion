#include "global.h"
#include "test/battle.h"
#include "battle_break_bar.h"
#include "battle_scripts.h"
#include "event_data.h"
#include "malloc.h"
#include "script.h"

static const struct BreakBarConfig sWildBars =
{
    .extraBars = 2,
    .phases[BREAK_BAR_BLUE - 1].onBreakScript = BattleScript_BreakBarRefill,
    .phases[BREAK_BAR_PURPLE - 1].afterBreakScript = BattleScript_BreakBarRefill,
};

static void SetWildOverride(struct ScriptContext *ctx, const struct BreakBarConfig *config)
{
    const struct __attribute__((packed)) { const struct BreakBarConfig *config; } args = {config};
    ctx->scriptPtr = (const u8 *)&args;
    Script_SetWildBreakBars(ctx);
    ctx->scriptPtr = NULL;
}

TEST("Wild Break Bar setter arms only for the scripted battle and consumes once")
{
    struct ScriptContext ctx;
    InitScriptContext(&ctx, NULL, NULL);
    SetWildOverride(&ctx, &sWildBars);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    ScriptContext_ContinueScript(&ctx);
    EXPECT_EQ(Script_TakeWildBreakBarOverride(), NULL);

    Script_ArmWildBreakBars(&ctx);
    ScriptContext_ContinueScript(&ctx);
    EXPECT_EQ(Script_TakeWildBreakBarOverride(), &sWildBars);
    EXPECT_EQ(Script_TakeWildBreakBarOverride(), NULL);
    ScriptContext_Init();
}

TEST("Wild Break Bar setter clears canceled and invalid profiles")
{
    const struct BreakBarConfig zeroBars = {.extraBars = 0};
    struct ScriptContext ctx;
    InitScriptContext(&ctx, NULL, NULL);
    SetWildOverride(&ctx, NULL);
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    SetWildOverride(&ctx, &zeroBars);
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    SetWildOverride(&ctx, &sWildBars);
    StopScript(&ctx);
    Script_ArmWildBreakBars(&ctx);
    ScriptContext_ContinueScript(&ctx);
    EXPECT_EQ(Script_TakeWildBreakBarOverride(), NULL);
    InitScriptContext(&ctx, NULL, NULL);
    SetWildOverride(&ctx, &sWildBars);
    Script_ArmWildBreakBars(&ctx);
    ScriptContext_ContinueScript(&ctx);
    Script_ClearWildBreakBarOverride(); // Battle completion or failed entry cleanup.
    EXPECT_EQ(Script_TakeWildBreakBarOverride(), NULL);
    ScriptContext_Init();
}

TEST("Wild Break Bar capture eligibility follows remaining and pending layers")
{
    u8 oldBattlersCount = gBattlersCount;
    u8 oldBattlerPosition = gBattlerPositions[B_POSITION_OPPONENT_LEFT];
    u16 oldPartyIndex = gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT];
    struct BattleStruct *oldBattleStruct = gBattleStruct;
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gBattlersCount = 2;
    gBattlerPositions[B_POSITION_OPPONENT_LEFT] = B_POSITION_OPPONENT_LEFT;
    gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT] = 0;

    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), FALSE);
    InitializeWildBreakBar(0, &sWildBars);
    struct PartyState *state = &gBattleStruct->partyState[B_SIDE_OPPONENT][0];
    EXPECT_EQ(state->breakBarConfig, &sWildBars);
    EXPECT_EQ((u32)state->breakBarsRemaining, 2);
    EXPECT_EQ(state->breakBarConfig->phases[BREAK_BAR_BLUE - 1].onBreakScript, BattleScript_BreakBarRefill);
    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), TRUE);
    state->breakBarsRemaining = 1;
    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), TRUE);
    state->breakBarsRemaining = 0;
    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), FALSE);
    state->breakBarPending = TRUE;
    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), TRUE);
    state->breakBarPending = FALSE;
    state->breakBarsRemaining = 1; // A dynamically granted layer also blocks capture.
    EXPECT_EQ(IsBreakBarCaptureBlocked(B_POSITION_OPPONENT_LEFT), TRUE);

    Free(gBattleStruct);
    gBattleStruct = oldBattleStruct;
    gBattlersCount = oldBattlersCount;
    gBattlerPositions[B_POSITION_OPPONENT_LEFT] = oldBattlerPosition;
    gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT] = oldPartyIndex;
}

WILD_BATTLE_TEST("Wild Break Bars reject ordinary and Master Balls until the final HP layer")
{
    u32 firstBall;
    PARAMETRIZE { firstBall = ITEM_POKE_BALL; }
    PARAMETRIZE { firstBall = ITEM_MASTER_BALL; }

    GIVEN {
        struct ScriptContext ctx;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_CELEBRATE); }
        InitScriptContext(&ctx, NULL, NULL);
        SetWildOverride(&ctx, &sWildBars);
        Script_ArmWildBreakBars(&ctx);
        ScriptContext_ContinueScript(&ctx);
    } WHEN {
        TURN { USE_ITEM(player, firstBall); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        TURN { USE_ITEM(player, ITEM_MASTER_BALL); }
    } SCENE {
        MESSAGE("The wild Wobbuffet's protective layers\nprevent capture!");
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
    } THEN {
        EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_SPECIES), SPECIES_WOBBUFFET);
        ScriptContext_Init();
    }
}
