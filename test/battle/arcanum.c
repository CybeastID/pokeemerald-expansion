#include "global.h"
#include "test/battle.h"
#include "castoria_name_theft.h"
#include "battle_ai_util.h"
#include "battle_controllers.h"

// Script-native entry point; normally referenced only from assembly.
extern void BS_TryInstruct(void);
extern AiScoreFunc sDynamicAiFunc;
extern s32 AI_RazeluxeTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);
extern s32 AI_AliceTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);

static s32 sArcanumTestSpellScore;
static s32 sArcanumTestRegularScore;
static u32 sArcanumTestPreferredMove;
static u32 sArcanumTestSpellCalls;
static u32 sArcanumTestRegularCalls;
static u32 sArcanumTestSeenSpells;
static bool32 sArcanumTestUsedVirtualIndex;

static s32 ArcanumTestDynamicScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (gAiThinkingStruct->movesetIndex >= MAX_MON_MOVES)
        sArcanumTestUsedVirtualIndex = TRUE;

    if (BattleAI_IsScoringArcanum())
    {
        u32 slot;
        sArcanumTestSpellCalls++;
        for (slot = 0; slot < CASTORIA_ARCANUM_COUNT; slot++)
        {
            if (move == Test_GetCastoriaArcanumMove(slot))
                sArcanumTestSeenSpells |= 1u << slot;
        }
        return move == sArcanumTestPreferredMove ? sArcanumTestSpellScore : 0;
    }

    sArcanumTestRegularCalls++;
    return sArcanumTestRegularScore;
}

static u32 ArcanumTestChooseMove(u32 battler, u64 flags, AiScoreFunc callback)
{
    sDynamicAiFunc = callback;
    gAiThinkingStruct->aiFlags[battler] = flags;
    sArcanumTestSpellCalls = 0;
    sArcanumTestRegularCalls = 0;
    sArcanumTestSeenSpells = 0;
    sArcanumTestUsedVirtualIndex = FALSE;
    SetAiLogicDataForTurn(gAiLogicData);
    BattleAI_SetupAIData(0xF, battler);
    return BattleAI_ChooseMoveIndex(battler);
}

ASSUMPTIONS
{
    ASSUME(Test_GetCastoriaArcanumMove(0) == MOVE_FIREBALL);
    ASSUME(Test_GetCastoriaArcanumMove(1) == MOVE_REVIVAL_BLESSING_2);
}

// Preserve production Wish scoring, but isolate it from the other spell policies.
static s32 ArcanumTestWishScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (!BattleAI_IsScoringArcanum())
        return sArcanumTestRegularScore;
    if (move == MOVE_ARCANUM_WISH)
        return AI_RazeluxeTrainer(battlerAtk, battlerDef, move, score);
    return 0;
}

static bool8 sArcanumWishSetup;
static bool8 sArcanumWishDelay;

static s32 ArcanumTestWishBattleScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    // Test fixture only: start her first decision after true-name seals ran out.
    if (sArcanumWishSetup && !IsOnPlayerSide(battlerAtk))
    {
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        gBattleMons[battlerAtk].volatiles.castoriaTrueNameCharges = 0;
        sArcanumWishSetup = FALSE;
    }
    if (sArcanumWishDelay && gBattleResults.battleTurnCounter == 0
     && BattleAI_IsScoringArcanum())
        return 0;
    return ArcanumTestWishScore(battlerAtk, battlerDef, move, score);
}

static bool32 ArcanumTestExecuteWish(u32 slot, u32 chosenMove, u32 currentMove)
{
    const u8 fail[] = {0};
    const struct __attribute__((packed)) {
        u8 opcode;
        void (*func)(void);
        const u8 *failInstr;
    } command = {0, BS_CastoriaTryWish, fail};
    const u8 *savedScript = gBattlescriptCurrInstr;
    bool32 success;

    gBattlerAttacker = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    gChosenMovePos = gCurrMovePos = slot;
    gChosenMove = chosenMove;
    gCurrentMove = currentMove;
    gBattlescriptCurrInstr = (const u8 *)&command;
    BS_CastoriaTryWish();
    success = gBattlescriptCurrInstr == (const u8 *)(&command + 1);
    gBattlescriptCurrInstr = savedScript;
    return success;
}

