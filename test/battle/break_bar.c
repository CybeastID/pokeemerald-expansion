#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_break_bar.h"
#include "battle_interface.h"
#include "battle_scripts.h"
#include "graphics.h"
#include "palette.h"
#include "sprite.h"
#include "task.h"
#include "constants/rgb.h"

extern AiScoreFunc sDynamicAiFunc;

static const struct BreakBarConfig sStacks[] =
{
    {.extraBars = 1},
    {.extraBars = 2},
    {.extraBars = 3},
    {.extraBars = 4},
    {.extraBars = 5},
};

static u8 sExtraBars[MAX_BATTLERS_COUNT];
static u8 sCountsAtTurn[MAX_BATTLERS_COUNT][8];

static s32 BreakBarTestScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    for (u32 battler = 0; battler < gBattlersCount; battler++)
    {
        // Explicit test registration, independent of species, ability, and side.
        u32 initialPartyIndex = battler >= 2 ? 1 : 0;
        if (sExtraBars[battler] != 0 && gBattlerPartyIndexes[battler] == initialPartyIndex
         && GetBattlerPartyState(battler)->breakBarConfig == NULL)
            ActivateBreakBar(battler, &sStacks[sExtraBars[battler] - 1]);
        if (gBattleResults.battleTurnCounter < ARRAY_COUNT(sCountsAtTurn[battler]))
            sCountsAtTurn[battler][gBattleResults.battleTurnCounter] = GetBreakBarCount(battler);
    }
    return score;
}

static void BreakBarTestFixture(void)
{
    memset(sExtraBars, 0, sizeof(sExtraBars));
    memset(sCountsAtTurn, 0xFF, sizeof(sCountsAtTurn));
    sDynamicAiFunc = BreakBarTestScore;
}

static u32 BreakBarTestPaletteSlot(u32 battler)
{
    u32 healthbox = gHealthboxSpriteIds[battler];
    u32 healthbar = gSprites[healthbox].data[5];
    return gSprites[healthbar].oam.paletteNum;
}

