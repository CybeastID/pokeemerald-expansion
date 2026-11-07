#include "global.h"
#include "battle.h"
#include "battle_interface.h"
#include "battle_status_icons.h"
#include "battle_util.h"
#include "palette.h"
#include "sprite.h"

#define TAG_NO_ORDER_CHANGE_ICON 0xD7BE

static u8 sNoOrderChangeSpriteIds[MAX_BATTLERS_COUNT];

// FGO Order Change Seal: 12x12 artwork centered in a 16x16 / 16-color sprite.
static const u8 ALIGNED(4) sNoOrderChangeGfx[] =
    INCBIN_U8("graphics/battle_interface/no_order_change.4bpp");
static const u16 sNoOrderChangePal[] =
    INCBIN_U16("graphics/battle_interface/no_order_change.gbapal");

static const struct SpriteSheet sNoOrderChangeSheet =
    {sNoOrderChangeGfx, sizeof(sNoOrderChangeGfx), TAG_NO_ORDER_CHANGE_ICON};
static const struct SpritePalette sNoOrderChangePalette =
    {sNoOrderChangePal, TAG_NO_ORDER_CHANGE_ICON};

static const struct OamData sNoOrderChangeOam =
{
    .shape = ST_OAM_SQUARE,
    .size = 1,
    .priority = 0,
};

static void SpriteCB_NoOrderChange(struct Sprite *sprite)
{
    u32 battler = sprite->data[0];
    if (battler >= MAX_BATTLERS_COUNT)
    {
        DestroySprite(sprite);
        return;
    }
    if (gBattleStruct == NULL || !HasNoOrderChange(battler))
    {
        sNoOrderChangeSpriteIds[battler] = MAX_SPRITES;
        DestroySprite(sprite);
        return;
    }

    u32 healthbox = gHealthboxSpriteIds[battler];
    if (healthbox >= MAX_SPRITES || !gSprites[healthbox].inUse
     || gSprites[healthbox].data[6] != battler)
    {
        sprite->invisible = TRUE;
        return;
    }

    // Sit outside the panel: player healthboxes are against the right edge,
    // opponent healthboxes against the left. This works for both battle layouts.
    sprite->x = gSprites[healthbox].x + (IsOnPlayerSide(battler) ? -40 : 104);
    sprite->y = gSprites[healthbox].y;
    sprite->x2 = gSprites[healthbox].x2;
    sprite->y2 = gSprites[healthbox].y2;
    sprite->invisible = gSprites[healthbox].invisible;
}

static const struct SpriteTemplate sNoOrderChangeTemplate =
{
    .tileTag = TAG_NO_ORDER_CHANGE_ICON,
    .paletteTag = TAG_NO_ORDER_CHANGE_ICON,
    .oam = &sNoOrderChangeOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_NoOrderChange,
};

void InitNoOrderChangeIcons(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        sNoOrderChangeSpriteIds[battler] = MAX_SPRITES;
}

void RefreshNoOrderChangeIcon(u32 battler)
{
    if (gBattleStruct == NULL || battler >= gBattlersCount)
        return;

    u32 spriteId = sNoOrderChangeSpriteIds[battler];
    if (spriteId < MAX_SPRITES && gSprites[spriteId].inUse
     && gSprites[spriteId].callback == SpriteCB_NoOrderChange
     && gSprites[spriteId].data[0] == battler)
    {
        SpriteCB_NoOrderChange(&gSprites[spriteId]);
        return;
    }
    sNoOrderChangeSpriteIds[battler] = MAX_SPRITES;
    if (!HasNoOrderChange(battler))
        return;

    if (IndexOfSpritePaletteTag(TAG_NO_ORDER_CHANGE_ICON) == 0xFF)
        LoadSpritePalette(&sNoOrderChangePalette);
    if (GetSpriteTileStartByTag(TAG_NO_ORDER_CHANGE_ICON) == 0xFFFF)
        LoadSpriteSheet(&sNoOrderChangeSheet);
    if (IndexOfSpritePaletteTag(TAG_NO_ORDER_CHANGE_ICON) == 0xFF
     || GetSpriteTileStartByTag(TAG_NO_ORDER_CHANGE_ICON) == 0xFFFF)
        return;

    spriteId = CreateSprite(&sNoOrderChangeTemplate, 0, 0, 0);
    if (spriteId == MAX_SPRITES)
        return;
    sNoOrderChangeSpriteIds[battler] = spriteId;
    gSprites[spriteId].data[0] = battler;
    SpriteCB_NoOrderChange(&gSprites[spriteId]);
}

void FreeNoOrderChangeIcons(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        u32 spriteId = sNoOrderChangeSpriteIds[battler];
        if (spriteId < MAX_SPRITES && gSprites[spriteId].inUse
         && gSprites[spriteId].callback == SpriteCB_NoOrderChange
         && gSprites[spriteId].data[0] == battler)
            DestroySprite(&gSprites[spriteId]);
    }
    InitNoOrderChangeIcons();
    FreeSpriteTilesByTag(TAG_NO_ORDER_CHANGE_ICON);
    FreeSpritePaletteByTag(TAG_NO_ORDER_CHANGE_ICON);
}

#if TESTING
u32 Test_GetNoOrderChangeIconSpriteId(u32 battler)
{
    return battler < MAX_BATTLERS_COUNT ? sNoOrderChangeSpriteIds[battler] : MAX_SPRITES;
}
#endif
