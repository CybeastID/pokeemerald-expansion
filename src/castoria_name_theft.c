// Castoria Name-Theft Mechanic
//
// When Castoria is sent out, she presents the player with a two-choice
// battle-textbox prompt using their real name as the top option.
// The result determines three branches:
//
//   TRUE  (CASTORIA_NAME_TRUE)
//     - castoriaTrueNameCharges = 3  (enables True Name Command turns)
//     - Castoria gets +1 all stats
//     - Player's active battler gets NAMELESS volatile (-1 SpAtk)
//
//   FALSE (CASTORIA_NAME_FALSE)
//     - Castoria gets +1 Atk / SpAtk / Spe
//     - Player's active battler gets NAMELESS volatile
//
//   BACKED OUT (CASTORIA_NAME_BACKED_OUT)
//     - Castoria gets +1 SpAtk only
//

#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_message.h"
#include "battle_scripts.h"
#include "castoria_name_theft.h"
#include "battle_ai_util.h"
#include "constants/battle.h"
#include "constants/battle_string_ids.h"
#include "constants/characters.h"
#include "constants/global.h"
#include "constants/songs.h"
#include "save.h"
#include "sound.h"
#include "string_util.h"
#include "window.h"
#include "text.h"
#include "fonts.h"
#include "menu.h"
#include "pokemon.h"
#include "random.h"

#define CHOICE_STATE_INIT       0
#define CHOICE_STATE_WAIT       1

static u8 sChoiceCursor;
static u8 sChoiceState;

// Bottom choice text — displayed in the battle choice box
static const u8 sText_FrancisXavier[] = _("F. Xavier");

// ---------------------------------------------------------------
// NEW WINDOW
// ---------------------------------------------------------------

#define CASTORIA_WIN_BASE_BLOCK  0x02C0   // free screen-entry space between vanilla battle windows and Kazuradrop's box

static u8 sCastoriaChoiceWindowId;

static const struct WindowTemplate sCastoriaChoiceWindowTemplate = {
    .bg          = 0,
    .tilemapLeft = 18,
    .tilemapTop  = 9,
    .width       = 11,
    .height      = 4,
    .paletteNum  = 5,
    .baseBlock   = CASTORIA_WIN_BASE_BLOCK,
};

// ---------------------------------------------------------------
// callnative entry point — called from BattleScript
// ---------------------------------------------------------------

static void PrintCastoriaChoiceText(void)
{
    static const u8 colorNormal[3]   = {13, 14, 15};
    static const u8 colorSelected[3] = {14, 13, 15};

    FillWindowPixelBuffer(sCastoriaChoiceWindowId, PIXEL_FILL(0xE));

    StringCopy(gDisplayedStringBattle, gSaveBlock2Ptr->playerName);
    AddTextPrinterParameterized4(sCastoriaChoiceWindowId, FONT_NORMAL,
                                  4, 2, 0, 0,
                                  (sChoiceCursor == 0) ? colorSelected : colorNormal,
                                  TEXT_SKIP_DRAW, gDisplayedStringBattle);

    StringCopy(gDisplayedStringBattle, sText_FrancisXavier);
    AddTextPrinterParameterized4(sCastoriaChoiceWindowId, FONT_NORMAL,
                                  4, 18, 0, 0,
                                  (sChoiceCursor == 1) ? colorSelected : colorNormal,
                                  TEXT_SKIP_DRAW, gDisplayedStringBattle);

    CopyWindowToVram(sCastoriaChoiceWindowId, COPYWIN_FULL);
}

