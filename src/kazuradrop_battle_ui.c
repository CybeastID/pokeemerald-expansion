// Kazuradrop Battle UI - FGO-style buff icons + Bug Space threshold panel
// Layout (top-left, above Kazuradrop's nameplate):
//
//   x=8              x=42  x=60
//   ┌─────────────┐   🔶    💜
//   │     150     │  Guts  Invin
//   └─────────────┘
//   (slides in)      (pop in when active)

#include "global.h"
#include "battle.h"
#include "battle_interface.h"
#include "battle_gimmick.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "kazuradrop_battle_ui.h"
#include "sprite.h"
#include "palette.h"
#include "string_util.h"
#include "window.h"
#include "text.h"
#include "fonts.h"
#include "menu.h"
#include "bg.h"
#include "line_break.h"
#include "battle_bg.h"

// =====================================================================
// Graphics
// =====================================================================

// 40x12 panel backing (baked border + fill, dark purple)
static const u8 ALIGNED(4) sKazuPanelGfx[] =
    INCBIN_U8("graphics/special/kazfightui/kazu_panel.4bpp");

// 16x16 buff icons
static const u8 ALIGNED(4) sKazuGutsIconGfx[] =
    INCBIN_U8("graphics/special/kazfightui/Guts8x8.4bpp");
static const u8 ALIGNED(4) sKazuInvincibleIconGfx[] =
    INCBIN_U8("graphics/special/kazfightui/Invin8x8.4bpp");

// Shared palette for panel + buff icons
static const u16 sKazuBuffIconsPal[] =
    INCBIN_U16("graphics/special/kazfightui/kazu_buff_icons.gbapal");

static const u8 ALIGNED(4) sKazuNumberGfx[256] = {1}; // 8 tiles for 32x16
static const struct SpriteSheet sSpriteSheet_KazuNumber =
    { sKazuNumberGfx, sizeof(sKazuNumberGfx), TAG_KAZU_NUMBER };

// Kazura's dialogue box (Moon Cell styled message window)
static const u8 ALIGNED(4) sKazuraBoxGfx[] =
    INCBIN_U8("graphics/battle_interface/textboxkaztiled.4bpp");
static const u16 sKazuraBoxPal[] =
    INCBIN_U16("graphics/battle_interface/textboxkaztiled.gbapal");
static const u8 ALIGNED(4) sKazuraBoxTilemap[] =
    INCBIN_U8("graphics/battle_interface/textboxkaztiled.bin");

// Purple HP bar palette for Kazuradrop's break bar
static const u16 sKazuHpBarPurplePal[] =
    INCBIN_U16("graphics/battle_interface/hpbar_purple.gbapal");

// =====================================================================
// Tags
// =====================================================================


static const struct SpriteSheet sSpriteSheet_KazuPanel =
    { sKazuPanelGfx, sizeof(sKazuPanelGfx), TAG_KAZU_PANEL };
static const struct SpriteSheet sSpriteSheet_KazuGutsIcon =
    { sKazuGutsIconGfx, sizeof(sKazuGutsIconGfx), TAG_KAZU_GUTS_ICON };
static const struct SpriteSheet sSpriteSheet_KazuInvincibleIcon =
    { sKazuInvincibleIconGfx, sizeof(sKazuInvincibleIconGfx), TAG_KAZU_INVINCIBLE_ICON };

static const struct SpritePalette sSpritePalette_KazuBuffIcons =
    { sKazuBuffIconsPal, TAG_KAZU_BUFF_PAL };

// =====================================================================
// OAM data
// =====================================================================

// Panel: 40x12 — closest GBA OAM fit is 32x16 (ST_OAM_H_RECTANGLE size 1)
// Adjust your art to fit within 32x16 or use 64x16 (size 2) if you need more width
static const struct OamData sOamData_Panel =
{
    .shape    = ST_OAM_H_RECTANGLE,
    .size     = 2,    // 32x16
    .priority = 1,
};

static const struct OamData sOamData_16x16 =
{
    .shape    = ST_OAM_SQUARE,
    .size     = 1,    // 16x16
    .priority = 1,
};

static const struct OamData sOamData_8x8 =
{
    .shape    = ST_OAM_SQUARE,
    .size     = 0,    // 8x8
    .priority = 1,
};
static const struct OamData sOamData_Number =
{
    .shape    = ST_OAM_H_RECTANGLE,
    .size     = 2,    // 32x8
    .priority = 0,
};

