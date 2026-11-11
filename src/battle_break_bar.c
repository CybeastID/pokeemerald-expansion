#include "global.h"
#include "battle.h"
#include "battle_break_bar.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "graphics.h"
#include "palette.h"
#include "sprite.h"
#include "task.h"
#include "constants/rgb.h"

#define BREAK_BAR_FADE_FRAMES 16
#define hMain_HealthBarSpriteId data[5]
#define hMain_Battler data[6]

static const u16 sBreakBarBasePalette[] =
    INCBIN_U16("graphics/battle_interface/hpbar_purple.gbapal");
STATIC_ASSERT(ARRAY_COUNT(sBreakBarBasePalette) == 16, BreakBarPaletteSize);

// FGO gauge-marker highlight/body colors, quantized to GBA RGB.
// Only the two green HP-fill entries are replaced; other healthbar colors remain.
static const u16 sBreakBarFillColors[][2] =
{
    [BREAK_BAR_PURPLE] = {RGB(22,  5, 26), RGB(11,  3, 20)},
    [BREAK_BAR_BLUE]   = {RGB( 5, 17, 27), RGB( 3,  9, 27)},
    [BREAK_BAR_GREEN]  = {RGB( 4, 25, 22), RGB( 1, 13, 11)},
    [BREAK_BAR_YELLOW] = {RGB(27, 23,  4), RGB(17, 15,  2)},
    [BREAK_BAR_ORANGE] = {RGB(25, 14, 17), RGB(27, 12, 11)},
};
STATIC_ASSERT(ARRAY_COUNT(sBreakBarFillColors) == BREAK_BAR_MAX_EXTRA_BARS + 1, BreakBarColors);

// Separate slots allow differently colored stacks to coexist in doubles.
static const u16 sBreakBarPaletteTags[MAX_BATTLERS_COUNT] =
{
    TAG_BREAK_BAR_PLAYER_LEFT_PAL,
    TAG_BREAK_BAR_OPPONENT_LEFT_PAL,
    TAG_BREAK_BAR_PLAYER_RIGHT_PAL,
    TAG_BREAK_BAR_OPPONENT_RIGHT_PAL,
};

static u8 sLoadedBreakBarColors[MAX_BATTLERS_COUNT];
static void Task_BreakBarPaletteFade(u8 taskId);
static bool8 sKazuraStackBreakerActive[MAX_BATTLERS_COUNT];
static u8 sKazuraStackBreakerPixels[MAX_BATTLERS_COUNT];

static const struct BreakBarConfig sSavedBreakBarConfig =
{
    .extraBars = 1,
    .flags = BREAK_BAR_DEFER_INITIAL_DRAW,
};

static struct PartyState *GetBreakBarOwner(u32 battler)
{
    if (gBattleStruct == NULL || battler >= gBattlersCount
     || gBattlerPartyIndexes[battler] >= PARTY_SIZE)
        return NULL;
    return GetBattlerPartyState(battler);
}

u32 GetBreakBarCount(u32 battler)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    if (owner == NULL || owner->breakBarConfig == NULL)
        return 0;
    return owner->breakBarsRemaining;
}

void InitializeWildBreakBar(u32 partySlot, const struct BreakBarConfig *config)
{
    if (gBattleStruct == NULL || partySlot >= PARTY_SIZE || config == NULL
     || config->extraBars == 0 || config->extraBars > BREAK_BAR_MAX_EXTRA_BARS)
        return;
    struct PartyState *state = &gBattleStruct->partyState[B_SIDE_OPPONENT][partySlot];
    state->breakBarConfig = config;
    state->breakBarsRemaining = config->extraBars;
}

bool32 IsBreakBarCaptureBlocked(u32 battler)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    return owner != NULL && (GetBreakBarCount(battler) != 0 || owner->breakBarPending);
}

bool32 IsBreakBarBase(u32 battler)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    return owner != NULL && owner->breakBarConfig != NULL
        && owner->breakBarsRemaining == 0;
}

bool32 CanInterceptBreakBar(u32 battler)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    return owner != NULL && owner->breakBarConfig != NULL && owner->breakBarsRemaining != 0
        && !(gAbsentBattlerFlags & (1u << battler))
        && !((owner->breakBarConfig->flags & BREAK_BAR_IGNORE_WHILE_TRANSFORMED)
          && gBattleMons[battler].volatiles.transformed);
}

