#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "random.h"
#include "constants/battle_ai.h"

// NOT RUN when authored (no ARM toolchain in the authoring environment).
// Selection logic is tested with injected scorers so the checks are deterministic; the
// ordinary-AI path is exercised separately by the tests that use the real scorer.

#define POOL_MAX MOVES_COUNT_GEN9

// Full-pool observations need 2544 bytes; recorded fixture scores are only 0..53.
// Ordinary static .bss goes in scarce IWRAM; keep these buffers in EWRAM instead.
static EWRAM_DATA u16 sSeenMoves[POOL_MAX] = {0};
static EWRAM_DATA s8 sSeenScores[POOL_MAX] = {0};
static u32 sSeenCount;
static u32 sFlagSeenDuringScoring;
static u32 sCallCount;
static u32 sHyperBeamCallsByTurn[3];

static void ResetScorerState(void)
{
    sSeenCount = 0;
    sFlagSeenDuringScoring = 0;
    sCallCount = 0;
    memset(sHyperBeamCallsByTurn, 0, sizeof(sHyperBeamCallsByTurn));
    Test_SetKoankoSmeargleScorer(NULL);
}

static s32 ScoreRecord(u32 battler, u16 move, s32 score)
{
    if (sSeenCount < POOL_MAX)
    {
        sSeenMoves[sSeenCount] = move;
        sSeenScores[sSeenCount] = score;
        sSeenCount++;
    }
    return score;
}

static s32 ScoreAllZero(u32 battler, u16 move) { return ScoreRecord(battler, move, 0); }
static s32 ScoreAllTen(u32 battler, u16 move) { return ScoreRecord(battler, move, 10); }
static s32 ScoreVaried(u32 battler, u16 move) { return ScoreRecord(battler, move, 1 + (move * 7) % 53); }
static bool32 IsOneOfFiveMoves(u16 move)
{
    return move == MOVE_MEGA_KICK || move == MOVE_DOUBLE_EDGE || move == MOVE_EARTHQUAKE
        || move == MOVE_FIRE_BLAST || move == MOVE_THUNDER;
}
static s32 ScoreFiveMoves(u32 battler, u16 move) { return ScoreRecord(battler, move, IsOneOfFiveMoves(move) ? 20 : 0); }
static s32 ScoreTwoTied(u32 battler, u16 move) { return ScoreRecord(battler, move, (move == MOVE_MEGA_KICK || move == MOVE_DOUBLE_EDGE) ? 30 : 0); }

static s32 ScoreOnlyBoomburst(u32 battler, u16 move)
{
    sCallCount++;
    sFlagSeenDuringScoring |= (gAiThinkingStruct->aiFlags[battler] & AI_FLAG_CHECK_VIABILITY) != 0;
    return move == MOVE_BOOMBURST ? 100 : 0;
}

static s32 ScoreOnlyHyperBeam(u32 battler, u16 move)
{
    sCallCount++;
    // Playback ends when the controller asks past the recorded actions, after
    // the next turn's AI may already have run. Observe only the specified turns.
    if (gBattleResults.battleTurnCounter < ARRAY_COUNT(sHyperBeamCallsByTurn))
        sHyperBeamCallsByTurn[gBattleResults.battleTurnCounter]++;
    return move == MOVE_HYPER_BEAM ? 100 : 0;
}

static s32 ScoreNothing(u32 battler, u16 move)
{
    sCallCount++;
    return 0;
}

static u32 ExpectedPoolSize(void)
{
    u32 n = 0;
    for (u32 m = 1; m < MOVES_COUNT_GEN9; m++)
        if (KoankoSmeargleIsEligibleMove(m))
            n++;
    return n;
}

static bool32 SeenHasDuplicate(void)
{
    for (u32 i = 0; i < sSeenCount; i++)
        for (u32 j = i + 1; j < sSeenCount; j++)
            if (sSeenMoves[i] == sSeenMoves[j])
                return TRUE;
    return FALSE;
}

// ---- Eligibility ----

