#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_break_bar.h"

// Focused battle-exit regression tests. This file deliberately does not
// depend on the existing break_bar.c test fixture.
extern AiScoreFunc sDynamicAiFunc;
extern u32 gDebugConsolidatedHp;
extern u32 gDebugMaxHp;
extern u32 gDebugFinalHp;
extern bool32 gDebugHasBreakBars;

static const struct BreakBarConfig sExitHpConfig = {.extraBars = 1};

static u16 sSavedHp;
static bool32 sRegistered;


static s32 RegisterExitHpBar(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    struct PartyState *state;

    if (sRegistered || gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT] != 0)
        return score;

    if (!ActivateBreakBar(B_POSITION_PLAYER_LEFT, &sExitHpConfig))
        return score;

    state = GetBattlerPartyState(B_POSITION_PLAYER_LEFT);
    // Model a surviving saved layer, using the same storage/validity fields
    // populated by GrantSavedBreakBar(). The active layer stays untouched.
    state->breakBarSavedHp[0] = sSavedHp;
    state->breakBarSavedMaxHp[0] = gBattleMons[B_POSITION_PLAYER_LEFT].maxHP;
    sRegistered = TRUE;
    return score;
}

static void SetupExitHpBar(u16 savedHp)
{
    sSavedHp = savedHp;
    sRegistered = FALSE;
    sDynamicAiFunc = RegisterExitHpBar;
}

AI_SINGLE_BATTLE_TEST("Break Bar exit HP adds active and saved layers")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetupExitHpBar(25);
        PLAYER(SPECIES_WOBBUFFET) { HP(30); MaxHP(100); Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN {
        
        EXPECT(sRegistered);
        
    }
    AFTER_CLEANUP { 
        struct PartyState *state =
        &gBattleStruct->partyState[B_SIDE_PLAYER][0];

    EXPECT(state->breakBarConfig != NULL);
    EXPECT_EQ(state->breakBarSavedHp[0], 25);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 55);
    EXPECT_EQ((s32)state->breakBarsRemaining, 1);
EXPECT_NE(state->breakBarSavedMaxHp[0], 0);
     }
}

AI_SINGLE_BATTLE_TEST("Break Bar exit HP caps consolidated HP at ordinary maximum")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetupExitHpBar(500);
        PLAYER(SPECIES_WOBBUFFET) { HP(30); MaxHP(100); Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN {
        EXPECT(sRegistered);
    }
    AFTER_CLEANUP { EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP)); }
}

AI_SINGLE_BATTLE_TEST("Break Bar exit HP consolidates a switched out party member")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetupExitHpBar(25);
        PLAYER(SPECIES_WOBBUFFET) { HP(30); MaxHP(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_GARDEVOIR) { Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN {
        EXPECT(sRegistered);
        
    }
    AFTER_CLEANUP { EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 55); }
}

AI_SINGLE_BATTLE_TEST("Break Bar exit HP ignores empty saved layers")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetupExitHpBar(0);
        PLAYER(SPECIES_WOBBUFFET) { HP(30); MaxHP(100); Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN {
        EXPECT(sRegistered);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 30);
    }
    AFTER_CLEANUP { EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 30); }
}
