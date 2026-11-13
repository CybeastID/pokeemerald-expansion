#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_dynamax.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "battle_koanko_smeargle_diagnostics.h"
#include "battle_setup.h"
#include "battle_z_move.h"
#include "random.h"
#include "constants/battle_move_effects.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"

// Ordinary moves only: ids 1..MOVES_COUNT_GEN9-1. Custom/boss moves (>= MOVES_GEN_CUSTOM_START) and
// Z/Max moves sit above that range; the explicit checks below keep the intent visible.
static EWRAM_DATA u16 sPool[MOVES_COUNT_GEN9] = {0};
static EWRAM_DATA u16 sPoolSize = 0;

#if TESTING
static struct KoankoSmeargleStats sStats;
static KoankoSmeargleScoreFn sTestScorer = NULL;
const struct KoankoSmeargleStats *Test_GetKoankoSmeargleStats(void) { return &sStats; }
void Test_SetKoankoSmeargleScorer(KoankoSmeargleScoreFn scorer) { sTestScorer = scorer; }
#endif

bool32 KoankoSmeargleAnyMoveActive(u32 battler)
{
    return gBattleStruct != NULL
        && gBattleStruct->koankoPhase == KOANKO_CONJURED_TEAM
        && (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        && TRAINER_BATTLE_PARAM.opponentA == TRAINER_KOANKO
        && GetBattlerSide(battler) == B_SIDE_OPPONENT
        && gBattlerPartyIndexes[battler] == 0
        && gBattleMons[battler].species == SPECIES_SMEARGLE;
}

// ComputeBattlerDecisions runs every turn, including turns where the move is already forced.
// These mirror the forced-action test in HandleTurnActionSelectionState.
bool32 KoankoSmeargleIsFreshChoice(u32 battler)
{
    return !(gBattleMons[battler].volatiles.multipleTurns
          || gBattleMons[battler].volatiles.rechargeTimer > 0
          || gBattleMons[battler].volatiles.timestop > 1);
}

bool32 KoankoSmeargleIsEligibleMove(u32 move)
{
    if (move == MOVE_NONE || move == MOVE_UNAVAILABLE || move == MOVE_STRUGGLE || move >= MOVES_COUNT_GEN9)
        return FALSE;
    if (IsZMove(move) || IsMaxMove(move))
        return FALSE;
    if (move != MOVE_FOUL_PLAY
    && GetMoveCategory(move) != DAMAGE_CATEGORY_STATUS
    && GetMovePower(move) > 0
    && GetMovePower(move) < 100)
    return FALSE;

    switch (GetMoveEffect(move))
    {
    case EFFECT_PLACEHOLDER:
    // Indirect move callers: their result would bypass the scored candidate.
    case EFFECT_METRONOME:
    case EFFECT_ASSIST:
    case EFFECT_COPYCAT:
    case EFFECT_ME_FIRST:
    case EFFECT_MIRROR_MOVE:
    case EFFECT_SLEEP_TALK:
    case EFFECT_NATURE_POWER:
    case EFFECT_INSTRUCT:
    // Rewrite the user's move slots, which would break the single-slot design.
    case EFFECT_SKETCH:
    case EFFECT_MIMIC:
    case EFFECT_TRANSFORM:
        return FALSE;
    default:
        return TRUE;
    }
}

static void BuildPool(void)
{
    // Rebuild in ID order for every decision: replaying the same RNG state must not
    // depend on a previous battle's in-place shuffle.
    sPoolSize = 0;
    for (u32 move = 1; move < MOVES_COUNT_GEN9; move++)
    {
        if (KoankoSmeargleIsEligibleMove(move))
            sPool[sPoolSize++] = move;
    }
}

static s32 ScoreCandidate(u32 battler, u16 move)
{
#if TESTING
    if (sTestScorer != NULL)
        return sTestScorer(battler, move);
#endif
    return BattleAI_ScoreSlot0Candidate(battler, move);
}

// Partial Fisher-Yates: each draw is a distinct move. Stops at KOANKO_SMEARGLE_POSITIVE_TARGET
// positively scored candidates or when the pool is exhausted, so it can never loop unboundedly.
u16 KoankoSmeargleSelect(u32 battler, KoankoSmeargleScoreFn scorer)
{
    u16 best = MOVE_NONE;
    s32 bestScore = 0;
    u32 positives = 0, ties = 0, draws = 0;

#if KOANKO_SMEARGLE_DIAGNOSTICS
    KoankoSmeargleDiagBegin(battler, TRUE);
#endif
    BuildPool();
    while (draws < sPoolSize && positives < KOANKO_SMEARGLE_POSITIVE_TARGET)
    {
        u32 j = draws + RandomUniform(RNG_KOANKO_SMEARGLE_DRAW, 0, sPoolSize - draws - 1);
        u16 move = sPool[j];
        sPool[j] = sPool[draws];
        sPool[draws] = move;
        draws++;

        s32 score = scorer != NULL ? scorer(battler, move) : ScoreCandidate(battler, move);
        if (score <= 0)
            continue;
        positives++;
        if (score > bestScore)
        {
            best = move;
            bestScore = score;
            ties = 1;
#if KOANKO_SMEARGLE_DIAGNOSTICS
            KoankoSmeargleDiagBest(best);
#endif
        }
        else if (score == bestScore)
        {
            // Reservoir: every equal-best candidate ends up chosen with equal probability.
            ties++;
            if (RandomUniform(RNG_KOANKO_SMEARGLE_TIE, 0, ties - 1) == 0)
            {
                best = move;
#if KOANKO_SMEARGLE_DIAGNOSTICS
                KoankoSmeargleDiagBest(best);
#endif
            }
        }
    }

#if KOANKO_SMEARGLE_DIAGNOSTICS
    KoankoSmeargleDiagEnd(sPoolSize, best == MOVE_NONE ? MOVE_STRUGGLE : best, bestScore, ties);
#endif
#if TESTING
    sStats.poolSize = sPoolSize;
    sStats.draws = draws;
    sStats.evaluations = draws;
    sStats.positives = positives;
    sStats.chosen = best;
    sStats.bestScore = bestScore;
    sStats.usedStruggle = FALSE;
#endif
    return best;
}

void KoankoSmeargleChooseAnyMove(u32 battler)
{
    u16 move = KoankoSmeargleSelect(battler, NULL);
    if (move == MOVE_NONE)
    {
        move = MOVE_STRUGGLE;
#if TESTING
        sStats.usedStruggle = TRUE;
#endif
    }
    // Commit to the real slot 0 and refresh its caches, then reset the thinking struct so the
    // ordinary chooser that follows scores exactly this move.
    BattleAI_PrepareSlot0Move(battler, move);
    BattleAI_SetupAIData(0xF, battler);
}