SINGLE_BATTLE_TEST("Koanko Smeargle pool excludes Struggle, custom, Z/Max and indirect move callers")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_NONE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_STRUGGLE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_UNAVAILABLE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_ROAR_OF_ETERNITY), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_DMS_PREROGATIVE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_BREAKNECK_BLITZ), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_METRONOME), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_ASSIST), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_COPYCAT), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_ME_FIRST), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_MIRROR_MOVE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_SLEEP_TALK), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_SKETCH), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_TRANSFORM), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_TACKLE), FALSE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_BOOMBURST), TRUE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_FOUL_PLAY), TRUE);
        EXPECT_EQ(KoankoSmeargleIsEligibleMove(MOVE_HYPER_BEAM), TRUE);
    }
}

// ---- Sampling ----

SINGLE_BATTLE_TEST("Koanko Smeargle draws distinct eligible moves without replacement")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        EXPECT_EQ(KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreAllZero), MOVE_NONE);
        EXPECT_EQ(sSeenCount, ExpectedPoolSize());           // exhausted the whole pool
        EXPECT_EQ(SeenHasDuplicate(), FALSE);
        for (u32 i = 0; i < sSeenCount; i++)
            EXPECT_EQ(KoankoSmeargleIsEligibleMove(sSeenMoves[i]), TRUE);
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle stops at exactly 60 positively scored candidates")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        u16 chosen = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreAllTen);
        EXPECT_NE(chosen, MOVE_NONE);
        EXPECT_EQ(sSeenCount, KOANKO_SMEARGLE_POSITIVE_TARGET);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->positives, KOANKO_SMEARGLE_POSITIVE_TARGET);
        EXPECT_EQ(SeenHasDuplicate(), FALSE);
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle needs more evaluations than 60 when candidates score zero")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        // Only five eligible moves score; the other draws are rejected, so evaluations >> positives.
        u16 chosen = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreFiveMoves);
        EXPECT_NE(chosen, MOVE_NONE);
        EXPECT_EQ(IsOneOfFiveMoves(chosen), TRUE);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->positives, 5);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->draws, ExpectedPoolSize()); // pool exhausted before 60 positives
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle falls back when no candidate scores above zero")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        EXPECT_EQ(KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreAllZero), MOVE_NONE);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->positives, 0);
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle picks a move with the best score among those drawn")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        u16 chosen = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreVaried);
        s32 maxScore = 0, chosenScore = -1;
        for (u32 i = 0; i < sSeenCount; i++)
        {
            if (sSeenScores[i] > maxScore)
                maxScore = sSeenScores[i];
            if (sSeenMoves[i] == chosen)
                chosenScore = sSeenScores[i];
        }
        EXPECT_EQ(sSeenCount, KOANKO_SMEARGLE_POSITIVE_TARGET);
        EXPECT_EQ(chosenScore, maxScore);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->bestScore, maxScore);
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle breaks equal best scores uniformly among the tied candidates")
{
    // The reservoir tie-break draws RandomUniform(RNG_KOANKO_SMEARGLE_TIE, 0, 1) when the second tied
    // candidate is found; the later candidate replaces the earlier one on trial 0 only: 1 pass in 2.
    // (Under the battle test runner untagged RNG is a fixed value, so a loop of plain selects cannot
    // show a spread; PASSES_RANDOMLY drives the tagged draw instead.)
    PASSES_RANDOMLY(50, 100, RNG_KOANKO_SMEARGLE_TIE);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        ResetScorerState();
        u16 chosen = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreTwoTied);
        u16 firstTied = MOVE_NONE, secondTied = MOVE_NONE;
        for (u32 i = 0; i < sSeenCount; i++)
        {
            if (sSeenScores[i] <= 0)
                continue;
            if (firstTied == MOVE_NONE)
                firstTied = sSeenMoves[i];
            else
                secondTied = sSeenMoves[i];
        }
        EXPECT_NE(firstTied, MOVE_NONE);
        EXPECT_NE(secondTied, MOVE_NONE);
        EXPECT(chosen == MOVE_MEGA_KICK || chosen == MOVE_DOUBLE_EDGE);
        EXPECT_EQ(chosen, secondTied);
    }
}

// ---- Isolation ----

