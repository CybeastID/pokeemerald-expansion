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
//     - Castoria gains permanent 30% Mage Armor resistance
//

#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_message.h"
#include "battle_scripts.h"
#include "castoria_name_theft.h"
#include "castoria_battle_ui.h"
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
#include "battle_controllers.h"
#include "battle_break_bar.h"
#include "battle_interface.h"
#include "daycare.h"
#include "item.h"

#define CHOICE_STATE_INIT       0
#define CHOICE_STATE_WAIT       1

static u8 sChoiceCursor;
static u8 sChoiceState;

// Bottom choice text — displayed in the battle choice box
static const u8 sText_FrancisXavier[] = _("F. Xavier");

// ---------------------------------------------------------------
// FUNCTION PROTOTYPES
u32 EvaluateMoveThreat(u16 move, u16 species);
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
#if TESTING
gBattleStruct->castoria.nameResult = CASTORIA_NAME_BACKED_OUT;
gBattlescriptCurrInstr = BattleScript_CastoriaNameTheftResult;
return;
#endif
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
        gBattleStruct->castoria.mageArmorPartyMask |= 1u << gBattlerPartyIndexes[battler];
        break;
    }

    gBattlescriptCurrInstr = cmd->nextInstr;
}

void BS_ShowMageArmorIcon(void)
{
    NATIVE_ARGS();
    CreateMageArmorIcon(gBattlerAttacker);
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

// Illusion, Evocation, Transformation themed starter set.
static const u16 sArcanumPool[4] = {
    MOVE_FIREBALL,      // Evocation
    MOVE_REVIVAL_BLESSING_2, // Revivify
    MOVE_ARCANUM_WISH,  // Once per battle; outcome selected by Special AI.
    MOVE_POLYMORPH,     // Transformation
};
STATIC_ASSERT(ARRAY_COUNT(sArcanumPool) == CASTORIA_ARCANUM_COUNT, ArcanumPoolSize);

void CastoriaInitArcanum(u32 battler)
{
    for (u8 i = 0; i < CASTORIA_ARCANUM_COUNT; i++)
        gBattleStruct->castoria.arcanumMoves[i] = sArcanumPool[i];
}

#if TESTING
u16 Test_GetCastoriaArcanumMove(u32 slot)
{
    if (slot >= CASTORIA_ARCANUM_COUNT || slot >= ARRAY_COUNT(sArcanumPool))
        return MOVE_NONE;
    return sArcanumPool[slot];
}
#endif

u16 CastoriaGetMoveAtSlot(u32 battler, u8 slot)
{
    return GetBattlerMoveAtSlot(battler, slot);
}

bool32 IsArcanumSlotUsable(u32 battler, u32 slot)
{
    u32 move;
    u32 choicedMove;
    enum HoldEffect holdEffect;
    enum BattleMoveEffects effect;

    if (slot < MAX_MON_MOVES
     || slot >= MAX_MON_MOVES + CASTORIA_ARCANUM_COUNT)
        return FALSE;

    if (gBattleStruct->castoria.arcanumMoves[0] == MOVE_NONE)
        return FALSE;

    move = GetBattlerMoveAtSlot(battler, slot);
    if (move == MOVE_NONE)
        return FALSE;

    if (move == MOVE_ARCANUM_WISH
     && !CastoriaCanWishFor(battler, CASTORIA_WISH_DOMINATION)
     && !CastoriaCanWishFor(battler, CASTORIA_WISH_ENDURANCE))
        return FALSE;

    effect = GetMoveEffect(move);
    if (effect == EFFECT_PLACEHOLDER)
        return FALSE;

    if (gDisableStructs[battler].encoreTimer
     && gDisableStructs[battler].encoredMove != move)
        return FALSE;

    if (gDisableStructs[battler].disabledMove == move)
        return FALSE;

    if (gBattleMons[battler].volatiles.torment
     && gLastMoves[battler] == move)
        return FALSE;

    if (gDisableStructs[battler].tauntTimer
     && IsBattleMoveStatus(move))
        return FALSE;

    if (GetImprisonedMovesCount(battler, move))
        return FALSE;

    holdEffect = GetBattlerHoldEffect(battler);
    choicedMove = gBattleStruct->choicedMove[battler];

    if (choicedMove != MOVE_NONE
     && choicedMove != MOVE_UNAVAILABLE
     && choicedMove != move
     && (IsHoldEffectChoice(holdEffect)
         || GetBattlerAbility(battler) == ABILITY_GORILLA_TACTICS))
        return FALSE;

    if (holdEffect == HOLD_EFFECT_ASSAULT_VEST
     && IsBattleMoveStatus(move)
     && effect != EFFECT_ME_FIRST)
        return FALSE;

    if (((gFieldStatuses & STATUS_FIELD_GRAVITY) && IsMoveGravityBanned(move))
     || IsHealBlockPreventingMove(battler, move)
     || IsBelchPreventingMove(battler, move))
        return FALSE;

    if (gDisableStructs[battler].throatChopTimer
     && IsSoundMove(move))
        return FALSE;

    if (effect == EFFECT_STUFF_CHEEKS
     && GetItemPocket(gBattleMons[battler].item) != POCKET_BERRIES)
        return FALSE;

    if (MoveCantBeUsedTwice(move)
     && gLastResultingMoves[battler] == move)
        return FALSE;

    return TRUE;
}

bool32 HasUsableArcanumMove(u32 battler)
{
    u32 slot;

    for (slot = MAX_MON_MOVES;
         slot < MAX_MON_MOVES + CASTORIA_ARCANUM_COUNT;
         slot++)
    {
        if (IsArcanumSlotUsable(battler, slot))
            return TRUE;
    }

    return FALSE;
}

bool32 HasMageArmor(u32 battler)
{
    return gBattleStruct->castoria.mageArmorPartyMask != 0
        && !IsOnPlayerSide(battler)
        && (gBattleStruct->castoria.mageArmorPartyMask
            & (1u << gBattlerPartyIndexes[battler]));
}

bool32 CastoriaCanWishFor(u32 battler, u32 outcome)
{
    if (battler >= gBattlersCount
     || IsOnPlayerSide(battler)
     || gBattleMons[battler].species != SPECIES_CASTORIA
     || !IsBattlerAlive(battler)
     || gBattleStruct->castoria.wishUsed)
        return FALSE;

    switch (outcome)
    {
    case CASTORIA_WISH_DOMINATION:
        return gBattleStruct->castoria.nameResult == CASTORIA_NAME_TRUE
            && gBattleMons[battler].volatiles.castoriaTrueNameCharges == 0;
    case CASTORIA_WISH_ENDURANCE:
        return (gBattleStruct->castoria.nameResult == CASTORIA_NAME_TRUE
             || gBattleStruct->castoria.nameResult == CASTORIA_NAME_FALSE)
            && GetBreakBarCount(battler) == 0;
    default:
        // Other named outcomes stay unavailable until their effects exist.
        return FALSE;
    }
}

u32 CastoriaGetAvailableWishOutcome(u32 battler)
{
    if (CastoriaCanWishFor(battler, CASTORIA_WISH_DOMINATION))
        return CASTORIA_WISH_DOMINATION;
    // Endurance has execution support; its production tactical policy is deferred.
    return CASTORIA_WISH_NONE;
}

void BS_CastoriaTryWish(void)
{
    NATIVE_ARGS(const u8 *failInstr);
    u32 battler = gBattlerAttacker;
    u32 outcome = gBattleStruct->castoria.wishOutcome;

    gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_NONE;
    if (!IsArcanumAction(battler)
     || gChosenMove != MOVE_ARCANUM_WISH
     || gCurrentMove != MOVE_ARCANUM_WISH
     || !CastoriaCanWishFor(battler, outcome))
    {
        gBattlescriptCurrInstr = cmd->failInstr;
        return;
    }

    switch (outcome)
    {
    case CASTORIA_WISH_DOMINATION:
        gBattleMons[battler].volatiles.castoriaTrueNameCharges = 3;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CASTORIA_WISH_DOMINATION;
        break;
    case CASTORIA_WISH_ENDURANCE:
        if (!GrantSavedBreakBar(battler))
        {
            gBattlescriptCurrInstr = cmd->failInstr;
            return;
        }
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CASTORIA_WISH_ENDURANCE;
        break;
    default:
        gBattlescriptCurrInstr = cmd->failInstr;
        return;
    }

    gBattleStruct->castoria.wishUsed = TRUE;
    gBattlescriptCurrInstr = cmd->nextInstr;
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


// ---------------------------------------------------------------
// Polymorph logic
// ---------------------------------------------------------------


static u32 sPolymorphRejectBattler;

#define POLYMORPH_TOP_CANDIDATES    5
#define POLYMORPH_PENALTY 120


// How bad a species is for the player's mon, judged from Castoria's own moves
// (higher = crueler).
static u32 ScorePolymorphCruelty(u32 species, u32 castoriaBattler, struct Pokemon *targetMon)
{

    struct PolymorphEVSpread
{
    u16 hp;
    u16 atk;
    u16 def;
    u16 spAtk;
    u16 spDef;
    u16 speed;
};
    enum Type t1 = gSpeciesInfo[species].types[0];
    enum Type t2 = gSpeciesInfo[species].types[1];
    u32 casterSpeed = gSpeciesInfo[gBattleMons[castoriaBattler].species].baseSpeed;
    u32 effSum = 0, weightSum = 0, avgEff, score, slot;
    enum Ability ability;

    // 1. Type exposure to every damaging move she has, Arcanum included.
    for (slot = 0; slot < MAX_MON_MOVES + CASTORIA_ARCANUM_COUNT; slot++)
    {
        u16 move = CastoriaGetMoveAtSlot(castoriaBattler, slot);
        u32 power, eff;

        if (move == MOVE_NONE || gMovesInfo[move].category == DAMAGE_CATEGORY_STATUS)
            continue;
        power = gMovesInfo[move].power != 0 ? gMovesInfo[move].power : 60;
        eff = GetTypeModifier(gMovesInfo[move].type, t1);
        if (t2 != t1)
            eff = (eff * GetTypeModifier(gMovesInfo[move].type, t2)) >> 12;
        effSum += power * eff;
        weightSum += power;
    }
    avgEff = weightSum != 0 ? effSum / weightSum : 4096;
    score = (avgEff * 50) / 4096;   // 0.5x -> 25, neutral -> 50, 2x -> 100

    // 2. Slower than her is worse for the player.
    if (casterSpeed > gSpeciesInfo[species].baseSpeed)
        score += casterSpeed - gSpeciesInfo[species].baseSpeed;

    // 3. Self-crippling abilities.
    u8 abilityNum = GetMonData(targetMon, MON_DATA_ABILITY_NUM);
    ability = GetAbilityBySpecies(species, abilityNum);
    if (ability == ABILITY_TRUANT || ability == ABILITY_SLOW_START || ability == ABILITY_DEFEATIST)
        score += 40;

    struct PolymorphEVSpread targetEV = {
        .hp = GetMonData(targetMon, MON_DATA_HP_EV),
        .atk = GetMonData(targetMon, MON_DATA_ATK_EV),
        .def = GetMonData(targetMon, MON_DATA_DEF_EV),
        .spAtk = GetMonData(targetMon, MON_DATA_SPATK_EV),
        .spDef = GetMonData(targetMon, MON_DATA_SPDEF_EV),
        .speed = GetMonData(targetMon, MON_DATA_SPEED_EV)
    };

    // HP: target invested in HP → give them a fragile mon (low base HP)
    if (targetEV.hp > 0)
    {
        u32 hpDeficit = 200 - gSpeciesInfo[species].baseHP;
        if (hpDeficit > 0)
            score += (hpDeficit * targetEV.hp) / 252 / 4;
    }

    // Atk: target physically invested → give them a physical wall (high Def)
    if (targetEV.atk > 0)
        score += (gSpeciesInfo[species].baseDefense * targetEV.atk) / 252 / 4;

    // Def: target defensively invested → give them a physical breaker (high Atk)
    if (targetEV.def > 0)
        score += (gSpeciesInfo[species].baseAttack * targetEV.def) / 252 / 4;

    // SpAtk: target specially invested → give them a special wall (high SpDef)
    if (targetEV.spAtk > 0)
        score += (gSpeciesInfo[species].baseSpDefense * targetEV.spAtk) / 252 / 4;

    // SpDef: target specially defensive → give them a special breaker (high SpAtk)
    if (targetEV.spDef > 0)
        score += (gSpeciesInfo[species].baseSpAttack * targetEV.spDef) / 252 / 4;

    // Speed: target speed invested → give them something slower
    if (targetEV.speed > 0)
    {
        s32 speedDiff = (s32)gSpeciesInfo[gBattleMons[castoriaBattler].species].baseSpeed
                      - (s32)gSpeciesInfo[species].baseSpeed;
        if (speedDiff > 0)
            score += (speedDiff * targetEV.speed) / 252 / 4;
    }

    return score;
}

static bool32 ShouldRejectPolymorphSpecies(u32 species)
{
    u32 targetBattler = sPolymorphRejectBattler;
    bool32 playerSide = IsOnPlayerSide(targetBattler);

    // Universal rejections for both sides
    if (species == SPECIES_NONE
     || !IsSpeciesEnabled(species)
     || species == SPECIES_SHEDINJA
     || species == gBattleMons[targetBattler].species
     || species > SPECIES_FALINKS_MEGA)   // custom/boss species guard
        return TRUE;

    bool32 isHighTier = gSpeciesInfo[species].isMegaEvolution
                     || gSpeciesInfo[species].isPrimalReversion
                     || gSpeciesInfo[species].isUltraBurst
                     || gSpeciesInfo[species].isGigantamax
                     || gSpeciesInfo[species].isTeraForm
                     || gSpeciesInfo[species].isLegendary
                     || gSpeciesInfo[species].isMythical
                     || gSpeciesInfo[species].isParadox
                     || gSpeciesInfo[species].isUltraBeast;

    if (playerSide)
    {
        // PLAYER TARGETING (The Game Master): Reject high-tier monsters to force downgrades
        return isHighTier;
    }
    else
    {
        // ALLY / SELF TARGETING (The Chaos Wish): REJECT weak/normal mons, ONLY allow absolute powerhouses
        return !isHighTier;
    }
}

static u32 ScaleHpToNewMax(u32 hp, u32 oldMax, u32 newMax)
{
    if (hp == 0 || oldMax == 0)
        return hp; // fainted stays fainted; avoid divide-by-zero
    hp = (hp * newMax) / oldMax;
    return hp == 0 ? 1 : hp;
}



struct MoveScore {
    u16 move;
    u32 score;
};

static EWRAM_DATA struct
{
    struct MoveScore pool[250];
    u16 count;
} sPolyScratch;

// Castoria buys 4 moves to hit the exact target budget
void ConstructPolymorphMoveset(struct Pokemon *mon, u16 species, u32 moveBudget)
{
    struct MoveScore *pool = sPolyScratch.pool;
    u16 poolCount = 0;
    sPolyScratch.count = 0; 
    u16 finalMoves[MAX_MON_MOVES] = {MOVE_NONE, MOVE_NONE, MOVE_NONE, MOVE_NONE};
    u8 level = GetMonData(mon, MON_DATA_LEVEL);
    u16 move, i, j;
    bool8 alreadyInPool;

    // 1. GATHER ALL LEGAL MOVES (Same logic as before, but we score them immediately)
    if (gSpeciesInfo[species].levelUpLearnset != NULL)
    {
        for (i = 0; gSpeciesInfo[species].levelUpLearnset[i].move != LEVEL_UP_MOVE_END; i++)
        {
            if (gSpeciesInfo[species].levelUpLearnset[i].level <= level)
            {
                move = gSpeciesInfo[species].levelUpLearnset[i].move;
                alreadyInPool = FALSE;
                for (j = 0; j < poolCount; j++)
                {
                    if (pool[j].move == move) { alreadyInPool = TRUE; break; }
                }
                if (!alreadyInPool && poolCount < 250)
                {
                    pool[poolCount].move = move;
                    pool[poolCount].score = EvaluateMoveThreat(move, species);
                    poolCount++;
                }
            }
        }
    }

    for (move = 1; move < MOVES_COUNT; move++)
    {
        if (CanLearnTeachableMove(species, move) || SpeciesCanLearnEggMove(species, move))
        {
            alreadyInPool = FALSE;
            for (j = 0; j < poolCount; j++)
            {
                if (pool[j].move == move) { alreadyInPool = TRUE; break; }
            }
            if (!alreadyInPool && poolCount < 250)
            {
                pool[poolCount].move = move;
                pool[poolCount].score = EvaluateMoveThreat(move, species);
                poolCount++;
            }
        }
    }

    // 2. GREEDY SHOPPING ALGORITHM
    u32 remainingBudget = moveBudget;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (poolCount == 0) break;

        // Ideal cost for this specific slot to smoothly spend the remaining funds
        u32 targetSlotScore = remainingBudget / (MAX_MON_MOVES - i);
        u16 bestIndex = 0;
        u32 bestDiff = 0xFFFFFFFF;

        // Find the move whose score is closest to our target for this slot
        for (j = 0; j < poolCount; j++)
        {
            u32 diff = (pool[j].score > targetSlotScore) ? (pool[j].score - targetSlotScore) : (targetSlotScore - pool[j].score);
            if (diff < bestDiff)
            {
                bestDiff = diff;
                bestIndex = j;
            }
        }

        finalMoves[i] = pool[bestIndex].move;
        
        // Deduct from budget (preventing underflow)
        if (remainingBudget >= pool[bestIndex].score)
            remainingBudget -= pool[bestIndex].score;
        else
            remainingBudget = 0;

        // Remove chosen move from pool
        pool[bestIndex] = pool[poolCount - 1];
        poolCount--;
    }

    // 3. ASSIGN MOVES & RESTORE PP
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &finalMoves[i]);
        if (finalMoves[i] != MOVE_NONE)
        {
            u8 maxPP = CalculatePPWithBonus(finalMoves[i], GetMonData(mon, MON_DATA_PP_BONUSES), i);
            SetMonData(mon, MON_DATA_PP1 + i, &maxPP);
        }
    }
}