SINGLE_BATTLE_TEST("Arcanum Wish Domination requires the true name and exactly zero seals")
{
    GIVEN {
        PLAYER(SPECIES_CASTORIA);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        u32 name, charges, used;
        EXPECT_EQ(Test_GetCastoriaArcanumMove(2), MOVE_ARCANUM_WISH);
        for (name = CASTORIA_NAME_NONE; name <= CASTORIA_NAME_TRUE; name++)
        {
            for (charges = 0; charges <= 3; charges++)
            {
                for (used = FALSE; used <= TRUE; used++)
                {
                    bool32 eligible = name == CASTORIA_NAME_TRUE && charges == 0 && !used;
                    gBattleStruct->castoria.nameResult = name;
                    opponent->volatiles.castoriaTrueNameCharges = charges;
                    gBattleStruct->castoria.wishUsed = used;
                    EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_DOMINATION), eligible);
                    EXPECT_EQ(IsArcanumSlotUsable(battler, 6), !used
                        && name != CASTORIA_NAME_NONE);
                }
            }
        }
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        gBattleStruct->castoria.wishUsed = FALSE;
        opponent->volatiles.castoriaTrueNameCharges = 0;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_PLAYER_LEFT, CASTORIA_WISH_DOMINATION), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(MAX_BATTLERS_COUNT, CASTORIA_WISH_DOMINATION), FALSE);
        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(CastoriaCanWishFor(battler, CASTORIA_WISH_DOMINATION), FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish leaves unimplemented and invalid outcomes unavailable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 name, outcome;
        for (name = CASTORIA_NAME_NONE; name <= CASTORIA_NAME_TRUE; name++)
        {
            gBattleStruct->castoria.nameResult = name;
            for (outcome = CASTORIA_WISH_REDUCTION; outcome <= CASTORIA_WISH_NORMALITY; outcome++)
                if (outcome != CASTORIA_WISH_REDUCTION && outcome != CASTORIA_WISH_ENDURANCE && outcome != CASTORIA_WISH_DEBILITATION
                 && outcome != CASTORIA_WISH_NORMALITY)
                    EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, outcome), FALSE);
        }
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_NONE), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, 255), FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish commits its outcome only when it wins and conserves ties")
{
    s32 regularScore;
    PARAMETRIZE { regularScore = 90; }
    PARAMETRIZE { regularScore = 100; }
    PARAMETRIZE { regularScore = 110; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        sArcanumTestRegularScore = regularScore;
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, ArcanumTestWishScore),
                  regularScore < 100 ? 6 : 0);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome,
                  regularScore < 100 ? CASTORIA_WISH_DOMINATION : CASTORIA_WISH_NONE);
        EXPECT_EQ(gBattleStruct->castoria.wishCandidate, CASTORIA_WISH_DOMINATION);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
        // A later ordinary choice clears the previously committed outcome.
        sArcanumTestRegularScore = 110;
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, ArcanumTestWishScore), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish preserves Taunt Encore Disable and Assault Vest restrictions")
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
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
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
        EXPECT_EQ(IsArcanumSlotUsable(battler, 6), FALSE);
        sArcanumTestRegularScore = 90;
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, ArcanumTestWishScore), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Domination restores three seals once without PP or stat writes")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 i, battler = B_POSITION_OPPONENT_LEFT;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_DOMINATION;
        EXPECT_EQ(ArcanumTestExecuteWish(6, MOVE_ARCANUM_WISH, MOVE_ARCANUM_WISH), TRUE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 3);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE) - 1);
        for (i = STAT_ATK; i <= STAT_SPDEF; i++)
            EXPECT_EQ(opponent->statStages[i], DEFAULT_STAT_STAGE);
        opponent->volatiles.castoriaTrueNameCharges = 0;
        // Cycling the spell out and in must not restore its use.
        gBattleStruct->castoria.arcanumMoves[2] = MOVE_NONE;
        gBattleStruct->castoria.arcanumMoves[2] = MOVE_ARCANUM_WISH;
        EXPECT_EQ(IsArcanumSlotUsable(battler, 6), FALSE);
        gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_DOMINATION;
        EXPECT_EQ(ArcanumTestExecuteWish(6, MOVE_ARCANUM_WISH, MOVE_ARCANUM_WISH), FALSE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish revalidates the stored outcome without choosing a replacement")
{
    u32 invalidation;
    PARAMETRIZE { invalidation = 0; }
    PARAMETRIZE { invalidation = 1; }
    PARAMETRIZE { invalidation = 2; }
    PARAMETRIZE { invalidation = 3; }
    PARAMETRIZE { invalidation = 4; }
    PARAMETRIZE { invalidation = 5; }
    PARAMETRIZE { invalidation = 6; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_DOMINATION;
        if (invalidation == 0)
            gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        else if (invalidation == 1)
            opponent->volatiles.castoriaTrueNameCharges = 1;
        else if (invalidation == 2)
            gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_NONE;
        else if (invalidation == 3)
            gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_MORE_WISHES;
        EXPECT_EQ(ArcanumTestExecuteWish(invalidation == 4 ? 0 : 6,
                                        invalidation == 5 ? MOVE_TACKLE : MOVE_ARCANUM_WISH,
                                        invalidation == 6 ? MOVE_TACKLE : MOVE_ARCANUM_WISH), FALSE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, invalidation == 1 ? 1 : 0);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish casts Domination through real selection and execution")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        sDynamicAiFunc = ArcanumTestWishBattleScore;
        sArcanumWishSetup = TRUE;
        sArcanumWishDelay = FALSE;
        sArcanumTestRegularScore = 90;
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        MESSAGE("The opposing Caster?'s Command Seals were restored!");
    } THEN {
        // SCENE verifies the executed cast; the runner can prepare the next choice.
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 3);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_NONE);
        EXPECT_EQ(gBattleStruct->wish[B_POSITION_OPPONENT_LEFT].counter, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish interruption does not consume the battle use")
{
    u32 move;
    PARAMETRIZE { move = MOVE_TAUNT; }
    PARAMETRIZE { move = MOVE_AIR_SLASH; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        sDynamicAiFunc = ArcanumTestWishBattleScore;
        sArcanumWishSetup = TRUE;
        sArcanumWishDelay = FALSE;
        sArcanumTestRegularScore = 90;
        PLAYER(SPECIES_WOBBUFFET) { Moves(move); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); Speed(10); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        EXPECT_EQ(gBattleStruct->chosenMovePositions[B_POSITION_OPPONENT_LEFT], 6);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish remains unspent when faster Encore forces the regular move")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        sDynamicAiFunc = ArcanumTestWishBattleScore;
        sArcanumWishSetup = TRUE;
        sArcanumWishDelay = TRUE;
        sArcanumTestRegularScore = 90;
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE, MOVE_ENCORE); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_ENCORE); }
    } SCENE {
        MESSAGE("The opposing Caster? used Celebrate!");
        MESSAGE("Wobbuffet used Encore!");
        MESSAGE("The opposing Caster? must do an encore!");
        MESSAGE("The opposing Caster? used Celebrate!");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE) - 2);
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_CELEBRATE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish stays spent after Castoria switches out and returns")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        sDynamicAiFunc = ArcanumTestWishBattleScore;
        sArcanumWishSetup = TRUE;
        sArcanumWishDelay = FALSE;
        sArcanumTestRegularScore = 90;
        // Low HP prevents Command Seals from interrupting the switch fixture.
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(100); Moves(MOVE_CELEBRATE, MOVE_WHIRLWIND); Speed(40); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_PICHU) { Moves(MOVE_CELEBRATE); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_WHIRLWIND); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        MESSAGE("The opposing Caster?'s Command Seals were restored!");
        MESSAGE("The opposing Pichu was dragged out!");
        MESSAGE("The opposing Caster? was dragged out!");
        MESSAGE("The opposing Caster? used Celebrate!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_CASTORIA);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
        EXPECT_EQ(IsArcanumSlotUsable(B_POSITION_OPPONENT_LEFT, 6), FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum spent Wish does not prevent Struggle when it is the only spell")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 slot, battler = B_POSITION_OPPONENT_LEFT;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
        for (slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            opponent->pp[slot] = 0;
            gBattleStruct->castoria.arcanumMoves[slot] = MOVE_NONE;
        }
        gBattleStruct->castoria.arcanumMoves[0] = MOVE_ARCANUM_WISH;
        EXPECT_EQ(AreAllMovesUnusable(battler), FALSE);
        gBattleStruct->castoria.wishUsed = TRUE;
        EXPECT_EQ(HasUsableArcanumMove(battler), FALSE);
        EXPECT_EQ(AreAllMovesUnusable(battler), TRUE);
        EXPECT_EQ((u32)gProtectStructs[battler].noValidMoves, TRUE);
    }
}

