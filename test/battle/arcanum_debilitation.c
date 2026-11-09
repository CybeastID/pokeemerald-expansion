#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_break_bar.h"
#include "battle_interface.h"
#include "battle_status_icons.h"
#include "battle_util.h"
#include "castoria_name_theft.h"
#include "task.h"
#include "sprite.h"
#include "palette.h"

extern AiScoreFunc sDynamicAiFunc;

static bool8 sSetup;
static bool8 sCastWish;
static u8 sTrappedBattler;
static u8 sInitialBars;
static u8 sWishTurn;
static bool8 sSeedToxic;
static u32 sStatusesAtApplication;
static const struct BreakBarConfig sOneBar = {.extraBars = 1};

static void Task_ObserveDebilitation(u8 taskId)
{
    if (HasNoOrderChange(B_POSITION_PLAYER_LEFT))
    {
        sStatusesAtApplication = gBattleMons[B_POSITION_PLAYER_LEFT].status1;
        DestroyTask(taskId);
    }
}

static s32 DebilitationTestScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (sSetup)
    {
        sSetup = FALSE;
        if (sSeedToxic)
            gBattleMons[B_POSITION_PLAYER_LEFT].status1 |= STATUS1_TOXIC_TURN(2);
        if (sCastWish)
        {
            gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
            CreateTask(Task_ObserveDebilitation, 0);
        }
        else
        {
            gBattleStruct->battlerState[sTrappedBattler].noOrderChange = TRUE;
            RefreshBattleStatusIcon(sTrappedBattler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
        }
        if (sInitialBars)
            ActivateBreakBar(sTrappedBattler, &sOneBar);
    }
    if (!sCastWish)
        return score;
    if (!BattleAI_IsScoringArcanum())
        return 90;
    if (move == MOVE_ARCANUM_WISH
     && gBattleResults.battleTurnCounter >= sWishTurn
     && CastoriaCanWishFor(battlerAtk, CASTORIA_WISH_DEBILITATION))
    {
        gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_DEBILITATION;
        return score;
    }
    return 0;
}

static void DebilitationFixture(bool32 castWish, u32 trappedBattler, u32 bars)
{
    sDynamicAiFunc = DebilitationTestScore;
    sSetup = TRUE;
    sCastWish = castWish;
    sTrappedBattler = trappedBattler;
    sInitialBars = bars;
    sWishTurn = 0;
    sSeedToxic = FALSE;
    sStatusesAtApplication = 0;
}

static void ExpectNoOrderChangeIcon(u32 battler, bool32 present)
{
    u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
    if (!present)
    {
        EXPECT_EQ(spriteId, MAX_SPRITES);
        return;
    }
    EXPECT_LT(spriteId, MAX_SPRITES);
    if (spriteId >= MAX_SPRITES)
        return;
    EXPECT_EQ((u32)gSprites[spriteId].inUse, TRUE);
    EXPECT_EQ((u32)gSprites[spriteId].invisible, FALSE);
}

static void ExpectDebilitationStatuses(u32 status)
{
    EXPECT_NE(status & STATUS1_SLEEP, 0);
    EXPECT_NE(status & STATUS1_POISON, 0);
    EXPECT_NE(status & STATUS1_BURN, 0);
    EXPECT_NE(status & STATUS1_FREEZE, 0);
    EXPECT_NE(status & STATUS1_PARALYSIS, 0);
    EXPECT_NE(status & STATUS1_FROSTBITE, 0);
}

