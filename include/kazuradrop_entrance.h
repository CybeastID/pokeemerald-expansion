#ifndef GUARD_KAZURADROP_ENTRANCE_H
#define GUARD_KAZURADROP_ENTRANCE_H

// Ball-less field entrance for special encounters.
// Hooked into StartSendOutAnim() in src/battle_controllers.c.

// TRUE if this species skips the Poké Ball send-out animation.
bool8 UseCustomSendOut(u16 species);

// Runs the replacement entrance. Signature/return match DoPokeballSendOutAnimation.
u8 DoKazuradropAppearAnimation(u32 battler);

#endif // GUARD_KAZURADROP_ENTRANCE_H
