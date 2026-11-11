#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_break_bar.h"
#include "battle_interface.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "castoria_name_theft.h"
#include "sprite.h"
#include "reshow_battle_screen.h"
#include "window.h"
#include "main.h"
#include "party_menu.h"
#include "battle_main.h"
#include "battle_controllers.h"
#include "task.h"
#include "party_menu.h"
#include "battle_status_icons.h"

extern AiScoreFunc sDynamicAiFunc;
extern void BS_TryRevivalBlessing(void);
extern void BS_TryRevivalBlessing2(void);
extern void BS_ItemRestoreHP(void);

static bool8 sSetup;
static bool8 sAlterSpeed;
static bool8 sReshow;
static bool8 sReviveAlly;
static u8 sWishTurn;
static u16 sEntryAbility;
static u16 sPostSummonSpell;
static u16 sTxOrdinaryMove;
static u8 sAmbushEffect;
static bool8 sSeedConditions;
static bool8 sObstructCreation;
static bool8 sObserveSlide;
static bool8 sSawSlideFrame;
static u32 sStatusAtAmbush;
static u32 sAmbushCalls;
static u32 sAmbushTurn;
static u32 sAmbushCursor;
static u32 sPlayerPpAtAmbush;
static u32 sAmbushBars;
static bool8 sAmbushNoOrderChange;
static BattleControllerFunc sReshowController;

static void Task_ObserveRollbackSlide(u8 taskId)
{
    u32 caster = B_POSITION_OPPONENT_LEFT;
    u32 sprite = gBattlerSpriteIds[caster];
    if (gBattleStruct->txRollback.stage == TX_ROLLBACK_ENTERING
     && sprite < MAX_SPRITES && gSprites[sprite].x > 176 && gSprites[sprite].x < 200)
        sSawSlideFrame = TRUE;
    if (gBattleStruct->txRollback.stage >= TX_ROLLBACK_AMBUSH_PENDING)
        DestroyTask(taskId);
}

static void Task_ObstructRollbackCreation(u8 taskId)
{
    if (gBattleStruct->txRollback.stage == TX_ROLLBACK_REQUESTED)
    {
        CreateMon(&gEnemyParty[PARTY_SIZE - 1], SPECIES_PICHU, 50, 31, TRUE, 0, OT_ID_PRESET, 123);
        DestroyTask(taskId);
    }
}

static void Task_SeedRollbackConditions(u8 taskId)
{
    if (gBattleStruct->txRollback.stage == TX_ROLLBACK_AMBUSH_PENDING)
    {
        gBattleMons[3].status1 = STATUS1_SLEEP_TURN(3) | STATUS1_FREEZE | STATUS1_PARALYSIS;
        SetMonData(&gEnemyParty[PARTY_SIZE - 1], MON_DATA_STATUS, &gBattleMons[3].status1);
        gBattleMons[3].volatiles.flinched = TRUE;
        gDisableStructs[3].tauntTimer = 3;
        DestroyTask(taskId);
    }
}

static void WaitForTestReshow(u32 battler)
{
    if (gMain.callback2 == BattleMainCB2)
    {
        gBattlerControllerFuncs[battler] = sReshowController;
        gBattleControllerExecFlags &= ~(1u << battler);
    }
}

