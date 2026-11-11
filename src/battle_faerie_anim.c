#include "global.h"
#include "battle_anim.h"
#include "task.h"

// Some custom animation tasks need their particles resident before the first
// sprite is created. Use target-managed assets; the script preserves the old
// one-frame loading wait explicitly after this synchronous preload task.
void AnimTask_PreloadFaerieParticles(u8 taskId)
{
    u32 tag = (u16)gBattleAnimArgs[0];
    TryLoadGfx(tag);
    TryLoadPal(tag);
    DestroyAnimVisualTask(taskId);
}
