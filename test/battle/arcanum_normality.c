#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_gimmick.h"
#include "battle_util.h"
#include "castoria_name_theft.h"

extern AiScoreFunc sDynamicAiFunc;

static bool8 sSetup;
static u8 sWishTurn;
static u16 sOriginalMaxHP;
static u16 sOriginalHP;

static s32 NormalityTestScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (sSetup)
    {
        sSetup = FALSE;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        sOriginalMaxHP = gBattleMons[B_POSITION_PLAYER_LEFT].maxHP;
        sOriginalHP = gBattleMons[B_POSITION_PLAYER_LEFT].hp;
    }
    if (!BattleAI_IsScoringArcanum())
        return 90;
    if (move == MOVE_ARCANUM_WISH
     && gBattleResults.battleTurnCounter >= sWishTurn
     && CastoriaCanWishFor(battlerAtk, CASTORIA_WISH_NORMALITY))
    {
        gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_NORMALITY;
        return score;
    }
    return 0;
}

static void NormalityFixture(void)
{
    sDynamicAiFunc = NormalityTestScore;
    sSetup = TRUE;
    sWishTurn = 0;
}

// Exercise doubles target resolution directly: virtual-slot AI is singles-only.
static bool32 ExecuteNormality(void)
{
    const u8 fail[] = {0};
    const struct __attribute__((packed)) {
        u8 opcode;
        void (*func)(void);
        const u8 *failInstr;
    } command = {0, BS_CastoriaTryWish, fail};
    const u8 *savedScript = gBattlescriptCurrInstr;
    bool32 success;

    gBattlerAttacker = B_POSITION_OPPONENT_LEFT;
    gChosenMovePos = gCurrMovePos = 6;
    gChosenMove = gCurrentMove = MOVE_ARCANUM_WISH;
    gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_NORMALITY;
    gBattlescriptCurrInstr = (const u8 *)&command;
    BS_CastoriaTryWish();
    success = gBattlescriptCurrInstr == (const u8 *)(&command + 1);
    gBattlescriptCurrInstr = savedScript;
    return success;
}

SINGLE_BATTLE_TEST("Arcanum Wish Normality requires a resolved route unused Wish and living target")
{
    GIVEN {
        PLAYER(SPECIES_GENGAR);
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
                EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_NORMALITY),
                    route != CASTORIA_NAME_NONE && !used);
            }
        }
        gBattleStruct->castoria.wishUsed = FALSE;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_PLAYER_LEFT, CASTORIA_WISH_NORMALITY), FALSE);
        EXPECT_EQ(CastoriaCanWishFor(MAX_BATTLERS_COUNT, CASTORIA_WISH_NORMALITY), FALSE);
        player->hp = 0;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_NORMALITY), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality ends active gimmicks and changes the target to pure Normal")
{
    u32 species, item, gimmick, expectedSpecies;
    bool32 gmax;
    PARAMETRIZE { species = SPECIES_GENGAR; item = ITEM_NONE; gimmick = GIMMICK_NONE; expectedSpecies = SPECIES_GENGAR; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; item = ITEM_VENUSAURITE; gimmick = GIMMICK_MEGA; expectedSpecies = SPECIES_VENUSAUR; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_GROUDON; item = ITEM_RED_ORB; gimmick = GIMMICK_NONE; expectedSpecies = SPECIES_GROUDON; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_KYOGRE; item = ITEM_BLUE_ORB; gimmick = GIMMICK_NONE; expectedSpecies = SPECIES_KYOGRE; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_NECROZMA_DUSK_MANE; item = ITEM_ULTRANECROZIUM_Z; gimmick = GIMMICK_ULTRA_BURST; expectedSpecies = SPECIES_NECROZMA_DUSK_MANE; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; item = ITEM_NONE; gimmick = GIMMICK_DYNAMAX; expectedSpecies = SPECIES_WOBBUFFET; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; item = ITEM_NONE; gimmick = GIMMICK_DYNAMAX; expectedSpecies = SPECIES_VENUSAUR; gmax = TRUE; }
    PARAMETRIZE { species = SPECIES_GENGAR; item = ITEM_NONE; gimmick = GIMMICK_TERA; expectedSpecies = SPECIES_GENGAR; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_OGERPON_TEAL; item = ITEM_NONE; gimmick = GIMMICK_TERA; expectedSpecies = SPECIES_OGERPON_TEAL; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_TERAPAGOS_NORMAL; item = ITEM_NONE; gimmick = GIMMICK_TERA; expectedSpecies = SPECIES_TERAPAGOS_NORMAL; gmax = FALSE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(species) {
            Item(item); Speed(100); Moves(MOVE_CELEBRATE);
            if (gmax) GigantamaxFactor(TRUE);
            if (gimmick == GIMMICK_TERA) TeraType(species == SPECIES_TERAPAGOS_NORMAL ? TYPE_STELLAR : TYPE_GRASS);
        }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: gimmick); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_FORM_CHANGE, player);
        if (expectedSpecies == SPECIES_GENGAR)
            MESSAGE("Gengar returned to normal and became Normal type!");
    } THEN {
        EXPECT_EQ(player->species, expectedSpecies);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), expectedSpecies);
        EXPECT_EQ(GetActiveGimmick(B_POSITION_PLAYER_LEFT), GIMMICK_NONE);
        EXPECT_EQ(player->types[0], TYPE_NORMAL);
        EXPECT_EQ(player->types[1], TYPE_NORMAL);
        EXPECT_EQ(player->types[2], TYPE_MYSTERY);
        EXPECT_EQ(player->ability, GetMonAbility(&gPlayerParty[0]));
        EXPECT_EQ(player->attack, GetMonData(&gPlayerParty[0], MON_DATA_ATK));
        EXPECT_EQ(player->defense, GetMonData(&gPlayerParty[0], MON_DATA_DEF));
        if (gimmick == GIMMICK_DYNAMAX)
        {
            EXPECT_EQ(player->maxHP, sOriginalMaxHP);
            EXPECT_EQ(player->hp, sOriginalHP);
            EXPECT_EQ(gBattleStruct->dynamax.dynamaxTurns[B_POSITION_PLAYER_LEFT], 0);
        }
        if (gimmick != GIMMICK_NONE)
            EXPECT_EQ(HasTrainerUsedGimmick(B_POSITION_PLAYER_LEFT, gimmick), TRUE);
        EXPECT_EQ(gBattleWeather & (B_WEATHER_SUN_PRIMAL | B_WEATHER_RAIN_PRIMAL), 0);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishOutcome, CASTORIA_WISH_NONE);
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_DOMINATION), FALSE);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality ends Dynamax before the target acts and preserves damaged HP")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_WOBBUFFET) { HP(101); Speed(1); Moves(MOVE_WATER_GUN); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1000); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN, gimmick: GIMMICK_DYNAMAX); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        MESSAGE("Wobbuffet used Water Gun!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, player);
    } THEN {
        EXPECT_EQ(player->hp, sOriginalHP);
        EXPECT_EQ(player->maxHP, sOriginalMaxHP);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality leaves one-off Z-Moves alone")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_NORMALIUM_Z); Speed(1); Moves(MOVE_SCRATCH); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1000); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH, gimmick: GIMMICK_Z_MOVE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        MESSAGE("Wobbuffet used Breakneck Blitz!");
    } THEN {
        EXPECT_EQ(HasTrainerUsedGimmick(B_POSITION_PLAYER_LEFT, GIMMICK_Z_MOVE), TRUE);
        EXPECT_EQ(GetActiveGimmick(B_POSITION_PLAYER_LEFT), GIMMICK_NONE);
    }
}

