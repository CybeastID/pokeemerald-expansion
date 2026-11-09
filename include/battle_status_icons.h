#ifndef GUARD_BATTLE_STATUS_ICONS_H
#define GUARD_BATTLE_STATUS_ICONS_H

#include "global.h"

enum BattleStatusIcon
{
    BATTLE_STATUS_ICON_GUTS,
    BATTLE_STATUS_ICON_INVINCIBLE,
    BATTLE_STATUS_ICON_MAGE_ARMOR,
    BATTLE_STATUS_ICON_NO_ORDER_CHANGE,
    BATTLE_STATUS_ICON_COUNT,
};

void InitBattleStatusIcons(void);
void RefreshBattleStatusIcon(u32 battler, u32 icon);
void RefreshBattlerStatusIcons(u32 battler);
void DestroyBattleStatusIcon(u32 battler, u32 icon);
void ReshowBattleStatusIcons(void);
void FreeBattleStatusIcons(void);

// The Bug Space panel shares this palette; FreeBattleStatusIcons owns cleanup.
bool32 LoadBattleStatusIconPalette(void);
void ShowInvincibleBattleStatusIcon(u32 battler);
void BS_ShowKazuGutsIcon(void);
void BS_ShowKazuInvincibleIcon(void);

#if TESTING
u32 Test_GetBattleStatusIconSpriteId(u32 battler, u32 icon);
#endif

#endif // GUARD_BATTLE_STATUS_ICONS_H
