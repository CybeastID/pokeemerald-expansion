// Kazuradrop ball-less entrance
//
// Replaces the Poké Ball send-out animation for SPECIES_KAZURADROP.
// The engine's send-out contract (see StartSendOutAnim / BtlController_HandleSwitchInTryShinyAnim):
//   - gDoingBattleAnim = TRUE and healthBoxesData[battler].ballAnimActive = TRUE while running
//   - the mon sprite must end visible in BATTLER_AFFINE_NORMAL
//     (each style reaches this a different way)
//   - gBattleControllerData[battler] (wait-sprite) must end at SpriteCallbackDummy
//   - both flags cleared when finished; last battler done frees ball gfx

#include "global.h"
#include "battle.h"                       // gDoingBattleAnim, gBattleSpritesDataPtr, gBattlerSpriteIds
#include "battle_controllers.h"           // gBattleControllerData
#include "data.h"                         // BATTLER_AFFINE_EMERGE / BATTLER_AFFINE_NORMAL
#include "kazuradrop_entrance.h"
#include "pokeball.h"                     // POKEBALL_COUNT, FreeBallGfx
#include "sound.h"                        // PlayCry_Normal
#include "sprite.h"
#include "task.h"
#include "constants/battle.h"             // MAX_BATTLERS_COUNT
#include "constants/species.h"            // SPECIES_KAZURADROP
#include "palette.h"                      // OBJ_PLTT_ID
#include "constants/songs.h"              // SE_BALL_TRAY_ENTER
#include "constants/rgb.h"                // RGB_BLACK
#include "util.h"                        // BlendPalette
#include "gpu_regs.h"   // SetGpuReg, GetGpuReg


// =====================================================================
// Style select — flip this, rebuild, compare in-game
// =====================================================================
#define KAZU_APPEAR_EMERGE  0   // scale-in where she stands (current, tested)
#define KAZU_APPEAR_FADE    1   // fades in from black where she stands
#define KAZU_APPEAR_SLIDE   2   // slides in from the right edge (opponent side)

#define KAZU_APPEAR_STYLE   KAZU_APPEAR_FADE

// =====================================================================
// Tunables
// =====================================================================
#define KAZU_APPEAR_DELAY   15  // frames hidden before she materializes
#define KAZU_APPEAR_HOLD    20  // frames held after the scale-in finishes
// Phase 1 (reveal) — only the line matching KAZU_APPEAR_STYLE matters:
#define KAZU_EMERGE_MIN_FRAMES  0   // emerge waits until affine ends AND at least this many frames
                                    // (0 = today's behavior; raise to stretch the reveal)
#define KAZU_FADE_ALPHA_FRAMES  64  // stage 1: transparent -> black silhouette
#define KAZU_FADE_MID_HOLD  24  // black-silhouette beat between the two stages
#define KAZU_FADE_COLOR_FRAMES  16  // stage 2: black silhouette -> full color
#define KAZU_SLIDE_STEP         5   // slide speed in px/frame
                                    // (~64 px / step from the right edge: 5 -> ~13 frames)
// =====================================================================
// Policy
// =====================================================================
bool8 UseCustomSendOut(u16 species)
{
    // return FALSE; // disabled for now, but can be re-enabled if desired
    return species == SPECIES_CASTORIA || species == SPECIES_KAZURADROP;
}

// =====================================================================
// Entrance
// =====================================================================

static void Task_KazuraAppear(u8 taskid);

u8 DoKazuradropAppearAnimation(u32 battler)
{
    gDoingBattleAnim = TRUE;
    gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive = TRUE;

    u8 taskId = CreateTask(Task_KazuraAppear, 5);
    gTasks[taskId].data[0] = battler;
    gTasks[taskId].data[1] = 0; // timer
    gTasks[taskId].data[2] = 0; // phase
    return 0;
}

static void Task_KazuraAppearFinish(u8 taskId, u32 battler)
{
    u8 i, doneBattlers = 0;
    
    // Release the send-out wait-sprite (SpriteCB_WaitForBattlerBallReleaseAnim)
    gSprites[gBattleControllerData[battler]].callback = SpriteCallbackDummy;
    gDoingBattleAnim = FALSE;
    gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive = FALSE;

    // Mirror HandleBallAnimEnd: last battler done frees ball gfx
    // (needed because the player's send-out DID load ball gfx)
    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (gBattleSpritesDataPtr->healthBoxesData[i].ballAnimActive == FALSE)
            doneBattlers++;
    }
    if (doneBattlers == MAX_BATTLERS_COUNT)
    {
        for (i = 0; i < POKEBALL_COUNT; i++)
            FreeBallGfx(i);
    }
    DestroyTask(taskId);
}

