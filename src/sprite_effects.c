#include "global.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "sprite.h"
#include "sprite_affine_anims.h"  // Your custom affine header
#include "script.h"

const union AffineAnimCmd gAffineAnim_Shrinking[] = {
    AFFINEANIMCMD_FRAME(0x0100, 0x0100, 0, 0),   // Start at 1x scale (256/256)
    AFFINEANIMCMD_FRAME(-0x0003, -0x0003, 0, 60), // Shrink to about 0.05x scale over 60 frames
    AFFINEANIMCMD_FRAME(0x0000, 0x0000, 0, 8),    // Pause for 8 frames
    AFFINEANIMCMD_END
};

const union AffineAnimCmd *const gAffineAnims_Shrinking[] = {
    gAffineAnim_Shrinking,
};

const union AffineAnimCmd gAffineAnim_ShrunkPlayer[] = {
    AFFINEANIMCMD_FRAME(0x004C, 0x004C, 0, 0), // Set to small scale instantly (16/256)
    AFFINEANIMCMD_END
};
const union AffineAnimCmd *const gAffineAnims_ShrunkPlayer[] = {
    gAffineAnim_ShrunkPlayer,
};

const union AffineAnimCmd gAffineAnim_Reset[] = {
    AFFINEANIMCMD_FRAME(0x0100, 0x0100, 0, 0), // Reset to 1x scale instantly
    AFFINEANIMCMD_END
};

const union AffineAnimCmd *const gAffineAnims_Reset[] = {
    gAffineAnim_Reset,
};

const union AffineAnimCmd gAffineAnim_EnemyMonGiant[] = {
    AFFINEANIMCMD_FRAME(0x200, 0x200, 0, 0),  // 2x scale instant
    AFFINEANIMCMD_END,
};

const union AffineAnimCmd *const gAffineAnims_EnemyMonGiant[] = {
    gAffineAnim_EnemyMonGiant,
};


const union AffineAnimCmd gAffineAnim_PlayerMonTiny[] = {
    AFFINEANIMCMD_FRAME(0x80, 0x80, 0, 0),  // 0.5x scale instant
    AFFINEANIMCMD_END,
};
const union AffineAnimCmd *const gAffineAnims_PlayerMonTiny[] = {
    gAffineAnim_PlayerMonTiny,
};


void PlayerStaysShrunk(struct ScriptContext *ctx)  // Add ctx parameter
{
    (void)ctx;// Ignore ctx if unused
    struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    u8 spriteId = playerObj->spriteId;
    struct Sprite *sprite = &gSprites[spriteId];

    // Keep the player shrunk by reapplying the shrunk affine animation
    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    sprite->affineAnims = gAffineAnims_ShrunkPlayer;
    StartSpriteAffineAnim(sprite, 0);
}

void StartPlayerShrinkAnimation(struct ScriptContext *ctx)  // Add ctx parameter
{
    // Ignore ctx if unused
    struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    u8 spriteId = playerObj->spriteId;
    struct Sprite *sprite = &gSprites[spriteId];
    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    sprite->affineAnims = gAffineAnims_Shrinking;
    StartSpriteAffineAnim(sprite, 0);

}

void ResetPlayerSprite(struct ScriptContext *ctx)  // Add ctx parameter
{
    // Ignore ctx if unused
    struct ObjectEvent *playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    u8 spriteId = playerObj->spriteId;
    struct Sprite *sprite = &gSprites[spriteId];

    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    sprite->affineAnims = gAffineAnims_Reset;
    StartSpriteAffineAnim(sprite, 0);
    sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
    InitSpriteAffineAnim(sprite);
    ScriptContext_ContinueScript(ctx);
    }