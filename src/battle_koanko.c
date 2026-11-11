#include "global.h"
#include "battle_script_native.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_ai_record.h"
#include "battle_controllers.h"
#include "battle_koanko.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "battle_tx_rollback.h"
#include "data.h"
#include "debug.h"
#include "main.h"
#include "overworld.h"
#include "pokemon.h"
#include "script.h"
#include "test_runner.h"
#include "trainer_break_bars.h"
#include "trainer_see.h"
#include "constants/abilities.h"
#include "constants/battle_ai.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/trainers.h"

#include "data/koanko_party.h"

static const u8 sKoankoConjuringMessage[] = _("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
static const u8 sDebugSmeargleDefeatText[] = _("That concludes the\nSmeargle demonstration!");
static EWRAM_DATA bool8 sDebugSmeargleBattlePending = FALSE;

static void CB2_EndKoankoSmeargleDebugBattle(void)
{
    sDebugSmeargleBattlePending = FALSE;
    // This singles encounter did not back up a follower party. Avoid the normal
    // trainer callback's follower restore and permanent trainer-defeated flags.
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// Debug Menu -> Scripts -> Script 3. Use the normal trainer identity/AI, but
// the debug battle launcher fixes the format to singles even with a follower.
void Script_StartKoankoSmeargleDebugBattle(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_ANY);
    memset(&gTrainerBattleParameter, 0, sizeof(gTrainerBattleParameter));
    // Zeroed target flags select singles; null introTextA skips intro text.
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_KOANKO;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sDebugSmeargleDefeatText;
    gNoOfApproachingTrainers = 0;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    // TRUE would skip normal party initialization and substitute gDebugAIFlags.
    gIsDebugBattle = FALSE;
    sDebugSmeargleBattlePending = TRUE;
    BattleSetup_StartTrainerBattle_Debug();
    gMain.savedCallback = CB2_EndKoankoSmeargleDebugBattle;
}

// Called after battle resources and normal trainer AI flags are initialized,
// before the opening controller requests copy party slot 0 into gBattleMons.

static void BuffKoankoSmeargle(struct Pokemon *party)
{
    // Effective base 120 Attack and Sp. Atk.
    // Level 100, 31 IVs, 252 EVs, Hasty nature.
    u32 attack = 339;
    u32 spAttack = 339;

    SetMonData(&party[0], MON_DATA_ATK, &attack);
    SetMonData(&party[0], MON_DATA_SPATK, &spAttack);
}

bool32 KoankoTryCreateDebugSmeargleParty(struct Pokemon *party, u16 trainerNum, bool32 firstTrainer)
{
    if (!sDebugSmeargleBattlePending)
        return FALSE;
    sDebugSmeargleBattlePending = FALSE;
    if (party != gParties[B_TRAINER_OPPONENT_A] || !firstTrainer || trainerNum != TRAINER_KOANKO
     || gBattleTypeFlags != BATTLE_TYPE_TRAINER || gBattleStruct == NULL)
        return FALSE;

    struct Trainer trainer = *GetTrainerStructFromId(TRAINER_KOANKO);
    trainer.party = sKoankoConjuredParty;
    trainer.partySize = 1; // Only the real conjured Smeargle, in its owned slot 0.
    trainer.poolSize = 0;
    trainer.overrideTrainer = TRAINER_NONE;
    // Only authored Tera types are eligible in this team. Target smart-Tera
    // generation otherwise assigns random types to every untyped reserve.
    // Keep the encounter's SMART_TERA decision flag unchanged.
    trainer.aiFlags &= ~AI_FLAG_SMART_TERA;
    CreateNPCTrainerPartyWithBreakBars(party, &trainer, TRUE, gBattleTypeFlags, &gKoankoConjuredBreakBars);
    BuffKoankoSmeargle(party);
    gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
    gBattleStruct->battlerState[B_POSITION_OPPONENT_LEFT].noOrderChange = TRUE;
    gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] |= AI_FLAG_SEQUENCE_SWITCHING;
    return TRUE;
}

