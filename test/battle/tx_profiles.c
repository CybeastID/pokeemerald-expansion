#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_break_bar.h"
#include "battle_controllers.h"
#include "battle_koanko.h"
#include "battle_reduction.h"
#include "battle_setup.h"
#include "battle_tx_rollback.h"
#include "battle_util.h"
#include "malloc.h"
#include "pokemon.h"

extern AiScoreFunc sDynamicAiFunc;

static void ProfileUnitFixture(u32 profile)
{
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gBattlersCount = 2;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gAbsentBattlerFlags = 0;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    for (u32 b = 0; b < MAX_BATTLERS_COUNT; b++)
    {
        gBattlerPositions[b] = b;
        gBattlerPartyIndexes[b] = b == B_POSITION_OPPONENT_RIGHT ? 5 : 0;
        gBattleMons[b].hp = gBattleMons[b].maxHP = 400;
    }
    gBattleStruct->txRollback.combatProfile = profile;
    gBattleStruct->txRollback.ownerTrainer = B_TRAINER_OPPONENT_A;
    gBattleStruct->txRollback.partyIndex = 0;
    if (profile == TX_PROFILE_CASTORIA_SUMMON)
        gBattleStruct->txRollback.stage = TX_ROLLBACK_ACTIVE;
}

TEST("TX profiles separate combat ownership from summon formation and revival policy")
{
    ProfileUnitFixture(TX_PROFILE_KOANKO_SOLO);
    gBattlersCount = 4;
    EXPECT_EQ(TxRollbackHasWishes(1), TRUE);
    EXPECT_EQ(TxRollbackHasWishes(3), FALSE);
    EXPECT_EQ(TxRollbackGetOwnerBattler(), 1);
    EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 0), TRUE);
    EXPECT_EQ(TxRollbackGetSlotPolicy(2), TX_SLOT_NORMAL);
    EXPECT_EQ(TxRollbackGetSlotPolicy(3), TX_SLOT_NORMAL);
    EXPECT_EQ(GetBreakBarCount(1), 0);
    EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);

    gBattleStruct->txRollback.combatProfile = TX_PROFILE_CASTORIA_SUMMON;
    gBattleStruct->txRollback.stage = TX_ROLLBACK_ACTIVE;
    gBattleStruct->txRollback.partyIndex = 5;
    EXPECT_EQ(TxRollbackHasWishes(3), TRUE);
    EXPECT_EQ(TxRollbackHasWishes(1), FALSE);
    EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 5), FALSE);
    EXPECT_EQ(TxRollbackGetSlotPolicy(2), TX_SLOT_DISABLED);
    EXPECT_EQ(TxRollbackGetSlotPolicy(3), TX_SLOT_RESERVED);
    gBattleStruct->txRollback.stage = TX_ROLLBACK_DEFEATED;
    gBattleStruct->txRollback.ownerDefeated = TRUE;
    EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 5), FALSE);
    EXPECT_EQ(TxRollbackGetSlotPolicy(3), TX_SLOT_RETIRED);
    EXPECT_EQ(TxRollbackHasWishes(3), FALSE);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles expose explicit distinct Wish lists and species grants no ownership")
{
    ProfileUnitFixture(TX_PROFILE_KOANKO_SOLO);
    EXPECT_EQ(TxRollbackGetWishCount(1), 5);
    EXPECT_EQ(TxRollbackGetWishAtIndex(1, 3), TX_WISH_REDUCTION);
    EXPECT_EQ(TxRollbackGetWishAtIndex(1, 5), TX_WISH_NONE);
    EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_RESTORATION, 1), FALSE);
    EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_REDUCTION, 1), TRUE);
    gBattleStruct->txRollback.combatProfile = TX_PROFILE_CASTORIA_SUMMON;
    gBattleStruct->txRollback.stage = TX_ROLLBACK_ACTIVE;
    EXPECT_EQ(TxRollbackGetWishAtIndex(1, 3), TX_WISH_RESTORATION);
    EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_REDUCTION, 1), FALSE);
    gBattleStruct->txRollback.combatProfile = TX_PROFILE_NONE;
    gBattleMons[1].species = SPECIES_TX_ROLLBACK;
    EXPECT_EQ(TxRollbackGetWishCount(1), 0);
    EXPECT_EQ(TxRollbackHasWishes(1), FALSE);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("Koanko Reduction activation is idempotent and preserves accumulated occupant loss")
{
    ProfileUnitFixture(TX_PROFILE_NONE);
    EXPECT_EQ(ReductionTryActivate(0, B_SIDE_PLAYER), FALSE);
    EXPECT_EQ(ReductionTryActivate(MAX_LEVEL + 1, B_SIDE_PLAYER), FALSE);
    EXPECT_EQ(ReductionTryActivate(1, NUM_BATTLE_SIDES), FALSE);
    EXPECT_EQ(ReductionIsActive(), FALSE);
    EXPECT_EQ(ReductionTryActivate(1, B_SIDE_PLAYER), TRUE);
    EXPECT_EQ(gBattleStruct->reduction.grace, TRUE);
    EXPECT_EQ(gBattleStruct->reduction.nextLoss, 1);
    EXPECT_EQ(gBattleStruct->reduction.affectedSide, B_SIDE_PLAYER);
    gBattleStruct->reduction.nextLoss = 7;
    gBattleStruct->reduction.occupant[0].lostLevels = 21;
    EXPECT_EQ(ReductionTryActivate(1, B_SIDE_PLAYER), FALSE);
    ReductionActivate();
    EXPECT_EQ(gBattleStruct->reduction.nextLoss, 7);
    EXPECT_EQ(gBattleStruct->reduction.occupant[0].lostLevels, 21);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles retain the original ten-plus Reduction compatibility initializer")
{
    ProfileUnitFixture(TX_PROFILE_NONE);
    ReductionActivate();
    EXPECT_EQ(ReductionIsActive(), TRUE);
    EXPECT_EQ(gBattleStruct->reduction.nextLoss, 10);
    EXPECT_EQ(gBattleStruct->reduction.affectedSide, B_SIDE_PLAYER);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles maxHP view survives repeated clean reloads without healing or duplicate growth")
{
    ProfileUnitFixture(TX_PROFILE_CASTORIA_SUMMON);
    CreateMon(&gEnemyParty[0], SPECIES_TX_ROLLBACK, 100, 31, TRUE, 0, OT_ID_PRESET, 0);
    u16 maximum = 401, hp = 123;
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &maximum);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    PokemonToBattleMon(&gEnemyParty[0], &gBattleMons[1]);
    TxRollbackInitializeHealth(1);
    WishfulThinkingGrowStat(1, STAT_SPATK);
    WishfulThinkingGrowStat(1, STAT_SPATK);
    u16 grown = gBattleMons[1].spAttack;
    gBattleStruct->txRollback.health.percent = 60;
    CopyMonLevelAndBaseStatsToBattleMon(1, &gEnemyParty[0]);
    EXPECT_EQ(gBattleMons[1].maxHP, 240);
    EXPECT_EQ(gBattleMons[1].hp, 74);
    for (u32 i = 0; i < 32; i++)
    {
        u16 value = gBattleMons[1].hp;
        TxRollbackNormalizeRequest(1, REQUEST_HP_BATTLE, 0, &value);
        SetMonData(&gEnemyParty[0], MON_DATA_HP, &value);
        EXPECT_EQ(value, 123);
        value = gBattleMons[1].maxHP;
        TxRollbackNormalizeRequest(1, REQUEST_MAX_HP_BATTLE, 0, &value);
        EXPECT_EQ(value, 401);
        CopyMonLevelAndBaseStatsToBattleMon(1, &gEnemyParty[0]);
        EXPECT_EQ(gBattleMons[1].hp, 74);
        EXPECT_EQ(gBattleMons[1].maxHP, 240);
        EXPECT_EQ(gBattleMons[1].spAttack, grown);
    }
    gBattleStruct->txRollback.health.percent = 30;
    CopyMonLevelAndBaseStatsToBattleMon(1, &gEnemyParty[0]);
    EXPECT_EQ(gBattleMons[1].maxHP, 120); // 30% of 401, not 30% of 240.
    EXPECT_EQ(gBattleMons[1].hp, 37);
    EXPECT_EQ(GetMonData(&gEnemyParty[0], MON_DATA_MAX_HP), 401);
    TxRollbackFinishBattle();
    EXPECT_EQ(gBattleMons[1].maxHP, 401);
    EXPECT_EQ(gBattleMons[1].hp, 123);
    EXPECT_EQ(GetMonData(&gEnemyParty[0], MON_DATA_HP), 123);
    EXPECT_EQ(gBattleStruct->txRollback.combatProfile, TX_PROFILE_NONE);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles saved layers cover the current maxHP view without changing its phase")
{
    ProfileUnitFixture(TX_PROFILE_CASTORIA_SUMMON);
    CreateMon(&gEnemyParty[0], SPECIES_TX_ROLLBACK, 100, 31, TRUE, 0, OT_ID_PRESET, 0);
    PokemonToBattleMon(&gEnemyParty[0], &gBattleMons[1]);
    TxRollbackInitializeHealth(1);
    gBattleStruct->txRollback.health.percent = 60;
    CopyMonLevelAndBaseStatsToBattleMon(1, &gEnemyParty[0]);
    gBattleMons[1].hp = 17;
    u16 maximum = gBattleMons[1].maxHP;
    EXPECT_EQ(GrantSavedBreakBar(1), TRUE);
    EXPECT_EQ(GetBattlerPartyState(1)->breakBarSavedHp[0], 17);
    EXPECT_EQ(GetBattlerPartyState(1)->breakBarSavedMaxHp[0], maximum);
    EXPECT_EQ(gBattleStruct->txRollback.health.percent, 60);
    EXPECT_EQ(gBattleMons[1].hp, 17);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles maxHP floors at one and normalizes masked full controller records")
{
    ProfileUnitFixture(TX_PROFILE_CASTORIA_SUMMON);
    CreateMon(&gEnemyParty[0], SPECIES_TX_ROLLBACK, 100, 31, TRUE, 0, OT_ID_PRESET, 0);
    u16 maximum = 2, hp = 1;
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &maximum);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    PokemonToBattleMon(&gEnemyParty[0], &gBattleMons[1]);
    TxRollbackInitializeHealth(1);
    gBattleStruct->txRollback.health.percent = 30;
    CopyMonLevelAndBaseStatsToBattleMon(1, &gEnemyParty[0]);
    EXPECT_EQ(gBattleMons[1].maxHP, 1);
    EXPECT_EQ(gBattleMons[1].hp, 1);

    struct BattlePokemon copy = gBattleMons[1];
    copy.attack = 777;
    // Another party entry on the same side cannot inherit the owner's overlay.
    TxRollbackNormalizeRequest(1, REQUEST_ALL_BATTLE, 1u << 1, &copy);
    EXPECT_EQ(copy.maxHP, 1);
    // Re-emitting the rounded one-HP view must not heal its canonical fraction.
    TxRollbackNormalizeRequest(1, REQUEST_ALL_BATTLE, 1u << 0, &copy);
    EXPECT_EQ(copy.hp, 1);
    EXPECT_EQ(copy.maxHP, 2);
    EXPECT_EQ(copy.attack, 777);
    copy.hp = 0;
    TxRollbackNormalizeRequest(1, REQUEST_ALL_BATTLE, 0, &copy);
    EXPECT_EQ(copy.hp, 0);
    EXPECT_EQ(copy.maxHP, 2);
    EXPECT_EQ(GetMonData(&gEnemyParty[0], MON_DATA_MAX_HP), 2);
    FREE_AND_SET_NULL(gBattleStruct);
}

TEST("TX profiles pending breaks retain local effects and genuine faint clears only the owner")
{
    ProfileUnitFixture(TX_PROFILE_KOANKO_SOLO);
    ReductionTryActivate(1, B_SIDE_PLAYER);
    gBattleStruct->txRollback.reciprocity = TRUE;
    gBattleStruct->txRollback.wishOutcome = TX_WISH_RECIPROCITY;
    gBattleStruct->txRollback.wishTargetPartyIndex = 0;
    gBattleMons[1].hp = 0;
    GetBattlerPartyState(1)->breakBarPending = TRUE;
    TxRollbackOnFaint(1);
    EXPECT_EQ(gBattleStruct->txRollback.ownerDefeated, FALSE);
    EXPECT_EQ(gBattleStruct->txRollback.reciprocity, TRUE);
    EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_REDUCTION, 1), FALSE);
    GetBattlerPartyState(1)->breakBarPending = FALSE;
    TxRollbackOnFaint(1);
    EXPECT_EQ(gBattleStruct->txRollback.ownerDefeated, TRUE);
    EXPECT_EQ(gBattleStruct->txRollback.reciprocity, FALSE);
    EXPECT_EQ(gBattleStruct->txRollback.wishOutcome, TX_WISH_NONE);
    EXPECT_EQ(gBattleStruct->txRollback.wishTargetPartyIndex, PARTY_SIZE);
    EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);
    EXPECT_EQ(ReductionIsActive(), TRUE);
    EXPECT_EQ(gBattleStruct->reduction.nextLoss, 1);
    EXPECT_EQ(TxRollbackGetOwnerBattler(), MAX_BATTLERS_COUNT);
    FREE_AND_SET_NULL(gBattleStruct);
}

