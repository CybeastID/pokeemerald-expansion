#include "global.h"
#include "battle.h"
#include "battle_interface.h"
#include "battle_status_icons.h"
#include "battle_util.h"
#include "castoria_name_theft.h"
#include "palette.h"
#include "sprite.h"

static void SpriteCB_BattleStatusIcon(struct Sprite *sprite);
static u8 sIconSpriteIds[MAX_BATTLERS_COUNT][BATTLE_STATUS_ICON_COUNT];

static const u8 ALIGNED(4) sGutsGfx[] = INCBIN_U8("graphics/special/kazfightui/Guts8x8.4bpp");
static const u8 ALIGNED(4) sInvincibleGfx[] = INCBIN_U8("graphics/special/kazfightui/Invin8x8.4bpp");
static const u8 ALIGNED(4) sMageArmorGfx[] = INCBIN_U8("graphics/special/CasterUI/MageArmor16.4bpp");
static const u8 ALIGNED(4) sNoOrderChangeGfx[] = INCBIN_U8("graphics/battle_interface/no_order_change.4bpp");

// Also used by the Bug Space panel. This module owns the shared palette.
static const u16 sBuffPal[] = INCBIN_U16("graphics/special/kazfightui/kazu_buff_icons.gbapal");
static const struct SpritePalette sBuffPalette = {sBuffPal, TAG_BATTLE_STATUS_ICON_PAL};
static const u16 sNoOrderChangePal[] = INCBIN_U16("graphics/battle_interface/no_order_change.gbapal");
static const struct SpritePalette sNoOrderChangePalette = {sNoOrderChangePal, TAG_BATTLE_STATUS_NO_ORDER_CHANGE};

static const struct OamData sOam8x8 =
{
    .shape = ST_OAM_SQUARE,
    .size = 0,
    .priority = 1,
};

static const struct OamData sOam16x16 =
{
    .shape = ST_OAM_SQUARE,
    .size = 1,
    .priority = 0,
};

static bool32 HasGutsIcon(u32 battler)
{
    return gBattleMons[battler].volatiles.kazuradropGuts != 0;
}

static bool32 HasInvincibleIcon(u32 battler)
{
    return (gBattleStruct->bugSpace.invincibleActive & (1u << battler)) != 0;
}

static void ExpireInvincibleIcon(u32 battler)
{
    gBattleStruct->bugSpace.invincibleActive &= ~(1u << battler);
}

struct BattleStatusIconInfo
{
    struct SpriteSheet sheet;
    struct SpriteTemplate template;
    const struct SpritePalette *palette;
    bool32 (*isActive)(u32 battler);
    void (*onExpire)(u32 battler);
    s16 playerX;
    s16 opponentX;
    s16 y;
    u16 duration;
    u8 flashInterval;
    bool8 singlesOnly;
    bool8 hideWhenFainted;
};

#define ICON_TEMPLATE(tiles, pal, oamData) \
    { .tileTag = tiles, .paletteTag = pal, .oam = oamData, \
      .anims = gDummySpriteAnimTable, .affineAnims = gDummySpriteAffineAnimTable, \
      .callback = SpriteCB_BattleStatusIcon }

