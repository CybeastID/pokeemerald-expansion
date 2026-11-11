// Bug Space threshold panel and Kazuradrop dialogue window.
#include "global.h"
#include "battle_script_native.h"
#include "battle.h"
#include "battle_interface.h"
#include "battle_gimmick.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "kazuradrop_battle_ui.h"
#include "battle_status_icons.h"
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
#include "graphics.h"
#include "servant_sigil.h"
#include "test_runner.h"

// =====================================================================
// Graphics
// =====================================================================

// 40x12 panel backing (baked border + fill, dark purple)
static const u8 ALIGNED(4) sKazuPanelGfx[] =
    INCBIN_U8("graphics/special/kazfightui/kazu_panel.4bpp");

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

static const struct SpriteSheet sSpriteSheet_KazuPanel =
    { sKazuPanelGfx, sizeof(sKazuPanelGfx), TAG_KAZU_PANEL };
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

// Panel slide states
#define PANEL_IDLE         0
#define PANEL_SLIDING_IN   1
#define PANEL_SLIDING_OUT  2

// Kazuradrop dialogue box
// NOTE: Must stay below BG0 tile 0x200 (physical VRAM < 0x4000). BG1/BG2 share
// char base 1 (VRAM + 0x4000), so BG0 tile indices >= 0x200 alias the
// BG_TRICK_ROOM / move-bg char data. The standard textbox is 256 tiles
// (0x000-0x0FF), and the action prompt/menu windows occupy 0x190-0x1F8, so
// this free band (0x100-0x190) is safe from both.
#define KAZ_WIN_BASE_TILE_NUM  0x0170
#define KAZ_WIN_PALETTE_NUM    8
// Opaque box palette index used as the dialogue interior background. The box's
// intended interior color is pure black (RGB 0x0000). A GBA BG pixel with
// palette index 0 is always fully transparent (it reveals the layer behind,
// e.g. the Trick Room bg), so index 0 can't be used. Palette slots 13-15 are
// currently unused and pure black, so we use one for an OPAQUE black interior.
#define KAZ_BOX_INTERIOR_COLOR 13
#define KAZ_TILE_HFLIP         0x0400
#define KAZ_TILE_VFLIP         0x0800
#define KAZ_TILE_HVFLIP        0x0C00


// =====================================================================
// State tracking
// =====================================================================

static u8 sBugSpacePanelSpriteId[MAX_BATTLERS_COUNT];
static u8 sNumberSpriteId[MAX_BATTLERS_COUNT];
static u8 sKazuraDialogWindowId;

// =====================================================================
// Forward declarations
// =====================================================================

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
// Sprite and Window templates
// =====================================================================

static const struct SpriteTemplate sSpriteTemplate_KazuPanel =
{
    .tileTag     = TAG_KAZU_PANEL,
    .paletteTag  = TAG_BATTLE_STATUS_ICON_PAL,
    .oam         = &sOamData_Panel,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_BugSpacePanel,
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
    .paletteTag  = TAG_BATTLE_STATUS_ICON_PAL,
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
    .baseBlock   = 0x0100,  // text tiles; frame gfx at KAZ_WIN_BASE_TILE_NUM (0x170)
};

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

void InitKazuradropBattleUI(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        sBugSpacePanelSpriteId[battler] = MAX_SPRITES;
        sNumberSpriteId[battler] = MAX_SPRITES;
    }
    InitServantSigils();
}

void FreeKazuradropBattleUI(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        DestroyBugSpacePanel(battler);
    DestroyAllServantSigils();
    FreeServantSigilGfx();
    FreeSpriteTilesByTag(TAG_KAZU_PANEL);
    FreeSpriteTilesByTag(TAG_KAZU_NUMBER);
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
    if (battler >= gBattlersCount)
        return;
    u32 healthbox = gHealthboxSpriteIds[battler];
    if (healthbox >= MAX_SPRITES || !gSprites[healthbox].inUse
     || gSprites[healthbox].data[6] != battler)
        return;
    if (sBugSpacePanelSpriteId[battler] != MAX_SPRITES)
        return; // already exists

    if (!LoadBattleStatusIconPalette())
        return;

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
    if (battler >= MAX_BATTLERS_COUNT)
        return;
    u8 spriteId = sBugSpacePanelSpriteId[battler];
    if (spriteId < MAX_SPRITES && gSprites[spriteId].inUse
     && gSprites[spriteId].callback == SpriteCb_BugSpacePanel
     && gSprites[spriteId].tBattler == battler)
        DestroySprite(&gSprites[spriteId]);
    sBugSpacePanelSpriteId[battler] = MAX_SPRITES;
    u8 numSpriteId = sNumberSpriteId[battler];
    if (numSpriteId < MAX_SPRITES && gSprites[numSpriteId].inUse
     && gSprites[numSpriteId].callback == SpriteCb_KazuNumber
     && gSprites[numSpriteId].tBattler == battler)
        DestroySprite(&gSprites[numSpriteId]);
    sNumberSpriteId[battler] = MAX_SPRITES;
}