// Pillar 2: Moveset Valuation
u32 EvaluateMoveThreat(u16 move, u16 species)
{
    if (move == MOVE_NONE) return 0;
    
    u32 score = 0;
    // Note: pokeemerald-expansion uses gMovesInfo for move data
    u8 type = gMovesInfo[move].type;
    u8 power = gMovesInfo[move].power;
    u8 accuracy = gMovesInfo[move].accuracy;
    
    if (power > 0)
    {
        // Expected Value: Power * Accuracy percentage
        score = (power * (accuracy == 0 ? 100 : accuracy)) / 100;
        
        // Apply STAB premium
        if (type == gSpeciesInfo[species].types[0] || type == gSpeciesInfo[species].types[1])
            score = (score * 15) / 10;
    }
    else
    {
        // Status Move Tiers
        switch (gMovesInfo[move].effect)
        {
            case EFFECT_RESTORE_HP:
            case EFFECT_MORNING_SUN:
            case EFFECT_SYNTHESIS:
            case EFFECT_MOONLIGHT:
            case EFFECT_ROOST:
                score = 45; break;
            case EFFECT_ATTACK_UP_2:
            case EFFECT_SPECIAL_ATTACK_UP_2:
            case EFFECT_DRAGON_DANCE:
            case EFFECT_QUIVER_DANCE:
            case EFFECT_SHELL_SMASH:
                score = 50; break;
            case EFFECT_NON_VOLATILE_STATUS:
            case EFFECT_YAWN:
                score = 40; break;
            case EFFECT_PROTECT:
            case EFFECT_SPIKES:
            case EFFECT_STEALTH_ROCK:
                score = 30; break;
            default:
                score = 15; break; // Generic debuffs/utility
        }
    }
    return score;
}