// Add icons here; allocation, healthbox tracking, restoration and cleanup are shared.
static const struct BattleStatusIconInfo sIcons[BATTLE_STATUS_ICON_COUNT] =
{
    [BATTLE_STATUS_ICON_GUTS] =
    {
        .sheet = {sGutsGfx, sizeof(sGutsGfx), TAG_BATTLE_STATUS_GUTS},
        .template = ICON_TEMPLATE(TAG_BATTLE_STATUS_GUTS, TAG_BATTLE_STATUS_ICON_PAL, &sOam8x8),
        .palette = &sBuffPalette,
        .isActive = HasGutsIcon,
        .playerX = 36, .opponentX = 36, .y = -22,
    },
    [BATTLE_STATUS_ICON_INVINCIBLE] =
    {
        .sheet = {sInvincibleGfx, sizeof(sInvincibleGfx), TAG_BATTLE_STATUS_INVINCIBLE},
        .template = ICON_TEMPLATE(TAG_BATTLE_STATUS_INVINCIBLE, TAG_BATTLE_STATUS_ICON_PAL, &sOam8x8),
        .palette = &sBuffPalette,
        .isActive = HasInvincibleIcon,
        .onExpire = ExpireInvincibleIcon,
        .playerX = 46, .opponentX = 46, .y = -22,
        .duration = 60,
        .flashInterval = 4,
    },
    [BATTLE_STATUS_ICON_MAGE_ARMOR] =
    {
        .sheet = {sMageArmorGfx, sizeof(sMageArmorGfx), TAG_BATTLE_STATUS_MAGE_ARMOR},
        .template = ICON_TEMPLATE(TAG_BATTLE_STATUS_MAGE_ARMOR, TAG_BATTLE_STATUS_ICON_PAL, &sOam16x16),
        .palette = &sBuffPalette,
        .isActive = HasMageArmor,
        .playerX = -6, .opponentX = -6, .y = -22,
        .singlesOnly = TRUE,
        .hideWhenFainted = TRUE,
    },
    [BATTLE_STATUS_ICON_NO_ORDER_CHANGE] =
    {
        .sheet = {sNoOrderChangeGfx, sizeof(sNoOrderChangeGfx), TAG_BATTLE_STATUS_NO_ORDER_CHANGE},
        .template = ICON_TEMPLATE(TAG_BATTLE_STATUS_NO_ORDER_CHANGE, TAG_BATTLE_STATUS_NO_ORDER_CHANGE, &sOam16x16),
        .palette = &sNoOrderChangePalette,
        .isActive = HasNoOrderChange,
        // Below the trailing corner. The horizontal stagger clears the next
        // healthbox in doubles instead of placing a badge above its name/HP.
        .playerX = -32, .opponentX = 96, .y = 24,
    },
};

#undef ICON_TEMPLATE

static bool32 IsIconSprite(u32 spriteId, u32 battler, u32 icon)
{
    return spriteId < MAX_SPRITES && gSprites[spriteId].inUse
        && gSprites[spriteId].callback == SpriteCB_BattleStatusIcon
        && gSprites[spriteId].data[0] == battler
        && gSprites[spriteId].data[1] == icon;
}

static bool32 IsIconActive(u32 battler, u32 icon)
{
    return gBattleStruct != NULL && battler < gBattlersCount
        && !(sIcons[icon].singlesOnly && IsDoubleBattle())
        && sIcons[icon].isActive(battler);
}

void DestroyBattleStatusIcon(u32 battler, u32 icon)
{
    if (battler >= MAX_BATTLERS_COUNT || icon >= BATTLE_STATUS_ICON_COUNT)
        return;
    u32 spriteId = sIconSpriteIds[battler][icon];
    if (IsIconSprite(spriteId, battler, icon))
        DestroySprite(&gSprites[spriteId]);
    sIconSpriteIds[battler][icon] = MAX_SPRITES;
}

static void UpdateIconPosition(struct Sprite *sprite)
{
    u32 battler = sprite->data[0];
    u32 icon = sprite->data[1];
    const struct BattleStatusIconInfo *info = &sIcons[icon];
    u32 healthbox = gHealthboxSpriteIds[battler];

    if (healthbox >= MAX_SPRITES || !gSprites[healthbox].inUse
     || gSprites[healthbox].data[6] != battler
     || (info->hideWhenFainted && !IsBattlerAlive(battler)))
    {
        sprite->invisible = TRUE;
        return;
    }

    sprite->x = gSprites[healthbox].x + (IsOnPlayerSide(battler) ? info->playerX : info->opponentX);
    sprite->y = gSprites[healthbox].y + info->y;
    sprite->x2 = gSprites[healthbox].x2;
    sprite->y2 = gSprites[healthbox].y2;
    sprite->invisible = gSprites[healthbox].invisible
        || (info->flashInterval && (sprite->data[2] / info->flashInterval) % 2);
}

static void SpriteCB_BattleStatusIcon(struct Sprite *sprite)
{
    u32 battler = sprite->data[0];
    u32 icon = sprite->data[1];
    if (!IsIconActive(battler, icon))
    {
        DestroyBattleStatusIcon(battler, icon);
        return;
    }
    const struct BattleStatusIconInfo *info = &sIcons[icon];
    if (info->duration && ++sprite->data[2] >= info->duration)
    {
        if (info->onExpire != NULL)
            info->onExpire(battler);
        DestroyBattleStatusIcon(battler, icon);
        return;
    }
    UpdateIconPosition(sprite);
}