static bool8 sSoloSetup;
static u8 sSoloWish;

// Mechanism fixture: use real Koanko identity/entry routing, but inject the
// nominated Wish. Production tactical ranking belongs to the Stage05 gate.
static s32 SoloWishScore(u32 attacker, u32 target, u32 move, s32 score)
{
    if (sSoloSetup)
    {
        sSoloSetup = FALSE;
        gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
    }
    if (!BattleAI_IsScoringArcanum())
        return move == (TxRollbackHasWishes(attacker) ? MOVE_PROTECT : MOVE_CELEBRATE) ? 100 : 0;
    if (move == MOVE_ARCANUM_WISH && TxRollbackCanWishFor(attacker, sSoloWish, attacker))
    {
        gBattleStruct->txRollback.wishCandidate = sSoloWish;
        gBattleStruct->txRollback.wishCandidateTarget = attacker;
        return 250;
    }
    return 0;
}

static void SoloWishFixture(u32 wish)
{
    sSoloSetup = TRUE;
    sSoloWish = wish;
    sDynamicAiFunc = SoloWishScore;
}

AI_SINGLE_BATTLE_TEST("Koanko TX entry registers only solo combat capabilities without summon effects")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SoloWishFixture(TX_WISH_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_TX_ROLLBACK) { Level(101); Speed(1); Moves(MOVE_WISH, MOVE_DOOM_DESIRE, MOVE_PROTECT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(gBattleStruct->txRollback.combatProfile, TX_PROFILE_KOANKO_SOLO);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);
        EXPECT_EQ(gBattleStruct->txRollback.partyIndex, 1);
        EXPECT_EQ(gBattlersCount, 2);
        EXPECT_EQ(IsDoubleBattle(), FALSE);
        EXPECT_EQ(GetBreakBarCount(1), 0);
        EXPECT_EQ(HasNoOrderChange(1), TRUE);
        EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 1), TRUE);
        EXPECT_EQ(gBattleStruct->castoria.wishUsed, FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Reduction Wish skips its casting turn then applies one two three")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SoloWishFixture(TX_WISH_REDUCTION);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_TX_ROLLBACK) { Level(101); Speed(1); Ability(ABILITY_WISHFUL_THINKING); HP(100); MaxHP(400);
            Moves(MOVE_WISH, MOVE_DOOM_DESIRE, MOVE_PROTECT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
        for (u32 i = 0; i < 4; i++)
            TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("A wish for Reduction fell upon your team!");
        HP_BAR(opponent, damage: -25);
        MESSAGE("Reduction lowered Wobbuffet to Level 99!");
        MESSAGE("Reduction lowered Wobbuffet to Level 97!");
        MESSAGE("Reduction lowered Wobbuffet to Level 94!");
        NOT MESSAGE("A wish for Reduction fell upon your team!");
    } THEN {
        EXPECT_EQ(player->level, 94);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, 4);
        EXPECT_EQ(opponent->hp, 125);
        EXPECT_EQ(GetMonData(GetBattlerMon(0), MON_DATA_LEVEL), 100);
        EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_REDUCTION, 1), FALSE);
        u32 growth = 0;
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            growth += GetBattlerPartyState(1)->wishfulGrowth[stat];
        EXPECT_EQ(growth, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Reduction persists through four successive opponents after solo TX defeat")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SoloWishFixture(TX_WISH_REDUCTION);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_TX_ROLLBACK) { Level(101); Speed(1); HP(1); MaxHP(400); Moves(MOVE_WISH, MOVE_DOOM_DESIRE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        for (u32 i = 0; i < 3; i++)
            TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(ReductionIsActive(), TRUE);
        EXPECT_EQ(player->level, 85);
        EXPECT_EQ(gBattleStruct->reduction.nextLoss, 6);
        EXPECT_EQ(gBattleStruct->txRollback.ownerDefeated, TRUE);
        EXPECT_EQ(gBattleStruct->txRollback.stage, TX_ROLLBACK_NONE);
        EXPECT_EQ(gBattleStruct->txRollback.wishOutcome, TX_WISH_NONE);
        EXPECT_EQ(gBattleStruct->txRollback.reciprocity, FALSE);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 5);
        EXPECT_EQ(TxRollbackHasWishes(1), FALSE);
        EXPECT_EQ(TxRollbackCanRevive(B_TRAINER_OPPONENT_A, 1), TRUE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko TX Reciprocity heals the registered left owner proportionally")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SoloWishFixture(TX_WISH_RECIPROCITY);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); HP(80); MaxHP(100); Speed(1000);
            Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE, MOVE_RECOVER); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_TX_ROLLBACK) { HP(100); MaxHP(400); Speed(1); Ability(ABILITY_WISHFUL_THINKING);
            Moves(MOVE_WISH, MOVE_DOOM_DESIRE, MOVE_PROTECT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 1); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_RECOVER); }
    } SCENE {
        MESSAGE("TX Rollback wished for Reciprocity!");
        HP_BAR(opponent, damage: -25);
        HP_BAR(player, damage: -20);
        HP_BAR(opponent, damage: -80);
        MESSAGE("Reciprocity restored TX Rollback's HP!");
    } THEN {
        EXPECT_EQ(opponent->hp, 205);
        EXPECT_EQ(TxRollbackGetOwnerBattler(), 1);
        EXPECT_EQ(TxRollbackCanWishFor(1, TX_WISH_RECIPROCITY, 1), FALSE);
    }
}