// Pillar 1: Hardware & Min-Max Tax
u32 EvaluateSpeciesHardware(u16 species)
{
    u32 hp = gSpeciesInfo[species].baseHP;
    u32 atk = gSpeciesInfo[species].baseAttack;
    u32 def = gSpeciesInfo[species].baseDefense;
    u32 spa = gSpeciesInfo[species].baseSpAttack;
    u32 spd = gSpeciesInfo[species].baseSpDefense;
    u32 spe = gSpeciesInfo[species].baseSpeed;
    
    u32 bst = hp + atk + def + spa + spd + spe;
    u32 mean = bst / 6;
    
    u32 variance = 0;
    variance += (hp > mean) ? (hp - mean) : (mean - hp);
    variance += (atk > mean) ? (atk - mean) : (mean - atk);
    variance += (def > mean) ? (def - mean) : (mean - def);
    variance += (spa > mean) ? (spa - mean) : (mean - spa);
    variance += (spd > mean) ? (spd - mean) : (mean - spd);
    variance += (spe > mean) ? (spe - mean) : (mean - spe);
    
    // Total Threat = BST + 50% of the stat deviation
    return bst + (variance / 2);
}

// Calculates total score of a specific Mon instance (Stats + Moves)
u32 CalculateMonTotalThreat(struct Pokemon *mon)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    u32 score = EvaluateSpeciesHardware(species);
    
    for (int i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = GetMonData(mon, MON_DATA_MOVE1 + i);
        score += EvaluateMoveThreat(move, species);
    }
    return score;
}
#if 0
/* u16 CastoriaSelectPolymorphTargetAndBudget(struct Pokemon *playerMon, u32 *outMoveBudget)
{
    u32 currentThreat = CalculateMonTotalThreat(playerMon);
    sPolymorphRejectBattler = gBattlerTarget;
    
    // Castoria's exact target for this player's mon
    u32 targetThreat = (currentThreat > POLYMORPH_PENALTY) ? (currentThreat - POLYMORPH_PENALTY) : 100;
    
    // THE GOLDEN RATIO: Hardware cannot consume more than 65% of the budget.
    // This guarantees at least 35% of the budget is purely dedicated to lethal moves.
    u32 maxHardwareBudget = (targetThreat * 65) / 100;
    u32 minHardwareBudget = (targetThreat * 40) / 100;
    
    u16 bestSpecies = SPECIES_NONE;
    u32 bestHardwareScore = 0;
    u32 highestCrueltyIndex = 0;

    // Roll 30 candidates to find the meanest one within the Golden Ratio
    for (int i = 0; i < 30; i++)
    {
        u16 testSpecies = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);
        u32 hardwareScore = EvaluateSpeciesHardware(testSpecies);
        
        // Enforce the ratio: Must leave enough room for a real moveset, but not be completely frail
        if (hardwareScore >= minHardwareBudget && hardwareScore <= maxHardwareBudget)
        {
            u32 crueltyIndex = 10; 
            
            // Example: Prioritize giving the player a species weak to Castoria's likely STAB
            u8 type1 = gSpeciesInfo[testSpecies].types[0];
            u8 type2 = gSpeciesInfo[testSpecies].types[1];
            if (type1 == TYPE_DRAGON || type1 == TYPE_DARK) crueltyIndex += 20;
            if (type2 == TYPE_DRAGON || type2 == TYPE_DARK) crueltyIndex += 20;

            if (crueltyIndex > highestCrueltyIndex || bestSpecies == SPECIES_NONE)
            {
                highestCrueltyIndex = crueltyIndex;
                bestSpecies = testSpecies;
                bestHardwareScore = hardwareScore;
            }
        }
    }
    
    // Fallback: If RNG fails to find a perfect ratio, force an average mid-tier mon 
    // to ensure there is always a healthy move budget.
    if (bestSpecies == SPECIES_NONE)
    {
        bestSpecies = SPECIES_RATICATE; // Or any reliable baseline species
        bestHardwareScore = EvaluateSpeciesHardware(bestSpecies);
    }
        
    // Hand the remaining funds (guaranteed to be ~35-60% of the total threat) to the Move Constructor
    *outMoveBudget = (targetThreat > bestHardwareScore) ? (targetThreat - bestHardwareScore) : 150;
    
    return bestSpecies;
} */
#endif

