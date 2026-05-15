// Kazuradrop Game Over Screen
// Triggered when the player loses while Bug Space is active.
// Displays a full-screen 8bpp artwork splash with a dark blend overlay,
// then fades in GAME OVER text as OBJ sprites before returning to the title screen.

#include "global.h"
#include "main.h"
#include "bg.h"
#include "gpu_regs.h"
#include "io_reg.h"
#include "palette.h"
#include "sprite.h"
#include "task.h"
#include "malloc.h"
#include "decompress.h"
#include "sound.h"
#include "event_data.h"
#include "scanline_effect.h"
#include "title_screen.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "kazuradrop_gameover.h"
#include "battle.h"
#include "overworld.h"
#include "save.h"
#include "battle_setup.h"
#include "battle_scripts.h"
#include "constants/battle.h"
#include "window.h"
#include "strings.h"
#include "menu.h"
#include "string_util.h"
#include "text.h"
#include "field_screen_effect.h"
#include "constants/songs.h"


// ---------------------------------------------------------------------------
// Asset INCBINs
// ---------------------------------------------------------------------------

static const u32 sKazGameOver_Tiles[] = INCBIN_U32("graphics/special/kazgameover.img.bin");
static const u16 sKazGameOver_Map[]   = INCBIN_U16("graphics/special/kazgameover.map.bin");
static const u16 sKazGameOver_Pal[]   = INCBIN_U16("graphics/special/kazgameover.pal.bin");

// GAME OVER text sprite assets (to be created separately)
static const u32 sGameOverText_TilesL[] = INCBIN_U32("graphics/special/gameover_text_l.img.bin");
static const u32 sGameOverText_TilesR[] = INCBIN_U32("graphics/special/gameover_text_r.img.bin");
static const u16 sGameOverText_Pal[]    = INCBIN_U16("graphics/special/gameover_text.pal.bin");
// ---------------------------------------------------------------------------
// BG Configuration
// ---------------------------------------------------------------------------

// BG2 = 8bpp Kazuradrop artwork
// BG0 = GAME OVER text (4bpp, sits on top)
static const struct BgTemplate sKazGameOver_BgTemplates[] =
{
    {   // BG0: GAME OVER text layer (front)
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,   // 4bpp
        .priority = 0,
        .baseTile = 0,
    },
    {   // BG2: Kazuradrop artwork (back)
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 1,   // 8bpp — full 256-color palette
        .priority = 3,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sKazGameOver_MsgWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 5,
    .width = 30,
    .height = 11,
    .paletteNum = 15,
    .baseBlock = 1,
};

// ---------------------------------------------------------------------------
// EWRAM state
// ---------------------------------------------------------------------------

// Tilemap buffer for BG2 (0x800 entries for a 32x32 screen map)
static EWRAM_DATA u16 sKazGameOver_Tilemap[0x800] = {0};

// ---------------------------------------------------------------------------
// Blend overlay state
// A: EVA (artwork weight), B: EVB (black weight)
// We animate from (16, 0) → (10, 6) over BLEND_DURATION frames
// ---------------------------------------------------------------------------

#define BLEND_START_DELAY   60      // frames to show artwork before darkening begins
#define BLEND_DURATION      36      // frames to animate the dark overlay
#define TEXT_FADE_DELAY     10      // frames after blend completes before text appears
#define HOLD_DURATION       180     // frames to hold the screen (3 seconds) before exit
#define BLEND_EVA_START     16
#define BLEND_EVA_END       10
#define BLEND_EVB_START     0
#define BLEND_EVB_END       6

// ---------------------------------------------------------------------------
// Task state indices
// ---------------------------------------------------------------------------

#define tState          data[0]
#define tTimer          data[1]
#define tBlendEVA       data[2]
#define tBlendEVB       data[3]
#define tWindowId data[4]

// ---------------------------------------------------------------------------
// Tag Defines
// ---------------------------------------------------------------------------
#define TAG_GAMEOVER_TEXT_L     0x5000
#define TAG_GAMEOVER_TEXT_R     0x5001
#define TAG_GAMEOVER_TEXT_PAL   0x5002

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

static bool8 InitKazGameOverScreen(void);
static void  CB2_KazGameOver(void);
static void  VBlankCB_KazGameOver(void);
static void  Task_KazGameOver_FadeIn(u8 taskId);
static void  Task_KazGameOver_BlendOverlay(u8 taskId);
static void  Task_KazGameOver_FadeInText(u8 taskId);
static void  Task_KazGameOver_Hold(u8 taskId);
static void  Task_KazGameOver_FadeOut(u8 taskId);
static void SpriteCB_GameOverText(struct Sprite *sprite);
static void CB2_KazGameOver_MessageInit (void);
static void Task_KazGameOver_Message(u8 taskId);

// ---------------------------------------------------------------------------
// OAM configuration for GAME OVER text sprites
// ---------------------------------------------------------------------------
static const struct OamData sGameOverText_OamData =
{
    .shape = SPRITE_SHAPE(64x64),
    .size  = SPRITE_SIZE(64x64),
    .priority = 0,
};

static const union AnimCmd sGameOverText_Anim[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END,
};

static const union AnimCmd *const sGameOverText_AnimTable[] =
{
    sGameOverText_Anim,
};

static const struct SpriteTemplate sGameOverTextL_Template =
{
    .tileTag     = TAG_GAMEOVER_TEXT_L,
    .paletteTag  = TAG_GAMEOVER_TEXT_PAL,
    .oam         = &sGameOverText_OamData,
    .anims       = sGameOverText_AnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_GameOverText, // custom callback to handle fade-in animation
};

static const struct SpriteTemplate sGameOverTextR_Template =
{
    .tileTag     = TAG_GAMEOVER_TEXT_R,
    .paletteTag  = TAG_GAMEOVER_TEXT_PAL,
    .oam         = &sGameOverText_OamData,
    .anims       = sGameOverText_AnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_GameOverText, // same callback for both halves of the text
};
static const u8 sKazGameOver_TextColors[] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY };
static u16 sFadeDelay;
// ---------------------------------------------------------------------------
// Entry point — call this from the battle loss handler
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Kaz Game Over message screen — runs before the artwork screen
// ---------------------------------------------------------------------------



