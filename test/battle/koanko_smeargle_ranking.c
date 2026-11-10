#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "battle_koanko_smeargle_diagnostics.h"
#include "constants/battle_ai.h"

static void SmeargleFixture(void)
{
    AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
    KoankoSmeargleDiagDisable();
    OPPONENT(SPECIES_SMEARGLE) {
        Level(100); HP(251); MaxHP(251); Attack(339); SpAttack(339);
        Defense(95); SpDefense(126); Speed(205); Ability(ABILITY_OWN_TEMPO);
        Item(ITEM_FOCUS_SASH); Moves(MOVE_CELEBRATE);
    }
}

static void PrepareRanking(void)
{
    u32 battler = B_POSITION_OPPONENT_LEFT;
    gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
    Test_SetKoankoSmeargleScorer(NULL);
    SetDynamicAiFunc(AI_KoankoTrainer);
    gAiThinkingStruct->aiFlags[battler] = GetTrainerAIFlagsFromId(TRAINER_KOANKO)
        | AI_FLAG_CHECK_VIABILITY | AI_FLAG_DYNAMIC_FUNC | AI_FLAG_SEQUENCE_SWITCHING;
    gAiLogicData->aiCalcInProgress = TRUE;
    SetAIUsingGimmick(battler, USE_GIMMICK);
    SetBattlerAiData(battler, gAiLogicData);
    SetBattlerAiData(B_POSITION_PLAYER_LEFT, gAiLogicData);
}