struct PolymorphCandidate
{
    u16 species;
    u16 score;
};

u16 CastoriaSelectPolymorphTargetAndBudget(struct Pokemon *playerMon, u32 *outMoveBudget)
{
    struct PolymorphCandidate top[POLYMORPH_TOP_CANDIDATES];

    u32 currentThreat = CalculateMonTotalThreat(playerMon);
    u32 targetThreat = (currentThreat > POLYMORPH_PENALTY) ? (currentThreat - POLYMORPH_PENALTY) : 100;
    u32 minHardwareBudget = (targetThreat * 40) / 100;
    u32 maxHardwareBudget = (targetThreat * 65) / 100;
    u32 topCount = 0;
    u32 i, hardware;
    u16 chosen;
    u16 score;

    sPolymorphRejectBattler = gBattlerTarget;

    u16 samples[60];
    u32 sampleScores[60];
    u32 sampleCount = 0;
    u32 attempts = 0;

     // Monte Carlo: sample up to 60 valid species from 600 RNG attempts.
    // Prevents the same top-scoring species from dominating every cast.
    while (sampleCount < 60 && attempts < 600)
    {
        attempts++;
        u16 testSpecies = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);
        hardware = EvaluateSpeciesHardware(testSpecies);

        if (hardware < minHardwareBudget || hardware > maxHardwareBudget)
            continue;

        score = ScorePolymorphCruelty(testSpecies, gBattlerAttacker, playerMon);

        samples[sampleCount] = testSpecies;
        sampleScores[sampleCount] = score;
        sampleCount++;
    }

    if (sampleCount == 0)
    {
        chosen = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);
    }
    else
    {
        // Pick the best-scoring sample
        u32 bestIdx = 0;
        for (i = 1; i < sampleCount; i++)
        {
            if (sampleScores[i] > sampleScores[bestIdx])
                bestIdx = i;
        }
        chosen = samples[bestIdx];
#if TESTING
        DebugPrintf("POLY chosen=%u (%s) hw=%u cruelty=%u samples=%u attempts=%u",
                    chosen, GetSpeciesName(chosen), hardware, sampleScores[bestIdx], sampleCount, attempts);
#endif
    }

    if (topCount == 0) // nothing fit the band: any legal species
        chosen = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);
    else
        chosen = top[RandomUniform(RNG_POLYMORPH, 0, topCount - 1)].species;

    hardware = EvaluateSpeciesHardware(chosen);
    *outMoveBudget = (targetThreat > hardware) ? (targetThreat - hardware) : 150;