bool32 TryStartKoankoConjuring(const u8 *returnInstr)
{
    if (gBattleStruct == NULL || gBattleStruct->koankoPhase != KOANKO_ORDINARY_TEAM
     || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
     || (gBattleTypeFlags & (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS
                           | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_LINK
                           | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SECRET_BASE
                           | BATTLE_TYPE_EREADER_TRAINER))
     || ((gBattleTypeFlags & (BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK)) && !gTestRunnerEnabled)
     || TRAINER_BATTLE_PARAM.opponentA != TRAINER_KOANKO
     || gBattleOutcome != 0 || gBattleControllerExecFlags != 0
     || NoAliveMonsForPlayer() || !NoAliveMonsForBattlerSide(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT))
     || gBattleResources->battleScriptsStack->size + 2 > ARRAY_COUNT(gBattleResources->battleScriptsStack->ptr))
        return FALSE;

    // Commit before yielding to controllers or dialogue: this can run only once.
    gBattleStruct->koankoPhase = KOANKO_CONJURING;
    gBattleStruct->trainerSlideMsg = sKoankoConjuringMessage;
    gBattleScripting.battler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    BattleScriptPush(returnInstr);
    gBattlescriptCurrInstr = BattleScript_KoankoConjureTeam;
    return TRUE;
}