// Copied from move_effect/wish.c; ordinary Wish retains its existing behavior.
SINGLE_BATTLE_TEST("Arcanum regression: ordinary Wish still restores HP on the following turn")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_WISH) == EFFECT_WISH);
        PLAYER(SPECIES_WYNAUT) { HP(50); MaxHP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WISH); }
        TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        MESSAGE("Wynaut's wish came true!");
        HP_BAR(player, damage: -50);
        MESSAGE("Wynaut's HP was restored.");
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor refusal casts before turns and replaces the Sp Atk boost")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Mage Armor!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MAGEARMOR, opponent);
        MESSAGE("The opposing Caster? used Celebrate!");
    } THEN {
        EXPECT_EQ(HasMageArmor(B_POSITION_OPPONENT_LEFT), TRUE);
        EXPECT_EQ(HasMageArmor(B_POSITION_PLAYER_LEFT), FALSE);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ((u32)opponent->volatiles.castoriaTrueNameCharges, 0);
        EXPECT_EQ(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_LIGHTSCREEN, 0);
        EXPECT_EQ(Test_GetCastoriaArcanumMove(1), MOVE_REVIVAL_BLESSING_2);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE) - 1);
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_CELEBRATE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor belongs only to the backed-out name result")
{
    u32 nameResult;
    PARAMETRIZE { nameResult = CASTORIA_NAME_TRUE; }
    PARAMETRIZE { nameResult = CASTORIA_NAME_FALSE; }
    PARAMETRIZE { nameResult = CASTORIA_NAME_BACKED_OUT; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA);
    } WHEN {
        TURN {}
    } THEN {
        const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
        } command = {0, BS_ApplyCastoriaNameResult};
        const u8 *savedScript = gBattlescriptCurrInstr;
        gBattlerAttacker = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        gBattleStruct->castoria.mageArmorPartyMask = 0;
        gBattleStruct->castoria.nameResult = nameResult;
        gBattlescriptCurrInstr = (const u8 *)&command;
        BS_ApplyCastoriaNameResult();
        gBattlescriptCurrInstr = savedScript;
        EXPECT_EQ(HasMageArmor(B_POSITION_OPPONENT_LEFT), nameResult == CASTORIA_NAME_BACKED_OUT);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + (nameResult != CASTORIA_NAME_BACKED_OUT));
        EXPECT_EQ(opponent->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE + (nameResult == CASTORIA_NAME_TRUE));
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor reduces physical and special calculated damage", s16 damage)
{
    u32 species, move;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; move = MOVE_AERIAL_ACE; }
    PARAMETRIZE { species = SPECIES_CASTORIA; move = MOVE_AERIAL_ACE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; move = MOVE_WATER_GUN; }
    PARAMETRIZE { species = SPECIES_CASTORIA; move = MOVE_WATER_GUN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(200); SpAttack(200); Ability(ABILITY_RUN_AWAY); }
        OPPONENT(species) { MaxHP(1000); HP(1000); Defense(100); SpDefense(100); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, UQ_4_12(0.7), results[1].damage);
        EXPECT_MUL_EQ(results[2].damage, UQ_4_12(0.7), results[3].damage);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor reduces each hit once", s16 first, s16 second)
{
    u32 species;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; }
    PARAMETRIZE { species = SPECIES_CASTORIA; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(200); }
        OPPONENT(species) { MaxHP(1000); HP(1000); Defense(100); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_BONEMERANG); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].first);
        HP_BAR(opponent, captureDamage: &results[i].second);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].first, UQ_4_12(0.7), results[1].first);
        EXPECT_MUL_EQ(results[0].second, UQ_4_12(0.7), results[1].second);
        EXPECT_EQ(results[1].first, results[1].second);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor preserves immunity and the one damage minimum")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Attack(1); }
        OPPONENT(SPECIES_CASTORIA) { MaxHP(1000); HP(1000); Defense(9999); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWIFT); }
        TURN { MOVE(player, MOVE_AERIAL_ACE); }
    } SCENE {
        NOT HP_BAR(opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_AERIAL_ACE, player);
        HP_BAR(opponent, captureDamage: &damage);
    } THEN {
        EXPECT_EQ(damage, 1);
        EXPECT_EQ(opponent->hp, opponent->maxHP - 1);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor reduces delayed Future Sight damage", s16 damage)
{
    u32 species;
    PARAMETRIZE { species = SPECIES_CLEFABLE; }
    PARAMETRIZE { species = SPECIES_CASTORIA; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { SpAttack(200); Ability(ABILITY_RUN_AWAY); }
        OPPONENT(species) { MaxHP(1000); HP(1000); SpDefense(100); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_FUTURE_SIGHT); }
        TURN {}
        TURN {}
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, UQ_4_12(0.7), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor leaves fixed and residual damage unchanged", s16 damage)
{
    u32 species;
    bool32 residual;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; residual = FALSE; }
    PARAMETRIZE { species = SPECIES_CASTORIA; residual = FALSE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; residual = TRUE; }
    PARAMETRIZE { species = SPECIES_CASTORIA; residual = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(species) {
            MaxHP(1000); HP(1000); Ability(ABILITY_RUN_AWAY);
            if (residual) Status1(STATUS1_POISON);
        }
    } WHEN {
        TURN { if (!residual) MOVE(player, MOVE_SEISMIC_TOSS); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
        EXPECT_EQ(results[2].damage, results[3].damage);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor survives Haze")
{
    s16 beforeHaze, afterHaze;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { MaxHP(1000); HP(1000); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_AERIAL_ACE); }
        TURN { MOVE(player, MOVE_HAZE); }
        TURN { MOVE(player, MOVE_AERIAL_ACE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &beforeHaze);
        MESSAGE("All stat changes were eliminated!");
        HP_BAR(opponent, captureDamage: &afterHaze);
    } THEN {
        EXPECT_EQ(beforeHaze, afterHaze);
        EXPECT_EQ(HasMageArmor(B_POSITION_OPPONENT_LEFT), TRUE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor stays with its party member across switches")
{
    s16 beforeSwitch, unarmored, afterSwitch;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { MaxHP(1000); HP(1000); Defense(100); Ability(ABILITY_RUN_AWAY); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); Defense(100); Ability(ABILITY_RUN_AWAY); }
    } WHEN {
        TURN { MOVE(player, MOVE_AERIAL_ACE); }
        TURN { SWITCH(opponent, 1); MOVE(player, MOVE_AERIAL_ACE); }
        TURN { SWITCH(opponent, 0); MOVE(player, MOVE_AERIAL_ACE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Mage Armor!");
        HP_BAR(opponent, captureDamage: &beforeSwitch);
        HP_BAR(opponent, captureDamage: &unarmored);
        NOT MESSAGE("The opposing Caster? cast Mage Armor!");
        HP_BAR(opponent, captureDamage: &afterSwitch);
    } THEN {
        EXPECT_EQ(beforeSwitch, afterSwitch);
        EXPECT_MUL_EQ(unarmored, UQ_4_12(0.7), afterSwitch);
        EXPECT_EQ(HasMageArmor(B_POSITION_OPPONENT_LEFT), TRUE);
    }
}

SINGLE_BATTLE_TEST("Arcanum AI includes Mage Armor in all three simulated damage rolls")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA);
    } WHEN {
        TURN {}
    } THEN {
        uq4_12_t effectiveness;
        struct SimulatedDamage armored = AI_CalcDamageSaveBattlers(MOVE_AERIAL_ACE,
            GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), &effectiveness, GIMMICK_NONE, GIMMICK_NONE);
        u8 savedArmor = gBattleStruct->castoria.mageArmorPartyMask;
        gBattleStruct->castoria.mageArmorPartyMask = 0;
        struct SimulatedDamage normal = AI_CalcDamageSaveBattlers(MOVE_AERIAL_ACE,
            GetBattlerAtPosition(B_POSITION_PLAYER_LEFT), GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT), &effectiveness, GIMMICK_NONE, GIMMICK_NONE);
        gBattleStruct->castoria.mageArmorPartyMask = savedArmor;
        EXPECT_GT(normal.minimum, 0);
        EXPECT_MUL_EQ(normal.minimum, UQ_4_12(0.7), armored.minimum);
        EXPECT_MUL_EQ(normal.median, UQ_4_12(0.7), armored.median);
        EXPECT_MUL_EQ(normal.maximum, UQ_4_12(0.7), armored.maximum);
    }
}

SINGLE_BATTLE_TEST("Arcanum Revivify restores a fainted ally to full HP without spending regular PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { HP(0); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1, partyIndex: 1); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("Pichu was revived and is ready to fight again!");
    } THEN {
        EXPECT_EQ(GetMonData(&gEnemyParty[1], MON_DATA_HP), GetMonData(&gEnemyParty[1], MON_DATA_MAX_HP));
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
    }
}