#if TESTING
    DebugPrintf("POLY cur=%d target=%d band=%d-%d chosen=%d", currentThreat, targetThreat,
                minHardwareBudget, maxHardwareBudget, chosen);
#endif
    return chosen;
}

void GiveMonPolymorphMoveset(struct Pokemon *mon, u16 species)
{
    u16 pool[250]; 
    u16 poolCount = 0;
    u16 finalMoves[MAX_MON_MOVES] = {MOVE_NONE, MOVE_NONE, MOVE_NONE, MOVE_NONE};
    u8 level = GetMonData(mon, MON_DATA_LEVEL);
    u16 move, i, j;
    bool8 alreadyInPool;

    // 1. GATHER LEVEL-UP MOVES
    if (gSpeciesInfo[species].levelUpLearnset != NULL)
    {
        for (i = 0; gSpeciesInfo[species].levelUpLearnset[i].move != LEVEL_UP_MOVE_END; i++)
        {
            if (gSpeciesInfo[species].levelUpLearnset[i].level <= level)
            {
                move = gSpeciesInfo[species].levelUpLearnset[i].move;
                alreadyInPool = FALSE;
                for (j = 0; j < poolCount; j++)
                {
                    if (pool[j] == move)
                    {
                        alreadyInPool = TRUE;
                        break;
                    }
                }
                if (!alreadyInPool && poolCount < ARRAY_COUNT(pool))
                    pool[poolCount++] = move;
            }
        }
    }

    // 2. GATHER TEACHABLE MOVES & EGG MOVES
    for (move = 1; move < MOVES_COUNT; move++)
    {
        if (CanLearnTeachableMove(species, move) || SpeciesCanLearnEggMove(species, move))
        {
            alreadyInPool = FALSE;
            for (j = 0; j < poolCount; j++)
            {
                if (pool[j] == move)
                {
                    alreadyInPool = TRUE;
                    break;
                }
            }
            if (!alreadyInPool && poolCount < ARRAY_COUNT(pool))
                pool[poolCount++] = move;
        }
    }

    // 3. RANDOMLY SELECT 4 UNIQUE MOVES
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (poolCount == 0)
            break; 

        u16 randIdx = Random() % poolCount;
        finalMoves[i] = pool[randIdx];

        // Remove the chosen move from the pool by swapping it with the last element
        pool[randIdx] = pool[poolCount - 1];
        poolCount--;
    }

    // 4. ASSIGN MOVES & RESTORE PP
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &finalMoves[i]);
        
        if (finalMoves[i] != MOVE_NONE)
        {
            u8 maxPP = CalculatePPWithBonus(finalMoves[i], GetMonData(mon, MON_DATA_PP_BONUSES), i);
            SetMonData(mon, MON_DATA_PP1 + i, &maxPP);
        }
    }
}

