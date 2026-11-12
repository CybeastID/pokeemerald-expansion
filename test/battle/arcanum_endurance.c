#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_break_bar.h"
#include "battle_interface.h"
#include "castoria_name_theft.h"
#include "palette.h"
#include "sprite.h"
#include "task.h"

extern AiScoreFunc sDynamicAiFunc;
extern s32 AI_RazeluxeTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);

static bool8 sEnduranceSetup;
static bool8 sEnduranceObserve;
static bool8 sEnduranceSawBothColors;
static bool8 sEnduranceAppearedEarly;
static bool8 sEnduranceChangeMaxHp;

// Classify the actual tile pixels: active-layer fill, red fill, or empty/frame.
static u32 EnduranceColumn(u32 battler, u32 column)
{
    u32 healthbar = gSprites[gHealthboxSpriteIds[battler]].data[5];
    const u8 *tile = (const u8 *)(OBJ_VRAM0
        + (gSprites[healthbar].oam.tileNum + 2 + column / 8) * TILE_SIZE_4BPP);
    for (u32 row = 0; row < 8; row++)
    {
        u32 color = (tile[row * 4 + (column % 8) / 2] >> ((column % 2) * 4)) & 15;
        if (color == 10 || color == 11)
            return 1;
        if (color == 14 || color == 15)
            return 2;
    }
    return 0;
}

static void Task_ObserveEndurance(u8 taskId)
{
    if (gBattleStruct == NULL)
    {
        DestroyTask(taskId);
        return;
    }
    struct PartyState *data = GetBattlerPartyState(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT));
    if (data->breakBarSavedMaxHp[0] == 0)
        return;
    u32 slot = IndexOfSpritePaletteTag(TAG_BREAK_BAR_OPPONENT_LEFT_PAL);
    if (slot == 0xFF)
        return;
    u32 battler = B_POSITION_OPPONENT_LEFT;
    if (gBattleMons[battler].hp != gBattleMons[battler].maxHP)
        sEnduranceAppearedEarly = TRUE;
    u32 basePixels = max(1, data->breakBarSavedHp[0] * B_HEALTHBAR_PIXELS / data->breakBarSavedMaxHp[0]);
    if (EnduranceColumn(battler, 0) == 1 && EnduranceColumn(battler, basePixels - 1) == 2)
        sEnduranceSawBothColors = TRUE;
    if (data->breakBarFillPixels == B_HEALTHBAR_PIXELS)
        DestroyTask(taskId);
}

static s32 EnduranceTestScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (sEnduranceSetup && !IsOnPlayerSide(battlerAtk)
     && gBattleMons[battlerAtk].species == SPECIES_CASTORIA)
    {
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        gBattleMons[battlerAtk].volatiles.castoriaTrueNameCharges = 0;
        sEnduranceSetup = FALSE;
        if (sEnduranceObserve)
            CreateTask(Task_ObserveEndurance, 0);
    }
    if (sEnduranceChangeMaxHp && gBattleResults.battleTurnCounter >= 3
     && gBattleMons[battlerAtk].species == SPECIES_CASTORIA)
    {
        u32 maxHp = 200;
        gBattleMons[battlerAtk].maxHP = maxHp;
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][gBattlerPartyIndexes[battlerAtk]], MON_DATA_MAX_HP, &maxHp);
        sEnduranceChangeMaxHp = FALSE;
    }
    if (!BattleAI_IsScoringArcanum())
        return 90;
    // Explicit test nomination; production tactics deliberately do not choose it yet.
    if (move == MOVE_ARCANUM_WISH
     && CastoriaCanWishFor(battlerAtk, CASTORIA_WISH_ENDURANCE))
    {
        gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_ENDURANCE;
        return score;
    }
    return 0;
}

static void EnduranceFixture(bool32 observe, bool32 changeMaxHp)
{
    sDynamicAiFunc = EnduranceTestScore;
    sEnduranceSetup = TRUE;
    sEnduranceObserve = observe;
    sEnduranceChangeMaxHp = changeMaxHp;
    sEnduranceSawBothColors = FALSE;
    sEnduranceAppearedEarly = FALSE;
}

static s32 EnduranceProductionWishScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (!BattleAI_IsScoringArcanum())
        return 90;
    if (move == MOVE_ARCANUM_WISH)
        return AI_RazeluxeTrainer(battlerAtk, battlerDef, move, score);
    return 0;
}

static u32 EnduranceChooseMove(AiScoreFunc callback)
{
    u32 battler = B_POSITION_OPPONENT_LEFT;
    sDynamicAiFunc = callback;
    gAiThinkingStruct->aiFlags[battler] = AI_FLAG_DYNAMIC_FUNC;
    SetAiLogicDataForTurn(gAiLogicData);
    BattleAI_SetupAIData(0xF, battler);
    return BattleAI_ChooseMoveIndex(battler);
}