bool32 QueueBreakBar(u32 battler)
{
    if (!CanInterceptBreakBar(battler) || gBattleMons[battler].hp != 0)
        return FALSE;
    GetBattlerPartyState(battler)->breakBarPending = TRUE;
    return TRUE;
}

s32 ClampBreakBarPassiveDamage(u32 battler, s32 damage)
{
    if (IsBreakBarPending(battler))
        return 0; // An empty layer cannot be damaged or healed before its transition.
    if (damage <= 0 || !CanInterceptBreakBar(battler))
        return damage;
    return min(damage, max(0, (s32)gBattleMons[battler].hp - 1));
}

bool32 GrantSavedBreakBar(u32 battler)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    if (owner == NULL || !IsBattlerAlive(battler) || IsBreakBarPending(battler)
     || owner->breakBarsRemaining >= BREAK_BAR_MAX_EXTRA_BARS)
        return FALSE;
    u32 layer = owner->breakBarsRemaining;
    if (layer == 0)
        owner->breakBarSavedBaseWasRegistered = owner->breakBarConfig != NULL;
    owner->breakBarSavedHp[layer] = ReductionCanonicalHp(battler);
    owner->breakBarSavedMaxHp[layer] = ReductionCanonicalMaxHp(battler);
    owner->breakBarFillPixels = 0;
    if (owner->breakBarConfig == NULL)
        owner->breakBarConfig = &sSavedBreakBarConfig;
    owner->breakBarsRemaining++;
    owner->breakBarsPeak = max(owner->breakBarsPeak, owner->breakBarsRemaining);
    return TRUE;
}

static u32 GetHealthbarSprite(u32 battler)
{
    if (battler >= gBattlersCount)
        return MAX_SPRITES;
    u32 healthbox = gHealthboxSpriteIds[battler];
    if (healthbox >= MAX_SPRITES || !gSprites[healthbox].inUse
     || gSprites[healthbox].hMain_Battler != battler)
        return MAX_SPRITES;
    u32 healthbar = gSprites[healthbox].hMain_HealthBarSpriteId;
    if (healthbar >= MAX_SPRITES || !gSprites[healthbar].inUse)
        return MAX_SPRITES;
    return healthbar;
}

static void StopBreakBarFade(u32 battler)
{
    for (u32 taskId = 0; taskId < NUM_TASKS; taskId++)
    {
        if (gTasks[taskId].isActive && gTasks[taskId].func == Task_BreakBarPaletteFade
         && gTasks[taskId].data[0] == battler)
            DestroyTask(taskId);
    }
}

static void BuildBreakBarPalette(u32 color, u16 *palette)
{
    memcpy(palette, sBreakBarBasePalette, sizeof(sBreakBarBasePalette));
    palette[10] = sBreakBarFillColors[color][0];
    palette[11] = sBreakBarFillColors[color][1];
}

void RefreshBreakBarPalette(u32 battler, bool32 animate)
{
    if (battler >= gBattlersCount)
        return;
    u32 color = GetBreakBarCount(battler);
    // Ordinary battlers need no palette lookup or task scan on each HP-bar frame.
    if (color == 0 && sLoadedBreakBarColors[battler] == 0)
        return;
    u32 healthbar = GetHealthbarSprite(battler);
    if (healthbar == MAX_SPRITES)
        return;

    u16 tag = sBreakBarPaletteTags[battler];
    u32 slot = IndexOfSpritePaletteTag(tag);
    if (color == 0)
    {
        StopBreakBarFade(battler);
        u32 normalSlot = IndexOfSpritePaletteTag(TAG_HEALTHBAR_PAL);
        if (normalSlot != 0xFF)
            gSprites[healthbar].oam.paletteNum = normalSlot;
        FreeSpritePaletteByTag(tag);
        sLoadedBreakBarColors[battler] = 0;
        return;
    }

    u16 palette[16];
    if (slot == 0xFF || sLoadedBreakBarColors[battler] != color)
    {
        StopBreakBarFade(battler);
        BuildBreakBarPalette(color, palette);
        if (slot == 0xFF)
        {
            const struct SpritePalette spritePalette = {palette, tag};
            slot = LoadSpritePalette(&spritePalette);
            if (slot == 0xFF)
                return;
        }
        else
            LoadPalette(palette, OBJ_PLTT_ID(slot), PLTT_SIZE_4BPP);
        sLoadedBreakBarColors[battler] = color;

        // CreateTask returns 0 on exhaustion, so check capacity before using its ID.
        if (animate && GetTaskCount() < NUM_TASKS)
        {
            LoadPalette(gBattleInterface_BallDisplayPal, OBJ_PLTT_ID(slot), PLTT_SIZE_4BPP);
            u32 taskId = CreateTask(Task_BreakBarPaletteFade, 0);
            gTasks[taskId].data[0] = battler;
            gTasks[taskId].data[1] = color;
        }
    }
    gSprites[healthbar].oam.paletteNum = slot;
}