SINGLE_BATTLE_TEST("Arcanum Wish Debilitation requires a resolved route unused Wish and living opponent")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        for (u32 route = CASTORIA_NAME_NONE; route <= CASTORIA_NAME_TRUE; route++)
        {
            for (u32 used = FALSE; used <= TRUE; used++)
            {
                gBattleStruct->castoria.nameResult = route;
                gBattleStruct->castoria.wishUsed = used;
                EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_DEBILITATION),
                    route != CASTORIA_NAME_NONE && !used);
            }
        }
        gBattleStruct->castoria.wishUsed = FALSE;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_PLAYER_LEFT, CASTORIA_WISH_DEBILITATION), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(MAX_BATTLERS_COUNT, CASTORIA_WISH_DEBILITATION), FALSE);
        player->hp = 0;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_DEBILITATION), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Debilitation applies all six ailments through type and ability prevention")
{
    u32 species, ability;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; ability = ABILITY_NONE; }
    PARAMETRIZE { species = SPECIES_CHARIZARD; ability = ABILITY_NONE; }
    PARAMETRIZE { species = SPECIES_MAGNETON; ability = ABILITY_NONE; }
    PARAMETRIZE { species = SPECIES_GLALIE; ability = ABILITY_NONE; }
    PARAMETRIZE { species = SPECIES_RAICHU; ability = ABILITY_NONE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; ability = ABILITY_PURIFYING_SALT; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; ability = ABILITY_INSOMNIA; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; ability = ABILITY_MAGIC_BOUNCE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(species) { Ability(ability); Speed(100); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        ExpectDebilitationStatuses(sStatusesAtApplication);
        ExpectNoOrderChangeIcon(B_POSITION_PLAYER_LEFT, TRUE);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_DOMINATION), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_ENDURANCE), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_DEBILITATION), FALSE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_NONE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Debilitation cures leave No Order Change intact")
{
    u32 item;
    PARAMETRIZE { item = ITEM_FULL_RESTORE; }
    PARAMETRIZE { item = ITEM_FULL_HEAL; }
    PARAMETRIZE { item = ITEM_ANTIDOTE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { USE_ITEM(player, item, partyIndex: 0); }
    } THEN {
        ExpectNoOrderChangeIcon(B_POSITION_PLAYER_LEFT, TRUE);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        if (item == ITEM_ANTIDOTE)
        {
            EXPECT_EQ(player->status1 & STATUS1_PSN_ANY, 0);
            EXPECT_NE(player->status1 & STATUS1_BURN, 0);
            EXPECT_NE(player->status1 & STATUS1_FREEZE, 0);
            EXPECT_NE(player->status1 & STATUS1_FROSTBITE, 0);
        }
        else
        {
            EXPECT_EQ(player->status1, 0);
            EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), 0);
        }
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Debilitation bypasses Safeguard and Misty Terrain")
{
    u32 protection;
    PARAMETRIZE { protection = MOVE_SAFEGUARD; }
    PARAMETRIZE { protection = MOVE_MISTY_TERRAIN; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        sWishTurn = 1;
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); Moves(protection, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, protection); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ExpectDebilitationStatuses(sStatusesAtApplication);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Debilitation preserves existing sleep and Toxic counters")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        sSeedToxic = TRUE;
        PLAYER(SPECIES_WOBBUFFET) {
            Ability(ABILITY_NONE); Speed(100); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE);
            Status1(STATUS1_SLEEP_TURN(3) | STATUS1_TOXIC_POISON);
        }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ExpectDebilitationStatuses(sStatusesAtApplication);
        EXPECT_EQ(sStatusesAtApplication & STATUS1_SLEEP, STATUS1_SLEEP_TURN(2));
        EXPECT_EQ(sStatusesAtApplication & STATUS1_TOXIC_COUNTER, STATUS1_TOXIC_TURN(2));
        EXPECT_NE(sStatusesAtApplication & STATUS1_TOXIC_POISON, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Debilitation interruption does not apply or spend Wish")
{
    u32 move;
    PARAMETRIZE { move = MOVE_TAUNT; }
    PARAMETRIZE { move = MOVE_AIR_SLASH; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Speed(100); Moves(move); }
        OPPONENT(SPECIES_CASTORIA) { Ability(ABILITY_RUN_AWAY); Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        EXPECT_EQ(player->status1, 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), FALSE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        ExpectNoOrderChangeIcon(B_POSITION_PLAYER_LEFT, FALSE);
        // An interrupted Wish never supplies an application for the observer.
        u32 taskId = FindTaskIdByFunc(Task_ObserveDebilitation);
        if (taskId != TASK_NONE)
            DestroyTask(taskId);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change suppresses damaging pivots but preserves their damage")
{
    u32 move;
    PARAMETRIZE { move = MOVE_U_TURN; }
    PARAMETRIZE { move = MOVE_VOLT_SWITCH; }
    PARAMETRIZE { move = MOVE_FLIP_TURN; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_GENGAR) { Item(ITEM_SHED_SHELL); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_LT(opponent->hp, opponent->maxHP);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        EXPECT_EQ(AI_CanBattlerEscape(B_POSITION_PLAYER_LEFT), FALSE);
        EXPECT_EQ(IsBattlerTrapped(B_POSITION_OPPONENT_LEFT, B_POSITION_PLAYER_LEFT), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change preserves Parting Shot stat drops")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_PARTING_SHOT); }
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change fails pure switches and Lunar Dance with extra bars")
{
    u32 move;
    PARAMETRIZE { move = MOVE_BATON_PASS; }
    PARAMETRIZE { move = MOVE_TELEPORT; }
    PARAMETRIZE { move = MOVE_SHED_TAIL; }
    PARAMETRIZE { move = MOVE_LUNAR_DANCE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(100); Speed(100); Moves(move); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, move, player);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(player->pp[0], GetMovePP(move) - 1);
        EXPECT_EQ((u32)gBattleStruct->battlerState[B_POSITION_PLAYER_LEFT].storedLunarDance, FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change allows Lunar Dance genuine faint and replacement")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { HP(1); MaxHP(100); Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        MESSAGE("Wynaut became cloaked in mystical moonlight!");
    } THEN {
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 1);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), FALSE);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change survives sacrificial bar breaks without blocking the move")
{
    u32 move;
    PARAMETRIZE { move = MOVE_EXPLOSION; }
    PARAMETRIZE { move = MOVE_SELF_DESTRUCT; }
    PARAMETRIZE { move = MOVE_MEMENTO; }
    PARAMETRIZE { move = MOVE_HEALING_WISH; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(100); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
    } THEN {
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change blocks phazing but preserves phazing attack damage")
{
    u32 move;
    PARAMETRIZE { move = MOVE_ROAR; }
    PARAMETRIZE { move = MOVE_WHIRLWIND; }
    PARAMETRIZE { move = MOVE_DRAGON_TAIL; }
    PARAMETRIZE { move = MOVE_CIRCLE_THROW; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(move); Speed(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, move); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        if (move == MOVE_DRAGON_TAIL || move == MOVE_CIRCLE_THROW)
            EXPECT_LT(player->hp, player->maxHP);
        else
            EXPECT_EQ(player->hp, player->maxHP);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change prevents Red Card from moving its affected attacker")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(opponent->item, ITEM_RED_CARD);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change does not stop a trapped Red Card holder moving an untrapped attacker")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_OPPONENT_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Item(ITEM_RED_CARD); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 1);
        EXPECT_EQ(opponent->item, ITEM_NONE);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_OPPONENT_LEFT), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change prevents switching items and abilities without consuming items")
{
    u32 item, ability, move;
    PARAMETRIZE { item = ITEM_EJECT_BUTTON; ability = ABILITY_NONE; move = MOVE_SEISMIC_TOSS; }
    PARAMETRIZE { item = ITEM_EJECT_PACK; ability = ABILITY_NONE; move = MOVE_GROWL; }
    PARAMETRIZE { item = ITEM_NONE; ability = ABILITY_EMERGENCY_EXIT; move = MOVE_SEISMIC_TOSS; }
    PARAMETRIZE { item = ITEM_NONE; ability = ABILITY_WIMP_OUT; move = MOVE_SEISMIC_TOSS; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Ability(ability); HP(100); MaxHP(100); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(60); Moves(move); Speed(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, move); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(player->item, item);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change persists through a break and clears on genuine faint replacement")
{
    bool32 genuineFaint;
    PARAMETRIZE { genuineFaint = FALSE; }
    PARAMETRIZE { genuineFaint = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(100); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Level(100); Moves(MOVE_SEISMIC_TOSS); Speed(10); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_SEISMIC_TOSS); }
        if (genuineFaint)
            TURN { EXPECT_MOVE(opponent, MOVE_SEISMIC_TOSS); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), !genuineFaint);
        ExpectNoOrderChangeIcon(B_POSITION_PLAYER_LEFT, !genuineFaint);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], genuineFaint ? 1 : 0);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
    }
}

AI_DOUBLE_BATTLE_TEST("Arcanum No Order Change prevents Ally Switch for either affected partner")
{
    u32 trapped;
    PARAMETRIZE { trapped = B_POSITION_PLAYER_LEFT; }
    PARAMETRIZE { trapped = B_POSITION_PLAYER_RIGHT; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, trapped, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(20); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ALLY_SWITCH); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ALLY_SWITCH, playerLeft);
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_RIGHT], 1);
        EXPECT_EQ(HasNoOrderChange(trapped), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change preserves Chilly Reception weather without withdrawing")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_SLOWKING_GALAR) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_CHILLY_RECEPTION); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CHILLY_RECEPTION, player);
#if B_PREFERRED_ICE_WEATHER == B_ICE_WEATHER_HAIL
        MESSAGE("It started to hail!");
#else
        MESSAGE("It started to snow!");
#endif
        NOT MESSAGE("Slowking went back to 1!");
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change blocks Gen 4 Healing Wish withdrawal after its bar breaks")
{
    GIVEN {
        WITH_CONFIG(CONFIG_HEALING_WISH_SWITCH, GEN_4);
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, B_POSITION_PLAYER_LEFT, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(100); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_NONE); Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_HEALING_WISH); }
    } THEN {
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(HasNoOrderChange(B_POSITION_PLAYER_LEFT), TRUE);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ((u32)gBattleStruct->battlerState[B_POSITION_PLAYER_LEFT].storedHealingWish, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change indicator follows healthbox movement and visibility")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_PLAYER_LEFT;
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
        ExpectNoOrderChangeIcon(battler, TRUE);
        if (spriteId < MAX_SPRITES)
        {
            struct Sprite *icon = &gSprites[spriteId];
            struct Sprite *healthbox = &gSprites[gHealthboxSpriteIds[battler]];
            s16 originalX = healthbox->x;
            s16 originalY = healthbox->y;
            s16 originalX2 = healthbox->x2;
            s16 originalY2 = healthbox->y2;
            s16 offsetX = icon->x - healthbox->x;
            s16 offsetY = icon->y - healthbox->y;

            // Repeated status refreshes must not allocate duplicate indicators.
            RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE), spriteId);

            healthbox->x += 7;
            healthbox->y -= 3;
            healthbox->x2 = 13;
            healthbox->y2 = -4;
            icon->callback(icon);
            EXPECT_EQ(icon->x - healthbox->x, offsetX);
            EXPECT_EQ(icon->y - healthbox->y, offsetY);
            EXPECT_EQ(icon->x2, healthbox->x2);
            EXPECT_EQ(icon->y2, healthbox->y2);

            SetHealthboxSpriteInvisible(gHealthboxSpriteIds[battler]);
            icon->callback(icon);
            EXPECT_EQ((u32)icon->invisible, TRUE);
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[battler]);
            healthbox->x = originalX;
            healthbox->y = originalY;
            healthbox->x2 = originalX2;
            healthbox->y2 = originalY2;
            icon->callback(icon);
            ExpectNoOrderChangeIcon(battler, TRUE);

            // A zero-HP pending bar break is not a reason to hide the condition.
            u32 hp = player->hp;
            player->hp = 0;
            icon->callback(icon);
            ExpectNoOrderChangeIcon(battler, TRUE);
            player->hp = hp;
        }
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum No Order Change indicator reloads after its graphics are discarded")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(TRUE, B_POSITION_PLAYER_LEFT, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); HP(1000); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_PLAYER_LEFT;
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
        ExpectNoOrderChangeIcon(battler, TRUE);
        if (spriteId < MAX_SPRITES)
        {
            u32 tileTag = gSprites[spriteId].template->tileTag;
            u32 paletteTag = gSprites[spriteId].template->paletteTag;
            FreeBattleStatusIcons();
            EXPECT_EQ(GetSpriteTileStartByTag(tileTag), 0xFFFF);
            EXPECT_EQ(IndexOfSpritePaletteTag(paletteTag), 0xFF);
            EXPECT_EQ(HasNoOrderChange(battler), TRUE);

            // The normal healthbox rebuild recreates the icon from battle state.
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], &gPlayerParty[0], HEALTHBOX_ALL);
            ExpectNoOrderChangeIcon(battler, TRUE);
            EXPECT_NE(GetSpriteTileStartByTag(tileTag), 0xFFFF);
            EXPECT_NE(IndexOfSpritePaletteTag(paletteTag), 0xFF);
        }
    }
}

