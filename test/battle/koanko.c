#include "global.h"
#include "test/battle.h"
#include "battle_koanko.h"
#include "battle_setup.h"
#include "data.h"
#include "malloc.h"
#include "trainer_break_bars.h"

TEST("Koanko replacement creates fresh opponents while preserving player, field, inventory and trainer identity")
{
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gAiPartyData = AllocZeroed(sizeof(*gAiPartyData));
    gAiLogicData = AllocZeroed(sizeof(*gAiLogicData));
    gBattleHistory = AllocZeroed(sizeof(*gBattleHistory));
    gBattlersCount = 2;
    gBattleStruct->koankoPhase = KOANKO_CONJURING;
    gBattleStruct->givenExpMons = 0x3F;
    gBattleStruct->opponentMonCanTera = 0x3F;
    gBattleStruct->opponentMonCanDynamax = 0x3F;
    gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_DYNAMAX] = TRUE;
    gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_TERA] = TRUE;
    gBattleStruct->gimmick.activated[B_POSITION_PLAYER_LEFT][GIMMICK_TERA] = TRUE;
    gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarConfig = &gTrainerBreakBarProfiles[0];
    gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarsRemaining = 1;
    gAiPartyData->mons[B_SIDE_PLAYER][0].species = SPECIES_PICHU;
    gBattleHistory->trainerItems[0] = ITEM_POTION;
    gBattleHistory->itemsNo = 1;
    gBattleWeather = B_WEATHER_RAIN;
    gSideStatuses[B_SIDE_OPPONENT] = SIDE_STATUS_REFLECT;
    gSideTimers[B_SIDE_OPPONENT].spikesAmount = 2;
    gWishFutureKnock.knockedOffMons[B_SIDE_OPPONENT] = 0x3F;
    gWishFutureKnock.futureSightCounter[B_POSITION_OPPONENT_LEFT] = 2;
    gBattleResults.opponentFaintCounter = 6;
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarConfig = &gTrainerBreakBarProfiles[4];
        gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarSavedHp[0] = 42;
        gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarSavedMaxHp[0] = 100;
        gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarsRemaining = 5;
        gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarItemUsed = TRUE;
        gAiPartyData->mons[B_SIDE_OPPONENT][i].species = SPECIES_WOBBUFFET;
        gAiPartyData->mons[B_SIDE_OPPONENT][i].isFainted = TRUE;
    }
    ConjureKoankoTeam();
    static const u16 species[] = {SPECIES_SMEARGLE, SPECIES_TX_ROLLBACK, SPECIES_CALYREX_ICE, SPECIES_ETERNATUS, SPECIES_DIALGA_ORIGIN, SPECIES_ARCEUS};
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        EXPECT_EQ(GetMonData(&gEnemyParty[i], MON_DATA_SPECIES), species[i]);
        EXPECT_EQ(GetMonData(&gEnemyParty[i], MON_DATA_LEVEL), 100 + i);
        EXPECT_EQ(GetMonData(&gEnemyParty[i], MON_DATA_HP), GetMonData(&gEnemyParty[i], MON_DATA_MAX_HP));
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarConfig, NULL);
        EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarSavedMaxHp[0], 0);
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_OPPONENT][i].breakBarItemUsed, FALSE);
        EXPECT_EQ((u32)gAiPartyData->mons[B_SIDE_OPPONENT][i].isFainted, FALSE);
    }
    EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_CONJURED_TEAM);
    EXPECT_EQ(gBattleStruct->givenExpMons, 0);
    EXPECT_EQ((u32)gBattleStruct->opponentMonCanTera, 1 << 2);
    EXPECT_EQ((u32)gBattleStruct->opponentMonCanDynamax, 0);
    EXPECT_EQ(gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_DYNAMAX], TRUE);
    // Only the opponent's Tera-used flag is renewed; the player's Tera usage is untouched.
    EXPECT_EQ(gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_TERA], FALSE);
    EXPECT_EQ(gBattleStruct->gimmick.activated[B_POSITION_PLAYER_LEFT][GIMMICK_TERA], TRUE);
    EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarsRemaining, 1);
    EXPECT_EQ(gAiPartyData->mons[B_SIDE_PLAYER][0].species, SPECIES_PICHU);
    EXPECT_EQ(gBattleHistory->trainerItems[0], ITEM_POTION);
    EXPECT_EQ(gBattleHistory->itemsNo, 1);
    EXPECT_EQ((u32)gSideTimers[B_SIDE_OPPONENT].spikesAmount, 2);
    EXPECT_EQ(gBattleWeather, B_WEATHER_RAIN);
    EXPECT_EQ(gSideStatuses[B_SIDE_OPPONENT], SIDE_STATUS_REFLECT);
    EXPECT_EQ(gWishFutureKnock.futureSightCounter[B_POSITION_OPPONENT_LEFT], 2);
    EXPECT_EQ(gWishFutureKnock.knockedOffMons[B_SIDE_OPPONENT], 0);
    EXPECT_EQ(gBattleResults.opponentFaintCounter, 6);
    EXPECT_EQ(gEnemyPartyCount, 6);
    // A second call cannot regenerate a defeated conjured Pokemon.
    u16 hp = 0;
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    ConjureKoankoTeam();
    EXPECT_EQ(GetMonData(&gEnemyParty[0], MON_DATA_HP), 0);
    // ...and cannot renew the Tera opportunity a second time once it has been consumed.
    gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_TERA] = TRUE;
    ConjureKoankoTeam();
    EXPECT_EQ(gBattleStruct->gimmick.activated[B_POSITION_OPPONENT_LEFT][GIMMICK_TERA], TRUE);
    Free(gBattleHistory);
    Free(gAiLogicData);
    Free(gAiPartyData);
    Free(gBattleStruct);
    gBattleHistory = NULL;
    gAiLogicData = NULL;
    gAiPartyData = NULL;
    gBattleStruct = NULL;
}