SINGLE_BATTLE_TEST("Arcanum Special AI requires both the flag and a registered callback")
{
    u64 flags;
    bool32 registered;
    u32 expectedSlot, expectedSpellCalls;
    PARAMETRIZE { flags = 0; registered = TRUE; expectedSlot = 0; expectedSpellCalls = 0; }
    PARAMETRIZE { flags = AI_FLAG_DYNAMIC_FUNC; registered = FALSE; expectedSlot = 0; expectedSpellCalls = 0; }
    PARAMETRIZE { flags = AI_FLAG_DYNAMIC_FUNC; registered = TRUE; expectedSlot = 4; expectedSpellCalls = 4; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        sArcanumTestPreferredMove = MOVE_FIREBALL;
        sArcanumTestSpellScore = 101;
        sArcanumTestRegularScore = 100;
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, flags,
                                      registered ? ArcanumTestDynamicScore : NULL), expectedSlot);
        EXPECT_EQ(sArcanumTestSpellCalls, expectedSpellCalls);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        EXPECT_EQ(sArcanumTestUsedVirtualIndex, FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Special AI preserves regular ties and can adjust regular scores")
{
    s32 spellScore, regularScore;
    u32 expectedSlot;
    PARAMETRIZE { spellScore = 101; regularScore = 100; expectedSlot = 4; }
    PARAMETRIZE { spellScore = 100; regularScore = 100; expectedSlot = 0; }
    PARAMETRIZE { spellScore = 99; regularScore = 100; expectedSlot = 0; }
    PARAMETRIZE { spellScore = 110; regularScore = 120; expectedSlot = 0; }
    PARAMETRIZE { spellScore = 100; regularScore = 90; expectedSlot = 4; }
    PARAMETRIZE { spellScore = 0; regularScore = 100; expectedSlot = 0; }
    PARAMETRIZE { spellScore = -10; regularScore = 100; expectedSlot = 0; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        // Same move ID in an ordinary slot and the virtual pool.
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_FIREBALL); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        sArcanumTestPreferredMove = MOVE_FIREBALL;
        sArcanumTestSpellScore = spellScore;
        sArcanumTestRegularScore = regularScore;
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      ArcanumTestDynamicScore), expectedSlot);
        EXPECT_EQ(sArcanumTestRegularCalls, 1);
        EXPECT_EQ(sArcanumTestSpellCalls, 4);
        EXPECT_EQ(gAiThinkingStruct->score[0], regularScore);
        EXPECT_EQ(opponent->moves[0], MOVE_FIREBALL);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_FIREBALL));
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        EXPECT_EQ(sArcanumTestUsedVirtualIndex, FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Special AI never offers unavailable spells to the callback")
{
    bool32 encore;
    PARAMETRIZE { encore = FALSE; }
    PARAMETRIZE { encore = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        sArcanumTestPreferredMove = MOVE_REVIVAL_BLESSING_2;
        sArcanumTestSpellScore = 200;
        sArcanumTestRegularScore = 100;
        if (encore)
        {
            gBattleMons[battler].volatiles.encoreTimer = 1;
            gBattleMons[battler].volatiles.encoredMove = MOVE_TACKLE;
        }
        else
        {
            gBattleMons[battler].volatiles.tauntTimer = 1;
        }
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, ArcanumTestDynamicScore), 0);
        EXPECT_EQ(sArcanumTestSeenSpells, encore ? 0 : (1u << 0));
        EXPECT_EQ(sArcanumTestSpellCalls, encore ? 0 : 1);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Special AI leaves non-owners and empty pools on ordinary selection")
{
    bool32 emptyPool;
    PARAMETRIZE { emptyPool = FALSE; }
    PARAMETRIZE { emptyPool = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        if (emptyPool)
            gBattleStruct->castoria.arcanumMoves[0] = MOVE_NONE;
        else
            opponent->species = SPECIES_WOBBUFFET;
        sArcanumTestPreferredMove = MOVE_FIREBALL;
        sArcanumTestSpellScore = 200;
        sArcanumTestRegularScore = 100;
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      ArcanumTestDynamicScore), 0);
        EXPECT_EQ(sArcanumTestSpellCalls, 0);
        EXPECT_EQ(sArcanumTestRegularCalls, 1);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Razeluxe Special AI scores spells without an ordinary PP slot")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 slot;
        for (slot = 0; slot < MAX_MON_MOVES; slot++)
            opponent->pp[slot] = 0;
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      AI_RazeluxeTrainer), 4);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        for (slot = 0; slot < MAX_MON_MOVES; slot++)
            EXPECT_EQ(opponent->pp[slot], 0);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Razeluxe prioritizes Polymorph against unrevealed Perish Song")
{
    u32 perishSlot;
    PARAMETRIZE { perishSlot = 0; }
    PARAMETRIZE { perishSlot = 1; }
    PARAMETRIZE { perishSlot = 2; }
    PARAMETRIZE { perishSlot = 3; }
    PARAMETRIZE { perishSlot = MAX_MON_MOVES; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        // Install the threat without revealing it through move history.
        if (perishSlot < MAX_MON_MOVES)
            player->moves[perishSlot] = MOVE_PERISH_SONG;
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      AI_RazeluxeTrainer), perishSlot < MAX_MON_MOVES ? 7 : 0);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Razeluxe prioritizes Revivify while affected by Perish Song")
{
    u32 countdown;
    PARAMETRIZE { countdown = 0; }
    PARAMETRIZE { countdown = 1; }
    PARAMETRIZE { countdown = 2; }
    PARAMETRIZE { countdown = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { HP(0); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        opponent->volatiles.perishSong = TRUE;
        gBattleMons[battler].volatiles.perishSongTimer = countdown;
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), 5);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Revivify priority outranks Polymorph independently of pool order")
{
    bool32 reversed;
    PARAMETRIZE { reversed = FALSE; }
    PARAMETRIZE { reversed = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { HP(0); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        player->moves[1] = MOVE_PERISH_SONG;
        opponent->volatiles.perishSong = TRUE;
        gBattleMons[battler].volatiles.perishSongTimer = 1;
        if (reversed)
        {
            gBattleStruct->castoria.arcanumMoves[1] = MOVE_POLYMORPH;
            gBattleStruct->castoria.arcanumMoves[3] = MOVE_REVIVAL_BLESSING_2;
        }
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), reversed ? 7 : 5);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Revivify priority requires a fainted ally and active Perish Song")
{
    bool32 fainted, affected;
    PARAMETRIZE { fainted = FALSE; affected = FALSE; }
    PARAMETRIZE { fainted = FALSE; affected = TRUE; }
    PARAMETRIZE { fainted = TRUE; affected = FALSE; }
    PARAMETRIZE { fainted = TRUE; affected = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { if (fainted) HP(0); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        opponent->volatiles.perishSong = affected;
        // A leftover timer alone must not trigger the rule.
        gBattleMons[battler].volatiles.perishSongTimer = 1;
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer),
                  fainted && affected ? 5 : 0);
        // With no viable ordinary move, an impossible Revivify must still lose.
        if (!fainted)
        {
            opponent->pp[0] = 0;
            EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), 4);
        }
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum unavailable Revivify does not override selection restrictions")
{
    u32 restriction;
    PARAMETRIZE { restriction = 0; }
    PARAMETRIZE { restriction = 1; }
    PARAMETRIZE { restriction = 2; }
    PARAMETRIZE { restriction = 3; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_PICHU) { HP(0); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        opponent->volatiles.perishSong = TRUE;
        gBattleMons[battler].volatiles.perishSongTimer = 1;
        if (restriction == 0)
            gBattleMons[battler].volatiles.tauntTimer = 1;
        else if (restriction == 1)
        {
            gBattleMons[battler].volatiles.encoreTimer = 1;
            gBattleMons[battler].volatiles.encoredMove = MOVE_CELEBRATE;
            gBattleMons[battler].volatiles.encoredMovePos = 0;
        }
        else if (restriction == 2)
        {
            gBattleMons[battler].volatiles.disabledMove = MOVE_REVIVAL_BLESSING_2;
            gBattleMons[battler].volatiles.disableTimer = 1;
        }
        else
        {
            opponent->volatiles.healBlockTimer = 1;
            gBattleMons[battler].volatiles.healBlockTimer = 1;
        }
        EXPECT_EQ(IsArcanumSlotUsable(battler, 5), FALSE);
        EXPECT_NE(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), 5);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Razeluxe chooses Revivify after being outsped by Perish Song")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_PERISH_SONG); Speed(30); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); Speed(10); }
        OPPONENT(SPECIES_PICHU) { HP(0); Speed(5); }
    } WHEN {
        TURN { MOVE(player, MOVE_PERISH_SONG); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet used Perish Song!");
        MESSAGE("The opposing Caster? used Celebrate!");
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ((u32)opponent->volatiles.perishSong, TRUE);
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), 5);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Perish Song priority respects Polymorph selection restrictions")
{
    u32 restriction;
    PARAMETRIZE { restriction = 0; }
    PARAMETRIZE { restriction = 1; }
    PARAMETRIZE { restriction = 2; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        player->moves[1] = MOVE_PERISH_SONG;
        if (restriction == 0)
            gBattleMons[battler].volatiles.tauntTimer = 1;
        else if (restriction == 1)
        {
            gBattleMons[battler].volatiles.encoreTimer = 1;
            gBattleMons[battler].volatiles.encoredMove = MOVE_TACKLE;
            gBattleMons[battler].volatiles.encoredMovePos = 0;
        }
        else
        {
            gBattleMons[battler].volatiles.disableTimer = 1;
            gBattleMons[battler].volatiles.disabledMove = MOVE_POLYMORPH;
        }
        EXPECT_EQ(ArcanumTestChooseMove(battler, AI_FLAG_DYNAMIC_FUNC, AI_RazeluxeTrainer), 0);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum Alice Special AI does not inherit Razeluxe spell policy")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      AI_AliceTrainer), 0);
        EXPECT_EQ(gAiThinkingStruct->score[0], AI_SCORE_DEFAULT);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum wiring preserves Razeluxe's existing regular Mewtwo scoring")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(1000); HP(1000); }
        OPPONENT(SPECIES_MEWTWO) { Moves(MOVE_DMS_PREROGATIVE, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(ArcanumTestChooseMove(B_POSITION_OPPONENT_LEFT, AI_FLAG_DYNAMIC_FUNC,
                                      AI_RazeluxeTrainer), 0);
        EXPECT_EQ(gAiThinkingStruct->score[0], AI_SCORE_DEFAULT + 30);
        EXPECT_EQ(gAiThinkingStruct->score[1], AI_SCORE_DEFAULT);
        EXPECT_EQ(BattleAI_IsScoringArcanum(), FALSE);
        SetDynamicAiFunc(NULL);
    }
}

SINGLE_BATTLE_TEST("Arcanum lookup bounds and boss-only access")
{
    GIVEN {
        PLAYER(SPECIES_CASTORIA);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 7), MOVE_POLYMORPH);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 8), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 255), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_PLAYER_LEFT, 4), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 0), MOVE_CELEBRATE);
        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 4), MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Arcanum ignores exhausted regular PP when another regular move remains usable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { MovesWithPP({MOVE_TACKLE, 0}, {MOVE_SPLASH, 0}, {MOVE_GROWL, 0}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 0); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Fireball!");
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_EQ(opponent->pp[2], 0);
        EXPECT_EQ(opponent->pp[3], 10);
    }
}

