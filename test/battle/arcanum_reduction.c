#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_controllers.h"
#include "battle_break_bar.h"
#include "battle_dynamax.h"
#include "battle_gimmick.h"
#include "battle_reduction.h"
#include "battle_util.h"
#include "castoria_name_theft.h"
#include "string_util.h"

extern AiScoreFunc sDynamicAiFunc;
extern void Test_RestorePlayerBattleStats(void);
static bool8 sSetup;
static bool8 sDirect;
static u8 sLevels[12];
static u16 sOriginalMaxHp;
static u16 sOriginalHp;

static s32 ReductionScore(u32 atk, u32 def, u32 move, s32 score)
{
    if (sSetup)
    {
        sSetup = FALSE;
        memset(sLevels, 0, sizeof(sLevels));
        sOriginalMaxHp = gBattleMons[0].maxHP;
        sOriginalHp = gBattleMons[0].hp;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        if (sDirect)
        {
            ReductionActivate();
            gBattleStruct->reduction.grace = FALSE;
        }
    }
    if (gBattleResults.battleTurnCounter < ARRAY_COUNT(sLevels))
        sLevels[gBattleResults.battleTurnCounter] = gBattleMons[0].level;
    if (!BattleAI_IsScoringArcanum())
        return 90;
    if (!sDirect && move == MOVE_ARCANUM_WISH && CastoriaCanWishFor(atk, CASTORIA_WISH_REDUCTION))
    {
        gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_REDUCTION;
        return score;
    }
    return 0;
}

static void Fixture(bool32 direct)
{
    sSetup = TRUE;
    sDirect = direct;
    sDynamicAiFunc = ReductionScore;
}

static void ReduceTo(u32 battler, u32 loss)
{
    if (!ReductionIsActive())
        ReductionActivate();
    ReductionBeginMutation(battler);
    gBattleStruct->reduction.occupant[battler].lostLevels = loss;
    ReductionEndMutation(battler);
}

static void InvokePolymorph(void)
{
    const struct __attribute__((packed)) {
        u8 opcode;
        void (*func)(void);
    } command = {0, BS_PolymorphTarget};
    const u8 *saved = gBattlescriptCurrInstr;
    gBattlerAttacker = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    gCurrentMove = MOVE_POLYMORPH;
    gBattlescriptCurrInstr = (const u8 *)&command;
    BS_PolymorphTarget();
    gBattlescriptCurrInstr = saved;
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction skips casting turn then decays 10 11 12 through Level 1")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        for (u32 i = 0; i < 9; i++)
            TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Wish!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
        MESSAGE("A wish for Reduction fell upon your team!");
        MESSAGE("Reduction lowered Wobbuffet to Level 90!");
        MESSAGE("Reduction lowered Wobbuffet to Level 79!");
        MESSAGE("Reduction lowered Wobbuffet to Level 1!");
    } THEN {
        static const u8 expected[] = {100, 100, 90, 79, 67, 54, 40, 25, 9};
        for (u32 i = 0; i < ARRAY_COUNT(expected); i++)
            EXPECT_EQ(sLevels[i], expected[i]);
        EXPECT_EQ(player->level, 1);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP), sOriginalMaxHp);
        EXPECT_EQ(player->hp, player->maxHP);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        Test_RestorePlayerBattleStats();
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP), sOriginalMaxHp);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), sOriginalHp);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction switching clears loss but preserves side acceleration")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(90); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
    } THEN {
        EXPECT_EQ(player->level, 88); // Fresh original mon, next shared tick is 12.
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, 13);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_LEVEL), 100);
    }
}