void ConjureKoankoTeam(void)
{
    if (gBattleStruct == NULL || gBattleStruct->koankoPhase != KOANKO_CONJURING)
        return;

    // Slot ownership ends here. Keep the player's state, field/side effects,
    // trainer inventory and trainer-wide gimmick consumption intact.
    memset(gBattleStruct->partyState[B_TRAINER_OPPONENT_A], 0, sizeof(gBattleStruct->partyState[B_TRAINER_OPPONENT_A]));
    memset(gBattleStruct->itemLost[B_TRAINER_OPPONENT_A], 0, sizeof(gBattleStruct->itemLost[B_TRAINER_OPPONENT_A]));
    memset(gBattleStruct->gimmick.activeGimmick[B_TRAINER_OPPONENT_A], 0, sizeof(gBattleStruct->gimmick.activeGimmick[B_TRAINER_OPPONENT_A]));
    memset(gAiPartyData->mons[B_TRAINER_OPPONENT_A], 0, sizeof(gAiPartyData->mons[B_TRAINER_OPPONENT_A]));
    // Knock Off is now owned by PartyState, cleared above. Tera/Dynamax
    // eligibility is encoded in the fresh Pokemon generated below, rather
    // than in the removed side bit masks. Preserve trainer consumption.
    gBattleStruct->givenExpMons[B_TRAINER_OPPONENT_A & BIT_FLANK] = 0;
    gBattleStruct->arenaLostOpponentMons = 0;

    // Confirmed design: the conjured team gets one fresh ordinary Tera opportunity. Clear ONLY the
    // opponent trainer's Tera-used flag. The player's Tera usage and the opponent's Dynamax/other
    // gimmick usage are deliberately preserved. This cannot repeat: the function only proceeds in
    // KOANKO_CONJURING and finishes in KOANKO_CONJURED_TEAM. It never activates Tera by itself.
    gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_TERA] = FALSE;

    // The conjured team enters in exactly party order and never switches mid-battle. First-team AI
    // flags are untouched (this runs only at the one-time transition). Guarded because the unit
    // test drives this function without an AI thinking struct.
    if (gAiThinkingStruct != NULL)
        gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] |= AI_FLAG_SEQUENCE_SWITCHING | AI_FLAG_DYNAMIC_FUNC;
    SetDynamicAiFunc(AI_KoankoTrainer);

    struct Trainer trainer = *GetTrainerStructFromId(TRAINER_KOANKO);
    trainer.party = sKoankoConjuredParty;
    trainer.partySize = ARRAY_COUNT(sKoankoConjuredParty);
    trainer.poolSize = 0;
    trainer.overrideTrainer = TRAINER_NONE;
    // Preserve the authored eligibility mask: only Fire Calyrex can Tera.
    // The encounter retains its SMART_TERA decision flag.
    trainer.aiFlags &= ~AI_FLAG_SMART_TERA;
    CreateNPCTrainerPartyWithBreakBars(gParties[B_TRAINER_OPPONENT_A], &trainer, TRUE, BATTLE_TYPE_TRAINER, &gKoankoConjuredBreakBars);
    BuffKoankoSmeargle(gParties[B_TRAINER_OPPONENT_A]);
    for (u32 slot = 0; slot < PARTY_SIZE; slot++)
    {
        TryFormChange(&gParties[B_TRAINER_OPPONENT_A][slot], FORM_CHANGE_BEGIN_BATTLE, B_TRAINER_OPPONENT_A);
        gBattleStruct->itemLost[B_TRAINER_OPPONENT_A][slot].originalItem = GetMonData(&gParties[B_TRAINER_OPPONENT_A][slot], MON_DATA_HELD_ITEM);
    }
    gPartiesCount[B_TRAINER_OPPONENT_A] = CalculatePartyCount(B_TRAINER_OPPONENT_A);
    gAiPartyData->count[B_TRAINER_OPPONENT_A] = gPartiesCount[B_TRAINER_OPPONENT_A];

    u32 battler = B_POSITION_OPPONENT_LEFT;
    memset(&gBattleStruct->battlerState[battler], 0, sizeof(gBattleStruct->battlerState[battler]));
    memset(&gSpecialStatuses[battler], 0, sizeof(gSpecialStatuses[battler]));
    memset(&gProtectStructs[battler], 0, sizeof(gProtectStructs[battler]));
    memset(&gBattleStruct->illusion[battler], 0, sizeof(gBattleStruct->illusion[battler]));
    ClearBattlerMoveHistory(battler);
    ClearBattlerAbilityHistory(battler);
    ClearBattlerItemEffectHistory(battler);
    gBattleHistory->heldItems[battler] = ITEM_NONE;
    gBattleStruct->dynamax.dynamaxTurns[battler] = 0;
    gBattleStruct->dynamax.baseMoves[battler] = MOVE_NONE;
    gBattleStruct->zmove.healReplacement &= ~(1u << battler);
    gBattleStruct->zmove.possibleZMoves[battler] = 0;
    gBattleStruct->zmove.baseMoves[battler] = MOVE_NONE;
    gBattleStruct->gimmick.toActivate &= ~(1u << battler);
    gBattleStruct->gimmick.usableGimmick[battler] = GIMMICK_NONE;
    gBattleStruct->monToSwitchIntoId[battler] = PARTY_SIZE;
    gBattleStruct->AI_monToSwitchIntoId[battler] = 0; // Smeargle leads the new team.
    // The target has no pending changedItems buffer; fresh switch-in loads
    // the generated held item and the trainer-owned itemLost row above.
    gBattleStruct->choicedMove[battler] = MOVE_NONE;
    gBattleStruct->prevTurnSpecies[battler] = SPECIES_NONE;
    gBattleStruct->moveResultFlags[battler] = 0;
    gBattleStruct->moveDamage[battler] = 0;
    gBattleStruct->passiveHpUpdate[battler] = 0;
    gBattleStruct->tracedAbility[battler] = ABILITY_NONE;
    gAiLogicData->shouldSwitch &= ~(1u << battler);
    gAiLogicData->monToSwitchInId[battler] = PARTY_SIZE;
    gAbsentBattlerFlags &= ~(1u << battler);
    for (u32 turn = 0; turn < gBattlersCount; turn++)
        if (gBattlerByTurnOrder[turn] == battler)
            gActionsByTurnOrder[turn] = B_ACTION_FINISHED;

    // Leave the fainted BattlePokemon and hit marker in place. The ordinary
    // replacement path must load new battler data, sprites and switch-in effects.
    gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
}

void BS_ConjureKoankoTeam(void)
{
    NATIVE_ARGS();
    ConjureKoankoTeam();
    gBattlescriptCurrInstr = cmd->nextInstr;
}

// Called at the end of SwitchInClearSetData, which clears noOrderChange. Living conjured owners cannot be
// moved out of sequence (Roar etc.). Genuine faint replacement is unaffected: the open-party-screen
// guard only blocks when the occupant is still alive.
void KoankoOnSwitchIn(u32 battler)
{
    if (gBattleStruct != NULL
     && gBattleStruct->koankoPhase == KOANKO_CONJURED_TEAM
     && GetBattlerSide(battler) == B_SIDE_OPPONENT)
    {
        gBattleStruct->battlerState[battler].noOrderChange = TRUE;
        TxRollbackRegisterKoanko(battler);
    }
}
