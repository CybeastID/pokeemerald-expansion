#ifndef GUARD_KOANKO_SMEARGLE_DIAGNOSTICS_H
#define GUARD_KOANKO_SMEARGLE_DIAGNOSTICS_H

#include "config/debug.h"
#include "constants/moves.h"
#define KOANKO_SMEARGLE_DIAGNOSTICS (TESTING || DEBUG_KOANKO_SMEARGLE_DIAGNOSTICS)

#if KOANKO_SMEARGLE_DIAGNOSTICS
#define SMEARGLE_DIAG_TOP 12
#define SMEARGLE_DIAG_WATCH 8
#define SMEARGLE_DIAG_PASSES 8

struct SmeargleDiagPass
{
    s32 before, after;
    u8 logic; // 64 = the separate damage-comparison pass
};

struct SmeargleDiagMove
{
    s32 score, initial;
    s32 viability[3]; // move effect, additional effects, item: aggregate deltas
    struct SmeargleDiagPass passes[SMEARGLE_DIAG_PASSES];
    u16 move, order, power, effectiveness;
    u16 damageMin, damageMedian, damageMax;
    u8 type, category, accuracy, passCount, omittedPasses;
    s8 priority;
    bool8 eligible, usable, positive, medianDamageKo;
};

struct SmeargleDiagDecision
{
    u64 flags;
    struct SmeargleDiagMove top[SMEARGLE_DIAG_TOP];
    struct SmeargleDiagMove watch[SMEARGLE_DIAG_WATCH];
    struct SmeargleDiagMove selected;
    u32 targetStatus, field, weather, elapsedFrames;
    u16 poolSize, evaluations, positives, chosen, ties, turn;
    u16 attackerSpecies, targetSpecies, hp, maxHp, targetHp, targetMaxHp;
    u16 attack, spAttack, targetAttack, targetDefense, targetSpDefense;
    u16 speed, targetSpeed, ability, targetAbility, item, targetItem;
    s32 bestScore;
    u8 targetTypes[3], topCount;
    bool8 sampled, complete;
    // Membership only, not a full-pool history of scores. Query before claiming a rejection.
    u8 evaluated[(MOVES_COUNT_GEN9 + 7) / 8];
};

void KoankoSmeargleDiagEnable(bool32 log, bool32 continuous);
void KoankoSmeargleDiagDisable(void);
const struct SmeargleDiagDecision *KoankoSmeargleDiagGet(void);
const struct SmeargleDiagMove *KoankoSmeargleDiagCurrent(void);
bool32 KoankoSmeargleDiagWasEvaluated(u32 move);
void KoankoSmeargleDiagBegin(u32 battler, bool32 sampled);
void KoankoSmeargleDiagCandidateBegin(u32 battler, u16 move);
void KoankoSmeargleDiagPrepared(u32 battler, u16 move);
void KoankoSmeargleDiagPriority(u32 battler, s32 priority);
void KoankoSmeargleDiagPass(u32 battler, u32 logic, s32 before, s32 after);
void KoankoSmeargleDiagViability(u32 battler, u32 part, s32 before, s32 after);
void KoankoSmeargleDiagCandidateEnd(u32 battler, s32 score);
void KoankoSmeargleDiagBest(u16 move);
void KoankoSmeargleDiagEnd(u32 pool, u16 chosen, s32 bestScore, u32 ties);
void KoankoSmeargleDiagPrint(void);
#endif
#endif