AI_DOUBLE_BATTLE_TEST("Arcanum No Order Change indicator fits every doubles healthbox")
{
    u32 battler;
    PARAMETRIZE { battler = B_POSITION_PLAYER_LEFT; }
    PARAMETRIZE { battler = B_POSITION_OPPONENT_LEFT; }
    PARAMETRIZE { battler = B_POSITION_PLAYER_RIGHT; }
    PARAMETRIZE { battler = B_POSITION_OPPONENT_RIGHT; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        DebilitationFixture(FALSE, battler, 0);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_WYNAUT) { Moves(MOVE_CELEBRATE); Speed(20); }
    } WHEN {
        TURN { }
    } THEN {
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
        ExpectNoOrderChangeIcon(battler, TRUE);
        if (spriteId < MAX_SPRITES)
        {
            EXPECT_GE(gSprites[spriteId].x - 8, 0);
            EXPECT_LE(gSprites[spriteId].x + 8, DISPLAY_WIDTH);
            EXPECT_GE(gSprites[spriteId].y - 8, 0);
            EXPECT_LE(gSprites[spriteId].y + 8, DISPLAY_HEIGHT);
        }
        for (u32 other = 0; other < gBattlersCount; other++)
            if (other != battler)
                ExpectNoOrderChangeIcon(other, FALSE);
    }
}