// =====================================================================
// Sprite data field aliases
// =====================================================================

#define tBattler    data[0]
#define tState      data[1]
#define tTargetX    data[2]
#define tFlashing   data[3]
#define tFlashTimer data[4]
#define tSlideBack  data[5]
#define tIconType   data[6]

// =====================================================================
// Panel layout constants
// =====================================================================

// Resting position of the panel's left edge on screen
#define BUGSPACE_PANEL_X_REST    30
// Width of the panel sprite (must match OAM size above)
#define BUGSPACE_PANEL_WIDTH     32
// Y offset relative to healthbox sprite origin — puts it above the nameplate
#define BUGSPACE_PANEL_Y_OFFSET  -22
// Slide speed in pixels per frame
#define BUGSPACE_PANEL_SLIDE_PX  6

// Icon positions: immediately right of panel, same Y
#define POS_GUTS_X_OFFSET       (BUGSPACE_PANEL_WIDTH + 4)
#define POS_GUTS_Y_OFFSET       BUGSPACE_PANEL_Y_OFFSET
#define POS_INVINCIBLE_X_OFFSET (BUGSPACE_PANEL_WIDTH + 14)
#define POS_INVINCIBLE_Y_OFFSET BUGSPACE_PANEL_Y_OFFSET

// Panel slide states
#define PANEL_IDLE         0
#define PANEL_SLIDING_IN   1
#define PANEL_SLIDING_OUT  2

// Kazuradrop dialogue box
#define KAZ_WIN_BASE_TILE_NUM  0x03D0
#define KAZ_WIN_PALETTE_NUM    8
#define KAZ_TILE_HFLIP         0x0400
#define KAZ_TILE_VFLIP         0x0800
#define KAZ_TILE_HVFLIP        0x0C00

// =====================================================================
// State tracking
// =====================================================================

static u8 sBugSpacePanelSpriteId[MAX_BATTLERS_COUNT];
static u8 sGutsSpriteId[MAX_BATTLERS_COUNT];
static u8 sInvincibleSpriteId[MAX_BATTLERS_COUNT];
static bool32 sKazuBuffIconsLoaded = FALSE;
static bool32 sKazuHpBarPalLoaded = FALSE;
static u8 sNumberSpriteId[MAX_BATTLERS_COUNT];
static u8 sKazuraDialogWindowId;

// =====================================================================
// Forward declarations
// =====================================================================

static void LoadKazuBuffIconGfx(void);
static void WriteThresholdIntoPanel(u32 spriteId);
static u8 *AddTextPrinterForKazuPanel(const u8 *str, u32 x, u32 y, u32 bgColor, u32 *windowId);
static void DrawKazuraWindowFrame(u8 windowId);

// =====================================================================
// Panel sprite callback
// =====================================================================

static void SpriteCb_KazuNumber(struct Sprite *sprite)
{
    u32 battler = sprite->tBattler;
    u8 panelId = sBugSpacePanelSpriteId[battler];
    if (panelId == MAX_SPRITES)
        return;
    // Center over panel: panel is 32px wide, number sprite is 32px wide, so same X
    // Panel is 16px tall, number is 8px, so Y + 4 to vertically center
    sprite->x = gSprites[panelId].x;
    sprite->y = gSprites[panelId].y + 1;
    sprite->invisible = gSprites[panelId].invisible;
}

static void SpriteCb_BugSpacePanel(struct Sprite *sprite)
{
    u32 battler = sprite->tBattler;

    // Y tracks healthbox, X is managed independently by slide state
    sprite->y = gSprites[gHealthboxSpriteIds[battler]].y + BUGSPACE_PANEL_Y_OFFSET;

    switch (sprite->tState)
    {
    case PANEL_SLIDING_IN:
        sprite->x += BUGSPACE_PANEL_SLIDE_PX;
        if (sprite->x >= BUGSPACE_PANEL_X_REST)
        {
            sprite->x = BUGSPACE_PANEL_X_REST;
            sprite->tState = PANEL_IDLE;
            WriteThresholdIntoPanel(sprite - gSprites);
        }
        break;

    case PANEL_SLIDING_OUT:
        sprite->x -= BUGSPACE_PANEL_SLIDE_PX;
        if (sprite->x <= -(s16)BUGSPACE_PANEL_WIDTH)
        {
            // Never destroy here: SlideOut is used for transformation and SlideIn
            // must reliably find the panel sprite later in the same script flow.
            sprite->x = -(s16)BUGSPACE_PANEL_WIDTH;
            sprite->invisible = TRUE;
            sprite->tState = PANEL_IDLE;
            sprite->tSlideBack = TRUE;
        }
        break;

    case PANEL_IDLE:
        if (sprite->tFlashing)
        {
            sprite->tFlashTimer++;
            sprite->invisible = (sprite->tFlashTimer / 3) % 2;
            if (sprite->tFlashTimer >= 30)
            {
                sprite->tFlashing = FALSE;
                sprite->tFlashTimer = 0;
                sprite->invisible = FALSE;
            }
        }
        break;
    }
}