static void CB2_KazGameOver_Message(void)
{
    RunTasks();
    UpdatePaletteFade();
}

static void VBlankCB_KazGameOver_Message(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_KazGameOver_MessageInit(void)
{
    SetVBlankCallback(NULL);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    PlayBGM(MUS_BB_GAMEOVER);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sKazGameOver_BgTemplates, ARRAY_COUNT(sKazGameOver_BgTemplates));
    ShowBg(0);
    SetVBlankCallback(VBlankCB_KazGameOver_Message);
    CreateTask(Task_KazGameOver_Message, 0);
    SetMainCallback2(CB2_KazGameOver_Message);
}

static void Task_KazGameOver_Message(u8 taskId)
{
    u32 windowId = 0;

    switch (gTasks[taskId].tState)
    {
    case 0:
        windowId = AddWindow(&sKazGameOver_MsgWindowTemplate);
        gTasks[taskId].tWindowId = windowId;
        Menu_LoadStdPalAt(BG_PLTT_ID(15));
        FillWindowPixelBuffer(windowId, PIXEL_FILL(0));
        PutWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_FULL);
        gTasks[taskId].tState++;
        break;

    case 1:
        windowId = gTasks[taskId].tWindowId;
        StringExpandPlaceholders(gStringVar4, gText_PlayerWasNeverSeenAgain);
        AddTextPrinterParameterized4(windowId, FONT_NORMAL, 2, 8, 1, 0, sKazGameOver_TextColors, 1, gStringVar4);
        gTextFlags.canABSpeedUpPrint = FALSE;
        gTasks[taskId].tState++;
        break;

    case 2:
        RunTextPrinters();
        if (!IsTextPrinterActive(gTasks[taskId].tWindowId))
            gTasks[taskId].tState++;
        break;

    case 3:
        windowId = gTasks[taskId].tWindowId;
        ClearWindowTilemap(windowId);
        CopyWindowToVram(windowId, COPYWIN_MAP);
        RemoveWindow(windowId);
        gTasks[taskId].tState++;
        break;

    case 4:
        // One frame gap after window cleanup before handing off
        gMain.state = 0;
        SetMainCallback2(CB2_DoKazGameOverScreen);
        DestroyTask(taskId);
        break;
    }
}

