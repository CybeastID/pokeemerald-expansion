#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_break_bar.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "castoria_name_theft.h"

extern AiScoreFunc sDynamicAiFunc;
extern const u8 BattleScript_TxRollbackEndurance[];

static bool8 sSetup;
static bool8 sDefaultEntry;
static bool8 sTie;
static u8 sWish;
static u8 sSeed;
static u8 sRecipient;
static u8 sStealCount;
static u32 sSeedHp;
static u16 sRecoveryHp;


enum
{
    SEED_NONE, SEED_BOOSTS, SEED_THEFT, SEED_RECOVERY, SEED_RESTORE,
    SEED_TEMPORARY, SEED_REDUCTION, SEED_BLOCKED, SEED_SUBSTITUTE, SEED_PLAYER_BAR,
    SEED_PENDING_DOOM,
};

static void SetTxHp(u32 hp)
{
    u32 tx = B_POSITION_OPPONENT_RIGHT;
    gBattleMons[tx].maxHP = 400;
    gBattleMons[tx].hp = hp;
    SetMonData(GetBattlerMon(tx), MON_DATA_MAX_HP, &gBattleMons[tx].maxHP);
    SetMonData(GetBattlerMon(tx), MON_DATA_HP, &gBattleMons[tx].hp);
    // This fixture intentionally supplies a synthetic canonical maxHP.
    // Seed the registered overlay baseline with the same value.
    struct TxRollbackHealth *health = &gBattleStruct->txRollback.health;
    health->originalMaxHp = health->appliedMaxHp = 400;
    health->percent = 100;
    health->hpFraction = ((u64)hp << 24) / 400;
    health->lastHp = hp;
}