AI_SINGLE_BATTLE_TEST("Break Bar Lunar Dance stacks onto an existing bar and respects the five bar cap")
{
    u32 layers;
    PARAMETRIZE { layers = 3; }
    PARAMETRIZE { layers = 5; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_PLAYER_LEFT] = layers;
        PLAYER(SPECIES_WOBBUFFET) { HP(13); MaxHP(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_GARDEVOIR) { Moves(MOVE_LUNAR_DANCE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 0); }
    } SCENE {
        MESSAGE("Wobbuffet became cloaked in mystical moonlight!");
        HP_BAR(player, hp: 100);
        if (layers < BREAK_BAR_MAX_EXTRA_BARS)
            MESSAGE("Wobbuffet gained an extra HP bar!");
        else
            NOT MESSAGE("Wobbuffet gained an extra HP bar!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), min(layers + 1, BREAK_BAR_MAX_EXTRA_BARS));
        EXPECT_EQ(player->hp, 100);
        if (layers < BREAK_BAR_MAX_EXTRA_BARS)
            EXPECT_EQ(GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->breakBarSavedHp[layers], 100);
    }
}

static bool32 BreakBarTestHasFillPixel(u32 battler, u32 color)
{
    u32 healthbar = gSprites[gHealthboxSpriteIds[battler]].data[5];
    const u8 *tile = (const u8 *)(OBJ_VRAM0
        + (gSprites[healthbar].oam.tileNum + 2) * TILE_SIZE_4BPP);
    for (u32 i = 0; i < TILE_SIZE_4BPP; i++)
    {
        if ((tile[i] & 15) == color || (tile[i] >> 4) == color)
            return TRUE;
    }
    return FALSE;
}

// Run the same native frame step used by the scripted dynamic-layer animation.
static void BreakBarTestAnimateSavedLayer(void)
{
    const struct __attribute__((packed))
    {
        u8 opcode;
        void (*func)(void);
        u8 battler;
    } cmd = {0, BS_AnimateSavedBreakBar, BS_OPPONENT1};
    const u8 *savedInstr = gBattlescriptCurrInstr;
    gBattlescriptCurrInstr = (const u8 *)&cmd;
    BS_AnimateSavedBreakBar();
    gBattlescriptCurrInstr = savedInstr;
}

SINGLE_BATTLE_TEST("Break Bar dynamic fill preserves the covered bar color")
{
    u32 hp;
    bool32 registeredBase;
    PARAMETRIZE { hp = 100; registeredBase = FALSE; } // Green ordinary HP.
    PARAMETRIZE { hp = 40; registeredBase = FALSE; }  // Yellow ordinary HP.
    PARAMETRIZE { hp = 10; registeredBase = FALSE; }  // Red ordinary HP.
    PARAMETRIZE { hp = 100; registeredBase = TRUE; }  // Registered red base.

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(hp); MaxHP(100); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        struct PartyState *state = GetBattlerPartyState(battler);
        if (registeredBase)
        {
            EXPECT_EQ(ActivateBreakBar(battler, &sStacks[0]), TRUE);
            state->breakBarsRemaining = 0;
            ReshowBreakBars();
        }
        EXPECT_EQ(GrantSavedBreakBar(battler), TRUE);
        EXPECT_EQ(state->breakBarSavedBaseWasRegistered, registeredBase);

        BreakBarTestAnimateSavedLayer(); // Zero purple pixels: covered bar still visible.
        u32 slot = BreakBarTestPaletteSlot(battler);
        u32 coveredIndex = registeredBase || hp <= 20 ? 14 : hp <= 50 ? 12 : 10;
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 14], gBattleInterface_BallDisplayPal[coveredIndex]);
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 15], gBattleInterface_BallDisplayPal[coveredIndex + 1]);
        EXPECT_EQ(BreakBarTestHasFillPixel(battler, 14), TRUE);

        BreakBarTestAnimateSavedLayer(); // Purple starts covering the saved color.
        EXPECT_EQ(BreakBarTestHasFillPixel(battler, 10), TRUE);
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 14], gBattleInterface_BallDisplayPal[coveredIndex]);
        while (state->breakBarFillPixels < B_HEALTHBAR_PIXELS)
            BreakBarTestAnimateSavedLayer();
        BreakBarTestAnimateSavedLayer(); // Finish and restore the normal red entries.
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 14], gBattleInterface_BallDisplayPal[14]);
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 15], gBattleInterface_BallDisplayPal[15]);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar four and five extra layers deplete before the red base can faint")
{
    u32 extraBars;
    PARAMETRIZE { extraBars = 4; }
    PARAMETRIZE { extraBars = 5; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = extraBars;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        for (u32 i = 0; i < extraBars; i++)
            TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
    } SCENE {
        for (u32 i = 0; i < extraBars; i++)
        {
            HP_BAR(opponent, hp: 0);
            HP_BAR(opponent, hp: 10);
        }
        HP_BAR(opponent, hp: 0);
        MESSAGE("The opposing Wobbuffet fainted!");
    } THEN {
        for (u32 turn = 0; turn <= extraBars; turn++)
            EXPECT_EQ(sCountsAtTurn[B_POSITION_OPPONENT_LEFT][turn], extraBars - turn);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarsRemaining, 0);
        EXPECT(GetBattlerPartyState(B_POSITION_OPPONENT_LEFT)->breakBarConfig == NULL);
        EXPECT_EQ(gBattleStruct->bugSpace.active, FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar overkill consumes only one layer and does not trigger faint effects")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 4;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
        NOT MESSAGE("The opposing Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 3);
        EXPECT_EQ(opponent->hp, 10);
        EXPECT_EQ(opponent->maxHP, 10);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
        EXPECT_EQ(gBattleStruct->givenExpMons, 0);
        EXPECT_EQ(gSideTimers[B_SIDE_OPPONENT].retaliateTimer, 0);
        EXPECT_EQ((u32)gBattleStruct->savedTargetCount, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar resolves once after the full multi-hit move before the slower action")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 2;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_DOUBLE_KICK); Attack(999); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(5); MaxHP(5); Defense(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_DOUBLE_KICK); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        MESSAGE("The Pokémon was hit 2 time(s)!");
        HP_BAR(opponent, hp: 5);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
        EXPECT_EQ(opponent->hp, 5);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar poison stops at one HP without consuming a layer")
{
    u32 hp;
    PARAMETRIZE { hp = 1; }
    PARAMETRIZE { hp = 2; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(hp); MaxHP(16); Status1(STATUS1_POISON); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        if (hp > 1)
            HP_BAR(opponent, hp: 1);
        else
            NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->status1, STATUS1_POISON);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar catches delayed Future Sight damage")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_FUTURE_SIGHT, MOVE_CELEBRATE); SpAttack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); SpDefense(100); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet took the Future Sight attack!");
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
    } THEN {
        EXPECT_EQ(opponent->hp, 10);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar recoil cannot consume a layer")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(10); Attack(100); Moves(MOVE_DOUBLE_EDGE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(player);
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
        EXPECT_EQ(player->maxHP, 1000);
        EXPECT_GT(player->hp, 100);
        EXPECT_EQ((u32)gBattleStruct->savedTargetCount, 0);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar depletion is not a Destiny Bond faint")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_DESTINY_BOND); Speed(20); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DESTINY_BOND, opponent);
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
    } THEN {
        EXPECT_EQ(player->hp, player->maxHP);
        EXPECT_EQ(opponent->hp, 10);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 0);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar Perish Song consumes a layer without a true faint")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_PERISH_SONG); }
        PLAYER(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(20); MaxHP(20); Moves(MOVE_CELEBRATE); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_PERISH_SONG); }
        TURN { SWITCH(player, 1); }
        TURN {}
        TURN {}
    } SCENE {
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 20);
    } THEN {
        EXPECT_EQ(opponent->hp, 20);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar does not end Neutralizing Gas on a temporary zero HP")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_CELEBRATE); Ability(ABILITY_NEUTRALIZING_GAS); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_NEUTRALIZING_GAS);
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
    } THEN {
        EXPECT_EQ(opponent->hp, 10);
        EXPECT_EQ((u32)gDisableStructs[B_POSITION_OPPONENT_LEFT].neutralizingGas, TRUE);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar belongs to its party member across switches")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 4;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS, MOVE_WHIRLWIND, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_CELEBRATE); Ability(ABILITY_RUN_AWAY); }
        OPPONENT(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 10);
        MESSAGE("The opposing Pichu was dragged out!");
        MESSAGE("The opposing Wobbuffet was dragged out!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 3);
        EXPECT(gBattleStruct->partyState[B_SIDE_OPPONENT][1].breakBarConfig == NULL);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Break Bar explicit registration also works on the player side")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_PLAYER_LEFT] = 1;
        PLAYER(SPECIES_WOBBUFFET) { HP(10); MaxHP(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(player, hp: 0);
        HP_BAR(player, hp: 10);
    } THEN {
        EXPECT_EQ(player->hp, 10);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 0);
    }
}

AI_DOUBLE_BATTLE_TEST("Break Bar different layer colors coexist in doubles and reshow independently")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        BreakBarTestFixture();
        sExtraBars[B_POSITION_OPPONENT_LEFT] = 1;
        sExtraBars[B_POSITION_OPPONENT_RIGHT] = 5;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN {}
    } THEN {
        ReshowBreakBars();
        u32 left = BreakBarTestPaletteSlot(B_POSITION_OPPONENT_LEFT);
        u32 right = BreakBarTestPaletteSlot(B_POSITION_OPPONENT_RIGHT);
        EXPECT_NE(left, right);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_RIGHT), 5);
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(left) + 10], RGB(22, 5, 26));
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(left) + 11], RGB(11, 3, 20));
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(right) + 10], RGB(25, 14, 17));
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(right) + 11], RGB(27, 12, 11));
        EXPECT_EQ(BreakBarTestPaletteSlot(B_POSITION_PLAYER_LEFT), IndexOfSpritePaletteTag(TAG_HEALTHBAR_PAL));
    }
}