SINGLE_BATTLE_TEST("Arcanum remains selectable when every regular move has zero PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { MovesWithPP({MOVE_TACKLE, 0}, {MOVE_SPLASH, 0}, {MOVE_GROWL, 0}, {MOVE_CELEBRATE, 0}); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 0); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Fireball!");
        HP_BAR(player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, opponent);
    } THEN {
        EXPECT_EQ(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_EQ(opponent->pp[2], 0);
        EXPECT_EQ(opponent->pp[3], 0);
    }
}

SINGLE_BATTLE_TEST("Arcanum availability checks bounds ownership empty pools and restrictions")
{
    GIVEN {
        PLAYER(SPECIES_CASTORIA);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        EXPECT_EQ(IsArcanumSlotUsable(battler, 3), FALSE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 8), FALSE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 255), FALSE);
        EXPECT_EQ(IsArcanumSlotUsable(B_POSITION_PLAYER_LEFT, 4), FALSE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 4), TRUE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 5), TRUE);
        EXPECT_EQ(HasUsableArcanumMove(battler), TRUE);

        gBattleMons[battler].volatiles.tauntTimer = 1;
        EXPECT_EQ(IsArcanumSlotUsable(battler, 5), FALSE);
        EXPECT_EQ(IsArcanumSlotUsable(battler, 4), TRUE);
        gBattleMons[battler].volatiles.tauntTimer = 0;

        gBattleMons[battler].volatiles.disabledMove = MOVE_FIREBALL;
        EXPECT_EQ(IsArcanumSlotUsable(battler, 4), FALSE);
        gBattleMons[battler].volatiles.disabledMove = MOVE_NONE;

        gBattleMons[battler].volatiles.encoreTimer = 1;
        gBattleMons[battler].volatiles.encoredMove = MOVE_CELEBRATE;
        EXPECT_EQ(HasUsableArcanumMove(battler), FALSE);
        gBattleMons[battler].volatiles.encoreTimer = 0;
        gBattleMons[battler].volatiles.encoredMove = MOVE_NONE;

        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(HasUsableArcanumMove(battler), FALSE);
        opponent->species = SPECIES_CASTORIA;
        gBattleStruct->castoria.arcanumMoves[0] = MOVE_NONE;
        EXPECT_EQ(HasUsableArcanumMove(battler), FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Struggle decision includes spells without changing ordinary battlers")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        u32 slot;
        for (slot = 0; slot < MAX_MON_MOVES; slot++)
            opponent->pp[slot] = 0;
        EXPECT_EQ(AreAllMovesUnusable(battler), FALSE);
        EXPECT_EQ((u32)gProtectStructs[battler].noValidMoves, FALSE);

        gBattleMons[battler].volatiles.encoreTimer = 1;
        gBattleMons[battler].volatiles.encoredMove = MOVE_CELEBRATE;
        EXPECT_EQ(AreAllMovesUnusable(battler), TRUE);
        EXPECT_EQ((u32)gProtectStructs[battler].noValidMoves, TRUE);
        gBattleMons[battler].volatiles.encoreTimer = 0;
        gBattleMons[battler].volatiles.encoredMove = MOVE_NONE;
        EXPECT_EQ(AreAllMovesUnusable(battler), FALSE);

        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(AreAllMovesUnusable(battler), TRUE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Revivify cannot bypass Taunt")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(20); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_TACKLE, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAUNT); MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(opponent, arcanumSlot: 1, allowed: FALSE); MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("The opposing Caster? fell for the taunt!");
        MESSAGE("The opposing Caster? used Tackle!");
        NOT MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("The opposing Caster? used Tackle!");
    }
}