static void BS_SeedTxWish(void)
{
    NATIVE_ARGS();
    u32 p = B_POSITION_PLAYER_LEFT;
    u32 tx = B_POSITION_OPPONENT_RIGHT;
    SetTxHp(sSeed == SEED_RECOVERY ? sRecoveryHp : 400);
    if (sSeed == SEED_BOOSTS)
    {
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            gBattleMons[p].statStages[stat] = DEFAULT_STAT_STAGE + stat % 6 + 1;
        gBattleMons[p].statStages[STAT_DEF] = DEFAULT_STAT_STAGE - 2;
    }
    if (sSeed == SEED_THEFT)
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            gBattleMons[p].statStages[stat] = DEFAULT_STAT_STAGE + 2;
    if (sSeed == SEED_TEMPORARY)
    {
        gSideStatuses[B_SIDE_PLAYER] |= SIDE_STATUS_REFLECT;
        gSideTimers[B_SIDE_PLAYER].reflectTimer = 8;
    }
    if (sSeed == SEED_SUBSTITUTE)
    {
        gBattleMons[p].volatiles.substitute = TRUE;
        gDisableStructs[p].substituteHP = 30;
    }
    if (sSeed == SEED_RESTORE)
    {
        GetBattlerPartyState(tx)->breakBarsRemaining = 1;
        SetTxHp(123);
    }
    if (sSeed == SEED_PLAYER_BAR)
        GrantSavedBreakBar(p);
    if (sSeed == SEED_REDUCTION)
    {
        ReductionActivate();
        gBattleStruct->reduction.nextLoss = 27;
    }
    if (sSeed == SEED_BLOCKED)
    {
        gBattleMons[tx].status1 = STATUS1_SLEEP_TURN(7);
        SetMonData(GetBattlerMon(tx), MON_DATA_STATUS, &gBattleMons[tx].status1);
        gDisableStructs[tx].tauntTimer = 7;
    }
    if (sSeed == SEED_PENDING_DOOM)
    {
        gWishFutureKnock.futureSightMove[p] = MOVE_FUTURE_SIGHT;
        gWishFutureKnock.futureSightCounter[p] = 6;
        gWishFutureKnock.futureSightBattlerIndex[p] = tx;
        gWishFutureKnock.futureSightPartyIndex[p] = gBattlerPartyIndexes[tx];
    }
    sSeedHp = gBattleMons[tx].hp;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

static const struct __attribute__((packed))
{
    u8 callnative;
    void (*func)(void);
    u8 ret;
} sSeedScript = {0xff, BS_SeedTxWish, 0x3c};

static const struct TxRollbackProfile sSeedProfile = {.ambushScript = (const u8 *)&sSeedScript};
static const struct TxRollbackProfile sEntryProfile = {.ambushScript = BattleScript_TxRollbackEndurance};

static s32 TxWishScore(u32 attacker, u32 target, u32 move, s32 score)
{
    if (sSetup)
    {
        sSetup = FALSE;
        gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
        gBattleStruct->txRollback.profile = sDefaultEntry ? &sEntryProfile : &sSeedProfile;
    }
    if (!BattleAI_IsScoringArcanum())
    {
        if (sSeed == SEED_BLOCKED && TxRollbackHasWishes(attacker))
            return move == MOVE_POWER_GEM ? 100 : 0;
        return move == (TxRollbackHasWishes(attacker) ? MOVE_WIDE_GUARD : MOVE_CELEBRATE) ? 100 : 0;
    }
    if (move == MOVE_ARCANUM_WISH && CastoriaCanWishFor(attacker, CASTORIA_WISH_MORE_WISHES))
    {
        gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_MORE_WISHES;
        return 250;
    }
    if (TxRollbackCanWishFor(attacker, sWish, sRecipient))
    {
        gBattleStruct->txRollback.wishCandidate = sWish;
        gBattleStruct->txRollback.wishCandidateTarget = sRecipient;
        if (sStealCount)
            gBattleTestRunnerState->data.battleRecordTurns[gBattleResults.battleTurnCounter][attacker].rng
                = (struct RiggedRNG){RNG_TX_RECLAMATION_COUNT, sStealCount};
        return sTie ? 100 : 250;
    }
    return 0;
}

static void TxWishFixture(u32 wish, u32 seed)
{
    sDynamicAiFunc = TxWishScore;
    sSetup = TRUE;
    sDefaultEntry = FALSE;
    sTie = FALSE;
    sWish = wish;
    sSeed = seed;
    sRecipient = B_POSITION_OPPONENT_RIGHT;
    sStealCount = 0;
    sSeedHp = 0;
    sRecoveryHp = 100;
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reversal directly reverses all positive stages")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_REVERSAL, SEED_BOOSTS);
        PLAYER(SPECIES_SHUCKLE) { Ability(ABILITY_CONTRARY); Speed(1); Moves(MOVE_PROTECT); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_PROTECT); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_PROTECT); }
    } SCENE {
        MESSAGE("A wish for Reversal turned your team's boosts into penalties!");
    } THEN {
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            EXPECT_EQ(player->statStages[stat], stat == STAT_DEF ? DEFAULT_STAT_STAGE - 2 : DEFAULT_STAT_STAGE - stat % 6 - 1);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_REVERSAL, 3), FALSE);
        EXPECT_EQ(gBattleMons[3].pp[3], GetMovePP(MOVE_POWER_GEM));
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes ordinary moves win ties without committing a Wish")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_NONE);
        sTie = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(&gBattleMons[3], MOVE_WIDE_GUARD); }
    } THEN {
        EXPECT_EQ(gBattleStruct->txRollback.reciprocity, FALSE);
        EXPECT_EQ(gBattleStruct->txRollback.wishOutcome, TX_WISH_NONE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Calamity bypasses Soundproof and uses ordinary countdown timing")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_CALAMITY, SEED_NONE);
        PLAYER(SPECIES_EXPLOUD) { Speed(1); Ability(ABILITY_SOUNDPROOF); Moves(MOVE_PROTECT); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_PROTECT); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_PROTECT); }
    } SCENE {
        MESSAGE("A wish for Calamity doomed your active Pokémon!");
        MESSAGE("Exploud's perish count fell to 3!");
        MESSAGE("Exploud's perish count fell to 2!");
        NOT MESSAGE("A wish for Calamity doomed your active Pokémon!");
    } THEN {
        EXPECT_EQ((u32)player->volatiles.perishSong, TRUE);
        EXPECT_EQ(gDisableStructs[0].perishSongTimer, 1);
        EXPECT_EQ((u32)opponent->volatiles.perishSong, FALSE);
        EXPECT_EQ((u32)gBattleMons[3].volatiles.perishSong, FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Calamity clears on switching and can affect the new occupant")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_CALAMITY, SEED_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WYNAUT) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet's perish count fell to 3!");
        MESSAGE("Wynaut's perish count fell to 3!");
    } THEN {
        EXPECT_EQ(gDisableStructs[0].perishSongTimer, 2);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reclamation transfers a side benefit with its remaining duration")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECLAMATION, SEED_TEMPORARY);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("TX Rollback reclaimed your team's benefits!");
    } THEN {
        EXPECT_EQ(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_REFLECT, 0);
        EXPECT_NE(gSideStatuses[B_SIDE_OPPONENT] & SIDE_STATUS_REFLECT, 0);
        EXPECT_EQ(gSideTimers[B_SIDE_OPPONENT].reflectTimer, 6);
        EXPECT_EQ(gSideTimers[B_SIDE_PLAYER].reflectTimer, 0);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reclamation steals one to three distinct boosted stats")
{
    u32 expected;
    PARAMETRIZE { expected = 1; }
    PARAMETRIZE { expected = 2; }
    PARAMETRIZE { expected = 3; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECLAMATION, SEED_THEFT);
        sStealCount = expected;
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 stolen = 0;
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
        {
            if (gBattleMons[3].statStages[stat] == DEFAULT_STAT_STAGE + 2)
            {
                stolen++;
                EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE);
            }
            else
                EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE + 2);
        }
        EXPECT_EQ(stolen, expected);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reclamation transfers proportional Substitute HP and its presentation")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECLAMATION, SEED_SUBSTITUTE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); MaxHP(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_FADE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_SUBSTITUTE_APPEAR, &gBattleMons[3]);
    } THEN {
        EXPECT_EQ((u32)player->volatiles.substitute, FALSE);
        EXPECT_EQ((u32)gBattleMons[3].volatiles.substitute, TRUE);
        EXPECT_EQ(gDisableStructs[3].substituteHP, 120);
        EXPECT_EQ(gDisableStructs[0].substituteHP, 0);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reciprocity mirrors actual recovery proportionally and bypasses Heal Block")
{
    u32 source;
    PARAMETRIZE { source = MOVE_RECOVER; }
    PARAMETRIZE { source = MOVE_CELEBRATE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_RECOVERY);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(100); HP(80); Speed(1); Moves(MOVE_CELEBRATE, MOVE_RECOVER, MOVE_HEAL_BLOCK); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_HEAL_BLOCK, target: &gBattleMons[3]); }
        if (source == MOVE_RECOVER)
            TURN { MOVE(player, MOVE_RECOVER); }
        else
            TURN { USE_ITEM(player, ITEM_POTION, partyIndex: 0); }
    } SCENE {
        MESSAGE("TX Rollback wished for Reciprocity!");
        HP_BAR(player, damage: -20);
        HP_BAR(&gBattleMons[3], damage: -80);
        MESSAGE("Reciprocity restored TX Rollback's HP!");
    } THEN {
        // Casting Reciprocity also triggers Wishful Thinking's 1/16 max-HP heal.
        EXPECT_EQ(gBattleMons[3].hp, sSeedHp + 400 / 16 + 80);
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RECIPROCITY, 3), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Restoration preserves the covered HP and historical layer ceiling")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RESTORATION, SEED_RESTORE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("A wish for Restoration restored an exhausted bar!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(3), 2);
        EXPECT_EQ(gBattleMons[3].hp, 400);
        EXPECT_EQ(GetBattlerPartyState(3)->breakBarSavedHp[1], 123);
        EXPECT_EQ((u32)GetBattlerPartyState(3)->breakBarsPeak, 2);
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 3), FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes arrival Endurance grants Castoria a saved full layer")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_NONE, SEED_NONE);
        sDefaultEntry = TRUE;
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { MaxHP(600); HP(123); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(GetBreakBarCount(1), 1);
        EXPECT_EQ(opponent->hp, 600);
        EXPECT_EQ(GetBattlerPartyState(1)->breakBarSavedHp[0], 123);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes original breaks schedule Wish and Reduction with Doom Desire")
{
    bool32 existingReduction;
    PARAMETRIZE { existingReduction = FALSE; }
    PARAMETRIZE { existingReduction = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_NONE, existingReduction ? SEED_REDUCTION : SEED_NONE);
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(1000); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WIDE_GUARD, &gBattleMons[3]);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SHEER_COLD, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, &gBattleMons[3]);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WIDE_GUARD, &gBattleMons[3]);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SHEER_COLD, player);
        if (!existingReduction)
            MESSAGE("A wish for Reduction fell upon your team!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DOOM_DESIRE, &gBattleMons[3]);
    } THEN {
        EXPECT_EQ(GetBreakBarCount(3), 0);
        EXPECT_EQ(ReductionIsActive(), TRUE);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, existingReduction ? 29 : 10);
        EXPECT_EQ(gWishFutureKnock.futureSightMove[0], MOVE_DOOM_DESIRE);
        EXPECT_EQ(gWishFutureKnock.futureSightCounter[0], 2);
        EXPECT_EQ(gBattleMons[3].pp[1], GetMovePP(MOVE_WIDE_GUARD) - 2);
        EXPECT_EQ(gBattleMons[3].maxHP, 120);
        EXPECT_EQ(gBattleMons[3].hp, 120);
        EXPECT_EQ(gBattleStruct->txRollback.health.percent, 30);
        EXPECT_EQ(GetMonData(GetBattlerMon(3), MON_DATA_MAX_HP), 400);
    }
}