SINGLE_BATTLE_TEST("Break Bar palette order is orange yellow green blue purple red regardless of HP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(100); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        const u16 expected[][2] =
        {
            {0, 0},
            {RGB(22, 5, 26), RGB(11, 3, 20)},
            {RGB(5, 17, 27), RGB(3, 9, 27)},
            {RGB(4, 25, 22), RGB(1, 13, 11)},
            {RGB(27, 23, 4), RGB(17, 15, 2)},
            {RGB(25, 14, 17), RGB(27, 12, 11)},
        };
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ(ARRAY_COUNT(expected), BREAK_BAR_MAX_EXTRA_BARS + 1);
        EXPECT_EQ(ActivateBreakBar(battler, &sStacks[4]), TRUE);
        for (s32 color = BREAK_BAR_ORANGE; color >= BREAK_BAR_PURPLE; color--)
        {
            GetBattlerPartyState(battler)->breakBarsRemaining = color;
            ReshowBreakBars();
            u32 slot = BreakBarTestPaletteSlot(battler);
            EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 10], expected[color][0]);
            EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 11], expected[color][1]);
            EXPECT_EQ(gPlttBufferFaded[OBJ_PLTT_ID(slot) + 10], expected[color][0]);
            EXPECT_EQ(gPlttBufferFaded[OBJ_PLTT_ID(slot) + 11], expected[color][1]);
        }
        GetBattlerPartyState(battler)->breakBarsRemaining = 0;
        ReshowBreakBars();
        EXPECT_EQ(IsBreakBarBase(battler), TRUE);
        EXPECT_EQ(BreakBarTestPaletteSlot(battler), IndexOfSpritePaletteTag(TAG_HEALTHBAR_PAL));
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->maxHP, 100);
    }
}

