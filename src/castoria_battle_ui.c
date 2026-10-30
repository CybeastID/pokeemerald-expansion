#include "global.h"
#include "battle.h"
#include "battle_interface.h"
#include "castoria_battle_ui.h"
#include "castoria_name_theft.h"
#include "graphics.h"
#include "palette.h"
#include "sprite.h"
// #include "task.h" // Optional healthbox slide below.

#define TAG_MAGE_ARMOR 0xD7BA
#define MAGE_ARMOR_X_OFFSET (-6)
#define MAGE_ARMOR_Y_OFFSET (-22)
// #define MAGE_ARMOR_SLIDE_FRAMES 16

static u8 sMageArmorSpriteIds[MAX_BATTLERS_COUNT];
// static u8 sMageArmorSlideTaskId;
// 12x12 artwork with transparent padding for the GBA's 16x16 OAM size.
static const u8 ALIGNED(4) sMageArmorGfx[] = INCBIN_U8("graphics/special/CasterUI/MageArmor16.4bpp");
static const u16 sMageArmorPal[] = INCBIN_U16("graphics/special/CasterUI/MageArmor16.gbapal");
static const struct SpriteSheet sMageArmorSheet = { sMageArmorGfx, sizeof(sMageArmorGfx), TAG_MAGE_ARMOR };
static const struct SpritePalette sMageArmorPalette = { sMageArmorPal, TAG_MAGE_ARMOR };

static const struct OamData sMageArmorOam =
{
    .shape = ST_OAM_SQUARE,
    .size = 1,
    .priority = 0,
};

static void SpriteCB_MageArmor(struct Sprite *sprite)
{
    u32 battler = sprite->data[0];
    u32 healthbox = gHealthboxSpriteIds[battler];

    // Optional slide: hide the icon while sMageArmorSlideTaskId != TASK_NONE.
    if (!HasMageArmor(battler) || !IsBattlerAlive(battler)
     || healthbox == MAX_SPRITES || !gSprites[healthbox].inUse)
    {
        sprite->invisible = TRUE;
        return;
    }

    sprite->x = gSprites[healthbox].x + MAGE_ARMOR_X_OFFSET;
    sprite->y = gSprites[healthbox].y + MAGE_ARMOR_Y_OFFSET;
    sprite->x2 = gSprites[healthbox].x2;
    sprite->y2 = gSprites[healthbox].y2;
    sprite->invisible = gSprites[healthbox].invisible;
}

static const struct SpriteTemplate sMageArmorTemplate =
{
    .tileTag = TAG_MAGE_ARMOR,
    .paletteTag = TAG_MAGE_ARMOR,
    .oam = &sMageArmorOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_MageArmor,
};

void InitMageArmorIcons(void)
{
    // sMageArmorSlideTaskId = TASK_NONE;
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        sMageArmorSpriteIds[battler] = MAX_SPRITES;
}

/*
 * Optional healthbox slide, retained for later use. Re-enable its task include,
 * constants, state, initialization, setup, visibility check, and cleanup together.
 * The coordinate offset in battle_interface.c must also be restored.
 *
static void Task_SlideMageArmorHealthbox(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u32 healthbox = task->data[0];

    task->data[3]++;
    gSprites[healthbox].y = task->data[1]
        + (task->data[2] - task->data[1]) * task->data[3] / MAGE_ARMOR_SLIDE_FRAMES;

    if (task->data[3] == MAGE_ARMOR_SLIDE_FRAMES)
    {
        sMageArmorSlideTaskId = TASK_NONE;
        DestroyTask(taskId);
    }
}
*/

void CreateMageArmorIcon(u32 battler)
{
    // The current icon row is positioned for singles.
    if (IsDoubleBattle() || !HasMageArmor(battler)
     || sMageArmorSpriteIds[battler] != MAX_SPRITES)
        return;

    if (IndexOfSpritePaletteTag(TAG_MAGE_ARMOR) == 0xFF)
        LoadSpritePalette(&sMageArmorPalette);
    if (GetSpriteTileStartByTag(TAG_MAGE_ARMOR) == 0xFFFF)
        LoadSpriteSheet(&sMageArmorSheet);
    if (IndexOfSpritePaletteTag(TAG_MAGE_ARMOR) == 0xFF
     || GetSpriteTileStartByTag(TAG_MAGE_ARMOR) == 0xFFFF)
        return;

    /* Optional slide setup. Use animate = FALSE when restoring after menus.
    bool32 animate = TRUE;
    u32 healthbox = gHealthboxSpriteIds[battler];
    s16 targetX, targetY;
    GetBattlerHealthboxCoords(battler, &targetX, &targetY);

    if (animate && gSprites[healthbox].y != targetY)
    {
        if (GetTaskCount() < NUM_TASKS)
        {
            u32 taskId = CreateTask(Task_SlideMageArmorHealthbox, 0);
            sMageArmorSlideTaskId = taskId;
            gTasks[taskId].data[0] = healthbox;
            gTasks[taskId].data[1] = gSprites[healthbox].y;
            gTasks[taskId].data[2] = targetY;
        }
        else
        {
            InitBattlerHealthboxCoords(battler);
        }
    }
    else
    {
        InitBattlerHealthboxCoords(battler);
    }
    */

    u32 spriteId = CreateSprite(&sMageArmorTemplate, 0, 0, 0);
    if (spriteId == MAX_SPRITES)
        return;

    sMageArmorSpriteIds[battler] = spriteId;
    gSprites[spriteId].data[0] = battler;
    SpriteCB_MageArmor(&gSprites[spriteId]);
}

void ReshowMageArmorIcons(void)
{
    // Menus clear sprites and VRAM, so discard the old IDs before recreating.
    InitMageArmorIcons();
    for (u32 battler = 0; battler < gBattlersCount; battler++)
        CreateMageArmorIcon(battler);
}

void FreeMageArmorIcons(void)
{
    // Optional slide cleanup:
    // if (sMageArmorSlideTaskId != TASK_NONE)
    //     DestroyTask(sMageArmorSlideTaskId);
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        if (sMageArmorSpriteIds[battler] != MAX_SPRITES)
            DestroySprite(&gSprites[sMageArmorSpriteIds[battler]]);
    }
    InitMageArmorIcons();
    FreeSpriteTilesByTag(TAG_MAGE_ARMOR);
    FreeSpritePaletteByTag(TAG_MAGE_ARMOR);
}
