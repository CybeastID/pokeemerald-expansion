#ifndef GUARD_BATTLE_KOANKO_SMEARGLE_H
#define GUARD_BATTLE_KOANKO_SMEARGLE_H

// Koanko's conjured Smeargle: "Any Move". On each fresh choice, ordinary moves are drawn without
// replacement, each is placed in real slot 0 and scored by the ordinary AI flag passes, and the best
// positively-scored candidate is committed to slot 0 and executed through the normal pipeline.

#define KOANKO_SMEARGLE_POSITIVE_TARGET 60

typedef s32 (*KoankoSmeargleScoreFn)(u32 battler, u16 move);

bool32 KoankoSmeargleAnyMoveActive(u32 battler);
bool32 KoankoSmeargleIsFreshChoice(u32 battler);
bool32 KoankoSmeargleIsEligibleMove(u32 move);
u16 KoankoSmeargleSelect(u32 battler, KoankoSmeargleScoreFn scorer); // MOVE_NONE when no candidate scored > 0
void KoankoSmeargleChooseAnyMove(u32 battler);

#if TESTING
struct KoankoSmeargleStats
{
    u16 poolSize;
    u16 draws;
    u16 evaluations;
    u16 positives;
    u16 chosen;
    s32 bestScore;
    u8 usedStruggle;
};
const struct KoankoSmeargleStats *Test_GetKoankoSmeargleStats(void);
void Test_SetKoankoSmeargleScorer(KoankoSmeargleScoreFn scorer); // NULL restores the real AI scorer
#endif

#endif // GUARD_BATTLE_KOANKO_SMEARGLE_H