SINGLE_BATTLE_TEST("Break Bar red base stays red even at full HP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ(ActivateBreakBar(battler, &sStacks[0]), TRUE);
        GetBattlerPartyState(battler)->breakBarsRemaining = 0;
        ReshowBreakBars();
        EXPECT_EQ(opponent->hp, opponent->maxHP);
        EXPECT_EQ(BreakBarTestHasFillPixel(battler, 14), TRUE);
        EXPECT_EQ(BreakBarTestHasFillPixel(battler, 10), FALSE);
    }
}

SINGLE_BATTLE_TEST("Break Bar registration validates counts and does not refill an active stack")
{
    static const struct BreakBarConfig empty = {.extraBars = 0};
    static const struct BreakBarConfig tooMany = {.extraBars = BREAK_BAR_MAX_EXTRA_BARS + 1};
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ(ActivateBreakBar(MAX_BATTLERS_COUNT, &sStacks[0]), FALSE);
        EXPECT_EQ(ActivateBreakBar(battler, NULL), FALSE);
        EXPECT_EQ(ActivateBreakBar(battler, &empty), FALSE);
        EXPECT_EQ(ActivateBreakBar(battler, &tooMany), FALSE);
        EXPECT_EQ(GetBreakBarCount(MAX_BATTLERS_COUNT), 0);
        EXPECT_EQ(ActivateBreakBar(battler, &sStacks[4]), TRUE);
        GetBattlerPartyState(battler)->breakBarsRemaining = 2;
        EXPECT_EQ(ActivateBreakBar(battler, &sStacks[4]), FALSE);
        EXPECT_EQ(GetBreakBarCount(battler), 2);
        GetBattlerPartyState(battler)->breakBarsRemaining = 0;
        EXPECT_EQ(ActivateBreakBar(battler, &sStacks[0]), TRUE);
        EXPECT_EQ(GetBreakBarCount(battler), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar dispatches each layer's own scripts and preserves the caller")
{
    // Sentinel addresses identify scheduling only; they are never interpreted.
    static const u8 onBreak[BREAK_BAR_MAX_EXTRA_BARS] = {0};
    static const u8 afterBreak[BREAK_BAR_MAX_EXTRA_BARS] = {0};
    static const struct BreakBarConfig config =
    {
        .extraBars = BREAK_BAR_MAX_EXTRA_BARS,
        .phases =
        {
            {&onBreak[0], &afterBreak[0]},
            {&onBreak[1], NULL},
            {&onBreak[2], &afterBreak[2]},
            {&onBreak[3], NULL},
            {&onBreak[4], &afterBreak[4]},
        },
    };
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        const u8 *savedScript = gBattlescriptCurrInstr;
        u32 savedTarget = gBattlerTarget;
        u32 savedScriptingBattler = gBattleScripting.battler;
        EXPECT_EQ(ActivateBreakBar(battler, &config), TRUE);
        for (s32 phase = BREAK_BAR_MAX_EXTRA_BARS - 1; phase >= 0; phase--)
        {
            u32 stackSize = gBattleResources->battleScriptsStack->size;
            u32 targetCount = gBattleStruct->savedTargetCount;
            gBattlerTarget = B_POSITION_PLAYER_LEFT;
            opponent->hp = 0;
            EXPECT_EQ(TryBreakBar(battler, savedScript), TRUE);
            EXPECT_EQ(GetBreakBarCount(battler), phase);
            EXPECT_EQ(gBattlerTarget, battler);
            EXPECT_EQ((u32)gBattleScripting.battler, savedScriptingBattler);
            EXPECT(gBattlescriptCurrInstr == &onBreak[phase]);
            EXPECT(gBattleResources->battleScriptsStack->ptr[stackSize] == savedScript);
            EXPECT(gBattleResources->battleScriptsStack->ptr[stackSize + 1] == BattleScript_BreakBarReturn);
            if (config.phases[phase].afterBreakScript != NULL)
                EXPECT(gBattleResources->battleScriptsStack->ptr[stackSize + 2] == &afterBreak[phase]);
            EXPECT_EQ(gBattleStruct->savedBattlerTarget[targetCount], B_POSITION_PLAYER_LEFT);
            // Discard this inspected schedule before returning control to the engine.
            gBattleResources->battleScriptsStack->size = stackSize;
            gBattleStruct->savedTargetCount = targetCount;
            gBattlescriptCurrInstr = savedScript;
        }
        EXPECT_EQ(TryBreakBar(battler, savedScript), FALSE);
        opponent->hp = opponent->maxHP;
        gBattlerTarget = savedTarget;
        gBattleScripting.battler = savedScriptingBattler;
    }
}

SINGLE_BATTLE_TEST("Break Bar task exhaustion leaves existing tasks intact")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u8 created[NUM_TASKS];
        u32 count = 0;
        while (GetTaskCount() < NUM_TASKS)
            created[count++] = CreateTask(TaskDummy, 0);
        TaskFunc original = gTasks[0].func;
        EXPECT_EQ(ActivateBreakBar(B_POSITION_OPPONENT_LEFT, &sStacks[0]), TRUE);
        EXPECT(gTasks[0].func == original);
        for (u32 i = 0; i < count; i++)
        {
            EXPECT(gTasks[created[i]].func == TaskDummy);
            DestroyTask(created[i]);
        }
        u32 slot = BreakBarTestPaletteSlot(B_POSITION_OPPONENT_LEFT);
        EXPECT_EQ(gPlttBufferUnfaded[OBJ_PLTT_ID(slot) + 10], RGB(22, 5, 26));
    }
}