struct PolymorphBackup {
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u32 exp;
};

static EWRAM_DATA struct PolymorphBackup sPolymorphBackup[2][PARTY_SIZE] = {0};

void BS_PolymorphTarget(void)
{
    NATIVE_ARGS();
    u32 target = gBattlerTarget;
    u32 monId = gBattlerPartyIndexes[target];
    u32 side = GetBattlerSide(target);
    u32 partyIndex = gBattlerPartyIndexes[target];
    struct Pokemon *party = GetBattlerParty(target);
    struct Pokemon *mon = &party[monId];
    u32 oldSpecies = gBattleMons[target].species;
    u32 oldHp = GetMonData(mon, MON_DATA_HP);
    u32 oldMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
    bool32 alreadyPolymorphed;
    u32 newSpecies, newHp, newExp, i, moveBudget;
    
    // ADD THESE: Grab the current Level and EXP before morphing
    u8 currentLevel = GetMonData(mon, MON_DATA_LEVEL);
    u32 oldExp = GetMonData(mon, MON_DATA_EXP);

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

    alreadyPolymorphed = GetBattlerPartyState(target)->polymorphed;
    sPolymorphRejectBattler = target;
    if (IsOnPlayerSide(target))
        newSpecies = CastoriaSelectPolymorphTargetAndBudget(mon, &moveBudget);
    else
        newSpecies = RandomUniformExcept(RNG_POLYMORPH, SPECIES_NONE + 1, NUM_SPECIES, ShouldRejectPolymorphSpecies);
    // Only the first cast records the true originals; later casts must not overwrite them.
    if (!alreadyPolymorphed)
    {
        sPolymorphBackup[side][partyIndex].exp = oldExp; // Save the raw EXP
        if (GetBattlerPartyState(target)->changedSpecies == SPECIES_NONE)
            GetBattlerPartyState(target)->changedSpecies = oldSpecies;
        GetMonData(mon, MON_DATA_NICKNAME, sPolymorphBackup[side][partyIndex].nickname);
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            sPolymorphBackup[side][partyIndex].moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
            sPolymorphBackup[side][partyIndex].pp[i] = GetMonData(mon, MON_DATA_PP1 + i);
        }
    }
    GetBattlerPartyState(target)->polymorphed = TRUE;

    SetMonData(mon, MON_DATA_SPECIES, &newSpecies);
    gBattleMons[target].species = newSpecies;

    // Calculate and assign the exact EXP needed to maintain the current level on the new curve
    newExp = gExperienceTables[gSpeciesInfo[newSpecies].growthRate][currentLevel];
    SetMonData(mon, MON_DATA_EXP, &newExp);

    RecalcBattlerStats(target, mon, FALSE);

    newHp = ScaleHpToNewMax(oldHp, oldMaxHp, GetMonData(mon, MON_DATA_MAX_HP));
    SetMonData(mon, MON_DATA_HP, &newHp);
    gBattleMons[target].hp = newHp;

    PREPARE_SPECIES_BUFFER(gBattleTextBuff2, oldSpecies)

    if (IsOnPlayerSide(target))
        ConstructPolymorphMoveset(mon, newSpecies, moveBudget);  // budget-aware, cruel
    else
        GiveMonPolymorphMoveset(mon, newSpecies);                 // random, chaotic
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        gBattleMons[target].moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);;
        gBattleMons[target].pp[i] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
    }
    gDisableStructs[target].disabledMove = MOVE_NONE;
    gDisableStructs[target].disableTimer = 0;
    gDisableStructs[target].mimickedMoves = 0;
    gDisableStructs[target].usedMoves = 0;
    gBattleStruct->choicedMove[target] = MOVE_NONE;
    BtlController_EmitResetActionMoveSelection(target, B_COMM_TO_CONTROLLER, RESET_MOVE_SELECTION);
    MarkBattlerForControllerExec(target);
    
    if (StringCompare(sPolymorphBackup[side][partyIndex].nickname,
                      GetSpeciesName(GetBattlerPartyState(target)->changedSpecies)) == 0)
    {
        const u8 *newName = GetSpeciesName(newSpecies);

        SetMonData(mon, MON_DATA_NICKNAME, newName);
        StringCopy(gBattleMons[target].nickname, newName);
    }

    PREPARE_SPECIES_BUFFER(gBattleTextBuff1, newSpecies)

    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_TRANSFORMED;
}

bool32 TryRestorePolymorphedMon(u32 partyIndex)
{
    struct PartyState *state = &gBattleStruct->partyState[B_SIDE_PLAYER][partyIndex];
    struct Pokemon *mon = &gPlayerParty[partyIndex];
    u32 original, formHp, formMaxHp, newHp;
    // FIX: Set side explicitly since this function iterates over the player's party
    u32 side = B_SIDE_PLAYER;
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
        SetMonData(mon, MON_DATA_MOVE1 + i, &sPolymorphBackup[side][partyIndex].moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &sPolymorphBackup[side][partyIndex].pp[i]);
    }
    // RESTORE original nickname and XP values.
    SetMonData(mon, MON_DATA_NICKNAME, sPolymorphBackup[side][partyIndex].nickname);
    u32 savedExp = sPolymorphBackup[side][partyIndex].exp;
    SetMonData(mon, MON_DATA_EXP, &savedExp);
    CalculateMonStats(mon);
    newHp = ScaleHpToNewMax(formHp, formMaxHp, GetMonData(mon, MON_DATA_MAX_HP));
    SetMonData(mon, MON_DATA_HP, &newHp);

    return TRUE;
}