AI_DOUBLE_BATTLE_TEST("Arcanum Wish Reduction doubles apply one shared increment per turn")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(90); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(2); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_CELEBRATE); MOVE(playerRight, MOVE_CELEBRATE); }
        TURN { MOVE(playerLeft, MOVE_CELEBRATE); MOVE(playerRight, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(playerLeft->level, 79);
        EXPECT_EQ(playerRight->level, 79);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, 12);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction uses exact lower-level stats without changing party data")
{
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(100); Nature(NATURE_MODEST); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        // The runner assigns artificial turn-order speeds when none are supplied.
        CalculateMonStats(&gPlayerParty[0]);
        CopyMonLevelAndBaseStatsToBattleMon(0, &gPlayerParty[0], TRUE);
        struct Pokemon original = gPlayerParty[0];
        struct Pokemon scratch = original;
        CalculateMonStatsAtLevel(&scratch, 70);
        ReduceTo(0, 30);
        EXPECT_EQ(player->level, 70);
        EXPECT_EQ(player->maxHP, GetMonData(&scratch, MON_DATA_MAX_HP));
        EXPECT_EQ(player->attack, GetMonData(&scratch, MON_DATA_ATK));
        EXPECT_EQ(player->defense, GetMonData(&scratch, MON_DATA_DEF));
        EXPECT_EQ(player->speed, GetMonData(&scratch, MON_DATA_SPEED));
        EXPECT_EQ(player->spAttack, GetMonData(&scratch, MON_DATA_SPATK));
        EXPECT_EQ(player->spDefense, GetMonData(&scratch, MON_DATA_SPDEF));
        EXPECT_EQ(memcmp(&original, &gPlayerParty[0], sizeof(original)), 0);
        RecalcBattlerStats(0, &gPlayerParty[0], FALSE);
        EXPECT_EQ(player->level, 70);
        EXPECT_EQ(player->maxHP, GetMonData(&scratch, MON_DATA_MAX_HP));
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction rescaling alone never manufactures HP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); HP(193); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 originalHp = player->hp;
        for (u32 i = 0; i < 100; i++)
        {
            ReduceTo(0, 99);
            EXPECT_GT(player->hp, 0);
            ReductionBeginMutation(0);
            ReductionEndMutation(0);
        }
        ReductionFinishBattle();
        EXPECT_EQ(player->hp, originalHp);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), originalHp);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction normalizes full controller writes and preserves damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 exp = GetMonData(&gPlayerParty[0], MON_DATA_EXP);
        u32 maxHp = player->maxHP;
        ReduceTo(0, 60);
        player->hp /= 2;
        struct BattlePokemon packet = *player;
        packet.experience = 1;
        ReductionNormalizeRequest(0, REQUEST_ALL_BATTLE, 0, &packet);
        EXPECT_EQ(packet.level, 100);
        EXPECT_EQ(packet.experience, exp);
        EXPECT_EQ(packet.maxHP, maxHp);
        EXPECT_EQ(packet.attack, GetMonData(&gPlayerParty[0], MON_DATA_ATK));
        EXPECT_LE(abs((s32)packet.hp * player->maxHP - (s32)player->hp * (s32)maxHp), player->maxHP);
        packet = *player;
        packet.experience = 1;
        BtlController_EmitSetMonData(0, B_COMM_TO_CONTROLLER, REQUEST_ALL_BATTLE, 0, sizeof(packet), &packet);
        BtlController_HandleSetMonData(0);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_EXP), exp);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP), maxHp);
        u16 canonicalHp = GetMonData(&gPlayerParty[0], MON_DATA_HP);
        ReductionFinishBattle();
        EXPECT_EQ(player->hp, canonicalHp);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction healing items heal displayed HP")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); HP(50); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { USE_ITEM(player, ITEM_POTION, partyIndex: 0); }
    } SCENE {
        HP_BAR(player, damage: -20);
    } THEN {
        EXPECT_GT(GetMonData(&gPlayerParty[0], MON_DATA_HP), sOriginalHp + 20);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction protects Polymorph originals in both cast orders and repeated casts")
{
    bool32 polymorphFirst;
    PARAMETRIZE { polymorphFirst = FALSE; }
    PARAMETRIZE { polymorphFirst = TRUE; }
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(70); HP(101); MovesWithPP({MOVE_TACKLE, 7}, {MOVE_GROWL, 3}); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        struct Pokemon *mon = &gPlayerParty[0];
        u32 exp = GetMonData(mon, MON_DATA_EXP) + 17;
        SetMonData(mon, MON_DATA_EXP, &exp);
        StringCopy(mon->box.nickname, COMPOUND_STRING("Original"));
        // Native Polymorph has a substantial stack frame. Keep fixture copies
        // outside the runner's invocation stack.
        static struct Pokemon original;
        original = *mon;
        if (!polymorphFirst)
            ReduceTo(0, 30);
        InvokePolymorph();
        // Inspect immediately after capture, before any restoration/tick.
        const struct PolymorphBackup *saved = Test_GetPolymorphBackup(B_SIDE_PLAYER, 0);
        EXPECT_EQ(saved->exp, exp);
        s32 originalHp = GetMonData(&original, MON_DATA_HP);
        s32 originalMaxHp = GetMonData(&original, MON_DATA_MAX_HP);
        s32 formHp = GetMonData(mon, MON_DATA_HP);
        s32 formMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
        EXPECT_LE(abs(formHp * originalMaxHp - originalHp * formMaxHp), originalMaxHp);
        EXPECT_EQ((u32)GetBattlerPartyState(0)->changedSpecies, SPECIES_VENUSAUR);
        EXPECT_EQ(StringCompare(saved->nickname, original.box.nickname), 0);
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
        {
            EXPECT_EQ(saved->moves[i], GetMonData(&original, MON_DATA_MOVE1 + i));
            EXPECT_EQ(saved->pp[i], GetMonData(&original, MON_DATA_PP1 + i));
        }
        static struct PolymorphBackup backup;
        backup = *saved;
        ReduceTo(0, 40);
        InvokePolymorph();
        EXPECT_EQ(memcmp(saved, &backup, sizeof(backup)), 0);
        EXPECT_EQ(player->level, 30);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), 70);
        EXPECT_EQ(GetMonData(mon, MON_DATA_EXP), gExperienceTables[gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES)].growthRate][70]);
        // Leaving restores the current Polymorph form's level, not its species.
        u32 form = GetMonData(mon, MON_DATA_SPECIES);
        ReductionResetOccupant(0);
        EXPECT_EQ(player->level, 70);
        EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), form);
        EXPECT_EQ(memcmp(saved, &backup, sizeof(backup)), 0);
        EXPECT_EQ(TryRestorePolymorphedMon(0), TRUE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_VENUSAUR);
        EXPECT_EQ(GetMonData(mon, MON_DATA_EXP), exp);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), 70);
        EXPECT_EQ(StringCompare(mon->box.nickname, original.box.nickname), 0);
        static struct Pokemon restoredStats;
        restoredStats = original;
        CalculateMonStats(&restoredStats);
        for (u32 i = STAT_ATK; i < NUM_STATS; i++)
            EXPECT_EQ(GetMonData(mon, MON_DATA_MAX_HP + i), GetMonData(&restoredStats, MON_DATA_MAX_HP + i));
        EXPECT_GT(GetMonData(mon, MON_DATA_HP), 0);
        EXPECT_LE(abs((s32)GetMonData(mon, MON_DATA_HP) - (s32)GetMonData(&original, MON_DATA_HP)), 3);
        for (u32 i = 0; i < MAX_MON_MOVES; i++)
        {
            EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1 + i), GetMonData(&original, MON_DATA_MOVE1 + i));
            EXPECT_EQ(GetMonData(mon, MON_DATA_PP1 + i), GetMonData(&original, MON_DATA_PP1 + i));
        }
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction Break Bars retain loss and store canonical covered HP")
{
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(100); HP(101); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 maxHp = player->maxHP;
        u32 hp = player->hp;
        ReduceTo(0, 30);
        EXPECT_EQ(GrantSavedBreakBar(0), TRUE);
        EXPECT_EQ(GetBattlerPartyState(0)->breakBarSavedMaxHp[0], maxHp);
        EXPECT_EQ(GetBattlerPartyState(0)->breakBarSavedHp[0], hp);
        EXPECT_EQ(player->level, 70);
        RecalcBattlerStats(0, &gPlayerParty[0], FALSE);
        EXPECT_EQ(player->level, 70);
        EXPECT_EQ(gBattleStruct->reduction.occupant[0].lostLevels, 30);
        // Reveal the saved layer through the same native used by break scripts.
        const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
        } command = {0, BS_RestoreSavedBreakBar};
        const u8 *saved = gBattlescriptCurrInstr;
        GetBattlerPartyState(0)->breakBarsRemaining = 0;
        player->hp = 0;
        gBattlerTarget = 0;
        gBattlescriptCurrInstr = (const u8 *)&command;
        BS_RestoreSavedBreakBar();
        gBattlescriptCurrInstr = saved;
        EXPECT_EQ(player->level, 70);
        EXPECT_LE(abs((s32)player->hp * (s32)maxHp - (s32)hp * player->maxHP), maxHp);
        EXPECT_EQ(gBattleStruct->reduction.occupant[0].lostLevels, 30);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction faint cleanup never revives and revival starts unreduced")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ReduceTo(0, 99);
        player->hp = 0;
        ReductionResetOccupant(0);
        EXPECT_EQ(player->hp, 0);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
        EXPECT_EQ(player->level, 100);
        EXPECT_EQ(gBattleStruct->reduction.occupant[0].lostLevels, 0);
        EXPECT_EQ(ReductionIsActive(), TRUE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction eligibility requires fake or backed-out name and an unused Wish")
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
                EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_REDUCTION), !used && (route == CASTORIA_NAME_FALSE || route == CASTORIA_NAME_BACKED_OUT));
            }
        }
        gBattleStruct->castoria.wishUsed = FALSE;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        ReductionActivate();
        EXPECT_EQ(CastoriaCanWishFor(1, CASTORIA_WISH_REDUCTION), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction survives gimmick activation and Dynamax expiration")
{
    u32 species, item, gimmick;
    bool32 gmax;
    PARAMETRIZE { species = SPECIES_VENUSAUR; item = ITEM_VENUSAURITE; gimmick = GIMMICK_MEGA; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; item = ITEM_NONE; gimmick = GIMMICK_DYNAMAX; gmax = FALSE; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; item = ITEM_NONE; gimmick = GIMMICK_DYNAMAX; gmax = TRUE; }
    PARAMETRIZE { species = SPECIES_OGERPON_TEAL; item = ITEM_NONE; gimmick = GIMMICK_TERA; gmax = FALSE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(species) {
            Level(100); Speed(100); Item(item); Moves(MOVE_CELEBRATE);
            if (gmax) GigantamaxFactor(TRUE);
            if (gimmick == GIMMICK_TERA) TeraType(TYPE_GRASS);
        }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE, gimmick: gimmick); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->level, 54);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_EQ(player->hp, player->maxHP);
        if (gimmick == GIMMICK_DYNAMAX)
        {
            EXPECT_EQ(GetActiveGimmick(0), GIMMICK_NONE);
            EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP), sOriginalMaxHp);
        }
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction interrupted casts preserve the shared Wish use")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_GENGAR) { Speed(100); Moves(MOVE_IRON_HEAD); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_IRON_HEAD); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_ARCANUM_WISH, opponent);
    } THEN {
        EXPECT_EQ(ReductionIsActive(), FALSE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction survives Transform without copying reduced originals into the party")
{
    GIVEN {
        PLAYER(SPECIES_DITTO) { Level(100); Moves(MOVE_TRANSFORM); }
        OPPONENT(SPECIES_VENUSAUR);
    } WHEN {
        TURN { MOVE(player, MOVE_TRANSFORM); }
    } THEN {
        u32 copiedAttack = player->attack;
        ReduceTo(0, 50);
        EXPECT_EQ((u32)player->volatiles.transformed, TRUE);
        EXPECT_EQ(player->species, SPECIES_VENUSAUR);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), SPECIES_DITTO);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 100);
        EXPECT_LT(player->attack, copiedAttack);
        ReductionResetOccupant(0);
        EXPECT_EQ(player->attack, copiedAttack);
        EXPECT_EQ(player->species, SPECIES_VENUSAUR);
        u32 copiedDefense = player->defense;
        ReduceTo(0, 50);
        u16 tmp;
        SWAP(player->attack, player->defense, tmp);
        player->volatiles.powerTrick = TRUE;
        ReductionResetOccupant(0);
        EXPECT_EQ(player->attack, copiedDefense);
        EXPECT_EQ(player->defense, copiedAttack);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction and Polymorph cooperate with Break Bar battle cleanup")
{
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(70); HP(101); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        InvokePolymorph();
        ReduceTo(0, 40);
        EXPECT_EQ(GrantSavedBreakBar(0), TRUE);
        Test_RestorePlayerBattleStats();
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), SPECIES_VENUSAUR);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 70);
        u32 expectedHp = min(202, GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP));
        EXPECT_LE(abs((s32)GetMonData(&gPlayerParty[0], MON_DATA_HP) - (s32)expectedHp), 4);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction saturates its shared counter instead of wrapping")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(TRUE);
        PLAYER(SPECIES_WOBBUFFET) { Level(1); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        gBattleStruct->reduction.nextLoss = MAX_LEVEL;
        // Level 1 produces no script, so the end-turn stage can be inspected directly.
        gBattlerByTurnOrder[0] = 0;
        gBattlerByTurnOrder[1] = 1;
        gBattleStruct->eventState.endTurnBattler = 0;
        EXPECT_EQ(ReductionEndTurn(0), FALSE);
        EXPECT_EQ(ReductionEndTurn(1), FALSE);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, MAX_LEVEL);
        EXPECT_EQ(player->level, 1);
        EXPECT_GT(player->hp, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction pivots reset occupant loss without transferring it")
{
    u32 move;
    PARAMETRIZE { move = MOVE_BATON_PASS; }
    PARAMETRIZE { move = MOVE_U_TURN; }
    PARAMETRIZE { move = MOVE_TELEPORT; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE, move); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(90); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, move); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[0], 1);
        EXPECT_EQ(player->level, 89);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, 12);
    }
}