static s32 Rank(u16 move)
{
    u32 battler = B_POSITION_OPPONENT_LEFT;
    gAiThinkingStruct->aiFlags[battler] &= ~AI_FLAG_DYNAMIC_FUNC;
    s32 ordinary = BattleAI_ScoreSlot0Candidate(battler, move);
    gAiThinkingStruct->aiFlags[battler] |= AI_FLAG_DYNAMIC_FUNC;
    s32 ranked = BattleAI_ScoreSlot0Candidate(battler, move);
    const struct SimulatedDamage *damage = &gAiLogicData->simulatedDmg[battler][B_POSITION_PLAYER_LEFT][0];
    Test_MgbaPrintf("RANK %S ordinary=%d ranked=%d damage=%d/%d/%d targetHP=%d accuracy=%d", GetMoveName(move),
        ordinary, ranked, damage->minimum, damage->median, damage->maximum,
        gBattleMons[B_POSITION_PLAYER_LEFT].hp, gAiLogicData->moveAccuracy[battler][B_POSITION_PLAYER_LEFT][0]);
    return ranked;
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking substantial Alomomola damage beats Lucky Chant")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_ALOMOMOLA) { Level(100); HP(473); MaxHP(473); Defense(196); SpDefense(180); Speed(100); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 utility = Rank(MOVE_LUCKY_CHANT);
        s32 attack = Rank(MOVE_ELECTRO_DRIFT);
        EXPECT_GT(attack, utility);
        const struct SimulatedDamage *damage = &gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0];
        EXPECT_LT(damage->maximum, player->hp);
        EXPECT_GT(damage->median * 100 / player->hp, 70);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking immediate Serperior KO beats Flatter")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_SERPERIOR) { Level(100); HP(224); MaxHP(291); Defense(226); SpDefense(226); Speed(357); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 utility = Rank(MOVE_FLATTER);
        s32 attack = Rank(MOVE_ERUPTION);
        EXPECT_GT(attack, utility);
        EXPECT_GE(gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].minimum, player->hp);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking avoids Weavile KO recoil and recharge")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WEAVILE) { Level(100); HP(281); MaxHP(281); Defense(166); SpDefense(206); Speed(383); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 light = Rank(MOVE_LIGHT_OF_RUIN);
        s32 beam = Rank(MOVE_HYPER_BEAM);
        Rank(MOVE_BOOMBURST); // A partial-roll KO should not be called guaranteed.
        s32 safe = Rank(MOVE_CLOSE_COMBAT);
        EXPECT_GT(safe, light);
        EXPECT_GT(safe, beam);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking accurate KOs beat coin flips and equal KOs avoid costs")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); HP(10); MaxHP(300); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 inaccurate = Rank(MOVE_ZAP_CANNON);
        s32 recharge = Rank(MOVE_METEOR_ASSAULT);
        s32 recoil = Rank(MOVE_STEEL_BEAM);
        s32 safe = Rank(MOVE_FUSION_BOLT);
        EXPECT_GT(safe, inaccurate);
        EXPECT_GT(safe, recharge);
        EXPECT_GT(safe, recoil);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking useful recovery and sleep remain selectable")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); HP(1000); MaxHP(1000); Defense(9999); SpDefense(9999); Speed(100); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        opponent->hp = 20;
        PrepareRanking();
        s32 damage = Rank(MOVE_BOOMBURST);
        s32 recovery = Rank(MOVE_RECOVER);
        s32 sleep = Rank(MOVE_SPORE);
        EXPECT_GT(recovery, damage);
        EXPECT_GT(sleep, damage);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking rejects redundant screens and full HP recovery")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 wasted = Rank(MOVE_RECOVER);
        EXPECT_LE(wasted, 1);
        s32 screen = Rank(MOVE_REFLECT);
        gSideStatuses[B_SIDE_OPPONENT] |= SIDE_STATUS_REFLECT;
        s32 redundant = Rank(MOVE_REFLECT);
        EXPECT_GT(screen, redundant);
        EXPECT_LE(redundant, 1);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking uses HP dependent Eruption damage and Foul Play target Attack")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_DRAGAPULT) { Level(100); Speed(100); HP(317); MaxHP(317); Attack(339); Defense(186); SpDefense(186); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_FOUL_PLAY), TRUE);
        s32 foul = Rank(MOVE_FOUL_PLAY);
        u32 foulDamage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        opponent->attack = 50;
        EXPECT_EQ(Rank(MOVE_FOUL_PLAY), foul);
        EXPECT_EQ(gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median, foulDamage);
        s32 eruption = Rank(MOVE_ERUPTION);
        opponent->hp = 10;
        SetBattlerAiData(B_POSITION_OPPONENT_LEFT, gAiLogicData);
        EXPECT_LT(Rank(MOVE_ERUPTION), eruption);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking trainer callback is isolated and sampler replays tagged RNG")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_SERPERIOR) { Level(100); Speed(100); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        gBattleStruct->koankoPhase = KOANKO_ORDINARY_TEAM;
        EXPECT_EQ(AI_KoankoTrainer(B_POSITION_OPPONENT_LEFT, B_POSITION_PLAYER_LEFT, MOVE_BOOMBURST, 103), 103);
        gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(AI_KoankoTrainer(B_POSITION_OPPONENT_LEFT, B_POSITION_PLAYER_LEFT, MOVE_BOOMBURST, 103), 103);
        opponent->species = SPECIES_SMEARGLE;
        // Compare work on the same fixture, not a different target/stat spread.
        gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] &= ~AI_FLAG_DYNAMIC_FUNC;
        KoankoSmeargleDiagEnable(FALSE, FALSE);
        rng_value_t baselineRng = gRngValue;
        KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, NULL);
        u32 baselineFrames = KoankoSmeargleDiagGet()->elapsedFrames;
        u32 baselineEvaluations = KoankoSmeargleDiagGet()->evaluations;
        gRngValue = baselineRng;
        gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] |= AI_FLAG_DYNAMIC_FUNC;
        KoankoSmeargleDiagEnable(FALSE, FALSE);
        rng_value_t initial = gRngValue;
        u16 first = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, NULL);
        s32 score = Test_GetKoankoSmeargleStats()->bestScore;
        rng_value_t final = gRngValue;
        EXPECT_EQ(KoankoSmeargleDiagGet()->selected.move, first);
        Test_MgbaPrintf("RANK timing baselineFrames=%d baselineEvaluations=%d rankedFrames=%d rankedEvaluations=%d",
            baselineFrames, baselineEvaluations, KoankoSmeargleDiagGet()->elapsedFrames, KoankoSmeargleDiagGet()->evaluations);
        KoankoSmeargleDiagPrint();
        gRngValue = initial;
        KoankoSmeargleDiagDisable();
        EXPECT_EQ(KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, NULL), first);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->bestScore, score);
        EXPECT_EQ(gRngValue.a, final.a);
        EXPECT_EQ(gRngValue.b, final.b);
        EXPECT_EQ(gRngValue.c, final.c);
        EXPECT_EQ(gRngValue.ctr, final.ctr);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking separates Sash survival and charging from immediate KOs")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); HP(10); MaxHP(10); Speed(100); Moves(MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareRanking();
        s32 immediate = Rank(MOVE_FUSION_BOLT);
        s32 charge = Rank(MOVE_SOLAR_BEAM);
        EXPECT_GT(immediate, charge);
        player->item = ITEM_FOCUS_SASH;
        SetBattlerAiData(B_POSITION_PLAYER_LEFT, gAiLogicData);
        EXPECT_GT(immediate, Rank(MOVE_FUSION_BOLT));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle ranking uses move order under Trick Room for lethal responses")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(100); SpAttack(400); Speed(100); Moves(MOVE_HYPER_BEAM, MOVE_CELEBRATE); }
        SmeargleFixture();
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        opponent->hp = 20;
        PrepareRanking();
        s32 first = Rank(MOVE_RECOVER);
        gFieldStatuses |= STATUS_FIELD_TRICK_ROOM;
        s32 last = Rank(MOVE_RECOVER);
        EXPECT_GT(first, last);
    }
}