// =====================================================================
// Buff icon sprite callbacks
// =====================================================================

static void SpriteCb_KazuBuffIcon(struct Sprite *sprite)
{
    u32 battler = sprite->tBattler;
    sprite->x = gSprites[gHealthboxSpriteIds[battler]].x + sprite->tTargetX;
    sprite->y = gSprites[gHealthboxSpriteIds[battler]].y + sprite->tState;
    // Mirror healthbox visibility
    sprite->invisible = gSprites[gHealthboxSpriteIds[battler]].invisible;
}

// Invincible icon: flashes then self-destructs
static void SpriteCb_KazuInvincibleIcon(struct Sprite *sprite)
{
    SpriteCb_KazuBuffIcon(sprite);

    sprite->tFlashTimer++;
    if (sprite->tFlashTimer < 60)
        sprite->invisible = (sprite->tFlashTimer / 4) % 2;
    else
        DestroyKazuradropBuffIcon(sprite->tBattler, KA_BUFF_INVINCIBLE);
}

// =====================================================================
// Sprite and Window templates
// =====================================================================

static const struct SpriteTemplate sSpriteTemplate_KazuPanel =
{
    .tileTag     = TAG_KAZU_PANEL,
    .paletteTag  = TAG_KAZU_BUFF_PAL,
    .oam         = &sOamData_Panel,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_BugSpacePanel,
};

static const struct SpriteTemplate sSpriteTemplate_KazuGutsIcon =
{
    .tileTag     = TAG_KAZU_GUTS_ICON,
    .paletteTag  = TAG_KAZU_BUFF_PAL,
    .oam         = &sOamData_8x8,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_KazuBuffIcon,
};

static const struct SpriteTemplate sSpriteTemplate_KazuInvincibleIcon =
{
    .tileTag     = TAG_KAZU_INVINCIBLE_ICON,
    .paletteTag  = TAG_KAZU_BUFF_PAL,
    .oam         = &sOamData_8x8,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_KazuInvincibleIcon,
};

static const struct WindowTemplate sKazuPanelWindowTemplate = {
    .bg          = 0,
    .tilemapLeft = 0,
    .tilemapTop  = 0,
    .width       = 4,
    .height      = 2,  // back to 2 — font is taller than 8px
    .paletteNum  = 0,
    .baseBlock   = 0
};

static const struct SpriteTemplate sSpriteTemplate_KazuNumber =
{
    .tileTag     = TAG_KAZU_NUMBER,
    .paletteTag  = TAG_KAZU_BUFF_PAL,
    .oam         = &sOamData_Number,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_KazuNumber,
};

// Kazuradrop dialogue box window template
static const struct WindowTemplate sKazuraDialogWindowTemplate = {
    .bg          = 0,
    .tilemapLeft = 1,
    .tilemapTop  = 15,
    .width       = 28,
    .height      = 4,
    .paletteNum  = KAZ_WIN_PALETTE_NUM,
    .baseBlock   = 0x0360,
};


// =====================================================================
// Internal helpers
// =====================================================================

static void LoadKazuBuffIconGfx(void)
{
    if (!sKazuBuffIconsLoaded)
    {
        LoadSpritePalette(&sSpritePalette_KazuBuffIcons);
        sKazuBuffIconsLoaded = TRUE;
    }
}

