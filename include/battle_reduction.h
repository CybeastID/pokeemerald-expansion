#ifndef GUARD_BATTLE_REDUCTION_H
#define GUARD_BATTLE_REDUCTION_H

#include "global.h"
#include "constants/pokemon.h"
#include "constants/battle.h"

// Canonical party data is never a workspace for the reduced stat calculation.
struct ReductionOccupant
{
    u32 hpFraction;
    u16 base[NUM_STATS];
    u16 applied[NUM_STATS];
    u16 lastHp;
    u8 lostLevels;
    u8 partyIndex;
    u8 depth;
    bool8 scaled;
    bool8 powerTrick;
};

struct ReductionData
{
    bool8 active;
    bool8 grace;
    u8 nextLoss;
    u8 affectedSide;
    struct ReductionOccupant occupant[MAX_BATTLERS_COUNT];
};

bool32 ReductionIsActive(void);
bool32 ReductionIsScaled(u32 battler);
void ReductionActivate(void);
bool32 ReductionTryActivate(u32 initialLoss, u32 affectedSide);
bool32 ReductionEndTurn(enum BattlerId battler);
void ReductionBeginMutation(u32 battler);
// Nested canonical mutations must finish updating stats and HP before End.
void ReductionEndMutation(u32 battler);
void ReductionOnStatsCopied(u32 battler);
void ReductionResetOccupant(u32 battler);
void ReductionFinishBattle(void);
void ReductionNormalizeRequest(u32 battler, u32 request, u32 monMask, void *data);
u16 ReductionCanonicalHp(u32 battler);
u16 ReductionCanonicalMaxHp(u32 battler);
void ReductionRefreshDisplay(u32 battler);

#endif