static void Task_BreakBarPaletteFade(u8 taskId)
{
    u32 battler = gTasks[taskId].data[0];
    u32 color = gTasks[taskId].data[1];
    u32 frame = gTasks[taskId].data[2];
    if (battler >= gBattlersCount || GetBreakBarCount(battler) != color
     || GetHealthbarSprite(battler) == MAX_SPRITES)
    {
        DestroyTask(taskId);
        return;
    }
    u32 slot = IndexOfSpritePaletteTag(sBreakBarPaletteTags[battler]);
    if (slot == 0xFF)
    {
        DestroyTask(taskId);
        return;
    }

    u16 palette[16];
    BuildBreakBarPalette(color, palette);
    for (u32 i = 0; i < ARRAY_COUNT(palette); i++)
    {
        u32 from = gBattleInterface_BallDisplayPal[i];
        u32 to = palette[i];
        s32 r = from & 31, g = (from >> 5) & 31, b = (from >> 10) & 31;
        r += (((s32)(to & 31) - r) * (s32)frame) / BREAK_BAR_FADE_FRAMES;
        g += (((s32)((to >> 5) & 31) - g) * (s32)frame) / BREAK_BAR_FADE_FRAMES;
        b += (((s32)((to >> 10) & 31) - b) * (s32)frame) / BREAK_BAR_FADE_FRAMES;
        palette[i] = RGB(r, g, b);
    }
    LoadPalette(palette, OBJ_PLTT_ID(slot), PLTT_SIZE_4BPP);
    if (frame == BREAK_BAR_FADE_FRAMES)
        DestroyTask(taskId);
    else
        gTasks[taskId].data[2]++;
}

bool32 ActivateBreakBar(u32 battler, const struct BreakBarConfig *config)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    if (owner == NULL || config == NULL || !IsBattlerAlive(battler)
     || config->extraBars == 0 || config->extraBars > BREAK_BAR_MAX_EXTRA_BARS
     || GetBreakBarCount(battler) != 0)
        return FALSE;

    owner->breakBarConfig = config;
    owner->breakBarsRemaining = config->extraBars;
    owner->breakBarsPeak = max(owner->breakBarsPeak, owner->breakBarsRemaining);
    if (!(config->flags & BREAK_BAR_DEFER_INITIAL_DRAW))
    {
        RefreshBreakBarPalette(battler, TRUE);
        if (GetHealthbarSprite(battler) != MAX_SPRITES)
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], GetBattlerMon(battler), HEALTHBOX_HEALTH_BAR);
    }
    return TRUE;
}