// Writes the current bpThreshold value into the panel sprite's VRAM tiles.
// Mirrors the pattern used by PrintHpOnHealthbox in battle_interface.c.
static void WriteThresholdIntoPanel(u32 panelSpriteId)
{
    u8 numberSpriteId = sNumberSpriteId[gSprites[panelSpriteId].tBattler];
    if (numberSpriteId == MAX_SPRITES)
        return;

    u8 text[8];
    u32 windowId;
    u8 *windowTileData;
    u16 threshold = gBattleStruct->bugSpace.bpThreshold;

    ConvertIntToDecimalStringN(text, threshold, STR_CONV_MODE_RIGHT_ALIGN, 3);

     void *objVram = (void *)(OBJ_VRAM0)
                  + gSprites[numberSpriteId].oam.tileNum * TILE_SIZE_4BPP;
    
    /* void *objVram = (void *)(OBJ_VRAM0)
              + gSprites[panelSpriteId].oam.tileNum * TILE_SIZE_4BPP; */

    windowTileData = AddTextPrinterForKazuPanel(text, 6, 0, 3, &windowId);
    CpuCopy32(windowTileData, objVram, 8 * TILE_SIZE_4BPP);
    RemoveWindowOnHealthbox(windowId);
}

// =====================================================================
// Initialization
// =====================================================================

void InitKazuradropBattleIcons(void)
{
    for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        sBugSpacePanelSpriteId[i] = MAX_SPRITES;
        sGutsSpriteId[i]          = MAX_SPRITES;
        sInvincibleSpriteId[i]    = MAX_SPRITES;
        sNumberSpriteId[i] = MAX_SPRITES;
    }
    sKazuBuffIconsLoaded = FALSE;
    sKazuHpBarPalLoaded = FALSE;
}

void FreeKazuradropBattleIconGfx(void)
{
    FreeSpritePaletteByTag(TAG_KAZU_BUFF_PAL);
    FreeSpriteTilesByTag(TAG_KAZU_PANEL);
    FreeSpriteTilesByTag(TAG_KAZU_GUTS_ICON);
    FreeSpriteTilesByTag(TAG_KAZU_INVINCIBLE_ICON);
    FreeSpriteTilesByTag(TAG_KAZU_NUMBER);
    sKazuBuffIconsLoaded = FALSE;
}


// =====================================================================
// Bug Space panel — public API
// =====================================================================

static u8 *AddTextPrinterForKazuPanel(const u8 *str, u32 x, u32 y, u32 bgColor, u32 *windowId)
{
    u16 winId;
    u8 color[3];
    struct WindowTemplate winTemplate = sKazuPanelWindowTemplate;

    winId = AddWindow(&winTemplate);
    // No FillWindowPixelBuffer — panel art provides the background

    color[0] = bgColor;
    color[1] = 6;
    color[2] = 3;

    AddTextPrinterParameterized4(winId, FONT_SMALL, x, y, 0, 0, color, TEXT_SKIP_DRAW, str);

    *windowId = winId;
    return (u8 *)(GetWindowAttribute(winId, WINDOW_TILE_DATA));
}

void CreateBugSpacePanel(u32 battler)
{
    if (sBugSpacePanelSpriteId[battler] != MAX_SPRITES)
        return; // already exists

    LoadKazuBuffIconGfx();

    if (GetSpriteTileStartByTag(TAG_KAZU_PANEL) == 0xFFFF)
        LoadSpriteSheet(&sSpriteSheet_KazuPanel);

    // Start the panel just off the left edge of the screen
    s16 startX   = -(s16)BUGSPACE_PANEL_WIDTH;
    s16 targetX  = BUGSPACE_PANEL_X_REST;
    // Y: use healthbox y at creation time; callback keeps it updated
    s16 startY   = gSprites[gHealthboxSpriteIds[battler]].y + BUGSPACE_PANEL_Y_OFFSET;

    u32 spriteId = CreateSprite(&sSpriteTemplate_KazuPanel, startX, startY, 0);
    if (spriteId == MAX_SPRITES)
        return;

    gSprites[spriteId].tBattler   = battler;
    gSprites[spriteId].tState     = PANEL_SLIDING_IN;
    gSprites[spriteId].tTargetX   = targetX;
    gSprites[spriteId].tFlashing  = FALSE;
    gSprites[spriteId].tFlashTimer = 0;
    gSprites[spriteId].tSlideBack  = FALSE;

    sBugSpacePanelSpriteId[battler] = spriteId;
    // Number will be written once the slide-in completes (see callback)

    // Activate purple HP bar for Kazuradrop's break bar
    SetKazuradropHpBarPalette(battler, TRUE);

    if (GetSpriteTileStartByTag(TAG_KAZU_NUMBER) == 0xFFFF)
    LoadSpriteSheet(&sSpriteSheet_KazuNumber);

u32 numSpriteId = CreateSprite(&sSpriteTemplate_KazuNumber, startX, startY, 0);
if (numSpriteId != MAX_SPRITES)
{
    gSprites[numSpriteId].tBattler = battler;
    sNumberSpriteId[battler] = numSpriteId;
}
}