SINGLE_BATTLE_TEST("Arcanum Instruct repeats the remembered regular move after a cast")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_SPLASH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_INSTRUCT); }
    } SCENE {
        MESSAGE("The opposing Caster? used Splash!");
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("Wobbuffet used Instruct!");
        MESSAGE("The opposing Caster? used Splash!");
    }
}

SINGLE_BATTLE_TEST("Arcanum Instruct fails on a missing remembered move with a virtual current slot")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        // Directly exercise a missing, Instruct-eligible remembered move.
        const u8 fail[] = {0};
        const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
            const u8 *failInstr;
        } command = {0, BS_TryInstruct, fail};
        const u8 *savedScript = gBattlescriptCurrInstr;
        gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
        gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        gLastPrintedMoves[gBattlerTarget] = MOVE_SPLASH;
        gCurrMovePos = 7;
        gBattlescriptCurrInstr = (const u8 *)&command;
        BS_TryInstruct();
        EXPECT(gBattlescriptCurrInstr == fail);
        EXPECT_EQ(gCurrMovePos, 7);
        gBattlescriptCurrInstr = savedScript;
    }
}

SINGLE_BATTLE_TEST("Arcanum AI damage reads handle virtual and invalid slots without changing battlers")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_TACKLE, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        u32 attacker = B_POSITION_OPPONENT_LEFT;
        u32 defender = B_POSITION_PLAYER_LEFT;
        u16 species = opponent->species;
        u16 hp = opponent->hp;
        u32 damage = AI_GetDamage(attacker, defender, 4, AI_ATTACKING, gAiLogicData);
        EXPECT_GT(damage, 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 5, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 8, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 255, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamageForMove(attacker, defender, MOVE_FIREBALL, AI_ATTACKING, gAiLogicData), damage);
        EXPECT_EQ(opponent->species, species);
        EXPECT_EQ(opponent->hp, hp);
        // The target determines AI ownership from the controller, not side.
        // Exercise own-slot caches and roll policies in an explicit AI context.
        u8 savedController = gBattlerBattleController[attacker];
        gBattlerBattleController[attacker] = BATTLE_CONTROLLER_OPPONENT;
        gAiLogicData->simulatedDmg[attacker][defender][0] = (struct SimulatedDamage){ .minimum = 11, .median = 22, .maximum = 33 };
        gAiThinkingStruct->aiFlags[attacker] = 0;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 22);
        EXPECT_EQ(AI_GetDamageForMove(attacker, defender, MOVE_TACKLE, AI_ATTACKING, gAiLogicData), 22);
        gAiThinkingStruct->aiFlags[attacker] = AI_FLAG_RISKY;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 33);
        gAiThinkingStruct->aiFlags[attacker] = AI_FLAG_CONSERVATIVE;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 11);
        gAiThinkingStruct->aiFlags[attacker] = 0;
        gBattlerBattleController[attacker] = savedController;
    }
}