void BS_DoCastoriaNameChoice(void)
{
    NATIVE_ARGS();
    u32 battler = gBattlerAttacker;

    switch (sChoiceState)
    {
    case CHOICE_STATE_INIT:
    sChoiceCursor = 0;
    sChoiceState = CHOICE_STATE_WAIT;
    sCastoriaChoiceWindowId = 0xFF;

    HandleBattleWindow(17, 8, 29, 13, 0);
    sCastoriaChoiceWindowId = AddWindow(&sCastoriaChoiceWindowTemplate);
    PrintCastoriaChoiceText();
    PutWindowTilemap(sCastoriaChoiceWindowId);
    CopyWindowToVram(sCastoriaChoiceWindowId, COPYWIN_FULL);
    break;

    case CHOICE_STATE_WAIT:
        // D-pad navigation
        if (JOY_NEW(DPAD_UP) && sChoiceCursor != 0)
    {
        PlaySE(SE_SELECT);
        sChoiceCursor = 0;
        PrintCastoriaChoiceText();
    }
    if (JOY_NEW(DPAD_DOWN) && sChoiceCursor == 0)
    {
        PlaySE(SE_SELECT);
        sChoiceCursor = 1;
        PrintCastoriaChoiceText();
    }

        // A = confirm choice
        if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            HandleBattleWindow(17, 8, 29, 13, WINDOW_CLEAR);
            ClearWindowTilemap(sCastoriaChoiceWindowId);
            CopyWindowToVram(sCastoriaChoiceWindowId, COPYWIN_FULL);
            RemoveWindow(sCastoriaChoiceWindowId);
            sCastoriaChoiceWindowId = 0xFF;
            sChoiceState = CHOICE_STATE_INIT;

            if (sChoiceCursor == 0) // Top choice = player's real name = TRUE
            {
                gBattleStruct->castoria.nameResult = CASTORIA_NAME_TRUE;
                gBattleMons[battler].volatiles.castoriaTrueNameCharges = 3;
            }
            else // Bottom choice = F. Xavier = FALSE
            {
                gBattleStruct->castoria.nameResult = CASTORIA_NAME_FALSE;
            }
            gBattlescriptCurrInstr = BattleScript_CastoriaNameTheftResult;
        }

        // B = back out
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            HandleBattleWindow(17, 8, 29, 13, WINDOW_CLEAR);
            ClearWindowTilemap(sCastoriaChoiceWindowId);
            CopyWindowToVram(sCastoriaChoiceWindowId, COPYWIN_FULL);
            RemoveWindow(sCastoriaChoiceWindowId);
            sCastoriaChoiceWindowId = 0xFF;
            sChoiceState = CHOICE_STATE_INIT;
            gBattleStruct->castoria.nameResult = CASTORIA_NAME_BACKED_OUT;
            gBattlescriptCurrInstr = BattleScript_CastoriaNameTheftResult;
        }
        break;
    }
}