bool32 LoadBattleStatusIconPalette(void)
{
    if (IndexOfSpritePaletteTag(sBuffPalette.tag) == 0xFF)
        LoadSpritePalette(&sBuffPalette);
    return IndexOfSpritePaletteTag(sBuffPalette.tag) != 0xFF;
}

void InitBattleStatusIcons(void)
{
    // Call after ResetSpriteData as well: old IDs may now belong to other sprites.
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        for (u32 icon = 0; icon < BATTLE_STATUS_ICON_COUNT; icon++)
            sIconSpriteIds[battler][icon] = MAX_SPRITES;
}

void RefreshBattleStatusIcon(u32 battler, u32 icon)
{
    if (battler >= MAX_BATTLERS_COUNT || icon >= BATTLE_STATUS_ICON_COUNT)
        return;
    if (!IsIconActive(battler, icon))
    {
        DestroyBattleStatusIcon(battler, icon);
        return;
    }
    u32 spriteId = sIconSpriteIds[battler][icon];
    if (IsIconSprite(spriteId, battler, icon))
    {
        // Refreshes must not advance a transient icon's frame timer.
        UpdateIconPosition(&gSprites[spriteId]);
        return;
    }
    sIconSpriteIds[battler][icon] = MAX_SPRITES;
    const struct BattleStatusIconInfo *info = &sIcons[icon];
    if (IndexOfSpritePaletteTag(info->palette->tag) == 0xFF)
        LoadSpritePalette(info->palette);
    if (GetSpriteTileStartByTag(info->sheet.tag) == 0xFFFF)
        LoadSpriteSheet(&info->sheet);
    if (IndexOfSpritePaletteTag(info->palette->tag) == 0xFF
     || GetSpriteTileStartByTag(info->sheet.tag) == 0xFFFF)
        return;

    spriteId = CreateSprite(&info->template, 0, 0, 0);
    if (spriteId == MAX_SPRITES)
        return;
    sIconSpriteIds[battler][icon] = spriteId;
    gSprites[spriteId].data[0] = battler;
    gSprites[spriteId].data[1] = icon;
    UpdateIconPosition(&gSprites[spriteId]);
}

void RefreshBattlerStatusIcons(u32 battler)
{
    for (u32 icon = 0; icon < BATTLE_STATUS_ICON_COUNT; icon++)
        RefreshBattleStatusIcon(battler, icon);
}

void ReshowBattleStatusIcons(void)
{
    // IDs were discarded at ResetSpriteData, before healthboxes were rebuilt.
    for (u32 battler = 0; battler < gBattlersCount; battler++)
        RefreshBattlerStatusIcons(battler);
}

void FreeBattleStatusIcons(void)
{
    for (u32 icon = 0; icon < BATTLE_STATUS_ICON_COUNT; icon++)
    {
        for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
            DestroyBattleStatusIcon(battler, icon);
        FreeSpriteTilesByTag(sIcons[icon].sheet.tag);
        FreeSpritePaletteByTag(sIcons[icon].palette->tag);
    }
}

void ShowInvincibleBattleStatusIcon(u32 battler)
{
    if (gBattleStruct == NULL || battler >= gBattlersCount)
        return;
    gBattleStruct->bugSpace.invincibleActive |= 1u << battler;
    RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_INVINCIBLE);
}

// Keep battle-script native symbols while using the shared implementation.
void BS_ShowKazuGutsIcon(void)
{
    NATIVE_ARGS();
    RefreshBattleStatusIcon(gBattlerAttacker, BATTLE_STATUS_ICON_GUTS);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ShowKazuInvincibleIcon(void)
{
    NATIVE_ARGS();
    ShowInvincibleBattleStatusIcon(gBattlerAttacker);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

#if TESTING
u32 Test_GetBattleStatusIconSpriteId(u32 battler, u32 icon)
{
    return battler < MAX_BATTLERS_COUNT && icon < BATTLE_STATUS_ICON_COUNT
        ? sIconSpriteIds[battler][icon] : MAX_SPRITES;
}
#endif