void RefreshBugSpaceNumber(u32 battler)
{
    u8 spriteId = sBugSpacePanelSpriteId[battler];
    if (spriteId == MAX_SPRITES)
        return;
    // Only write if the panel is resting — mid-slide it doesn't matter
    if (gSprites[spriteId].tState == PANEL_IDLE)
        WriteThresholdIntoPanel(spriteId);
}

void TriggerBugSpaceNumberFlash(u32 battler)
{
    u8 spriteId = sBugSpacePanelSpriteId[battler];
    if (spriteId == MAX_SPRITES)
        return;
    if (gSprites[spriteId].tState != PANEL_IDLE)
        return;
    gSprites[spriteId].tFlashing   = TRUE;
    gSprites[spriteId].tFlashTimer = 0;
}

void TriggerBugSpacePanelTransformSlide(u32 battler)
{
    u32 panelBattler = battler;

    // During transform scripts, gBattlerAttacker may not equal the battler that owns Bug Space.
    // Ensure we slide the correct panel sprite (created on bugSpace.sourceBattler).
    if (gBattleStruct != NULL)
        panelBattler = gBattleStruct->bugSpace.sourceBattler;

    u8 spriteId = sBugSpacePanelSpriteId[panelBattler];
    if (spriteId == MAX_SPRITES)
        return;

    // Hold offscreen after sliding out; SlideIn callable will bring it back later.
    gSprites[spriteId].tSlideBack = TRUE;
    gSprites[spriteId].tState     = PANEL_SLIDING_OUT;
}

void TriggerBugSpacePanelSlideIn(u32 battler)
{
    u32 panelBattler = battler;

    // During transform scripts, gBattlerAttacker may not equal the battler that owns Bug Space.
    // Ensure we slide the correct panel sprite (created on bugSpace.sourceBattler).
    if (gBattleStruct != NULL)
        panelBattler = gBattleStruct->bugSpace.sourceBattler;

    u8 spriteId = sBugSpacePanelSpriteId[panelBattler];
    if (spriteId == MAX_SPRITES)
        return;

    // Start sliding back in now.
    gSprites[spriteId].tSlideBack = FALSE;
    gSprites[spriteId].invisible  = FALSE;
    gSprites[spriteId].x          = -(s16)BUGSPACE_PANEL_WIDTH;
    gSprites[spriteId].tState     = PANEL_SLIDING_IN;

    // Snap the number immediately on activation (do not wait for slide-in to finish).
    WriteThresholdIntoPanel(spriteId);
}

void DestroyBugSpacePanel(u32 battler)
{
    u8 spriteId = sBugSpacePanelSpriteId[battler];
    if (spriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[spriteId]);
        sBugSpacePanelSpriteId[battler] = MAX_SPRITES;
        u8 numSpriteId = sNumberSpriteId[battler];
        if (numSpriteId != MAX_SPRITES)
        {
            DestroySprite(&gSprites[numSpriteId]);
            sNumberSpriteId[battler] = MAX_SPRITES;
        }
    }
}

void ReshowKazuradropBattleUI(void)
{
    // During ReshowBattleScreenAfterMenu / ReshowBlankBattleScreenAfterMenu,
    // battle code clears sprite data + VRAM (and effectively frees palettes/tiles).
    // Our cached "loaded" flags can be stale, so force reload paths.
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        sBugSpacePanelSpriteId[battler] = MAX_SPRITES;
        sGutsSpriteId[battler]          = MAX_SPRITES;
        sInvincibleSpriteId[battler]    = MAX_SPRITES;
        sNumberSpriteId[battler] = MAX_SPRITES;
    }

    sKazuBuffIconsLoaded = FALSE;
    sKazuHpBarPalLoaded = FALSE;

    if (gBattleStruct == NULL)
        return;

    // Panel only matters if Bug Space is active.
    if (gBattleStruct->bugSpace.active == TRUE)
        CreateBugSpacePanel(gBattleStruct->bugSpace.sourceBattler);

    // Recreate icons.
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        if (gBattleMons[battler].volatiles.kazuradropGuts)
            CreateKazuradropBuffIcon(battler, KA_BUFF_GUTS);

        if (gBattleStruct->bugSpace.invincibleActive & (1u << battler))
            CreateKazuradropBuffIcon(battler, KA_BUFF_INVINCIBLE);
    }

    // Ensure number is written if panel exists and is idle.
    if (gBattleStruct->bugSpace.active == TRUE)
        RefreshBugSpaceNumber(gBattleStruct->bugSpace.sourceBattler);
}