bool8 TrySetKazGameOverWhiteOut(void)
{
    if (FlagGet(FLAG_KAZURADROP_ENTERED_BATTLE))
    {
        FlagClear(FLAG_KAZURADROP_ENTERED_BATTLE);
        SetMainCallback2(CB2_KazGameOver_MessageInit);
        return TRUE;
    }
    else
    {
        SetMainCallback2(CB2_WhiteOut);
        return FALSE;
    }
}



void CB2_DoKazGameOverScreen(void)
{
    if (InitKazGameOverScreen())
        return;
    CreateTask(Task_KazGameOver_FadeIn, 0);
}

// ---------------------------------------------------------------------------
// Screen initialiser — runs as a state machine across frames
// Returns TRUE while still initialising, FALSE when done
// ---------------------------------------------------------------------------


static bool8 InitKazGameOverScreen(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        DmaFill16(3, 0, VRAM, VRAM_SIZE);
        DmaFill32(3, 0, OAM, OAM_SIZE);
        DmaFill16(3, 0, PLTT, PLTT_SIZE);
        ScanlineEffect_Stop();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        gMain.state++;
        break;

    case 1:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sKazGameOver_BgTemplates, ARRAY_COUNT(sKazGameOver_BgTemplates));
        SetBgTilemapBuffer(2, sKazGameOver_Tilemap);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        gMain.state++;
        break;

    case 2:
        CpuFastSet(sKazGameOver_Tiles, (void *)(BG_VRAM + 0x8000), sizeof(sKazGameOver_Tiles) / 4);
        DmaCopy16(3, sKazGameOver_Map, (void *)(BG_VRAM + 0xF800), sizeof(sKazGameOver_Map));
        while (REG_DMA3CNT & DMA_ENABLE) { }
        LoadPalette(sKazGameOver_Pal, BG_PLTT_ID(0), sizeof(u16) * 256);
        gMain.state++;
        break;

    case 3:
        SetGpuReg(REG_OFFSET_BLDCNT,   BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(BLEND_EVA_START, BLEND_EVB_START));
        SetGpuReg(REG_OFFSET_BLDY,     0);
        HideBg(0);
        ShowBg(2);
        sFadeDelay = 0;
        gMain.state++;
        break;

    case 4:
        sFadeDelay++;
        if (sFadeDelay >= 30)
            gMain.state++;
        break;

    case 5:
        SetVBlankCallback(VBlankCB_KazGameOver);
        BeginNormalPaletteFade(PALETTES_ALL, 5, 16, 0, RGB_BLACK);
        gMain.state++;
        break;

    case 6:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            SetMainCallback2(CB2_KazGameOver);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Main callback — just runs tasks and updates palette fade
// ---------------------------------------------------------------------------