static u32 GrownByCount(u32 value, u32 count)
{
    while (count--)
        value = (value * 11 + 5) / 10;
    return value;
}

// The scripted first-break ordinary Wish (BattleScript_TxRollbackBreakWish) must trigger
// Wishful Thinking exactly once; the second break (Doom Desire) is not a Wish and must not.
AI_SINGLE_BATTLE_TEST("TX Wishes first-break Wish triggers Wishful Thinking once, Doom Desire does not")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_NONE, SEED_NONE);
        PLAYER(SPECIES_ARTICUNO) { Ability(ABILITY_WISHFUL_THINKING); Level(100); Speed(1000); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        // After the first break: Wish ran, so exactly one growth.
    } THEN {
        struct PartyState *state = GetBattlerPartyState(3);
        u32 total = 0;
        EXPECT_EQ(gBattleMons[3].ability, ABILITY_WISHFUL_THINKING);
        EXPECT_EQ(gBattleMons[3].maxHP, 240);
        EXPECT_EQ(gBattleMons[3].hp, 240);
        EXPECT_EQ(gBattleStruct->txRollback.health.percent, 60);
        EXPECT_EQ(GetMonData(GetBattlerMon(3), MON_DATA_MAX_HP), 400);
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            total += state->wishfulGrowth[stat];
        EXPECT_EQ(total, 1);
        total = 0;
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            total += GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->wishfulGrowth[stat];
        EXPECT_EQ(total, 1);
        EXPECT_EQ(gBattleMons[3].statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(gBattleMons[3].attack,    GrownByCount(GetMonData(GetBattlerMon(3), MON_DATA_ATK),   state->wishfulGrowth[STAT_ATK]));
        EXPECT_EQ(gBattleMons[3].defense,   GrownByCount(GetMonData(GetBattlerMon(3), MON_DATA_DEF),   state->wishfulGrowth[STAT_DEF]));
        EXPECT_EQ(gBattleMons[3].speed,     GrownByCount(GetMonData(GetBattlerMon(3), MON_DATA_SPEED), state->wishfulGrowth[STAT_SPEED]));
        EXPECT_EQ(gBattleMons[3].spAttack,  GrownByCount(GetMonData(GetBattlerMon(3), MON_DATA_SPATK), state->wishfulGrowth[STAT_SPATK]));
        EXPECT_EQ(gBattleMons[3].spDefense, GrownByCount(GetMonData(GetBattlerMon(3), MON_DATA_SPDEF), state->wishfulGrowth[STAT_SPDEF]));
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes scripted Reversal activates an opposing Wishful Thinking holder")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_REVERSAL, SEED_BOOSTS);
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("A wish for Reversal turned your team's boosts into penalties!");
        ABILITY_POPUP(player, ABILITY_WISHFUL_THINKING);
        HP_BAR(player, damage: -20);
        MESSAGE("Wynaut's HP was restored.");
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        ABILITY_POPUP(&gBattleMons[3], ABILITY_WISHFUL_THINKING);
    } THEN {
        u32 total = 0;
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            total += GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->wishfulGrowth[stat];
        EXPECT_EQ(total, 1);
        EXPECT_EQ(player->hp, 120);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Calamity expires through normal faint and saved bar handling")
{
    bool32 playerBar;
    PARAMETRIZE { playerBar = FALSE; }
    PARAMETRIZE { playerBar = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_CALAMITY, playerBar ? SEED_PLAYER_BAR : SEED_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("Wobbuffet's perish count fell to 3!");
        MESSAGE("Wobbuffet's perish count fell to 2!");
        MESSAGE("Wobbuffet's perish count fell to 1!");
        MESSAGE("Wobbuffet's perish count fell to 0!");
        if (!playerBar)
            MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(IsBattlerAlive(0), playerBar);
        EXPECT_EQ(GetBreakBarCount(0), 0);
        EXPECT_EQ(gBattleResults.playerFaintCounter, playerBar ? 0 : 1);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reciprocity excludes Pain Split and reserve healing")
{
    bool32 reserve;
    PARAMETRIZE { reserve = FALSE; }
    PARAMETRIZE { reserve = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_RECOVERY);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); HP(50); MaxHP(100); Moves(MOVE_CELEBRATE, MOVE_PAIN_SPLIT); }
        PLAYER(SPECIES_WYNAUT) { Speed(1); HP(10); MaxHP(100); }
        OPPONENT(SPECIES_CASTORIA) { HP(600); MaxHP(600); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        if (reserve)
            TURN { USE_ITEM(player, ITEM_POTION, partyIndex: 1); }
        else
            TURN { MOVE(player, MOVE_PAIN_SPLIT, target: opponent); }
    } SCENE {
        MESSAGE("TX Rollback wished for Reciprocity!");
        NOT MESSAGE("Reciprocity restored TX Rollback's HP!");
    } THEN {
        EXPECT_EQ(gBattleMons[3].hp, sSeedHp + 400 / 16);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Restoration saved layers do not replay original phase effects")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RESTORATION, SEED_RESTORE);
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(1000); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("A wish for Restoration restored an exhausted bar!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SHEER_COLD, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, &gBattleMons[3]);
        MESSAGE("A wish for Restoration restored an exhausted bar!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(3), 2);
        EXPECT_EQ(GetBattlerPartyState(3)->breakBarSavedHp[1], 123);
        EXPECT_EQ(gBattleStruct->txRollback.health.percent, 100);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reciprocity mirrors passive and ability recovery")
{
    bool32 ability;
    PARAMETRIZE { ability = FALSE; }
    PARAMETRIZE { ability = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_RECOVERY);
        PLAYER(SPECIES_LOTAD) { Speed(1); HP(80); MaxHP(100); Ability(ability ? ABILITY_RAIN_DISH : ABILITY_SWIFT_SWIM); Item(ability ? ITEM_NONE : ITEM_LEFTOVERS); Moves(MOVE_CELEBRATE, MOVE_RAIN_DANCE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, ability ? MOVE_RAIN_DANCE : MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattleMons[3].hp, sSeedHp + 400 / 16 + 48);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reciprocity mirrors draining recovery using the actual gain")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_RECOVERY);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); HP(1); MaxHP(100); Moves(MOVE_CELEBRATE, MOVE_GIGA_DRAIN); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_GIGA_DRAIN, target: opponent); }
    } THEN {
        EXPECT_GT(player->hp, 1);
        EXPECT_EQ(gBattleMons[3].hp, min(400, sSeedHp + 400 / 16 + (player->hp - 1) * 4));
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes Reciprocity rounds small gains and caps healing at the current layer")
{
    bool32 cap;
    PARAMETRIZE { cap = FALSE; }
    PARAMETRIZE { cap = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_RECOVERY);
        sRecoveryHp = cap ? 399 : 100;
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); HP(999); MaxHP(1000); Moves(MOVE_CELEBRATE, MOVE_RECOVER); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_RECOVER); }
    } THEN {
        EXPECT_EQ(gBattleMons[3].hp, cap ? 400 : sSeedHp + 400 / 16 + 1);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes break actions bypass turn blockers and preserve pending delayed attacks")
{
    bool32 occupied;
    PARAMETRIZE { occupied = FALSE; }
    PARAMETRIZE { occupied = TRUE; }
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_NONE, occupied ? SEED_PENDING_DOOM : SEED_BLOCKED);
        PLAYER(SPECIES_ARTICUNO) { Level(100); Speed(1000); Moves(MOVE_CELEBRATE, MOVE_SHEER_COLD); }
        OPPONENT(SPECIES_CASTORIA) { Level(50); Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
        TURN { MOVE(player, MOVE_SHEER_COLD, target: &gBattleMons[3]); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, &gBattleMons[3]);
        MESSAGE("A wish for Reduction fell upon your team!");
        if (occupied)
            NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_DOOM_DESIRE, &gBattleMons[3]);
        else
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DOOM_DESIRE, &gBattleMons[3]);
    } THEN {
        EXPECT_EQ(ReductionIsActive(), TRUE);
        EXPECT_EQ(gWishFutureKnock.futureSightMove[0], occupied ? MOVE_FUTURE_SIGHT : MOVE_DOOM_DESIRE);
        EXPECT_EQ(gWishFutureKnock.futureSightCounter[0], occupied ? 3 : 2);
        if (!occupied)
            EXPECT_EQ(gBattleMons[3].status1 & STATUS1_SLEEP, STATUS1_SLEEP_TURN(5));
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes ownership and Restoration eligibility reject replacement and pending layers")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_NONE, SEED_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 1), FALSE);
        EXPECT_EQ(GrantSavedBreakBar(1), TRUE);
        GetBattlerPartyState(1)->breakBarsRemaining = 0;
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 1), TRUE);
        GetBattlerPartyState(1)->breakBarPending = TRUE;
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 1), FALSE);
        GetBattlerPartyState(1)->breakBarPending = FALSE;
        gBattlerPartyIndexes[1] = 1;
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 1), FALSE);
        gBattlerPartyIndexes[1] = 0;
        gBattleMons[1].hp = 0;
        EXPECT_EQ(TxRollbackCanWishFor(3, TX_WISH_RESTORATION, 1), FALSE);
        gBattleMons[1].hp = 1;
        gBattleMons[3].species = SPECIES_JIRACHI;
        EXPECT_EQ(TxRollbackHasWishes(3), TRUE);
        EXPECT_EQ(GetBattlerMoveAtSlot(3, MAX_MON_MOVES), MOVE_ARCANUM_WISH);
        EXPECT_EQ(GetBattlerMoveSlotCount(3), MAX_MON_MOVES + 1);
        gDisableStructs[3].tauntTimer = 2;
        EXPECT_EQ(IsArcanumSlotUsable(3, MAX_MON_MOVES), FALSE);
        gDisableStructs[3].tauntTimer = 0;
        EXPECT_EQ(IsArcanumSlotUsable(3, MAX_MON_MOVES), TRUE);
        gBattleMons[3].species = SPECIES_TX_ROLLBACK;
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("TX Wishes interruption does not activate Reciprocity")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        TxWishFixture(TX_WISH_RECIPROCITY, SEED_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1000); Moves(MOVE_CELEBRATE, MOVE_TAUNT); }
        OPPONENT(SPECIES_CASTORIA) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        SUMMONED_OPPONENT();
        TURN { MOVE(player, MOVE_TAUNT, target: &gBattleMons[3]); }
    } SCENE {
        NOT MESSAGE("TX Rollback wished for Reciprocity!");
    } THEN {
        EXPECT_EQ(gBattleStruct->txRollback.reciprocity, FALSE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, TRUE);
    }
}