SINGLE_BATTLE_TEST("Arcanum Wish Endurance requires a name route and an unused Wish")
{
    GIVEN {
        PLAYER(SPECIES_CASTORIA);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        const struct BreakBarConfig existing = {.extraBars = 1};
        u32 battler = B_POSITION_OPPONENT_LEFT;
        for (u32 name = CASTORIA_NAME_NONE; name <= CASTORIA_NAME_TRUE; name++)
        {
            for (u32 used = FALSE; used <= TRUE; used++)
            {
                bool32 eligible = !used && (name == CASTORIA_NAME_TRUE || name == CASTORIA_NAME_FALSE);
                gBattleStruct->castoria.nameResult = name;
                gBattleStruct->castoria.wishUsed = used;
                EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE), eligible);
            }
        }
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        gBattleStruct->castoria.wishUsed = FALSE;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_PLAYER_LEFT, CASTORIA_WISH_ENDURANCE), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(MAX_BATTLERS_COUNT, CASTORIA_WISH_ENDURANCE), FALSE);
        EXPECT_EQ(ActivateBreakBar(battler, &existing), TRUE);
        EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE), FALSE);
        // Retire this local test configuration before it leaves scope.
        GetBattlerPartyState(battler)->breakBarsRemaining = 0;
        GetBattlerPartyState(battler)->breakBarConfig = NULL;
        RefreshBreakBarPalette(battler, FALSE);
        opponent->hp = 0;
        EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE), FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Endurance remains outside production tactical selection")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE), TRUE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 6), TRUE);
        EXPECT_EQ(EnduranceChooseMove(EnduranceProductionWishScore), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        opponent->volatiles.castoriaTrueNameCharges = 0;
        EXPECT_EQ(EnduranceChooseMove(EnduranceProductionWishScore), 6);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_DOMINATION);
        SetDynamicAiFunc(NULL);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance fills over saved HP after its move animation")
{
    bool32 fasterDamage;
    PARAMETRIZE { fasterDamage = FALSE; }
    PARAMETRIZE { fasterDamage = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(TRUE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); Moves(MOVE_CELEBRATE, MOVE_SEISMIC_TOSS); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { HP(40); MaxHP(100); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, fasterDamage ? MOVE_SEISMIC_TOSS : MOVE_CELEBRATE); }
    } SCENE {
        if (fasterDamage)
            HP_BAR(opponent, hp: 20);
        MESSAGE("The opposing Caster? cast Wish!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        MESSAGE("The opposing Caster? gained an extra layer of endurance!");
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, 100);
        EXPECT_EQ(opponent->maxHP, 100);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 100);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedHp[0], fasterDamage ? 20 : 40);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 100);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(sEnduranceSawBothColors, TRUE);
        EXPECT_EQ(sEnduranceAppearedEarly, FALSE);
        for (u32 column = 0; column < B_HEALTHBAR_PIXELS; column++)
            EXPECT_EQ(EnduranceColumn(B_POSITION_OPPONENT_LEFT, column), 1);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_NONE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance reveals saved red HP without overkill carry or a refill")
{
    bool32 finalHit;
    PARAMETRIZE { finalHit = FALSE; }
    PARAMETRIZE { finalHit = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Moves(MOVE_CELEBRATE, MOVE_SEISMIC_TOSS); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { HP(30); MaxHP(80); Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        if (finalHit) TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("The opposing Caster? gained an extra layer of endurance!");
        HP_BAR(opponent, hp: 0);
        if (finalHit)
        {
            HP_BAR(opponent, hp: 0);
            MESSAGE("The opposing Caster? fainted!");
        }
        else
        {
            NONE_OF {
                HP_BAR(opponent);
                MESSAGE("The opposing Caster? fainted!");
            }
        }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), finalHit ? 0 : 30);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP), 80);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, finalHit ? 1 : 0);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedHp[0], 0);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        if (!finalHit)
        {
            EXPECT_EQ(opponent->hp, 30);
            EXPECT_EQ(opponent->maxHP, 80);
            EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
            EXPECT_EQ(EnduranceColumn(B_POSITION_OPPONENT_LEFT, 0), 2);
            EXPECT_EQ(EnduranceColumn(B_POSITION_OPPONENT_LEFT, 30), 0);
            EXPECT_EQ((u32)gBattleStruct->savedTargetCount, 0);
            EXPECT_EQ(gSideTimers[B_SIDE_OPPONENT].retaliateTimer, 0);
        }
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance survives switches and restores its original max HP")
{
    bool32 changeMaxHp;
    PARAMETRIZE { changeMaxHp = FALSE; }
    PARAMETRIZE { changeMaxHp = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, changeMaxHp);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Moves(MOVE_CELEBRATE, MOVE_WHIRLWIND, MOVE_SEISMIC_TOSS); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { HP(40); MaxHP(100); Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_PICHU) { HP(11); MaxHP(30); Moves(MOVE_CELEBRATE); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        MESSAGE("The opposing Caster? gained an extra layer of endurance!");
        MESSAGE("The opposing Pichu was dragged out!");
        MESSAGE("The opposing Caster? was dragged out!");
        HP_BAR(opponent, hp: 0);
        NOT MESSAGE("The opposing Caster? fainted!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_CASTORIA);
        EXPECT_EQ(opponent->hp, 40);
        EXPECT_EQ(opponent->maxHP, 100);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 40);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP), 100);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_HP), 11);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_MAX_HP), 30);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][1].breakBarConfig == NULL, TRUE);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance synchronizes its owner outside the first party slot")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Moves(MOVE_CELEBRATE, MOVE_WHIRLWIND, MOVE_SEISMIC_TOSS); Speed(40); }
        OPPONENT(SPECIES_PICHU) { HP(11); MaxHP(30); Moves(MOVE_CELEBRATE); Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { HP(40); MaxHP(100); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        MESSAGE("The opposing Caster? was dragged out!");
        MESSAGE("The opposing Caster? gained an extra layer of endurance!");
        HP_BAR(opponent, hp: 0);
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT], 1);
        EXPECT_EQ(opponent->hp, 40);
        EXPECT_EQ(opponent->maxHP, 100);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 11);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP), 30);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_HP), 40);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_MAX_HP), 100);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance resolves after all hits and protects the saved base")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Attack(200); Moves(MOVE_CELEBRATE, MOVE_DOUBLE_KICK); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { HP(3); MaxHP(8); Defense(10); Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_DOUBLE_KICK); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        NOT MESSAGE("The opposing Caster? fainted!");
    } THEN {
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 0);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 0);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 3);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Endurance obeys Taunt Encore Disable and Assault Vest")
{
    u32 restriction;
    PARAMETRIZE { restriction = 0; }
    PARAMETRIZE { restriction = 1; }
    PARAMETRIZE { restriction = 2; }
    PARAMETRIZE { restriction = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        sEnduranceSetup = FALSE;
        sEnduranceChangeMaxHp = FALSE;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        if (restriction == 0)
            gBattleMons[battler].volatiles.tauntTimer = 1;
        else if (restriction == 1)
        {
            gBattleMons[battler].volatiles.encoreTimer = 1;
            gBattleMons[battler].volatiles.encoredMove = MOVE_TACKLE;
            gBattleMons[battler].volatiles.encoredMovePos = 0;
        }
        else if (restriction == 2)
            gBattleMons[battler].volatiles.disabledMove = MOVE_ARCANUM_WISH;
        else
            opponent->item = ITEM_ASSAULT_VEST;
        EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE), TRUE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 6), FALSE);
        EXPECT_EQ(EnduranceChooseMove(EnduranceTestScore), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 0);
        SetDynamicAiFunc(NULL);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance interruption leaves HP and the Wish use untouched")
{
    u32 move;
    PARAMETRIZE { move = MOVE_TAUNT; }
    PARAMETRIZE { move = MOVE_AIR_SLASH; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { SpAttack(1); Moves(move); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { HP(40); MaxHP(100); SpDefense(200); Moves(MOVE_CELEBRATE); Speed(10); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 0);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 0);
        EXPECT_EQ(opponent->maxHP, 100);
        EXPECT(opponent->hp <= 40);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Endurance overlay draws pixel boundaries without changing HP")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        EnduranceFixture(FALSE, FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { HP(40); MaxHP(100); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        const u8 boundaries[] = {0, 1, 7, 8, 9, 19, 47, 48};
        u32 battler = B_POSITION_OPPONENT_LEFT;
        for (u32 i = 0; i < ARRAY_COUNT(boundaries); i++)
        {
            EXPECT_EQ(DrawHealthbarOverlay(battler, 40, 100, boundaries[i]), TRUE);
            for (u32 column = 0; column < B_HEALTHBAR_PIXELS; column++)
                EXPECT_EQ(EnduranceColumn(battler, column), column < boundaries[i] ? 1 : column < 19 ? 2 : 0);
        }
        EXPECT_EQ(DrawHealthbarOverlay(battler, 1, 65535, 0), TRUE);
        EXPECT_EQ(EnduranceColumn(battler, 0), 2);
        EXPECT_EQ(EnduranceColumn(battler, 1), 0);
        EXPECT_EQ(DrawHealthbarOverlay(MAX_BATTLERS_COUNT, 40, 100, 8), FALSE);
        EXPECT_EQ(DrawHealthbarOverlay(battler, 40, 0, 8), FALSE);
        EXPECT_EQ(DrawHealthbarOverlay(battler, 101, 100, 8), FALSE);
        EXPECT_EQ(DrawHealthbarOverlay(battler, 40, 100, B_HEALTHBAR_PIXELS + 1), FALSE);
        EXPECT_EQ(opponent->hp, 100);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP), 100);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedHp[0], 40);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarSavedMaxHp[0], 100);
        DrawHealthbarOverlay(battler, 40, 100, B_HEALTHBAR_PIXELS);
    }
}
