#ifndef GUARD_BATTLE_STATUS_ICONS_H
#define GUARD_BATTLE_STATUS_ICONS_H

#include "global.h"

void InitNoOrderChangeIcons(void);
void RefreshNoOrderChangeIcon(u32 battler);
void FreeNoOrderChangeIcons(void);

#if TESTING
u32 Test_GetNoOrderChangeIconSpriteId(u32 battler);
#endif

#endif // GUARD_BATTLE_STATUS_ICONS_H