static void BS_TestMoreWishesAmbush(void)
{
    NATIVE_ARGS();
    sAmbushCalls++;
    sAmbushTurn = gBattleResults.battleTurnCounter;
    sAmbushCursor = gCurrentTurnActionNumber;
    sPlayerPpAtAmbush = gBattleMons[B_POSITION_PLAYER_LEFT].pp[0];
    sAmbushBars = GetBreakBarCount(gBattlerAttacker);
    sAmbushNoOrderChange = HasNoOrderChange(gBattlerAttacker);
    sStatusAtAmbush = gBattleMons[gBattlerAttacker].status1;
    if (sAmbushEffect != 0)
    {
        if (sAmbushEffect == 1)
            GrantSavedBreakBar(B_POSITION_PLAYER_LEFT);
        gBattleMons[B_POSITION_PLAYER_LEFT].hp = 0;
        SetMonData(&gPlayerParty[gBattlerPartyIndexes[0]], MON_DATA_HP, &gBattleMons[0].hp);
    }
    if (sAlterSpeed)
    {
        // A scripted effect must not cause the pending player action to vanish.
        gBattleMons[B_POSITION_PLAYER_LEFT].speed = 1;
        gBattleMons[gBattlerAttacker].speed = 999;
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
    if (sReshow)
    {
        sReshowController = gBattlerControllerFuncs[0];
        gBattlerControllerFuncs[0] = WaitForTestReshow;
        MarkBattlerForControllerExec(0);
        FreeAllWindowBuffers(); // The party-menu transition frees these before returning.
        ReshowBattleScreenAfterMenu();
    }
}

static const struct __attribute__((packed))
{
    u8 callnative;
    void (*func)(void);
    u8 pause;
    u16 frames;
    u8 tryfaintmon;
    u8 target;
    bool8 isSpikes;
    const u8 *faintInstr;
    u8 ret;
} sAmbushScript = {0xff, BS_TestMoreWishesAmbush, 0x39, 3, 0x19, BS_TARGET, FALSE, NULL, 0x3c};

static const struct TxRollbackProfile sProfile =
{
    .ambushScript = (const u8 *)&sAmbushScript,
};

static s32 MoreWishesScore(u32 attacker, u32 target, u32 move, s32 score)
{
    if (sSetup && gBattleMons[attacker].species == SPECIES_CASTORIA)
    {
        sSetup = FALSE;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        gBattleStruct->txRollback.profile = &sProfile;
        if (sEntryAbility != ABILITY_NONE)
            gBattleTestRunnerState->data.forcedAbilities[B_SIDE_OPPONENT][PARTY_SIZE - 1] = sEntryAbility;
        if (sSeedConditions)
            CreateTask(Task_SeedRollbackConditions, 0);
        if (sObstructCreation)
            CreateTask(Task_ObstructRollbackCreation, 0);
        if (sObserveSlide)
            CreateTask(Task_ObserveRollbackSlide, 0);
    }
    if (BattleAI_IsScoringArcanum())
    {
        if (gBattleStruct->txRollback.stage == TX_ROLLBACK_ACTIVE
         && move == sPostSummonSpell)
            return 250;
        if (move == MOVE_ARCANUM_WISH && gBattleResults.battleTurnCounter >= sWishTurn
         && CastoriaCanWishFor(attacker, CASTORIA_WISH_MORE_WISHES))
        {
            gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_MORE_WISHES;
            return 200;
        }
        return 0;
    }
    if (sReviveAlly && gBattleStruct->txRollback.stage == TX_ROLLBACK_DEFEATED
     && gBattleMons[attacker].species == SPECIES_CASTORIA && move == MOVE_REVIVAL_BLESSING_2)
        return 200;
    return move == (TxRollbackHasWishes(attacker) ? sTxOrdinaryMove : MOVE_CELEBRATE) ? 100 : 0;
}

static void MoreWishesFixture(void)
{
    sDynamicAiFunc = MoreWishesScore;
    sSetup = TRUE;
    sAlterSpeed = FALSE;
    sReshow = FALSE;
    sReviveAlly = FALSE;
    sWishTurn = 0;
    sEntryAbility = ABILITY_NONE;
    sPostSummonSpell = MOVE_NONE;
    sTxOrdinaryMove = MOVE_WIDE_GUARD;
    sAmbushEffect = 0;
    sSeedConditions = FALSE;
    sObstructCreation = FALSE;
    sObserveSlide = FALSE;
    sSawSlideFrame = FALSE;
    sStatusAtAmbush = 0;
    sAmbushCalls = 0;
    sAmbushBars = 0;
    sAmbushNoOrderChange = FALSE;
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes converts live Singles and preserves the pending action")
{
    bool32 casterFirst;
    bool32 alterSpeed;
    PARAMETRIZE { casterFirst = TRUE; alterSpeed = FALSE; }
    PARAMETRIZE { casterFirst = FALSE; alterSpeed = FALSE; }
    PARAMETRIZE { casterFirst = TRUE; alterSpeed = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sAlterSpeed = alterSpeed;
        sObserveSlide = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(600); HP(600); Speed(casterFirst ? 10 : 40); MovesWithPP({MOVE_SEISMIC_TOSS, 5}); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { MaxHP(600); HP(600); Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }

    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        if (!casterFirst) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, player);
            HP_BAR(opponent);
        }
        MESSAGE("The opposing Caster? cast Wish!");
        MESSAGE("The wish created TX Rollback!");
        if (casterFirst) {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, player);
            HP_BAR(opponent);
        }
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, player);
    } THEN {
        EXPECT_EQ(gBattlersCount, 4);
        EXPECT_EQ(gPlayerPartyCount, 2);
        EXPECT_EQ(gEnemyPartyCount, 6);
        EXPECT_EQ(gBattleMons[3].species, SPECIES_TX_ROLLBACK);
        EXPECT_EQ(gBattleMons[3].moves[0], MOVE_FOLLOW_ME);
        EXPECT_EQ(gBattleMons[3].moves[1], MOVE_WIDE_GUARD);
        EXPECT_EQ(gBattleMons[3].moves[2], MOVE_LIFE_DEW);
        EXPECT_EQ(gBattleMons[3].moves[3], MOVE_POWER_GEM);
        EXPECT_EQ(gBattlerPartyIndexes[3], PARTY_SIZE - 1);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_ACTIVE);
        EXPECT_EQ(sSawSlideFrame, TRUE);
        EXPECT_EQ(gSprites[gBattlerSpriteIds[1]].x, 200);
        EXPECT_EQ(sAmbushCalls, 1);
        EXPECT_EQ(sAmbushTurn, 0);
        EXPECT_EQ(sAmbushCursor, casterFirst ? 0 : 1);
        EXPECT_EQ(sPlayerPpAtAmbush, casterFirst ? 5 : 4);
        EXPECT_EQ(player->pp[0], 4);
        EXPECT_LT(opponent->hp, opponent->maxHP);
        EXPECT_EQ(sAmbushBars, 2);
        EXPECT_EQ(sAmbushNoOrderChange, TRUE);
        EXPECT_EQ(GetBreakBarCount(3), 2);
        EXPECT_EQ(HasNoOrderChange(3), TRUE);
        EXPECT_EQ(IsBattlerAlive(2), FALSE);
        EXPECT_EQ(TxRollbackGetSlotPolicy(2), TX_SLOT_DISABLED);
        EXPECT_EQ(HasNoMonsToSwitch(2, PARTY_SIZE, PARTY_SIZE), TRUE);
        EXPECT_EQ(CanBattlerSwitch(3), FALSE);
        EXPECT_EQ(GetBattlerCoordsIndex(0), BATTLE_COORDS_SINGLES);
        EXPECT_EQ(gBattlerSpriteIds[2], MAX_SPRITES);
        EXPECT_EQ(gHealthboxSpriteIds[2], MAX_SPRITES);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes joins ordinary selection next turn without repeating the ambush")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }

    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(&gBattleMons[3], MOVE_WIDE_GUARD); }
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(&gBattleMons[3], MOVE_WIDE_GUARD); }
    } THEN {
        EXPECT_EQ(sAmbushCalls, 1);
        EXPECT_EQ(gBattleMons[3].pp[1], GetMovePP(MOVE_WIDE_GUARD) - 2);
        EXPECT_EQ(gBattleMons[3].pp[0], GetMovePP(MOVE_FOLLOW_ME));
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_ACTIVE);
        EXPECT_EQ(IsBattlerAlive(2), FALSE);
        EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_HP), GetMonData(&gPlayerParty[1], MON_DATA_MAX_HP));
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish More Wishes requires an explicit supported profile and fake name")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        for (u32 route = CASTORIA_NAME_NONE; route <= CASTORIA_NAME_TRUE; route++)
        {
            gBattleStruct->castoria.nameResult = route;
            EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_MORE_WISHES), FALSE);
            gBattleStruct->txRollback.profile = &sProfile;
            EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_MORE_WISHES), route == CASTORIA_NAME_FALSE);
            gBattleStruct->castoria.wishUsed = TRUE;
            EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_MORE_WISHES), FALSE);
            gBattleStruct->castoria.wishUsed = FALSE;
            gBattleStruct->txRollback.profile = NULL;
        }
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes skips hazards but resolves entry abilities and items")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sWishTurn = 3;
        sEntryAbility = ABILITY_INTIMIDATE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(40); Item(ITEM_WHITE_HERB); Moves(MOVE_SPIKES, MOVE_TOXIC_SPIKES, MOVE_STEALTH_ROCK, MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPIKES); }
        TURN { MOVE(player, MOVE_TOXIC_SPIKES); }
        TURN { MOVE(player, MOVE_STEALTH_ROCK); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The wish created TX Rollback!");
        ABILITY_POPUP(&gBattleMons[3], ABILITY_INTIMIDATE);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_HELD_ITEM_EFFECT, player);
        NOT HP_BAR(&gBattleMons[3]);
    } THEN {
        EXPECT_EQ(gBattleMons[3].hp, gBattleMons[3].maxHP);
        EXPECT_EQ(gBattleMons[3].status1, 0);
        EXPECT_EQ(gBattleMons[3].ability, ABILITY_INTIMIDATE);
        EXPECT_EQ(player->item, ITEM_NONE);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(HasNoOrderChange(3), TRUE);
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes rebuilds the screen without creating a second player")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sReshow = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); MovesWithPP({MOVE_SEISMIC_TOSS, 5}); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SEISMIC_TOSS, target: opponent); }
    } THEN {
        EXPECT_EQ(sAmbushCalls, 1);
        EXPECT_EQ(player->pp[0], 3);
        EXPECT_EQ(gBattlerSpriteIds[2], MAX_SPRITES);
        EXPECT_EQ(gHealthboxSpriteIds[2], MAX_SPRITES);
        EXPECT_LT(gBattlerSpriteIds[3], MAX_SPRITES);
        EXPECT_LT(gHealthboxSpriteIds[3], MAX_SPRITES);
        EXPECT_EQ(GetBreakBarCount(3), 2);
        EXPECT_EQ(HasNoOrderChange(3), TRUE);
        EXPECT_LT(Test_GetBattleStatusIconSpriteId(3, BATTLE_STATUS_ICON_NO_ORDER_CHANGE), MAX_SPRITES);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes retires its defeated slot while Castoria revives another ally")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sReviveAlly = TRUE;
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(10); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(20); Moves(MOVE_CELEBRATE, MOVE_REVIVAL_BLESSING_2); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing TX Rollback fainted!");
        NOT MESSAGE("The opposing Wobbuffet was sent out!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(3), 0);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_DEFEATED);
        EXPECT_EQ(TxRollbackGetSlotPolicy(3), TX_SLOT_RETIRED);
        EXPECT_EQ(IsBattlerAlive(3), FALSE);
        EXPECT_EQ(IsBattlerAlive(1), TRUE);
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_PLAYER_TELEPORTED);
        EXPECT_EQ(GetMonData(&gEnemyParty[1], MON_DATA_HP), GetMonData(&gEnemyParty[1], MON_DATA_MAX_HP));
        EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, PARTY_SIZE - 1), FALSE);
        EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 1), TRUE);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
        gBattleStruct->castoria.wishUsed = FALSE;
        EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_MORE_WISHES), FALSE);
        // Even stale/preselected revival targets must fail at execution.
        const u8 fail[] = {0};
        const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
            const u8 *failInstr;
            const u8 *restoreInstr;
        } command = {0xff, NULL, fail, fail};
        const u8 *savedScript = gBattlescriptCurrInstr;
        gBattlerAttacker = 1;
        for (u32 i = 0; i < 2; i++)
        {
            gSelectedMonPartyId = PARTY_SIZE - 1;
            gBattlescriptCurrInstr = (const u8 *)&command;
            if (i == 0)
                BS_TryRevivalBlessing();
            else
                BS_TryRevivalBlessing2();
            EXPECT_EQ(gBattlescriptCurrInstr, fail);
            EXPECT_EQ(GetMonData(&gEnemyParty[PARTY_SIZE - 1], MON_DATA_HP), 0);
        }
        gLastUsedItem = ITEM_MAX_REVIVE;
        gBattleStruct->itemPartyIndex[1] = PARTY_SIZE - 1;
        gBattlescriptCurrInstr = (const u8 *)&command;
        BS_ItemRestoreHP();
        EXPECT_EQ(gBattlescriptCurrInstr, fail);
        EXPECT_EQ(GetMonData(&gEnemyParty[PARTY_SIZE - 1], MON_DATA_HP), 0);
        gBattlescriptCurrInstr = savedScript;
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes keeps TX Rollback fighting when Castoria falls first")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(10); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: opponent); }
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(&gBattleMons[3], MOVE_WIDE_GUARD); }
    } THEN {
        EXPECT_EQ(IsBattlerAlive(1), FALSE);
        EXPECT_EQ(IsBattlerAlive(3), TRUE);
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_PLAYER_TELEPORTED);
        EXPECT_EQ(GetBreakBarCount(3), 2);
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes replaces only player-left and does not auto-enter a revived reserve")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE, MOVE_MEMENTO); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_MEMENTO, target: opponent); SEND_OUT(player, 1); }
        TURN { USE_ITEM(player, ITEM_MAX_REVIVE, partyIndex: 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[0], 1);
        EXPECT_EQ(IsBattlerAlive(0), TRUE);
        EXPECT_EQ(IsBattlerAlive(2), FALSE);
        EXPECT_EQ(gBattlerSpriteIds[2], MAX_SPRITES);
        EXPECT_EQ(gHealthboxSpriteIds[2], MAX_SPRITES);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP));
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes resolves a bar broken by its scripted ambush before the pending move")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sAmbushEffect = 1;
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); MovesWithPP({MOVE_SEISMIC_TOSS, 5}); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        MESSAGE("The wish created TX Rollback!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, player);
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(player->hp, player->maxHP);
        EXPECT_EQ(GetBreakBarCount(0), 0);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 0);
        EXPECT_EQ(player->pp[0], 4);
        EXPECT_EQ(sAmbushCalls, 1);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_ACTIVE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes ambush fainting does not give the replacement its predecessor's action")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sAmbushEffect = 2;
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); MovesWithPP({MOVE_SEISMIC_TOSS, 5}); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); SEND_OUT(player, 1); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The wish created TX Rollback!");
        MESSAGE("Wobbuffet fainted!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SEISMIC_TOSS, player);
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[0], 1);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 1);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_PP1), 5);
        EXPECT_EQ(IsBattlerAlive(2), FALSE);
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes ambush ignores action blockers without consuming an ordinary status turn")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sSeedConditions = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(sAmbushCalls, 1);
        EXPECT_EQ(sStatusAtAmbush & STATUS1_SLEEP, STATUS1_SLEEP_TURN(3));
        EXPECT_NE(sStatusAtAmbush & STATUS1_FREEZE, 0);
        EXPECT_EQ(gBattleMons[3].status1 & STATUS1_SLEEP, STATUS1_SLEEP_TURN(2));
        EXPECT_NE(gBattleMons[3].status1 & STATUS1_FREEZE, 0);
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
            EXPECT_EQ(gBattleMons[3].pp[i], GetMovePP(gBattleMons[3].moves[i]));
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes victory requires both combatants in either knockout order")
{
    bool32 casterFirst;
    PARAMETRIZE { casterFirst = TRUE; }
    PARAMETRIZE { casterFirst = FALSE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(10); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        if (casterFirst) TURN { MOVE(player, MOVE_SHEER_COLD, target: opponent); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        if (!casterFirst) TURN { MOVE(player, MOVE_SHEER_COLD, target: opponent); }
    } SCENE {
        if (casterFirst) MESSAGE("The opposing Caster? fainted!");
        MESSAGE("The opposing TX Rollback fainted!");
        if (!casterFirst) MESSAGE("The opposing Caster? fainted!");
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 2);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_DEFEATED);
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes handles simultaneous enemy KOs without refilling retired or disabled slots")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        // Wide Guard would block Misty Explosion and leave both foes alive,
        // requiring an unrecorded player replacement rather than ending battle.
        // Follow Me is a legal summoned move and allows the simultaneous KOs.
        sTxOrdinaryMove = MOVE_FOLLOW_ME;
        PLAYER(SPECIES_ARTICUNO) { Level(100); SpAttack(9999); Speed(10); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD, MOVE_MISTY_EXPLOSION); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_MISTY_EXPLOSION); EXPECT_MOVE(&gBattleMons[3], MOVE_FOLLOW_ME); }
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 2);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 1);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_DEFEATED);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes failure during preparation does not spend Wish or convert the battle")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sObstructCreation = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        MESSAGE("But it failed!");
        NOT MESSAGE("The wish created TX Rollback!");
    } THEN {
        EXPECT_EQ(gBattlersCount, 2);
        EXPECT_EQ(IsDoubleBattle(), FALSE);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(sAmbushCalls, 0);
        EXPECT_EQ(GetMonData(&gEnemyParty[PARTY_SIZE - 1], MON_DATA_SPECIES), SPECIES_PICHU);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes interruption never creates its creature")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); Status1(STATUS1_SLEEP_TURN(3)); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattlersCount, 2);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(sAmbushCalls, 0);
        EXPECT_EQ(GetMonData(&gEnemyParty[PARTY_SIZE - 1], MON_DATA_SPECIES), SPECIES_NONE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish More Wishes encounter properties do not belong to its species")
{
    GIVEN {
        PLAYER(SPECIES_TX_ROLLBACK);
        OPPONENT(SPECIES_JIRACHI);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_NE(SPECIES_TX_ROLLBACK, SPECIES_JIRACHI);
        EXPECT_EQ(GetBreakBarCount(0), 0);
        EXPECT_EQ(HasNoOrderChange(0), FALSE);
        EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_PLAYER, 0), TRUE);
        EXPECT_EQ(gSpeciesInfo[SPECIES_TX_ROLLBACK].baseHP, 125);
        EXPECT_EQ(gSpeciesInfo[SPECIES_TX_ROLLBACK].abilities[0], ABILITY_WISHFUL_THINKING);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes preserves Castoria's virtual Arcanum selection in doubles")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        sPostSummonSpell = MOVE_FIREBALL;
        PLAYER(SPECIES_WOBBUFFET) { HP(600); MaxHP(600); Speed(10); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The wish created TX Rollback!");
        MESSAGE("The opposing Caster? cast Fireball!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FIREBALL, opponent);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(gBattleStruct->chosenMovePositions[1], MAX_MON_MOVES);
        EXPECT_EQ(gChosenMoveByBattler[1], MOVE_FIREBALL);
        EXPECT_EQ(opponent->moves[0], MOVE_CELEBRATE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(sAmbushCalls, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish More Wishes simultaneous player and creation fainting replaces only the player's occupied position")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        MoreWishesFixture();
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(10); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD, MOVE_FINAL_GAMBIT); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(20); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(5); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_FINAL_GAMBIT, target: &gBattleMons[3]); SEND_OUT(player, 1); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[0], 1);
        EXPECT_EQ(gBattlerPartyIndexes[1], 0);
        EXPECT_EQ(gBattlerPartyIndexes[3], PARTY_SIZE - 1);
        EXPECT_EQ(IsBattlerAlive(0), TRUE);
        EXPECT_EQ(IsBattlerAlive(1), TRUE);
        EXPECT_EQ(IsBattlerAlive(2), FALSE);
        EXPECT_EQ(IsBattlerAlive(3), FALSE);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 1);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
        EXPECT_EQ(TxRollbackGetSlotPolicy(3), TX_SLOT_RETIRED);
    }
}