// =====================================================================
// Buff icons — public API
// =====================================================================

void CreateKazuradropBuffIcon(u32 battler, u32 iconType)
{
    LoadKazuBuffIconGfx();

    switch (iconType)
    {
    case KA_BUFF_GUTS:
        if (sGutsSpriteId[battler] != MAX_SPRITES)
            return;
        if (GetSpriteTileStartByTag(TAG_KAZU_GUTS_ICON) == 0xFFFF)
            LoadSpriteSheet(&sSpriteSheet_KazuGutsIcon);
        {
            // Spawn at final position immediately (pop in)
            s16 x = gSprites[gHealthboxSpriteIds[battler]].x + POS_GUTS_X_OFFSET;
            s16 y = gSprites[gHealthboxSpriteIds[battler]].y + POS_GUTS_Y_OFFSET;
            u32 spriteId = CreateSprite(&sSpriteTemplate_KazuGutsIcon, x, y, 0);
            if (spriteId == MAX_SPRITES)
                return;
            gSprites[spriteId].tBattler  = battler;
            gSprites[spriteId].tTargetX  = POS_GUTS_X_OFFSET;  // X offset stored here
            gSprites[spriteId].tState    = POS_GUTS_Y_OFFSET;   // Y offset stored here
            gSprites[spriteId].tIconType = KA_BUFF_GUTS;
            sGutsSpriteId[battler] = spriteId;
        }
        break;

    case KA_BUFF_INVINCIBLE:
        if (gBattleStruct != NULL)
            gBattleStruct->bugSpace.invincibleActive |= (1u << battler);
        if (sInvincibleSpriteId[battler] != MAX_SPRITES)
            return;
        if (GetSpriteTileStartByTag(TAG_KAZU_INVINCIBLE_ICON) == 0xFFFF)
            LoadSpriteSheet(&sSpriteSheet_KazuInvincibleIcon);
        {
            s16 x = gSprites[gHealthboxSpriteIds[battler]].x + POS_INVINCIBLE_X_OFFSET;
            s16 y = gSprites[gHealthboxSpriteIds[battler]].y + POS_INVINCIBLE_Y_OFFSET;
            u32 spriteId = CreateSprite(&sSpriteTemplate_KazuInvincibleIcon, x, y, 0);
            if (spriteId == MAX_SPRITES)
                return;
            gSprites[spriteId].tBattler    = battler;
            gSprites[spriteId].tTargetX    = POS_INVINCIBLE_X_OFFSET;
            gSprites[spriteId].tState      = POS_INVINCIBLE_Y_OFFSET;
            gSprites[spriteId].tIconType   = KA_BUFF_INVINCIBLE;
            gSprites[spriteId].tFlashTimer = 0;
            sInvincibleSpriteId[battler] = spriteId;
        }
        break;
    }
}

void DestroyKazuradropBuffIcon(u32 battler, u32 iconType)
{
    u8 spriteId;
    switch (iconType)
    {
    case KA_BUFF_GUTS:
        spriteId = sGutsSpriteId[battler];
        if (spriteId != MAX_SPRITES)
        {
            DestroySprite(&gSprites[spriteId]);
            sGutsSpriteId[battler] = MAX_SPRITES;
        }
        break;
    case KA_BUFF_INVINCIBLE:
        spriteId = sInvincibleSpriteId[battler];
        if (gBattleStruct != NULL)
            gBattleStruct->bugSpace.invincibleActive &= ~(1u << battler);
        if (spriteId != MAX_SPRITES)
        {
            DestroySprite(&gSprites[spriteId]);
            sInvincibleSpriteId[battler] = MAX_SPRITES;
        }
        break;
    }
}

void DestroyAllKazuradropBuffIcons(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        DestroyKazuradropBuffIcon(battler, KA_BUFF_GUTS);
        DestroyKazuradropBuffIcon(battler, KA_BUFF_INVINCIBLE);
    }
}

// =====================================================================
// BS_callable wrappers
// =====================================================================