// ---------------------------------------------------------------
// ApplyCastoriaNameResult — stat boosts + volatiles per branch
// ---------------------------------------------------------------
void BS_ApplyCastoriaNameResult(void)
{
    NATIVE_ARGS();
    u32 battler = gBattlerAttacker;  // Castoria
    u32 result = gBattleStruct->castoria.nameResult;
    u32 target = GetBattlerAtPosition(BATTLE_OPPOSITE(GetBattlerPosition(battler)));

    switch (result)
    {
    case CASTORIA_NAME_TRUE:
        // +1 all stats for Castoria (clamped)
        if (gBattleMons[battler].statStages[STAT_ATK]   < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_ATK]++;
        if (gBattleMons[battler].statStages[STAT_DEF]   < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_DEF]++;
        if (gBattleMons[battler].statStages[STAT_SPATK] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPATK]++;
        if (gBattleMons[battler].statStages[STAT_SPDEF] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPDEF]++;
        if (gBattleMons[battler].statStages[STAT_SPEED] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPEED]++;
        // NAMELESS on active player mon
        if (IsBattlerAlive(target))
            gBattleMons[target].volatiles.castoriaNameless = TRUE;
        break;

    case CASTORIA_NAME_FALSE:
        if (gBattleMons[battler].statStages[STAT_ATK]   < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_ATK]++;
        if (gBattleMons[battler].statStages[STAT_SPATK] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPATK]++;
        if (gBattleMons[battler].statStages[STAT_SPEED] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPEED]++;
        if (IsBattlerAlive(target))
            gBattleMons[target].volatiles.castoriaNameless = TRUE;
        break;

    case CASTORIA_NAME_BACKED_OUT:
        if (gBattleMons[battler].statStages[STAT_SPATK] < MAX_STAT_STAGE) gBattleMons[battler].statStages[STAT_SPATK]++;
        break;
    }

    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_CastoriaPrepareWorstMove(void)
{
    NATIVE_ARGS();
    u32 battler, target = MAX_BATTLERS_COUNT;

    // Explicitly find Castoria and the player's active mon
    // (do not rely on gBattlerAttacker, which may point to the wrong battler
    //  depending on script context — e.g. after Dynamax activation)
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (gBattleMons[battler].species == SPECIES_CASTORIA
            && IsBattlerAlive(battler)
            && !IsOnPlayerSide(battler))
        {
            break;
        }
    }
    if (battler >= gBattlersCount)
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    // Find the player's active mon on the opposite side
    for (target = 0; target < gBattlersCount; target++)
    {
        if (IsBattlerAlive(target)
            && IsOnPlayerSide(target)
            && GetBattlerPosition(target) == BATTLE_OPPOSITE(GetBattlerPosition(battler)))
        {
            break;
        }
    }

    // Safety check: skip if target == battler (self-referencing)
    if (target >= gBattlersCount || target == battler)
    {
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;
    }

    u8 worstSlot = CastoriaFindWorstMove(battler, target);
    if (worstSlot != 0xFF)
    {
        u16 move = gBattleMons[target].moves[worstSlot];
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, move);
        gBattleStruct->castoria.useCommand &= ~CASTORIA_FORCED_MOVE_SLOT_MASK;
        gBattleStruct->castoria.useCommand |= (worstSlot << CASTORIA_FORCED_MOVE_SLOT_SHIFT);
        gBattleStruct->castoria.useCommand |= CASTORIA_COMMAND_THIS_TURN | CASTORIA_COMMAND_UI_LOCKED;
    }
    
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_CastoriaDecTrueNameCharges(void)
{
    NATIVE_ARGS();
    u32 battler = gBattlerAttacker;
    if (gBattleMons[battler].volatiles.castoriaTrueNameCharges > 0)
        gBattleMons[battler].volatiles.castoriaTrueNameCharges--;
    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_JumpCastoriaNameResult(void)
{
    NATIVE_ARGS();
    switch (gBattleStruct->castoria.nameResult)
    {
    case CASTORIA_NAME_TRUE:
        gBattlescriptCurrInstr = BattleScript_CastoriaTrueName;
        break;
    case CASTORIA_NAME_FALSE:
        gBattlescriptCurrInstr = BattleScript_CastoriaFalseName;
        break;
    case CASTORIA_NAME_BACKED_OUT:
    default:
        gBattlescriptCurrInstr = BattleScript_CastoriaBackedOut;
        break;
    }
}



// ---------------------------------------------------------------
// CastoriaDecideUseCommand — AI decides whether to spend a charge
// ---------------------------------------------------------------
bool32 CastoriaDecideUseCommand(u32 battler)
{
    // Already decided this turn — script already invoked
    if (gBattleStruct->castoria.useCommand & CASTORIA_COMMAND_THIS_TURN)
        return FALSE;

    // Must have charges remaining
    if (!gBattleMons[battler].volatiles.castoriaTrueNameCharges)
        return FALSE;

    // Cooldown: skip if used last turn, unless HP < 33%
    if ((gBattleStruct->castoria.useCommand & CASTORIA_COMMAND_LAST_TURN)
        && gBattleMons[battler].hp > gBattleMons[battler].maxHP / 3)
        return FALSE;

    // Find the player's active mon opposite Castoria
    u32 target = GetBattlerAtPosition(BATTLE_OPPOSITE(GetBattlerPosition(battler)));
    if (!IsBattlerAlive(target))
        return FALSE;

    // If no bad move exists, don't waste a charge
    if (CastoriaFindWorstMove(battler, target) == 0xFF)
        return FALSE;

    // Basic threat check — target has enough HP to be worth controlling
    if (gBattleMons[target].hp < gBattleMons[target].maxHP / 4)
        return FALSE;

    // All conditions met — caller should invoke the Command Seal script
    return TRUE;
}

// ---------------------------------------------------------------
// CastoriaFindWorstMove — scores target's moves from Castoria's POV
// ---------------------------------------------------------------
u8 CastoriaFindWorstMove(u32 castoriaBattler, u32 targetBattler)
{
    // Pass 1: self-termination moves are the absolute worst (remove the user's own mon)
    for (u8 i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gBattleMons[targetBattler].moves[i];
        switch (move)
        {
        case MOVE_EXPLOSION:
        case MOVE_SELF_DESTRUCT:
        case MOVE_FINAL_GAMBIT:
        case MOVE_MEMENTO:
        case MOVE_HEALING_WISH:
        case MOVE_LUNAR_DANCE:
        case MOVE_MISTY_EXPLOSION:
            return i;
        }
    }

    // Pass 2: score by damage from Castoria's perspective (lower = worse for the player)
    // Hierarchy: type-immune damaging (-1) < status (0) < normal damage (HP value)
    u8 worstSlot = 0xFF;
    u32 lowestDamage = UINT32_MAX;

    for (u8 i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gBattleMons[targetBattler].moves[i];
        if (move == MOVE_NONE)
            continue;

        u32 damage = AI_GetDamage(targetBattler, castoriaBattler, i, AI_ATTACKING, gAiLogicData);

        // Type-immune damaging moves: damage is 0 but the move is NOT status.
        // To Castoria, this is worse than status (which might still have utility).
        if (!IsBattleMoveStatus(move) && damage == 0)
            damage = (u32)-1;  // treat as -1, forced minimum

        if (worstSlot == 0xFF || damage < lowestDamage)
        {
            lowestDamage = damage;
            worstSlot = i;
        }
    }

    return worstSlot;
}

// ---------------------------------------------------------------
// Arcanum — extra magical move slots for Castoria
// ---------------------------------------------------------------

void CastoriaInitArcanum(u32 battler)
{
    // Illusion, Evocation, Transformation themed starter set
    static const u16 sArcanumPool[] = {
        MOVE_MOONBLAST,       // Evocation
        MOVE_LIGHT_SCREEN,    // Illusion
        MOVE_DAZZLING_GLEAM,  // Evocation
        // MOVE_POLYMORPH,    // Transformation
        MOVE_FIRE_BLAST,        // Evocation
        MOVE_SPLASH,          // Do nothing to fill space
    };

    for (u8 i = 0; i < CASTORIA_ARCANUM_COUNT; i++)
        gBattleStruct->castoria.arcanumMoves[i] = sArcanumPool[i];
}

u16 CastoriaGetMoveAtSlot(u32 battler, u8 slot)
{
    if (slot < MAX_MON_MOVES)
        return gBattleMons[battler].moves[slot];

    if (gBattleMons[battler].species == SPECIES_CASTORIA)
    {
        u8 arcanumSlot = slot - MAX_MON_MOVES;
        if (arcanumSlot < CASTORIA_ARCANUM_COUNT)
            return gBattleStruct->castoria.arcanumMoves[arcanumSlot];
    }

    return MOVE_NONE;
}

bool32 TryCastoriaNameTheft(u32 battler, bool32 viaCall)
{
    if (gBattleMons[battler].species != SPECIES_CASTORIA
        || gBattleStruct->castoria.nameResult != CASTORIA_NAME_NONE
        || IsOnPlayerSide(battler))
    {
        return FALSE;
    }

    gBattlerAttacker = battler;
    CastoriaInitArcanum(battler);

    // Switch-in runs inside a script: push the cursor and return to it.
    // First-turn events have no running script: start a fresh one.
    if (viaCall)
        BattleScriptCall(BattleScript_CastoriaNameTheft);
    else
        BattleScriptExecute(BattleScript_CastoriaNameTheftExecute);
    return TRUE;
}

static bool32 ShouldRejectPolymorphSpecies(u32 species)
{
    if (species == SPECIES_NONE
     || !IsSpeciesEnabled(species)
     || species == SPECIES_SHEDINJA
     || species == gBattleMons[gBattlerTarget].species
     || species > SPECIES_FALINKS_MEGA)   // custom/boss species (Castoria, Raze Kirlia, Kazuradrop, EGG)
        return TRUE;

    return gSpeciesInfo[species].isMegaEvolution
        || gSpeciesInfo[species].isPrimalReversion
        || gSpeciesInfo[species].isUltraBurst
        || gSpeciesInfo[species].isGigantamax
        || gSpeciesInfo[species].isTeraForm
        || gSpeciesInfo[species].isLegendary
        || gSpeciesInfo[species].isMythical
        || gSpeciesInfo[species].isParadox
        || gSpeciesInfo[species].isUltraBeast;
}

static u32 ScaleHpToNewMax(u32 hp, u32 oldMax, u32 newMax)
{
    if (hp == 0 || oldMax == 0)
        return hp; // fainted stays fainted; avoid divide-by-zero
    hp = (hp * newMax) / oldMax;
    return hp == 0 ? 1 : hp;
}

// Saves original moves/PP during polymorph so TMs aren't lost on revert
static u16 sPolymorphedSavedMoves[PARTY_SIZE][MAX_MON_MOVES];
static u8 sPolymorphedSavedPP[PARTY_SIZE][MAX_MON_MOVES];

void BS_PolymorphTarget(void)
{
    NATIVE_ARGS();
    u32 target = gBattlerTarget;
    u32 monId = gBattlerPartyIndexes[target];
    struct Pokemon *party = GetBattlerParty(target);
    struct Pokemon *mon = &party[monId];
    u32 oldSpecies = gBattleMons[target].species;
    u32 oldHp = GetMonData(mon, MON_DATA_HP);
    u32 oldMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
    u32 newSpecies;
    u32 newHp;

    gBattlescriptCurrInstr = cmd->nextInstr;

    if (gBattleMons[target].volatiles.transformed
     || gBattleStruct->illusion[target].state == ILLUSION_ON
     || IsSemiInvulnerable(target, EXCLUDE_COMMANDER)
     || IsBattlerMegaEvolved(target)
     || IsBattlerUltraBursted(target)
     || IsBattlerInTeraForm(target)
     || GetActiveGimmick(target) == GIMMICK_DYNAMAX)
    {
        gBattleStruct->moveResultFlags[target] |= MOVE_RESULT_FAILED;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TRANSFORM_FAILED;
        return;
    }

    newSpecies = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);

    if (GetBattlerPartyState(target)->changedSpecies == SPECIES_NONE)
        GetBattlerPartyState(target)->changedSpecies = oldSpecies;
    GetBattlerPartyState(target)->polymorphed = TRUE;

    SetMonData(mon, MON_DATA_SPECIES, &newSpecies);
    gBattleMons[target].species = newSpecies;
    RecalcBattlerStats(target, mon, FALSE);

    newHp = ScaleHpToNewMax(oldHp, oldMaxHp, GetMonData(mon, MON_DATA_MAX_HP));
    SetMonData(mon, MON_DATA_HP, &newHp);
    gBattleMons[target].hp = newHp;

      // Save original moves before replacing with new species' moveset
    u32 partyIndex = gBattlerPartyIndexes[target];
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        sPolymorphedSavedMoves[partyIndex][i] = GetMonData(mon, MON_DATA_MOVE1 + i);
        sPolymorphedSavedPP[partyIndex][i] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
    }

     // Give the target the new species' moveset (not its old moves)
    GiveMonInitialMoveset(mon);
     for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        gBattleMons[target].moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);;
        gBattleMons[target].pp[i] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
    }

    PREPARE_SPECIES_BUFFER(gBattleTextBuff1, newSpecies)

    PREPARE_SPECIES_BUFFER(gBattleTextBuff1, newSpecies)

    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TRANSFORMED;
}

bool32 TryRestorePolymorphedMon(u32 partyIndex)
{
    struct PartyState *state = &gBattleStruct->partyState[B_SIDE_PLAYER][partyIndex];
    struct Pokemon *mon = &gPlayerParty[partyIndex];
    u32 original, formHp, formMaxHp, newHp;

    if (!state->polymorphed)
        return FALSE;

    formHp = GetMonData(mon, MON_DATA_HP);
    formMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
    original = state->changedSpecies;

    state->polymorphed = FALSE;
    state->changedSpecies = SPECIES_NONE;
    if (original == SPECIES_NONE)
        return TRUE;

    SetMonData(mon, MON_DATA_SPECIES, &original);

    // Restore original moves/PP so TMs aren't permanently lost
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &sPolymorphedSavedMoves[partyIndex][i]);
        SetMonData(mon, MON_DATA_PP1 + i, &sPolymorphedSavedPP[partyIndex][i]);
    }

    CalculateMonStats(mon);
    newHp = ScaleHpToNewMax(formHp, formMaxHp, GetMonData(mon, MON_DATA_MAX_HP));
    SetMonData(mon, MON_DATA_HP, &newHp);
    return TRUE;
}