SINGLE_BATTLE_TEST("Arcanum casts from virtual slots without consuming regular PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE, MOVE_SPLASH, MOVE_TACKLE, MOVE_FIREBALL); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 0); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Fireball!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FIREBALL, opponent);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(opponent->pp[1], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponent->pp[2], GetMovePP(MOVE_TACKLE));
        EXPECT_EQ(opponent->pp[3], GetMovePP(MOVE_FIREBALL));
    }
}

SINGLE_BATTLE_TEST("Arcanum spell IDs in regular slots still say used")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA);
    } WHEN {
        TURN { MOVE(opponent, MOVE_FIREBALL); }
    } SCENE {
        MESSAGE("The opposing Caster? used Fireball!");
        HP_BAR(player);
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Encore and enforces its lock")
{
    GIVEN {
        WITH_CONFIG(B_ENCORE_TARGET, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_ENCORE); }
        TURN { MOVE(opponent, arcanumSlot: 1, allowed: FALSE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? used Celebrate!");
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("Wobbuffet used Encore!");
        MESSAGE("The opposing Caster? must do an encore!");
        MESSAGE("The opposing Caster? used Celebrate!");
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Disable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_DISABLE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("The opposing Caster?'s Celebrate was disabled!");
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Mimic")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_SPLASH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_MIMIC); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("Wobbuffet learned Splash!");
    }
}

SINGLE_BATTLE_TEST("Arcanum failed casts preserve regular history")
{
    GIVEN {
        WITH_CONFIG(B_ENCORE_TARGET, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_ENCORE); }
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("The opposing Caster? cast Revivify!");
        MESSAGE("But it failed!");
        MESSAGE("The opposing Caster? must do an encore!");
        MESSAGE("The opposing Caster? used Celebrate!");
    }
}

SINGLE_BATTLE_TEST("Arcanum KO under Grudge leaves regular PP intact")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(20); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(20); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE, MOVE_SPLASH, MOVE_TACKLE, MOVE_FIREBALL); }
    } WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, arcanumSlot: 0); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Grudge!");
        MESSAGE("The opposing Caster? cast Fireball!");
        MESSAGE("Wobbuffet fainted!");
        NOT MESSAGE("The opposing Caster?'s Fireball lost all its PP due to the grudge!");
    } THEN {
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(opponent->pp[1], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponent->pp[2], GetMovePP(MOVE_TACKLE));
        EXPECT_EQ(opponent->pp[3], GetMovePP(MOVE_FIREBALL));
    }
}

