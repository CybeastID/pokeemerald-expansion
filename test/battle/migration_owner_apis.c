#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_controllers.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle.h"
#include "battle_script_commands.h"
#include "castoria_name_theft.h"
#include "string_util.h"

MULTI_BATTLE_TEST("Migration Roar selects the opposing trainer's own reserve slots")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PARTNER(SPECIES_WOBBUFFET);
        OPPONENT_A(SPECIES_WOBBUFFET);
        OPPONENT_B(SPECIES_WOBBUFFET);
        OPPONENT_B(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ROAR, target: opponentRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR, playerLeft);
        MESSAGE("The opposing Wynaut was dragged out!");
    } THEN {
        EXPECT_EQ(opponentRight->species, SPECIES_WYNAUT);
        EXPECT_EQ(opponentLeft->species, SPECIES_WOBBUFFET);
    }
}

MULTI_BATTLE_TEST("Migration side survival counts each trainer's pending Break Bar")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PARTNER(SPECIES_WOBBUFFET);
        OPPONENT_A(SPECIES_WOBBUFFET);
        OPPONENT_B(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {}
    } THEN {
        u16 zero = 0;
        u16 savedHp[MAX_BATTLE_TRAINERS];
        for (u32 trainer = 0; trainer < MAX_BATTLE_TRAINERS; trainer++)
        {
            savedHp[trainer] = GetMonData(&gParties[trainer][0], MON_DATA_HP);
            SetMonData(&gParties[trainer][0], MON_DATA_HP, &zero);
        }
        for (u32 battler = 0; battler < gBattlersCount; battler++)
        {
            enum BattleTrainer trainer = GetBattlerTrainer(battler);
            EXPECT_EQ(NoAliveMonsForBattlerSide(battler), TRUE);
            gBattleStruct->partyState[trainer][0].breakBarPending = TRUE;
            EXPECT_EQ(NoAliveMonsForBattlerSide(battler), FALSE);
            EXPECT_EQ(NoAliveMonsForBattlerSide(GetPartnerBattler(battler)), FALSE);
            EXPECT_EQ(NoAliveMonsForBattlerSide(GetOppositeBattler(battler)), TRUE);
            gBattleStruct->partyState[trainer][0].breakBarPending = FALSE;
        }
        for (u32 trainer = 0; trainer < MAX_BATTLE_TRAINERS; trainer++)
            SetMonData(&gParties[trainer][0], MON_DATA_HP, &savedHp[trainer]);
    }
}

MULTI_BATTLE_TEST("Migration Polymorph snapshots do not alias independent trainers at slot zero")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
        PARTNER(SPECIES_WYNAUT) { Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_CHANSEY) { Moves(MOVE_POUND); }
        OPPONENT_B(SPECIES_BLISSEY) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN {}
    } THEN {
        static EWRAM_DATA struct PolymorphBackup expected[MAX_BATTLE_TRAINERS];
        static const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
        } command = {0, BS_PolymorphTarget};
        const u8 *savedInstr = gBattlescriptCurrInstr;
        u32 savedControllerFlags = gBattleControllerExecFlags;
        enum BattlerId savedAttacker = gBattlerAttacker, savedTarget = gBattlerTarget;
        enum Move savedMove = gCurrentMove;

        for (u32 battler = 0; battler < gBattlersCount; battler++)
        {
            enum BattleTrainer trainer = GetBattlerTrainer(battler);
            struct Pokemon *mon = GetBattlerMon(battler);
            expected[trainer].exp = GetMonData(mon, MON_DATA_EXP);
            GetMonData(mon, MON_DATA_NICKNAME, expected[trainer].nickname);
            for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            {
                expected[trainer].moves[slot] = GetMonData(mon, MON_DATA_MOVE1 + slot);
                expected[trainer].pp[slot] = GetMonData(mon, MON_DATA_PP1 + slot);
            }
            gBattlerAttacker = GetOppositeBattler(battler);
            gBattlerTarget = battler;
            gCurrentMove = MOVE_POLYMORPH;
            gBattlescriptCurrInstr = (const u8 *)&command;
            BS_PolymorphTarget();
            EXPECT_EQ((u32)GetBattlerPartyState(battler)->polymorphed, TRUE);
        }
        for (u32 trainer = 0; trainer < MAX_BATTLE_TRAINERS; trainer++)
        {
            const struct PolymorphBackup *saved = Test_GetPolymorphBackup(trainer, 0);
            EXPECT_EQ(saved->exp, expected[trainer].exp);
            EXPECT_EQ(StringCompare(saved->nickname, expected[trainer].nickname), 0);
            for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            {
                EXPECT_EQ(saved->moves[slot], expected[trainer].moves[slot]);
                EXPECT_EQ(saved->pp[slot], expected[trainer].pp[slot]);
            }
        }
        gBattlescriptCurrInstr = savedInstr;
        gBattleControllerExecFlags = savedControllerFlags;
        gBattlerAttacker = savedAttacker;
        gBattlerTarget = savedTarget;
        gCurrentMove = savedMove;
    }
}

SINGLE_BATTLE_TEST("Migration final Smeargle target uses committed slot zero rather than stale controller data")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_SMEARGLE);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        enum BattlerId battler = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)&gBattleResources->bufferA[battler][4];
        u32 savedControllerFlags = gBattleControllerExecFlags;
        gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
        EXPECT_EQ(KoankoSmeargleAnyMoveActive(battler), TRUE);
        moveInfo->moves[0] = MOVE_TACKLE;
        gBattleMons[battler].moves[0] = MOVE_RECOVER;
        gAiBattleData->chosenMoveIndex[battler] = 0;
        gAiBattleData->chosenTarget[battler] = GetOppositeBattler(battler);
        SetFinalChosenTarget(battler, FALSE);
        EXPECT_EQ(gBattlerTarget, battler);
        // The helper emitted a controller reply; no engine action consumes it here.
        gBattleControllerExecFlags = savedControllerFlags;
    }
}