bool32 TryBreakBar(u32 battler, const u8 *returnInstr)
{
    struct PartyState *owner = GetBreakBarOwner(battler);
    if (owner == NULL || GetBreakBarCount(battler) == 0
     || gBattleMons[battler].hp != 0 || (gAbsentBattlerFlags & (1u << battler)))
        return FALSE;

    const struct BreakBarConfig *config = owner->breakBarConfig;
    if ((config->flags & BREAK_BAR_IGNORE_WHILE_TRANSFORMED)
     && gBattleMons[battler].volatiles.transformed)
        return FALSE;

    u32 phase = owner->breakBarsRemaining - 1;
    bool32 savedLayer = owner->breakBarSavedMaxHp[phase] != 0;
    const u8 *afterScript = savedLayer ? NULL : config->phases[phase].afterBreakScript;
    u32 scriptCount = 2 + (afterScript != NULL);
    // Preserve the original target while phase scripts operate on the bar owner.
    if (gBattleStruct->savedTargetCount >= ARRAY_COUNT(gBattleStruct->savedBattlerTarget)
     || gBattleResources->battleScriptsStack->size + scriptCount
        > ARRAY_COUNT(gBattleResources->battleScriptsStack->ptr))
        return FALSE;
    SaveBattlerTarget(gBattlerTarget);
    gBattlerTarget = battler;

    owner->breakBarPending = FALSE;
    owner->breakBarsRemaining--;
    RefreshBreakBarPalette(battler, FALSE);
    BattleScriptPush(returnInstr);
    BattleScriptPush(BattleScript_BreakBarReturn);
    if (afterScript != NULL)
        BattleScriptPush(afterScript);
    gBattlescriptCurrInstr = savedLayer ? BattleScript_RestoreSavedBreakBar : config->phases[phase].onBreakScript;
    if (gBattlescriptCurrInstr == NULL)
        gBattlescriptCurrInstr = BattleScript_BreakBarRefill;
    return TRUE;
}