SINGLE_BATTLE_TEST("Break Bar Kazuradrop keeps her phase two effects after a direct KO")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_KAZURADROP) { HP(1); MaxHP(10); Moves(MOVE_CELEBRATE); Status1(STATUS1_POISON); Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        HP_BAR(opponent, hp: 20);
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ(GetBreakBarCount(battler), 0);
        EXPECT_EQ(opponent->maxHP, 20);
        EXPECT_EQ(opponent->hp, 20);
        EXPECT_EQ(opponent->status1, 0);
        EXPECT_EQ(opponent->moves[0], MOVE_MELT_VIRUS);
        EXPECT_EQ(opponent->moves[1], MOVE_TRASH_CRUSH);
        EXPECT_EQ(opponent->moves[2], MOVE_INFINITE_GROWTH);
        EXPECT_EQ(opponent->moves[3], MOVE_CRACK_ICE);
        EXPECT_GT((u32)opponent->volatiles.kazuradropGuts, 0);
        EXPECT_EQ(gBattleStruct->bugSpace.bpDecayRate, gBattleStruct->bugSpace.bpDecayRatePhase2);
        EXPECT(gBattleStruct->bugSpace.moveSwapActive & (1u << battler));
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
        EXPECT_EQ((u32)gBattleStruct->savedTargetCount, 0);
    }
}

SINGLE_BATTLE_TEST("Break Bar Kazuradrop recoil cannot trigger her phase script")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_KAZURADROP) { HP(1); MaxHP(100); Attack(100); Moves(MOVE_DOUBLE_EDGE); Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DOUBLE_EDGE); MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(player);
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->maxHP, 100);
        EXPECT_EQ(opponent->hp, 1);
        EXPECT_EQ(opponent->moves[0], MOVE_DOUBLE_EDGE);
        EXPECT_EQ(player->maxHP, 1000);
        EXPECT_EQ(player->moves[0], MOVE_CELEBRATE);
        EXPECT_GT(player->hp, 100);
        EXPECT_EQ((u32)gBattleStruct->savedTargetCount, 0);
    }
}
