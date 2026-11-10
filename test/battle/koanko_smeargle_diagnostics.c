#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "battle_koanko_smeargle_diagnostics.h"
#include "constants/battle_ai.h"
#include "constants/global.h"

static void PrepareProbe(void)
{
    u32 battler = B_POSITION_OPPONENT_LEFT;
    gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
    Test_SetKoankoSmeargleScorer(NULL);
    gAiLogicData->aiCalcInProgress = TRUE;
    gAiThinkingStruct->aiFlags[battler] |= AI_FLAG_CHECK_VIABILITY;
    SetAIUsingGimmick(battler, USE_GIMMICK);
}

// Explicitly evaluated alternatives, not a claim about which moves a live seed drew.
static void Probe(const u16 *moves, u32 count)
{
    u32 battler = B_POSITION_OPPONENT_LEFT, ties = 0;
    u16 best = MOVE_NONE;
    s32 bestScore = 0;
    KoankoSmeargleDiagEnable(FALSE, FALSE);
    KoankoSmeargleDiagBegin(battler, FALSE);
    for (u32 i = 0; i < count; i++)
    {
        s32 score = BattleAI_ScoreSlot0Candidate(battler, moves[i]);
        const struct SmeargleDiagMove *record = KoankoSmeargleDiagCurrent();
        s32 accounted = record->initial;
        for (u32 j = 0; j < record->passCount; j++)
            accounted += record->passes[j].after - record->passes[j].before;
        EXPECT_EQ(record->score, score);
        EXPECT_EQ(accounted, score);
        EXPECT_EQ(record->omittedPasses, 0);
        EXPECT_EQ(record->order, i + 1);
        if (score > bestScore)
        {
            bestScore = score;
            best = moves[i];
            ties = 1;
            KoankoSmeargleDiagBest(best);
        }
        else if (score == bestScore && score > 0)
            ties++;
    }
    KoankoSmeargleDiagEnd(0, best, bestScore, ties);
    EXPECT_EQ(KoankoSmeargleDiagGet()->evaluations, count);
    KoankoSmeargleDiagPrint();
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle diagnostics Dragapult real scores and Foul Play damage")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        KoankoSmeargleDiagDisable();
        PLAYER(SPECIES_DRAGAPULT) { Level(100); HP(317); MaxHP(317); Attack(339); Defense(186); SpDefense(186); Speed(421); Moves(MOVE_DRAGON_DARTS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_SMEARGLE) { Level(100); HP(314); MaxHP(314); Attack(68); SpAttack(76); Defense(169); SpDefense(126); Speed(205); Ability(ABILITY_OWN_TEMPO); Item(ITEM_FOCUS_SASH); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        const u16 moves[] = {MOVE_FOUL_PLAY, MOVE_SHADOW_BALL, MOVE_ICE_BEAM, MOVE_ICE_SHARD, MOVE_WATER_SHURIKEN, MOVE_TACKLE};
        PrepareProbe();
        Probe(moves, ARRAY_COUNT(moves));
        const struct SmeargleDiagDecision *report = KoankoSmeargleDiagGet();
        EXPECT_EQ(report->watch[0].effectiveness, Q_4_12(2.0));
        EXPECT_GT(report->watch[0].damageMedian, 0);
        EXPECT_EQ(KoankoSmeargleDiagCurrent()->damageMedian, 0); // Tackle into Ghost

        // Foul Play must ignore the user's Attack but track Dragapult's Attack.
        u32 battler = B_POSITION_OPPONENT_LEFT;
        u32 foulDamage = report->watch[0].damageMedian;
        opponent->attack = 400;
        BattleAI_ScoreSlot0Candidate(battler, MOVE_FOUL_PLAY);
        EXPECT_EQ(gAiLogicData->simulatedDmg[battler][B_POSITION_PLAYER_LEFT][0].median, foulDamage);
        player->attack = 100;
        BattleAI_ScoreSlot0Candidate(battler, MOVE_FOUL_PLAY);
        EXPECT_LT(gAiLogicData->simulatedDmg[battler][B_POSITION_PLAYER_LEFT][0].median, foulDamage);
        KoankoSmeargleDiagDisable();
        gAiLogicData->aiCalcInProgress = FALSE;
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle diagnostics Serperior real priority resistance and alternatives")
{
    u32 hp;
    PARAMETRIZE { hp = 314; }
    PARAMETRIZE { hp = 1; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        KoankoSmeargleDiagDisable();
        PLAYER(SPECIES_SERPERIOR) { Level(100); HP(291); MaxHP(291); Attack(186); SpAttack(249); Defense(226); SpDefense(226); Speed(357); Ability(ABILITY_CONTRARY); Moves(MOVE_LEAF_STORM, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_SMEARGLE) { Level(100); HP(314); MaxHP(314); Attack(68); SpAttack(76); Defense(169); SpDefense(126); Speed(205); Ability(ABILITY_OWN_TEMPO); Item(ITEM_FOCUS_SASH); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        const u16 moves[] = {MOVE_BULLET_PUNCH, MOVE_ZIPPY_ZAP, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_ICE_SHARD, MOVE_WATER_SHURIKEN, MOVE_V_CREATE, MOVE_BLAST_BURN};
        opponent->hp = hp;
        PrepareProbe();
        Probe(moves, ARRAY_COUNT(moves));
        const struct SmeargleDiagDecision *report = KoankoSmeargleDiagGet();
        EXPECT_EQ(report->watch[1].priority, 1);
        EXPECT_EQ(report->watch[1].effectiveness, Q_4_12(1.0)); // Steel is neutral against Grass
        EXPECT_EQ(report->watch[2].effectiveness, Q_4_12(0.5));
        EXPECT_EQ(report->watch[5].effectiveness, Q_4_12(2.0));
        EXPECT_GT(report->watch[5].damageMedian, report->watch[1].damageMedian);
        KoankoSmeargleDiagDisable();
        gAiLogicData->aiCalcInProgress = FALSE;
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle diagnostics preserve real scorer scores and RNG")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        KoankoSmeargleDiagDisable();
        PLAYER(SPECIES_SERPERIOR) { Level(100); Moves(MOVE_LEAF_STORM, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_SMEARGLE) { Level(100); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        const u16 moves[] = {MOVE_BULLET_PUNCH, MOVE_ZIPPY_ZAP, MOVE_FLAMETHROWER, MOVE_CELEBRATE, MOVE_FOUL_PLAY};
        s32 scores[ARRAY_COUNT(moves)];
        u32 battler = B_POSITION_OPPONENT_LEFT;
        PrepareProbe();
        rng_value_t initial = gRngValue;
        for (u32 i = 0; i < ARRAY_COUNT(moves); i++)
            scores[i] = BattleAI_ScoreSlot0Candidate(battler, moves[i]);
        rng_value_t expected = gRngValue;
        gRngValue = initial;
        KoankoSmeargleDiagEnable(FALSE, FALSE);
        KoankoSmeargleDiagBegin(battler, FALSE);
        for (u32 i = 0; i < ARRAY_COUNT(moves); i++)
            EXPECT_EQ(BattleAI_ScoreSlot0Candidate(battler, moves[i]), scores[i]);
        EXPECT_EQ(gRngValue.a, expected.a);
        EXPECT_EQ(gRngValue.b, expected.b);
        EXPECT_EQ(gRngValue.c, expected.c);
        EXPECT_EQ(gRngValue.ctr, expected.ctr);
        KoankoSmeargleDiagDisable();
        gAiLogicData->aiCalcInProgress = FALSE;
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Smeargle diagnostics capture real sampler and distinguish unsampled moves")
{
    u32 species;
    PARAMETRIZE { species = SPECIES_DRAGAPULT; }
    PARAMETRIZE { species = SPECIES_SERPERIOR; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        KoankoSmeargleDiagDisable();
        PLAYER(species) { Level(100); Moves(MOVE_DRAGON_PULSE, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_SMEARGLE) { Level(100); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        PrepareProbe();
        KoankoSmeargleDiagEnable(FALSE, FALSE);
        KoankoSmeargleChooseAnyMove(B_POSITION_OPPONENT_LEFT);
        const struct SmeargleDiagDecision *report = KoankoSmeargleDiagGet();
        const struct KoankoSmeargleStats *stats = Test_GetKoankoSmeargleStats();
        EXPECT_EQ(report->complete, TRUE);
        EXPECT_EQ(report->sampled, TRUE);
        EXPECT_EQ(report->evaluations, stats->evaluations);
        EXPECT_EQ(report->positives, stats->positives);
        EXPECT_EQ(report->chosen, gBattleMons[B_POSITION_OPPONENT_LEFT].moves[0]);
        EXPECT_EQ(report->selected.move, report->chosen);
        EXPECT_EQ(report->selected.score, report->bestScore);
        EXPECT_GE(report->ties, 1);
        EXPECT_LE(report->evaluations, report->poolSize);
        for (u32 i = 0; i < report->topCount; i++)
            EXPECT_EQ(KoankoSmeargleDiagWasEvaluated(report->top[i].move), TRUE);
        EXPECT_EQ(KoankoSmeargleDiagWasEvaluated(MOVE_METRONOME), FALSE);
        KoankoSmeargleDiagPrint();
        KoankoSmeargleDiagDisable();
        gAiLogicData->aiCalcInProgress = FALSE;
    }
}