AI_SINGLE_BATTLE_TEST("Arcanum Wish Reduction forced switches preserve the side effect")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        Fixture(FALSE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(90); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(1); Moves(MOVE_ROAR); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[0], 1);
        EXPECT_EQ(player->level, 90);
        EXPECT_EQ(ReductionIsActive(), TRUE);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction genuine level gains preserve the accumulated loss")
{
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(50); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ReduceTo(0, 30);
        u32 exp = gExperienceTables[gSpeciesInfo[SPECIES_VENUSAUR].growthRate][51];
        SetMonData(&gPlayerParty[0], MON_DATA_EXP, &exp);
        CalculateMonStats(&gPlayerParty[0]);
        CopyMonLevelAndBaseStatsToBattleMon(0, &gPlayerParty[0], TRUE);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 51);
        EXPECT_EQ(player->level, 21);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_EXP), exp);
    }
}

SINGLE_BATTLE_TEST("Arcanum Wish Reduction Polymorph cleanup retains a genuine faint")
{
    GIVEN {
        PLAYER(SPECIES_VENUSAUR) { Level(70); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        ReduceTo(0, 40);
        InvokePolymorph();
        player->hp = 0;
        ReductionFinishBattle();
        EXPECT_EQ(TryRestorePolymorphedMon(0), TRUE);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), SPECIES_VENUSAUR);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 70);
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
    }
}