void ReshowKazuradropBattleUI(void)
{
    // Menus reset sprites and VRAM, so discard the old panel IDs.
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        sBugSpacePanelSpriteId[battler] = MAX_SPRITES;
        sNumberSpriteId[battler] = MAX_SPRITES;
    }
    ReshowServantSigils();
    if (gBattleStruct != NULL && gBattleStruct->bugSpace.active)
        CreateBugSpacePanel(gBattleStruct->bugSpace.sourceBattler);
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
#if TESTING
        if (gTestRunnerEnabled)
        {
            TestRunner_Battle_RecordMessage(gBattleStringsTable[cmd->stringId]);
            if (gTestRunnerHeadless)
            {
                gBattlescriptCurrInstr = cmd->nextInstr;
                return;
            }
        }
#endif
        ShowKazuraDialogueBox(gBattleStringsTable[cmd->stringId]);
        gBattleStruct->bugSpace.kazuraDialogState = 1;
        return;
    }
    if (IsTextPrinterActive(sKazuraDialogWindowId))
        return;
    if (!gTestRunnerEnabled && !JOY_NEW(A_BUTTON))
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
    // DebugPrintf("KazuraDialogWindowId: %d", sKazuraDialogWindowId);
    FillWindowPixelBuffer(sKazuraDialogWindowId, PIXEL_FILL(KAZ_BOX_INTERIOR_COLOR));

    // Bake an OPAQUE version of the box's interior ("grid") tile into the whole
    // text window. The grid tile's raw background pixels are index 0 (pure
    // black), but index 0 is transparent on a GBA BG layer (it would reveal the
    // scene behind). Rewrite every index-0 pixel to an opaque black slot
    // (KAZ_BOX_INTERIOR_COLOR) so the grid keeps its pattern on a solid black
    // box. GLYPH_COPY (src/text.c) then only stamps the non-zero glyph colors
    // on top, so the text prints directly on the box.
    {
        u32 tile, i;
        u8 gridTile[TILE_SIZE_4BPP];
        const u8 *rawGrid = sKazuraBoxGfx + (5 * TILE_SIZE_4BPP); // tile 5 = interior grid fill
        u32 tileCount = sKazuraDialogWindowTemplate.width
                      * sKazuraDialogWindowTemplate.height; // 28 * 4 = 112

        for (i = 0; i < TILE_SIZE_4BPP; i++)
        {
            u8 px = rawGrid[i];
            u8 left  = px >> 4;      // left pixel (4bpp)
            u8 right = px & 0x0F;    // right pixel (4bpp)
            if (left  == 0) left  = KAZ_BOX_INTERIOR_COLOR;
            if (right == 0) right = KAZ_BOX_INTERIOR_COLOR;
            gridTile[i] = (left << 4) | right;
        }

        for (tile = 0; tile < tileCount; tile++)
            CopyToWindowPixelBuffer(sKazuraDialogWindowId, gridTile, TILE_SIZE_4BPP, tile);
    }

    DrawKazuraWindowFrame(sKazuraDialogWindowId);

    StringCopy(buffer, str);
    BreakStringAutomatic(buffer, 220, 2, FONT_NORMAL, SHOW_SCROLL_PROMPT);

    static const u8 color[3] = {0, 9, 11}; // bgColor=transparent (keeps baked grid), fgColor, shadowColor
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
// Undefine data field aliases
// =====================================================================
#undef tBattler
#undef tState
#undef tTargetX
#undef tFlashing
#undef tFlashTimer
#undef tSlideBack