SINGLE_BATTLE_TEST("Koanko Smeargle: an ordinary Smeargle outside Koanko's encounter is unaffected")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(KoankoSmeargleAnyMoveActive(B_POSITION_OPPONENT_LEFT), FALSE);
        EXPECT_EQ(opponent->moves[0], MOVE_TACKLE);
        EXPECT_LT(opponent->pp[0], GetMovePP(MOVE_TACKLE)); // PP is spent normally
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle: Koanko's first team is not Any Move and keeps default AI flags")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        // koankoPhase is still ORDINARY here, so even a Smeargle is not the conjured one.
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_ORDINARY_TEAM);
        EXPECT_EQ(KoankoSmeargleAnyMoveActive(B_POSITION_OPPONENT_LEFT), FALSE);
        EXPECT_EQ((gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] & AI_FLAG_CHECK_VIABILITY) != 0, FALSE);
        EXPECT_LT(opponent->pp[0], GetMovePP(MOVE_TACKLE));
    }
}

// ---- Conjured encounter, real pipeline ----
// Shaped on "Koanko AI sends the conjured lead..." in test/battle/koanko.c.

AI_SINGLE_BATTLE_TEST("Koanko Smeargle commits the sampled move to slot 0 and executes it ordinarily")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreOnlyBoomburst);
        PLAYER(SPECIES_WOBBUFFET) { SpDefense(9999); Speed(1000); Defense(500); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Smeargle used Boomburst!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(opponent->moves[0], MOVE_BOOMBURST);
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->chosen, MOVE_BOOMBURST);
        // CHECK_VIABILITY was on while scoring but is restored afterwards for the rest of Koanko's team.
        EXPECT_EQ(sFlagSeenDuringScoring, TRUE);
        EXPECT_EQ((gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] & AI_FLAG_CHECK_VIABILITY) != 0, FALSE);
        ResetScorerState();
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle spends no PP, including against Pressure")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreOnlyBoomburst);
        PLAYER(SPECIES_WOBBUFFET) { SpDefense(9999); Speed(1000); Ability(ABILITY_PRESSURE); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Smeargle used Boomburst!");
    } THEN {
        EXPECT_EQ(opponent->moves[0], MOVE_BOOMBURST);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_BOOMBURST));
        ResetScorerState();
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle uses Struggle when no candidate scores above zero")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreNothing);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Smeargle used Struggle!");
    } THEN {
        EXPECT_EQ(Test_GetKoankoSmeargleStats()->usedStruggle, TRUE);
        EXPECT_EQ(opponent->moves[0], MOVE_STRUGGLE);
        ResetScorerState();
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle cannot be Encored")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreOnlyBoomburst);
        PLAYER(SPECIES_WOBBUFFET) { SpDefense(9999); Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE, MOVE_ENCORE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_ENCORE); }
    } SCENE {
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(gDisableStructs[B_POSITION_OPPONENT_LEFT].encoredMove, MOVE_NONE);
        ResetScorerState();
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle does not re-sample during a forced recharge turn")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreOnlyHyperBeam);
        PLAYER(SPECIES_WOBBUFFET) { Speed(1000); Defense(9999); SpDefense(9999); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        // Recharge bypasses the AI move-choice callback. Do not queue a choice
        // expectation that could only be consumed on the following fresh turn.
        TURN { MOVE(player, MOVE_CELEBRATE); SKIP_TURN(opponent); }
    } SCENE {
        MESSAGE("The opposing Smeargle used Hyper Beam!");
        MESSAGE("The opposing Smeargle must recharge!");
    } THEN {
        EXPECT_EQ(opponent->moves[0], MOVE_HYPER_BEAM);
        // Counters are zero-based. Assert the actual turn, excluding a possible
        // fresh turn-4 selection before playback exhaustion reaches THEN.
        EXPECT_EQ(sHyperBeamCallsByTurn[0], 0);
        EXPECT_EQ(sHyperBeamCallsByTurn[1], ExpectedPoolSize());
        EXPECT_EQ(sHyperBeamCallsByTurn[2], 0);
        ResetScorerState();
    }
}

// ---- Real AI scoring (no injected scorer) ----