SINGLE_BATTLE_TEST("Koanko conjures a second team and victory requires twelve genuine faints")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_HERACROSS) { Attack(9999); Speed(9999); Ability(ABILITY_SKILL_LINK); Moves(MOVE_ARM_THRUST); }
        for (u32 i = 0; i < PARTY_SIZE; i++)
            OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        for (u32 i = 1; i < PARTY_SIZE; i++)
            TURN { MOVE(player, MOVE_ARM_THRUST); SEND_OUT(opponent, i); }
        TURN { MOVE(player, MOVE_ARM_THRUST); SEND_OUT(opponent, 0); }
        for (u32 i = 1; i < PARTY_SIZE; i++)
            TURN { MOVE(player, MOVE_ARM_THRUST); SEND_OUT(opponent, i); }
        TURN { MOVE(player, MOVE_ARM_THRUST); }
    } SCENE {
        for (u32 i = 0; i < PARTY_SIZE; i++)
            MESSAGE("The opposing Wobbuffet fainted!");
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
        MESSAGE("The opposing Smeargle fainted!");
        MESSAGE("The opposing TX Rollback fainted!");
        MESSAGE("The opposing Calyrex fainted!");
        MESSAGE("The opposing Eternatus fainted!");
        MESSAGE("The opposing Dialga fainted!");
        MESSAGE("The opposing Arceus fainted!");
        NOT MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 12);
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_CONJURED_TEAM);
        EXPECT_EQ(TRAINER_BATTLE_PARAM.opponentA, TRAINER_KOANKO);
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
    }
}

SINGLE_BATTLE_TEST("Koanko conjures after residual damage defeats her final Pokemon")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Status1(STATUS1_POISON); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); SEND_OUT(opponent, 0); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet fainted!");
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
    }
}

SINGLE_BATTLE_TEST("Koanko conjures after recoil defeats her final Pokemon")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Moves(MOVE_DOUBLE_EDGE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_DOUBLE_EDGE); SEND_OUT(opponent, 0); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet fainted!");
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
    }
}

SINGLE_BATTLE_TEST("Koanko conjured lead takes existing hazards without inheriting the old Pokemon's status")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPIKES); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Status1(STATUS1_POISON); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPIKES); MOVE(opponent, MOVE_CELEBRATE); SEND_OUT(opponent, 0); }
    } SCENE {
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
        HP_BAR(opponent);
        MESSAGE("The opposing Smeargle was hurt by the spikes!");
    } THEN {
        EXPECT_EQ(opponent->hp, opponent->maxHP - opponent->maxHP / 8);
        EXPECT_EQ(opponent->status1, 0);
    }
}

SINGLE_BATTLE_TEST("Koanko conjures after simultaneous faints when the player has a reserve")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_VOLTORB) { Attack(9999); Moves(MOVE_EXPLOSION); }
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); SEND_OUT(opponent, 0); }
    } SCENE {
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(player->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(gBattleResults.opponentFaintCounter, 1);
    }
}

SINGLE_BATTLE_TEST("Koanko does not conjure when simultaneous faints exhaust the player")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_VOLTORB) { Attack(9999); Moves(MOVE_EXPLOSION); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_EXPLOSION); }
    } SCENE {
        NOT MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_ORDINARY_TEAM);
    }
}

