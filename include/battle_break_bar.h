#ifndef GUARD_BATTLE_BREAK_BAR_H
#define GUARD_BATTLE_BREAK_BAR_H

#include "global.h"

// Red is the base HP bar. Extra bars are stacked above it in this order.
enum BreakBarColor
{
    BREAK_BAR_RED,
    BREAK_BAR_PURPLE,
    BREAK_BAR_BLUE,
    BREAK_BAR_GREEN,
    BREAK_BAR_YELLOW,
    BREAK_BAR_ORANGE,
};

#define BREAK_BAR_MAX_EXTRA_BARS 5
#define BREAK_BAR_IGNORE_WHILE_TRANSFORMED (1 << 0)
#define BREAK_BAR_DEFER_INITIAL_DRAW (1 << 1)

struct BreakBarPhase
{
    // Both scripts must return. NULL onBreakScript uses the standard HP refill.
    const u8 *onBreakScript;
    const u8 *afterBreakScript;
};

struct BreakBarConfig
{
    struct BreakBarPhase phases[BREAK_BAR_MAX_EXTRA_BARS];
    u8 extraBars;
    u8 flags;
};

// Registers the current HP bar; it does not heal or change max HP.
// Once all extra bars are broken, callers may explicitly activate a new stack.
// BREAK_BAR_DEFER_INITIAL_DRAW lets the caller provide its own entry animation.
bool32 ActivateBreakBar(u32 battler, const struct BreakBarConfig *config);
void InitializeWildBreakBar(u32 partySlot, const struct BreakBarConfig *config);
u32 GetBreakBarCount(u32 battler);
bool32 IsBreakBarCaptureBlocked(u32 battler);
bool32 IsBreakBarBase(u32 battler);
bool32 CanInterceptBreakBar(u32 battler);
bool32 TryBreakBar(u32 battler, const u8 *returnInstr);
bool32 QueueBreakBar(u32 battler);
s32 ClampBreakBarPassiveDamage(u32 battler, s32 damage);
bool32 GrantSavedBreakBar(u32 battler);
bool32 TryResolveBreakBarsAfterMove(void);

void RefreshBreakBarPalette(u32 battler, bool32 animate);
void ReshowBreakBars(void);
void FreeBreakBarGraphics(void);
void BS_RefillBreakBar(void);
void BS_ResolveBreakBars(void);
void BS_StartKazuraStackBreaker(void);
void BS_AnimateKazuraStackBreaker(void);
void BS_ExhaustBreakBars(void);
void BS_TryGrantSavedBreakBar(void);
void BS_FillSavedBreakBar(void);
void BS_AnimateSavedBreakBar(void);
void BS_RestoreSavedBreakBar(void);
void BS_SyncSavedBreakBarHp(void);
void BS_RevealSavedBreakBar(void);

#endif // GUARD_BATTLE_BREAK_BAR_H