void BS_RefillBreakBar(void)
{
    NATIVE_ARGS();
    SetHealAmount(gBattlerTarget, gBattleMons[gBattlerTarget].maxHP);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ResolveBreakBars(void)
{
    NATIVE_ARGS();
    if (gBattleControllerExecFlags)
        return;
    for (u32 battler = 0; battler < gBattlersCount; battler++)
        if (IsBreakBarPending(battler) && TryBreakBar(battler, gBattlescriptCurrInstr))
            return;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_StartKazuraStackBreaker(void)
{
    NATIVE_ARGS();

    u32 battler = gBattlerTarget;
    u32 pixels = B_HEALTHBAR_PIXELS;

    if (battler >= gBattlersCount)
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    // The first bar begins at its actual HP percentage.
    if (gBattleMons[battler].maxHP != 0)
    {
        pixels = (u32)gBattleMons[battler].hp
               * B_HEALTHBAR_PIXELS
               / gBattleMons[battler].maxHP;

        if (gBattleMons[battler].hp != 0 && pixels == 0)
            pixels = 1;
    }

    sKazuraStackBreakerPixels[battler] = pixels;
    sKazuraStackBreakerActive[battler] = TRUE;

    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_AnimateKazuraStackBreaker(void)
{
    NATIVE_ARGS();

    if (gBattleControllerExecFlags)
        return;

    u32 battler = gBattlerTarget;

    if (battler >= gBattlersCount)
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    struct PartyState *owner = GetBreakBarOwner(battler);

    if (!sKazuraStackBreakerActive[battler])
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    u32 pixels = sKazuraStackBreakerPixels[battler];
    bool32 hasExtraBars = owner != NULL
                       && owner->breakBarConfig != NULL
                       && owner->breakBarsRemaining != 0;

    // Colored layers use the overlay; the final red layer
    // uses the base bar. Neither operation changes battle HP.
    bool32 drew;

    if (hasExtraBars)
        drew = DrawHealthbarOverlay(battler, 0, 1, pixels);
    else
        drew = DrawHealthbarOverlay(battler, pixels,
                                   B_HEALTHBAR_PIXELS, 0);

    // Only animate numeric HP during the final red bar.
    if (!hasExtraBars && drew)
    {
        u32 displayedHp = (u32)gBattleMons[battler].hp
                        * pixels / B_HEALTHBAR_PIXELS;

        if (gBattleMons[battler].hp != 0 && pixels != 0 && displayedHp == 0)
            displayedHp = 1;

        UpdateHpTextInHealthbox(
            gHealthboxSpriteIds[battler],
            HP_CURRENT,
            displayedHp,
            gBattleMons[battler].maxHP);
    }

    // Do not hang the battle if the healthbox is unavailable.
    if (!drew)
    {
        if (owner != NULL)
        {
            owner->breakBarsRemaining = 0;
            owner->breakBarPending = FALSE;
            owner->breakBarFillPixels = 0;
        }

        sKazuraStackBreakerActive[battler] = FALSE;
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    // Drain three pixels per frame.
    if (pixels > 0)
    {
        sKazuraStackBreakerPixels[battler] =
            pixels > 3 ? pixels - 3 : 0;
        return;
    }

    // This layer has been completely destroyed.
    if (hasExtraBars)
    {
        owner->breakBarsRemaining--;
        owner->breakBarPending = FALSE;
        owner->breakBarFillPixels = 0;

        // Change to the next color without invoking phase scripts.
        RefreshBreakBarPalette(battler, FALSE);

        // The newly exposed layer appears immediately at 100%.
        sKazuraStackBreakerPixels[battler] = B_HEALTHBAR_PIXELS;

        return;
    }

    // The red base layer has now been destroyed too.
    sKazuraStackBreakerActive[battler] = FALSE;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ExhaustBreakBars(void)
{
    NATIVE_ARGS();

    if (gBattleControllerExecFlags)
        return;

    u32 battler = gBattlerTarget;
    struct PartyState *owner = GetBreakBarOwner(battler);

    if (owner != NULL)
    {
        owner->breakBarPending = FALSE;
        owner->breakBarsRemaining = 0;
        owner->breakBarFillPixels = 0;

        RefreshBreakBarPalette(battler, FALSE);
    }

    gBattlescriptCurrInstr = cmd->nextInstr;
}

bool32 TryResolveBreakBarsAfterMove(void)
{
    for (u32 battler = 0; battler < gBattlersCount; battler++)
    {
        if (IsBreakBarPending(battler))
        {
            BattleScriptExecute(BattleScript_ResolveBreakBarsEnd2);
            return TRUE;
        }
    }
    return FALSE;
}

void BS_FillSavedBreakBar(void)
{
    NATIVE_ARGS(u8 battler);
    if (gBattleControllerExecFlags)
        return;
    u32 battler = GetBattlerForBattleScript(cmd->battler);
    gBattleMons[battler].hp = gBattleMons[battler].maxHP;
    BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_HP_BATTLE,
        1u << gBattlerPartyIndexes[battler], sizeof(gBattleMons[battler].hp), &gBattleMons[battler].hp);
    MarkBattlerForControllerExec(battler);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_TryGrantSavedBreakBar(void)
{
    NATIVE_ARGS(u8 battler, const u8 *failInstr);
    u32 battler = GetBattlerForBattleScript(cmd->battler);
    gBattlescriptCurrInstr = GrantSavedBreakBar(battler) ? cmd->nextInstr : cmd->failInstr;
}

void BS_AnimateSavedBreakBar(void)
{
    NATIVE_ARGS(u8 battler);
    if (gBattleControllerExecFlags)
        return;

    u32 battler = GetBattlerForBattleScript(cmd->battler);
    struct PartyState *owner = GetBreakBarOwner(battler);

    if (owner == NULL || owner->breakBarsRemaining == 0)
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    u32 covered = owner->breakBarsRemaining - 1;
    u32 savedHp = owner->breakBarSavedHp[covered];
    u32 savedMaxHp = owner->breakBarSavedMaxHp[covered];

    RefreshBreakBarPalette(battler, FALSE);

    u32 slot = IndexOfSpritePaletteTag(sBreakBarPaletteTags[battler]);

    if (slot != 0xFF)
    {
        const u16 *coveredColors;
        if (covered != 0)
            coveredColors = sBreakBarFillColors[covered];
        else if (owner->breakBarSavedBaseWasRegistered || savedMaxHp == 0)
            coveredColors = &sBreakBarBasePalette[14];
        else
        {
            // The overlay draws the covered HP with the red fill tile (palette
            // entries 14/15). Give those entries the covered ordinary bar's
            // actual color until the new layer finishes filling.
            u32 pixels = min(savedHp * B_HEALTHBAR_PIXELS / savedMaxHp, B_HEALTHBAR_PIXELS);
            if (savedHp != 0 && pixels == 0)
                pixels = 1;
            u32 colorIndex = pixels > B_HEALTHBAR_PIXELS * 50 / 100 ? 10
                           : pixels > B_HEALTHBAR_PIXELS * 20 / 100 ? 12 : 14;
            coveredColors = &gBattleInterface_BallDisplayPal[colorIndex];
        }
        LoadPalette(coveredColors, OBJ_PLTT_ID(slot) + 14, 2 * sizeof(u16));
    }

    bool32 drew = DrawHealthbarOverlay(
        battler,
        savedHp,
        savedMaxHp,
        owner->breakBarFillPixels);

    // Animate the numeric HP alongside the graphical bar.
    if (drew && IsOnPlayerSide(battler))
    {
        u32 displayedHp =
            savedMaxHp * owner->breakBarFillPixels / B_HEALTHBAR_PIXELS;

        if (savedHp != 0
         && owner->breakBarFillPixels != 0
         && displayedHp == 0)
            displayedHp = 1;

        UpdateHpTextInHealthbox(
            gHealthboxSpriteIds[battler],
            HP_CURRENT,
            displayedHp,
            savedMaxHp);
    }

    if (!drew || owner->breakBarFillPixels == B_HEALTHBAR_PIXELS)
    {
        if (slot != 0xFF)
            LoadPalette(&sBreakBarBasePalette[14],
                OBJ_PLTT_ID(slot) + 14, 2 * sizeof(u16));

        if (drew)
            UpdateHealthboxAttribute(
                gHealthboxSpriteIds[battler],
                GetBattlerMon(battler),
                HEALTHBOX_HEALTH_BAR);

        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    owner->breakBarFillPixels++;
}

void BS_RestoreSavedBreakBar(void)
{
    NATIVE_ARGS();
    if (gBattleControllerExecFlags)
        return;
    u32 battler = gBattlerTarget;
    struct PartyState *owner = GetBattlerPartyState(battler);
    u32 layer = owner->breakBarsRemaining;
    ReductionBeginMutation(battler);
    gBattleMons[battler].hp = owner->breakBarSavedHp[layer];
    gBattleMons[battler].maxHP = owner->breakBarSavedMaxHp[layer];
    owner->breakBarSavedHp[layer] = owner->breakBarSavedMaxHp[layer] = 0;
    owner->breakBarFillPixels = 0;
    ReductionEndMutation(battler);
    BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_MAX_HP_BATTLE,
        1u << gBattlerPartyIndexes[battler], sizeof(gBattleMons[battler].maxHP), &gBattleMons[battler].maxHP);
    // BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_HP_BATTLE,
    //    1u << gBattlerPartyIndexes[battler], sizeof(gBattleMons[battler].hp), &gBattleMons[battler].hp);
    MarkBattlerForControllerExec(battler);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_SyncSavedBreakBarHp(void)
{
    NATIVE_ARGS(u8 battler);
    if (gBattleControllerExecFlags)
        return;
    u32 battler = GetBattlerForBattleScript(cmd->battler);
    BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_HP_BATTLE,
        1u << gBattlerPartyIndexes[battler], sizeof(gBattleMons[battler].hp), &gBattleMons[battler].hp);
    MarkBattlerForControllerExec(battler);
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_RevealSavedBreakBar(void)
{
    NATIVE_ARGS(u8 battler);
    if (gBattleControllerExecFlags)
        return;
    u32 battler = GetBattlerForBattleScript(cmd->battler);
    if (GetHealthbarSprite(battler) != MAX_SPRITES)
    {
    UpdateHealthboxAttribute(
        gHealthboxSpriteIds[battler],
        GetBattlerMon(battler),
        HEALTHBOX_HEALTH_BAR);

    if (IsOnPlayerSide(battler))
        UpdateHpTextInHealthbox(
            gHealthboxSpriteIds[battler],
            HP_BOTH,
            gBattleMons[battler].hp,
            gBattleMons[battler].maxHP);
    }
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void ReshowBreakBars(void)
{
    for (u32 battler = 0; battler < gBattlersCount; battler++)
    {
        StopBreakBarFade(battler);
        if (GetBreakBarCount(battler) != 0)
            sLoadedBreakBarColors[battler] = 0;
        RefreshBreakBarPalette(battler, FALSE);
        if (GetHealthbarSprite(battler) != MAX_SPRITES)
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], GetBattlerMon(battler), HEALTHBOX_HEALTH_BAR);
    }
}

void FreeBreakBarGraphics(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
    {
        StopBreakBarFade(battler);
        FreeSpritePaletteByTag(sBreakBarPaletteTags[battler]);
        sLoadedBreakBarColors[battler] = 0;
    }
}