static void Task_KazuraAppear(u8 taskId)
{
    u32 battler = gTasks[taskId].data[0];
    struct Sprite *mon = &gSprites[gBattlerSpriteIds[battler]];

    gTasks[taskId ].data[1]++; // timer

    switch (gTasks[taskId].data[2]) // phase
    {
        case 0: // wait while "ALICE sent out Kazura!" prints
            if (gTasks[taskId].data[1] >= KAZU_APPEAR_DELAY)
            {
                mon->invisible = FALSE;
                StartSpriteAffineAnim(mon, BATTLER_AFFINE_EMERGE);
                AnimateSprite(mon);
                PlayCry_Normal(GetBattlerVisualSpecies(battler), 0); // vanilla ball path passes pan = 0
    #if KAZU_APPEAR_STYLE == KAZU_APPEAR_EMERGE
                StartSpriteAffineAnim(mon, BATTLER_AFFINE_EMERGE);
    #elif KAZU_APPEAR_STYLE == KAZU_APPEAR_FADE
                // True transparency: semi-transparent OBJ blended against the backdrop.
                // EVA 0 -> 16 across the fade; backdrop is the blend partner.
                // Stage 1 setup: palette fully black, sprite fully transparent.
                StartSpriteAffineAnim(mon, BATTLER_AFFINE_NORMAL);
                BlendPalette(OBJ_PLTT_ID(mon->oam.paletteNum), 16, 16, RGB_BLACK);
                mon->oam.objMode = ST_OAM_OBJ_BLEND;
                gTasks[taskId].data[4] = GetGpuReg(REG_OFFSET_BLDCNT);   // saved, restored below
                gTasks[taskId].data[5] = GetGpuReg(REG_OFFSET_BLDALPHA);
                SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
                SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));    // transparent
    #elif KAZU_APPEAR_STYLE == KAZU_APPEAR_SLIDE
                // Hook is opponent-side only, so she enters from the screen's right edge.
                StartSpriteAffineAnim(mon, BATTLER_AFFINE_NORMAL);
                gTasks[taskId].data[3] = mon->x;             // target x (spawn position)
                mon->x = DISPLAY_WIDTH + 24;                  // offscreen right
                PlaySE(SE_BALL_TRAY_ENTER);                   // same cue as SpriteCB_PlayerMonSlideIn
    #endif
                    gTasks[taskId].data[1] = 0;
                    gTasks[taskId].data[2] = 1;
                }
                break;
        case 1: // wait for scale-in to finish
    #if KAZU_APPEAR_STYLE == KAZU_APPEAR_EMERGE
            if (mon->affineAnimEnded)                         // wait for scale-in to finish
            {
                StartSpriteAffineAnim(mon, BATTLER_AFFINE_NORMAL);
                gTasks[taskId].data[1] = 0;
                gTasks[taskId].data[2] = 2;
            }
    #elif KAZU_APPEAR_STYLE == KAZU_APPEAR_FADE
            // BlendPalette's coeff is >> 4 (range 0..16) regardless of the frame
            // count — scale the timer into it. Passing raw frames > 16 overshoots
            // past black into wrapped garbage colors.
    if (gTasks[taskId].data[1] > KAZU_FADE_ALPHA_FRAMES)
    {
        // Alpha done: ordinary OBJ again, backdrop blend registers restored.
        // Palette is still fully black — stage 2 takes it from there.
        mon->oam.objMode = ST_OAM_OBJ_NORMAL;
        SetGpuReg(REG_OFFSET_BLDCNT, gTasks[taskId].data[4]);
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].data[5]);
        gTasks[taskId].data[1] = 0;
        gTasks[taskId].data[2] = 4;
    }
    else
    {
        u8 eva = (u16)gTasks[taskId].data[1] * 16 / KAZU_FADE_ALPHA_FRAMES;
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(eva, 16 - eva));
    }
    #elif KAZU_APPEAR_STYLE == KAZU_APPEAR_SLIDE
            mon->x -= KAZU_SLIDE_STEP;
            if (mon->x <= gTasks[taskId].data[3])
            {
                mon->x = gTasks[taskId].data[3];              // snap, avoid overshoot
                gTasks[taskId].data[1] = 0;
                gTasks[taskId].data[2] = 2;
            }
    #endif
            break;
        case 2: // hold, then release everything the send-out chain is polling
             if (gTasks[taskId].data[1] >= KAZU_APPEAR_HOLD)
            Task_KazuraAppearFinish(taskId, battler);
        break;
        case 3: // FADE stage 2 only: black silhouette -> full color
    #if KAZU_APPEAR_STYLE == KAZU_APPEAR_FADE
        if (gTasks[taskId].data[1] > KAZU_FADE_COLOR_FRAMES)
        {
            gTasks[taskId].data[1] = 0;
            gTasks[taskId].data[2] = 2;
        }
        else
        {
            u8 coeff = 16 - ((u16)gTasks[taskId].data[1] * 16 / KAZU_FADE_COLOR_FRAMES);
            BlendPalette(OBJ_PLTT_ID(mon->oam.paletteNum), 16, coeff, RGB_BLACK);
        }
    #else
        gTasks[taskId].data[2] = 2;   // unreachable safety net
    #endif
        break;
        case 4: // FADE pause only: hold the black silhouette
    #if KAZU_APPEAR_STYLE == KAZU_APPEAR_FADE
        if (gTasks[taskId].data[1] >= KAZU_FADE_MID_HOLD)
        {
            gTasks[taskId].data[1] = 0;
            gTasks[taskId].data[2] = 3;
        }
    #else
        gTasks[taskId].data[2] = 2;   // unreachable safety net
    #endif
        break;
    }
}