#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_koanko.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "data.h"
#include "pokemon.h"
#include "test_runner.h"
#include "trainer_break_bars.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/trainers.h"

#include "data/koanko_party.h"

static const u8 sKoankoConjuringMessage[] = _("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");

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
     || NoAliveMonsForPlayer() || !NoAliveMonsForBattlerSide(B_POSITION_OPPONENT_LEFT)
     || gBattleResources->battleScriptsStack->size + 2 > ARRAY_COUNT(gBattleResources->battleScriptsStack->ptr))
        return FALSE;

    // Commit before yielding to controllers or dialogue: this can run only once.
    gBattleStruct->koankoPhase = KOANKO_CONJURING;
    gBattleStruct->trainerSlideMsg = sKoankoConjuringMessage;
    gBattleScripting.battler = B_POSITION_OPPONENT_LEFT;
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
    memset(gBattleStruct->partyState[B_SIDE_OPPONENT], 0, sizeof(gBattleStruct->partyState[B_SIDE_OPPONENT]));
    memset(gBattleStruct->itemLost[B_SIDE_OPPONENT], 0, sizeof(gBattleStruct->itemLost[B_SIDE_OPPONENT]));
    memset(gBattleStruct->gimmick.activeGimmick[B_SIDE_OPPONENT], 0, sizeof(gBattleStruct->gimmick.activeGimmick[B_SIDE_OPPONENT]));
    memset(gAiPartyData->mons[B_SIDE_OPPONENT], 0, sizeof(gAiPartyData->mons[B_SIDE_OPPONENT]));
    gWishFutureKnock.knockedOffMons[B_SIDE_OPPONENT] = 0;
    gBattleStruct->givenExpMons = 0;
    gBattleStruct->opponentMonCanTera = 0;
    gBattleStruct->opponentMonCanDynamax = 0;
    gBattleStruct->arenaLostOpponentMons = 0;

    struct Trainer trainer = *GetTrainerStructFromId(TRAINER_KOANKO);
    trainer.party = sKoankoConjuredParty;
    trainer.partySize = ARRAY_COUNT(sKoankoConjuredParty);
    trainer.poolSize = 0;
    trainer.overrideTrainer = TRAINER_NONE;
    CreateNPCTrainerPartyWithBreakBars(gEnemyParty, &trainer, TRUE, BATTLE_TYPE_TRAINER, &gKoankoConjuredBreakBars);
    for (u32 slot = 0; slot < PARTY_SIZE; slot++)
    {
        TryFormChange(slot, B_SIDE_OPPONENT, FORM_CHANGE_BEGIN_BATTLE);
        gBattleStruct->itemLost[B_SIDE_OPPONENT][slot].originalItem = GetMonData(&gEnemyParty[slot], MON_DATA_HELD_ITEM);
    }
    gAiPartyData->count[B_SIDE_OPPONENT] = CalculateEnemyPartyCount();

    u32 battler = B_POSITION_OPPONENT_LEFT;
    memset(&gBattleStruct->battlerState[battler], 0, sizeof(gBattleStruct->battlerState[battler]));
    memset(&gDisableStructs[battler], 0, sizeof(gDisableStructs[battler]));
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
    gBattleStruct->changedItems[battler] = ITEM_NONE;
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