AI_SINGLE_BATTLE_TEST("Koanko Smeargle real scoring: CHECK_VIABILITY raises a damaging candidate and slot 0 caches refresh")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        // Make the setup turn deterministic; direct scoring below uses the real AI.
        ResetScorerState();
        Test_SetKoankoSmeargleScorer(ScoreOnlyBoomburst);
        PLAYER(SPECIES_WOBBUFFET) { SpDefense(9999); Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 atk = B_POSITION_OPPONENT_LEFT, def = B_POSITION_PLAYER_LEFT;
        gAiLogicData->aiCalcInProgress = TRUE;

        gAiThinkingStruct->aiFlags[atk] &= ~AI_FLAG_CHECK_VIABILITY;
        s32 without = BattleAI_ScoreSlot0Candidate(atk, MOVE_TACKLE);
        u16 tackleMedian = gAiLogicData->simulatedDmg[atk][def][0].median;

        gAiThinkingStruct->aiFlags[atk] |= AI_FLAG_CHECK_VIABILITY;
        s32 with = BattleAI_ScoreSlot0Candidate(atk, MOVE_TACKLE);
        // A lone damaging move trivially "deals the most damage" in AI_CompareDamagingMoves.
        EXPECT_GE(with, without + BEST_DAMAGE_MOVE);
        EXPECT_GT(tackleMedian, 0);

        // Cache refresh: a status move in slot 0 must not inherit Tackle's damage.
        BattleAI_ScoreSlot0Candidate(atk, MOVE_CELEBRATE);
        EXPECT_EQ(gBattleMons[atk].moves[0], MOVE_CELEBRATE);
        EXPECT_EQ(gAiLogicData->simulatedDmg[atk][def][0].median, 0);

        // A limited candidate must clear the preceding move's entire slot cache.
        gDisableStructs[atk].disabledMove = MOVE_TACKLE;
        EXPECT_EQ(BattleAI_ScoreSlot0Candidate(atk, MOVE_TACKLE), 0);
        EXPECT_EQ(gAiLogicData->simulatedDmg[atk][def][0].median, 0);
        EXPECT_EQ(gAiLogicData->effectiveness[atk][def][0], 0);
        EXPECT_EQ(gAiLogicData->moveAccuracy[atk][def][0], 0);
        gDisableStructs[atk].disabledMove = MOVE_NONE;
        gDisableStructs[atk].tauntTimer = 1;
        EXPECT_EQ(BattleAI_ScoreSlot0Candidate(atk, MOVE_CELEBRATE), 0);
        gDisableStructs[atk].tauntTimer = 0;

        gAiThinkingStruct->aiFlags[atk] &= ~AI_FLAG_CHECK_VIABILITY;
        gAiLogicData->aiCalcInProgress = FALSE;
        ResetScorerState();
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle sampling repeats with fixed tagged RNG after earlier shuffles")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u16 firstDraws[KOANKO_SMEARGLE_POSITIVE_TARGET];
        rng_value_t initialRng = gRngValue;
        ResetScorerState();
        u16 firstChoice = KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreAllTen);
        for (u32 i = 0; i < ARRAY_COUNT(firstDraws); i++)
            firstDraws[i] = sSeenMoves[i];
        rng_value_t finalRng = gRngValue;

        gRngValue = initialRng;
        ResetScorerState();
        EXPECT_EQ(KoankoSmeargleSelect(B_POSITION_OPPONENT_LEFT, ScoreAllTen), firstChoice);
        for (u32 i = 0; i < ARRAY_COUNT(firstDraws); i++)
            EXPECT_EQ(sSeenMoves[i], firstDraws[i]);
        EXPECT_EQ(gRngValue.a, finalRng.a);
        EXPECT_EQ(gRngValue.b, finalRng.b);
        EXPECT_EQ(gRngValue.c, finalRng.c);
        EXPECT_EQ(gRngValue.ctr, finalRng.ctr);
    }
}

SINGLE_BATTLE_TEST("Koanko Smeargle ownership excludes another conjured party slot")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        u32 savedIndex = gBattlerPartyIndexes[battler];
        gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
        gBattlerPartyIndexes[battler] = 0;
        EXPECT_EQ(KoankoSmeargleAnyMoveActive(battler), TRUE);
        gBattlerPartyIndexes[battler] = 1;
        EXPECT_EQ(KoankoSmeargleAnyMoveActive(battler), FALSE);
        gBattlerPartyIndexes[battler] = savedIndex;
    }
}