static void CB2_KazGameOver(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

// ---------------------------------------------------------------------------
// VBlank callback
// ---------------------------------------------------------------------------

static void VBlankCB_KazGameOver(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ---------------------------------------------------------------------------
// Task: Wait, then trigger the dark blend overlay
// ---------------------------------------------------------------------------

static void Task_KazGameOver_FadeIn(u8 taskId)
{
    gTasks[taskId].tTimer++;
    if (gTasks[taskId].tTimer >= BLEND_START_DELAY)
    {
        gTasks[taskId].tTimer    = 0;
        gTasks[taskId].tBlendEVA = BLEND_EVA_START;
        gTasks[taskId].tBlendEVB = BLEND_EVB_START;
        gTasks[taskId].func      = Task_KazGameOver_BlendOverlay;
    }
}

// ---------------------------------------------------------------------------
// Task: Animate the dark overlay fading in over the artwork
// ---------------------------------------------------------------------------

static void Task_KazGameOver_BlendOverlay(u8 taskId)
{
    gTasks[taskId].tTimer++;

    // Interpolate EVA down and EVB up over BLEND_DURATION frames
    if (gTasks[taskId].tTimer <= BLEND_DURATION)
    {
        s16 progress = gTasks[taskId].tTimer;
        u8 eva = BLEND_EVA_START - ((BLEND_EVA_START - BLEND_EVA_END) * progress / BLEND_DURATION);
        u8 evb = BLEND_EVB_START + ((BLEND_EVB_END - BLEND_EVB_START) * progress / BLEND_DURATION);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(eva, evb));
        gTasks[taskId].tBlendEVA = eva;
        gTasks[taskId].tBlendEVB = evb;
    }

    if (gTasks[taskId].tTimer >= BLEND_DURATION + TEXT_FADE_DELAY)
    {
        gTasks[taskId].tTimer = 0;
        gTasks[taskId].func   = Task_KazGameOver_FadeInText;
    }
}

// ---------------------------------------------------------------------------
// Task: Fade in GAME OVER text layer
// TODO: Load GAME OVER sprite/tilemap assets here once created
// ---------------------------------------------------------------------------

// Sprite data indices
#define sTimer      data[0]
#define sFadeStep   data[1]

static void SpriteCB_GameOverText(struct Sprite *sprite)
{
    if (sprite->sTimer < 90)
        sprite->sTimer++;
    else
    {
        sprite->callback = SpriteCallbackDummy;
        return;
    }

    // fade
    if (sprite->sTimer <= 45)
    {
        u8 slot = IndexOfSpritePaletteTag(TAG_GAMEOVER_TEXT_PAL);
        u8 coeff = 16 - ((sprite->sTimer * 16) / 45);
        BlendPalettes(1 << (16 + slot), coeff, RGB_BLACK);
    }

    // drift
    if (sprite->sTimer <= 90)
    {
        sprite->data[2] += (32 * 256) / 90;
        sprite->y = sprite->data[3] + (sprite->data[2] >> 8);
    }
}

static void LoadGameOverTextSprites(void)
{
    struct SpriteSheet sheetL = { sGameOverText_TilesL, sizeof(sGameOverText_TilesL), TAG_GAMEOVER_TEXT_L };
    struct SpriteSheet sheetR = { sGameOverText_TilesR, sizeof(sGameOverText_TilesR), TAG_GAMEOVER_TEXT_R };
    struct SpritePalette pal  = { sGameOverText_Pal,    TAG_GAMEOVER_TEXT_PAL };

    LoadSpriteSheet(&sheetL);
    LoadSpriteSheet(&sheetR);
    LoadSpritePalette(&pal);

    // Center the two 64×64 sprites on the 240×160 screen
    // Left half:  x = 56, Right half: x = 120, y = 48 centers 64px tall graphic vertically
    u8 spriteL = CreateSprite(&sGameOverTextL_Template, 88,  56, 0);
    u8 spriteR = CreateSprite(&sGameOverTextR_Template, 152, 56, 0);
    gSprites[spriteL].data[3] = 56;
    gSprites[spriteR].data[3] = 56;
}
static void Task_KazGameOver_FadeInText(u8 taskId)
{
    LoadGameOverTextSprites();
    gTasks[taskId].tTimer = 0;
    gTasks[taskId].func   = Task_KazGameOver_Hold;
}
// ---------------------------------------------------------------------------
// Task: Hold the screen, wait for button press or timer
// ---------------------------------------------------------------------------

static void Task_KazGameOver_Hold(u8 taskId)
{
    gTasks[taskId].tTimer++;
    if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON))
    {
        gTasks[taskId].tTimer = 0;
        gTasks[taskId].func   = Task_KazGameOver_FadeOut;
    }
}

// ---------------------------------------------------------------------------
// Task: Fade to black, clear flag, return to title screen
// ---------------------------------------------------------------------------

static void Task_KazGameOver_FadeOut(u8 taskId)
{
    if (gTasks[taskId].tTimer == 0)
    {
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    }

    gTasks[taskId].tTimer++;
    UpdatePaletteFade();

    if (!gPaletteFade.active)
    {
        // Clear the flag so a future run doesn’t re-trigger this
        FlagClear(FLAG_KAZURADROP_ENTERED_BATTLE);

        // Return to title screen
        LoadGameSave(SAVE_NORMAL);
        SetMainCallback2(CB2_InitTitleScreen);
        DestroyTask(taskId);
    }
}

// callnative: Called from BattleScript_BugSpacePassiveOHKO
// Sets the game to end and jumps to Kazuradrop game over screen
void TrySetKazGameOverFromBattle(void)
{
    FlagSet(FLAG_KAZURADROP_ENTERED_BATTLE);
    gBattleOutcome |= B_OUTCOME_LOST;
}