void BS_ShowKazuGutsIcon(void)
{
    NATIVE_ARGS();
    CreateKazuradropBuffIcon(gBattlerAttacker, KA_BUFF_GUTS);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ShowKazuInvincibleIcon(void)
{
    NATIVE_ARGS();
    CreateKazuradropBuffIcon(gBattlerAttacker, KA_BUFF_INVINCIBLE);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_BugSpacePanelSlideIn(void)
{
    NATIVE_ARGS();
    TriggerBugSpacePanelSlideIn(gBattlerAttacker);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_BugSpacePanelSlideOut(void)
{
    NATIVE_ARGS();
    TriggerBugSpacePanelTransformSlide(gBattlerAttacker);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ShowKazuraDialogueBox(void)
{
    NATIVE_ARGS(u16 stringId, u8 keep);

    if (gBattleStruct->bugSpace.kazuraDialogState == 0)
    {
        ShowKazuraDialogueBox(gBattleStringsTable[cmd->stringId]);
        gBattleStruct->bugSpace.kazuraDialogState = 1;
        return;
    }
    if (IsTextPrinterActive(sKazuraDialogWindowId))
        return;
    if (!JOY_NEW(A_BUTTON))
        return;
    HideKazuraDialogueBox();
    gBattleStruct->bugSpace.kazuraDialogState = 0;
    if (!cmd->keep)
        LoadBattleTextboxAndBackground();
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_RestoreTextbox(void)
{
    NATIVE_ARGS();
    LoadBattleTextboxAndBackground();
    gBattlescriptCurrInstr = cmd->nextInstr;
}

// =====================================================================
// Kazuradrop dialogue box — public API
// =====================================================================

static void DrawKazuraWindowFrame(u8 windowId)
{
    u8  bg     = GetWindowAttribute(windowId, WINDOW_BG);
    u16 left   = GetWindowAttribute(windowId, WINDOW_TILEMAP_LEFT);
    u16 top    = GetWindowAttribute(windowId, WINDOW_TILEMAP_TOP);
    u16 width  = GetWindowAttribute(windowId, WINDOW_WIDTH);
    u16 height = GetWindowAttribute(windowId, WINDOW_HEIGHT);
    u8  pal    = KAZ_WIN_PALETTE_NUM;
    u16 base   = KAZ_WIN_BASE_TILE_NUM;

    // Top row
    FillBgTilemapBufferRect(bg, base + 0,                    left - 1,          top - 1, 1,          1, pal); // top-left solid
    FillBgTilemapBufferRect(bg, base + 1,                    left,              top - 1, 1,          1, pal); // top-left curve
    FillBgTilemapBufferRect(bg, base + 2,                    left + 1,          top - 1, width - 2,  1, pal); // top edge repeat
    FillBgTilemapBufferRect(bg, base + 3,                    left + width - 1,  top - 1, 1,          1, pal); // top-right curve
    FillBgTilemapBufferRect(bg, (base + 0) | KAZ_TILE_HFLIP,  left + width,      top - 1, 1,          1, pal); // top-right solid

    // Middle rows
    FillBgTilemapBufferRect(bg, base + 4,                    left - 1,          top,     1,      height, pal); // left edge
    FillBgTilemapBufferRect(bg, base + 5,                    left,              top,     width,  height, pal); // fill (grid baked in)
    FillBgTilemapBufferRect(bg, (base + 4) | KAZ_TILE_HFLIP,  left + width,      top,     1,      height, pal); // right edge

    // Bottom row
    FillBgTilemapBufferRect(bg, (base + 0) | KAZ_TILE_VFLIP,  left - 1,          top + height, 1,         1, pal); // bottom-left solid
    FillBgTilemapBufferRect(bg, base + 6,                    left,              top + height, 1,         1, pal); // bottom-left curve
    FillBgTilemapBufferRect(bg, base + 7,                    left + 1,          top + height, width - 2, 1, pal); // bottom edge repeat
    FillBgTilemapBufferRect(bg, base + 8,                    left + width - 1,  top + height, 1,         1, pal); // bottom-right curve
    FillBgTilemapBufferRect(bg, (base + 0) | KAZ_TILE_HVFLIP, left + width,      top + height, 1,         1, pal); // bottom-right solid
}

void ShowKazuraDialogueBox(const u8 *str)
{
    u8 buffer[512];

    sKazuraDialogWindowId = 0xFF; // sentinel reset before use
    LoadBgTiles(0, sKazuraBoxGfx, sizeof(sKazuraBoxGfx), KAZ_WIN_BASE_TILE_NUM);
    LoadPalette(sKazuraBoxPal, BG_PLTT_ID(KAZ_WIN_PALETTE_NUM), PLTT_SIZE_4BPP);

    sKazuraDialogWindowId = AddWindow(&sKazuraDialogWindowTemplate);
    DebugPrintf("KazuraDialogWindowId: %d", sKazuraDialogWindowId);
    FillWindowPixelBuffer(sKazuraDialogWindowId, PIXEL_FILL(0));

    DrawKazuraWindowFrame(sKazuraDialogWindowId);

    StringCopy(buffer, str);
    BreakStringAutomatic(buffer, 220, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);

    static const u8 color[3] = {0, 9, 11};
    AddTextPrinterParameterized4(sKazuraDialogWindowId, FONT_NORMAL,
                                 4, 2, 0, 0, color, 1, buffer);

    PutWindowTilemap(sKazuraDialogWindowId);
    CopyWindowToVram(sKazuraDialogWindowId, COPYWIN_FULL);
    // CopyBgTilemapBufferToVram(0);
}

void HideKazuraDialogueBox(void)
{
    if (sKazuraDialogWindowId == 0xFF)
        return;
    ClearWindowTilemap(sKazuraDialogWindowId);
    CopyWindowToVram(sKazuraDialogWindowId, COPYWIN_FULL);
    CopyBgTilemapBufferToVram(0);
    RemoveWindow(sKazuraDialogWindowId);
    sKazuraDialogWindowId = 0xFF;
}

// =====================================================================
// HP bar palette override (purple break bar → red after break)
// =====================================================================

// Accessor alias — matches the same field used in battle_interface.c
// healthbox sprite's data[5] holds the healthbar sprite ID.
#define hMain_HealthBarSpriteId     data[5]

static const struct SpritePalette sSpritePalette_KazuHpBarPurple =
    { sKazuHpBarPurplePal, TAG_KAZU_HP_BAR_PAL };

void LoadKazuradropHpBarPalette(void)
{
    DebugPrintf("LKHBP called, sKazuHpBarPalLoaded=%d, preExistingSlot=%d", sKazuHpBarPalLoaded, IndexOfSpritePaletteTag(TAG_KAZU_HP_BAR_PAL));
    if (!sKazuHpBarPalLoaded)
    {
        LoadSpritePalette(&sSpritePalette_KazuHpBarPurple);
        sKazuHpBarPalLoaded = TRUE;
    }
    DebugPrintf("LKHBP done, slotAfter=%d", IndexOfSpritePaletteTag(TAG_KAZU_HP_BAR_PAL));
}

void SetKazuradropHpBarPalette(u32 battler, bool32 usePurple)
{
    // Ensure the purple palette is loaded if we need it
    if (usePurple)
        LoadKazuradropHpBarPalette();

    // Get the healthbar sprite ID for this battler
    u8 healthboxSpriteId = gHealthboxSpriteIds[battler];
    if (healthboxSpriteId == MAX_SPRITES)
        return;

    u8 healthbarSpriteId = gSprites[healthboxSpriteId].hMain_HealthBarSpriteId;
    if (healthbarSpriteId == MAX_SPRITES)
        return;

    if (usePurple)
    {
        // Switch to the purple palette slot
        // Find which OBJ palette slot TAG_KAZU_HP_BAR_PAL was loaded into
        u8 palSlot = IndexOfSpritePaletteTag(TAG_KAZU_HP_BAR_PAL);
        DebugPrintf("SKHBP purple branch palSlot=%d", palSlot);
        if (palSlot != 0xFF)
            gSprites[healthbarSpriteId].oam.paletteNum = palSlot;
    }
    else
    {
        // Restore the original healthbar palette
        u8 palSlot = IndexOfSpritePaletteTag(TAG_HEALTHBAR_PAL);
        DebugPrintf("SKHBP restore branch palSlot=%d", palSlot);
        if (palSlot != 0xFF)
            gSprites[healthbarSpriteId].oam.paletteNum = palSlot;
    }
    DebugPrintf("SKHBP final oam.paletteNum=%d", gSprites[healthbarSpriteId].oam.paletteNum);
}

#undef hMain_HealthBarSpriteId

// =====================================================================
// Undefine data field aliases
// =====================================================================
#undef tBattler
#undef tState
#undef tTargetX
#undef tFlashing
#undef tFlashTimer
#undef tSlideBack
#undef tIconType