DOUBLE_BATTLE_TEST("Arcanum Wish Normality targets the opposite opponent with a living doubles fallback")
{
    GIVEN {
        PLAYER(SPECIES_GENGAR) { Speed(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_STEELIX) { Speed(90); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(80); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_CELEBRATE); MOVE(playerRight, MOVE_CELEBRATE); }
    } THEN {
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        EXPECT_EQ(ExecuteNormality(), TRUE);
        EXPECT_EQ(playerLeft->types[0], TYPE_NORMAL);
        EXPECT_EQ(playerRight->types[0], TYPE_STEEL);
        EXPECT_EQ(playerRight->types[1], TYPE_GROUND);
        // Eligibility falls back to the surviving opponent in doubles.
        gBattleStruct->castoria.wishUsed = FALSE;
        playerLeft->hp = 0;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_NORMALITY), TRUE);
        EXPECT_EQ(ExecuteNormality(), TRUE);
        EXPECT_EQ(playerRight->types[0], TYPE_NORMAL);
        gBattleStruct->castoria.wishUsed = FALSE;
        playerRight->hp = 0;
        EXPECT_EQ(CastoriaCanWishFor(B_POSITION_OPPONENT_LEFT, CASTORIA_WISH_NORMALITY), FALSE);
        EXPECT_EQ(ExecuteNormality(), FALSE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality removes Tera immunity before a later attack")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_GENGAR) { Speed(100); TeraType(TYPE_GHOST); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_SCRATCH); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: GIMMICK_TERA); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponent);
        HP_BAR(player);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality changes the target's type through Protect")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_STEELIX) { Speed(100); Moves(MOVE_PROTECT); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_PROTECT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_FORM_CHANGE, player);
    } THEN {
        EXPECT_EQ(player->types[0], TYPE_NORMAL);
        EXPECT_EQ(player->types[1], TYPE_NORMAL);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality type change clears on switching and Tera stays spent")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_GENGAR) { Speed(100); TeraType(TYPE_GHOST); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(90); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: GIMMICK_TERA); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
    } THEN {
        EXPECT_EQ(GetActiveGimmick(B_POSITION_PLAYER_LEFT), GIMMICK_NONE);
        EXPECT_EQ(HasTrainerUsedGimmick(B_POSITION_PLAYER_LEFT, GIMMICK_TERA), TRUE);
        EXPECT_EQ(player->types[0], TYPE_GHOST);
        EXPECT_EQ(player->types[1], TYPE_POISON);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Normality interruption preserves Wish and the target's gimmick")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        NormalityFixture();
        PLAYER(SPECIES_GENGAR) { Speed(100); TeraType(TYPE_GHOST); Moves(MOVE_IRON_HEAD); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_IRON_HEAD, gimmick: GIMMICK_TERA); }
    } SCENE {
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_FORM_CHANGE, player);
        }
    } THEN {
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
        EXPECT_EQ(GetActiveGimmick(B_POSITION_PLAYER_LEFT), GIMMICK_TERA);
    }
}
