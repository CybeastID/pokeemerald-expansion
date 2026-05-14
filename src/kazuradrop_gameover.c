// Kazuradrop Game Over Screen
// Triggered when the player loses while Bug Space is active.
// Displays a full-screen 8bpp artwork splash with a dark blend overlay,
// then fades in GAME OVER text as OBJ sprites before returning to the title screen.

#include "global.h"
#include "main.h"
#include "bg.h"
#include "gpu_regs.h"
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

// ---------------------------------------------------------------------------
// Asset INCBINs
// ---------------------------------------------------------------------------

static const u32 sKazGameOver_Tiles[] = INCBIN_U32("graphics/special/kazgameover.img.bin");
static const u16 sKazGameOver_Map[]   = INCBIN_U16("graphics/special/kazgameover.map.bin");
static const u16 sKazGameOver_Pal[]   = INCBIN_U16("graphics/special/kazgameover.pal.bin");

// GAME OVER text sprite assets (to be created separately)
// static const u32 sGameOverText_Tiles[] = INCBIN_U32("graphics/kazuradrop/gameover/gameover_text.4bpp");
// static const u16 sGameOverText_Pal[]   = INCBIN_U16("graphics/kazuradrop/gameover/gameover_text.gbapal");

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
#define BLEND_DURATION      20      // frames to animate the dark overlay
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

// ---------------------------------------------------------------------------
// Entry point — call this from the battle loss handler
// ---------------------------------------------------------------------------

bool8 TrySetKazGameOverWhiteOut(void)
{
    if (FlagGet(FLAG_KAZURADROP_ENTERED_BATTLE))
    {
        FlagClear(FLAG_KAZURADROP_ENTERED_BATTLE);
        SetMainCallback2(CB2_DoKazGameOverScreen);
        return TRUE;
    }
    else
    {
        SetMainCallback2(CB2_WhiteOut);
        return FALSE;
    }
}

static bool8 sKazInitStarted = FALSE;

void CB2_DoKazGameOverScreen(void)
{
    if (!sKazInitStarted)
    {
        gMain.state = 0;
        sKazInitStarted = TRUE;
    }
    if (!InitKazGameOverScreen())
    {
        CreateTask(Task_KazGameOver_FadeIn, 0);
    }
}

// ---------------------------------------------------------------------------
// Screen initialiser — runs as a state machine across frames
// Returns TRUE while still initialising, FALSE when done
// ---------------------------------------------------------------------------
static u16 sFadeDelay;
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
        // Initialise BGs
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sKazGameOver_BgTemplates, ARRAY_COUNT(sKazGameOver_BgTemplates));
        SetBgTilemapBuffer(2, sKazGameOver_Tilemap);
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        gMain.state++;
        break;

    /* case 2:
        // Load Kazuradrop artwork tiles and palette into VRAM/palette RAM
        // BG palette bank 0 = our 256-color palette for BG2
        LoadBgTiles(2, sKazGameOver_Tiles, sizeof(sKazGameOver_Tiles), 0);
    while (IsDma3ManagerBusyWithBgCopy());
    CopyToBgTilemapBuffer(2, sKazGameOver_Map, 0, 0);
    LoadPalette(sKazGameOver_Pal, BG_PLTT_ID(0), sizeof(u16) * 256);
    CopyBgTilemapBufferToVram(2);
    gMain.state++;
    break;

    */ 

    case 2:
    // Direct DMA to VRAM - bypass BG system
    CpuFastSet(sKazGameOver_Tiles, (void *)(BG_VRAM + 0x8000), sizeof(sKazGameOver_Tiles) / 4);
    DmaCopy16(3, sKazGameOver_Map, (void *)(BG_VRAM + 0xF800), sizeof(sKazGameOver_Map));
    LoadPalette(sKazGameOver_Pal, BG_PLTT_ID(0), sizeof(u16) * 256);
    gMain.state++;
    break;

    case 3:
        // Set up hardware blending:
        // BG2 blends against the backdrop (black) — starts fully unblended
        SetGpuReg(REG_OFFSET_BLDCNT,   BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BD);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(BLEND_EVA_START, BLEND_EVB_START));
        SetGpuReg(REG_OFFSET_BLDY,     0);

        // BG0 hidden initially (text layer) — shown after blend completes
        HideBg(0);
        ShowBg(2);
        sFadeDelay = 0;
        gMain.state++;
        break;
    case 4:
        sFadeDelay++;
        if (sFadeDelay >= 30)  // wait 30 frames before fade
            gMain.state++;
        break;
    case 5:
        // Fade in from black
        SetVBlankCallback(VBlankCB_KazGameOver);
        BeginNormalPaletteFade(PALETTES_ALL, 5, 16, 0, RGB_BLACK);
        gMain.state++;
        break;

    case 6:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            SetMainCallback2(CB2_KazGameOver);
            // Optionally stop music here and play a sting/silence
            // PlayBGM(MUS_KAZURADROP_GAME_OVER);
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

static void Task_KazGameOver_FadeInText(u8 taskId)
{
    // For now, simply show BG0 immediately
    // Once GAME OVER text tiles are ready, load them here and fade in
    // via a per-palette fade on BG_PLTT_ID(15) or similar
    ShowBg(0);

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
        // Clear the flag so a future run doesn't re-trigger this
        FlagClear(FLAG_KAZURADROP_ENTERED_BATTLE);

        // Return to title screen
        LoadGameSave(SAVE_NORMAL);
        SetMainCallback2(CB2_InitTitleScreen);
        DestroyTask(taskId);
    }
}