SINGLE_BATTLE_TEST("Koanko conjures after Destiny Bond with a surviving player reserve")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(100); Moves(MOVE_DESTINY_BOND); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_SCRATCH); }
    } WHEN {
        TURN { MOVE(player, MOVE_DESTINY_BOND); MOVE(opponent, MOVE_SCRATCH); SEND_OUT(player, 1); SEND_OUT(opponent, 0); }
    } SCENE {
        MESSAGE("Wobbuffet took its attacker down with it!");
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_CONJURED_TEAM);
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI sends the conjured lead and selects from its new moves")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_SEISMIC_TOSS, MOVE_NIGHT_SHADE, MOVE_SPORE, MOVE_ENDEAVOR); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVES(opponent, MOVE_SEISMIC_TOSS, MOVE_NIGHT_SHADE, MOVE_SPORE, MOVE_ENDEAVOR); }
    } SCENE {
        MESSAGE("You thought that was all?\nThen watch what I can create!{PAUSE_UNTIL_PRESS}");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(gAiPartyData->count[B_SIDE_OPPONENT], PARTY_SIZE);
        // The living conjured occupant cannot be moved out of sequence, and the AI now follows party order.
        EXPECT(HasNoOrderChange(B_POSITION_OPPONENT_LEFT));
        EXPECT(gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] & AI_FLAG_SEQUENCE_SWITCHING);
    }
}

// NOT RUN when authored. Shaped on the AI test above (same transition setup).
AI_SINGLE_BATTLE_TEST("Koanko living conjured Pokemon cannot be phazed out of sequence")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(1000); Moves(MOVE_SEISMIC_TOSS, MOVE_ROAR); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(50); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); EXPECT_SEND_OUT(opponent, 0); }
        TURN { MOVE(player, MOVE_ROAR); }
    } THEN {
        // Roar would normally drag in another of the five living conjured Pokemon.
        EXPECT_EQ(opponent->species, SPECIES_SMEARGLE);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT], 0);
    }
}


SINGLE_BATTLE_TEST("Koanko Loaded Dice allows 102 and 101 percent damage and retains 85 percent minimum")
{
    s16 damage;
    u32 roll;
    s16 expectedDamage;

    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;

    PARAMETRIZE { roll = 0;  expectedDamage = 173; } // 102%
    PARAMETRIZE { roll = 1;  expectedDamage = 171; } // 101%
    PARAMETRIZE { roll = 17; expectedDamage = 144; } // 85%

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Level(100);
            Defense(100);
            HP(1000);
            MaxHP(1000);
            Moves(MOVE_CELEBRATE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Level(100);
            Attack(500);
            Moves(MOVE_SCRATCH);
        }
    } WHEN {
        TURN {
            MOVE(player, MOVE_CELEBRATE);
            MOVE(opponent, MOVE_SCRATCH,
                 WITH_RNG(RNG_DAMAGE_MODIFIER, roll),
                 criticalHit: FALSE);
        }
    } SCENE {
        HP_BAR(player, captureDamage: &damage);
    } THEN {
        EXPECT_EQ(expectedDamage, damage);
    }
}

SINGLE_BATTLE_TEST("Koanko Loaded Dice does not affect ordinary trainers")
{
    s16 damage;
    u32 roll;
    s16 expectedDamage;

    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ROXANNE_1;

    PARAMETRIZE { roll = 0;  expectedDamage = 170; } // 100%
    PARAMETRIZE { roll = 15; expectedDamage = 144; } // 85%

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Level(100);
            Defense(100);
            HP(1000);
            MaxHP(1000);
            Moves(MOVE_CELEBRATE);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Level(100);
            Attack(500);
            Moves(MOVE_SCRATCH);
        }
    } WHEN {
        TURN {
            MOVE(player, MOVE_CELEBRATE);
            MOVE(opponent, MOVE_SCRATCH,
                 WITH_RNG(RNG_DAMAGE_MODIFIER, roll),
                 criticalHit: FALSE);
        }
    } SCENE {
        HP_BAR(player, captureDamage: &damage);
    } THEN {
        EXPECT_EQ(expectedDamage, damage);
    }
}

SINGLE_BATTLE_TEST("Koanko Loaded Dice does not improve the player's damage")
{
    s16 damage;
    u32 roll;
    s16 expectedDamage;

    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;

    PARAMETRIZE { roll = 0;  expectedDamage = 170; } // 100%
    PARAMETRIZE { roll = 15; expectedDamage = 144; } // 85%

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            Level(100);
            Attack(500);
            Moves(MOVE_SCRATCH);
        }
        OPPONENT(SPECIES_WOBBUFFET) {
            Level(100);
            Defense(100);
            HP(1000);
            MaxHP(1000);
            Moves(MOVE_CELEBRATE);
        }
    } WHEN {
        TURN {
            MOVE(player, MOVE_SCRATCH,
                 WITH_RNG(RNG_DAMAGE_MODIFIER, roll),
                 criticalHit: FALSE);
            MOVE(opponent, MOVE_CELEBRATE);
        }
    } SCENE {
        HP_BAR(opponent, captureDamage: &damage);
    } THEN {
        EXPECT_EQ(expectedDamage, damage);
    }
}