// Regular-slot regressions kept here so TESTS=Arcanum is sufficient.

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge depletes all PP of the move that fainted the target")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); };
    } WHEN {
        TURN {
            MOVE(player, MOVE_GRUDGE);
            MOVE(opponent, MOVE_SCRATCH);
            SEND_OUT(player, 1);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponent);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge does not deplete PP of a Z-Move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_NORMALIUM_Z); Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); };
    } WHEN {
        TURN {
            MOVE(player, MOVE_GRUDGE);
            MOVE(opponent, MOVE_SCRATCH, gimmick: GIMMICK_Z_MOVE);
            SEND_OUT(player, 1);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_ZMOVE_ACTIVATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BREAKNECK_BLITZ, opponent);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_GT(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge depletes all PP from a Max Move's base move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) {  Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); Item(ITEM_LAGGING_TAIL); }
    } WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, MOVE_SCRATCH, gimmick: GIMMICK_DYNAMAX); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Grudge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        MESSAGE("The opposing Wobbuffet used Max Strike!");
        MESSAGE("The opposing Wobbuffet lost all of Scratch's PP due to the grudge!");
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge does not activate for Struggle")
{
    GIVEN {
        PLAYER (SPECIES_WOBBUFFET) { HP(1); }
        PLAYER (SPECIES_WOBBUFFET);
        OPPONENT (SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE, MOVE_STRUGGLE, MOVE_POUND, MOVE_SURF); };
    }
    WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, MOVE_STRUGGLE); SEND_OUT(player, 1); }
    }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, opponent);
        MESSAGE("Wobbuffet fainted!");
        NOT MESSAGE("The opposing Wobbuffet's Struggle lost all its PP due to the grudge!");
    }
    THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_GT(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore forces consecutive move uses for 3 turns: Encore used before move")
{
    struct BattlePokemon *encoreUser = NULL;
    struct BattlePokemon *encoreTarget = NULL;
    u32 speedPlayer, speedOpponent;
    PARAMETRIZE { encoreUser = opponent; encoreTarget = player; speedPlayer = 10; speedOpponent = 20; }
    PARAMETRIZE { encoreUser = player; encoreTarget = opponent; speedPlayer = 20; speedOpponent = 10; }
    GIVEN {
        WITH_CONFIG(B_ENCORE_TARGET, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Speed(speedPlayer); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(speedOpponent); }
    } WHEN {
        TURN { MOVE(encoreUser, MOVE_CELEBRATE); MOVE(encoreTarget, MOVE_CELEBRATE); }
        TURN { MOVE(encoreUser, MOVE_ENCORE); MOVE(encoreTarget, MOVE_CELEBRATE); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { MOVE(encoreTarget, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, encoreTarget);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore forces consecutive move uses for 3 turns for player: Encore used after move")
{
    struct BattlePokemon *encoreUser = NULL;
    struct BattlePokemon *encoreTarget = NULL;
    u32 speedPlayer, speedOpponent;
    PARAMETRIZE { encoreUser = opponent; encoreTarget = player; speedPlayer = 20; speedOpponent = 10; }
    PARAMETRIZE { encoreUser = player; encoreTarget = opponent; speedPlayer = 10; speedOpponent = 20; }
    GIVEN {
        WITH_CONFIG(B_ENCORE_TARGET, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Speed(speedPlayer); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(speedOpponent); }
    } WHEN {
        TURN { MOVE(encoreTarget, MOVE_CELEBRATE); MOVE(encoreUser, MOVE_ENCORE); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { MOVE(encoreTarget, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, encoreTarget);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore has no effect if no previous move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_ENCORE); MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet used Encore!");
        MESSAGE("But it failed!");
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore overrides the chosen move if it occurs first")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, MOVE_ENCORE); MOVE(player, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
}

// Copied from test/battle/move_effect/mimic.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Mimic doesn't error when the last move used by the target resolves to MOVE_UNAVAILABLE")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {MOVE(opponent, MOVE_TRANSFORM); MOVE(player, MOVE_MIMIC); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, opponent);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_MIMIC, player);
    } THEN {
        EXPECT_EQ(gLastMoves[1], MOVE_UNAVAILABLE); // This test depends on the current implementation of Transform, if this changes, the test should be changed
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Instruct causes the target to use its last used move again")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); MOVE(opponent, MOVE_INSTRUCT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, opponent);
        MESSAGE("Wobbuffet followed the opposing Wynaut's instructions!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if target hasn't made a move")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if move is banned by Instruct")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(IsMoveInstructBanned(MOVE_BIDE));
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_BIDE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_BIDE); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BIDE, playerRight);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called move targets the target of the move picked on its last use")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_FAKE_OUT); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        HP_BAR(opponentLeft);
        NOT HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        HP_BAR(opponentLeft);
        NOT HP_BAR(opponentRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct doesn't bypass sleep")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_SPORE) == EFFECT_NON_VOLATILE_STATUS);
        ASSUME(GetMoveNonVolatileStatus(MOVE_SPORE) == MOVE_EFFECT_SLEEP);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(opponentLeft, MOVE_SPORE, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        MESSAGE("Wobbuffet is fast asleep.");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if target doesn't know the last move it used")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(IsDanceMove(MOVE_DRAGON_DANCE));
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_ORICORIO) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_DRAGON_DANCE); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, opponentLeft);
        ABILITY_POPUP(playerRight, ABILITY_DANCER);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, playerRight);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, playerRight);
        }
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called move fails if it can only be used on the first turn but consumes PP")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_FAKE_OUT) == EFFECT_FIRST_TURN_ONLY);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_FAKE_OUT); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_FAKE_OUT, target: opponentLeft); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FAKE_OUT, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_FAKE_OUT, playerRight);
    } THEN {
        EXPECT_EQ(playerRight->pp[3], GetMovePP(MOVE_FAKE_OUT) - 2);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called status move fails if taunted")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_TAUNT) == EFFECT_TAUNT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_GROWL); MOVE(opponentLeft, MOVE_TAUNT, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, playerRight);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentLeft);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TAUNT, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, playerRight);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentRight);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentLeft);
        }
    } THEN {
        EXPECT_EQ(playerRight->pp[3], GetMovePP(MOVE_GROWL) - 1);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called moves fail if disabled")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_DISABLE) == EFFECT_DISABLE);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(opponentLeft, MOVE_DISABLE, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    } THEN {
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SCRATCH) - 1);
    }
}
