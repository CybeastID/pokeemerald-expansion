#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_setup.h"
#include "data.h"

// "Razeluxe Tactics": fixtures for the trainer-gated TRAINER_RAZELUXE policy (AR_RazeMoves and the
// virtual-spell helpers) in src/battle_ai_main.c. AUTHORED WITHOUT A BUILD. Expected scores come from the
// proposal's arithmetic (base 100; best damage +3; secure KO +8; status while a finish exists -18;
// Authority 6 + swing/2 capped at +30, or -40 when it would fail). Assertions are relational.
//
// NOT COVERED YET: Alakazam Trace/Unaware setup, Hoopa-Unbound Drain Punch/Fury/Protect, Castoria Wish
// ranking (More Wishes / Endurance / Reduction / Debilitation), emergency Perish Revivify, Polymorph
// selection, Fireball vs ordinary score scale, summoned-TX virtual Wishes, callback isolation.

extern AiScoreFunc sDynamicAiFunc;
s32 AI_RazeluxeTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Mewtwo uses Authority on a healthy stage-neutral target (base and Mega)")
{
    u32 species;
    PARAMETRIZE { species = SPECIES_MEWTWO; }
    PARAMETRIZE { species = SPECIES_MEWTWO_MEGA_Y; } // regression: the old dispatch switched on the raw species
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(species) { Level(102); Speed(1000);
            Moves(MOVE_SPACIAL_REND, MOVE_ROAR_OF_TIME, MOVE_SHADOW_FORCE, MOVE_DMS_PREROGATIVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_DMS_PREROGATIVE, MOVE_SPACIAL_REND); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Mewtwo finishes instead of using Authority")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_MEWTWO_MEGA_Y) { Level(102); Speed(1000);
            Moves(MOVE_SPACIAL_REND, MOVE_ROAR_OF_TIME, MOVE_SHADOW_FORCE, MOVE_DMS_PREROGATIVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_SPACIAL_REND, MOVE_DMS_PREROGATIVE); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Authority is rejected when the target has Contrary or Illuminate (Gen 9)")
{
    u32 ability;
    PARAMETRIZE { ability = ABILITY_CONTRARY; }
    PARAMETRIZE { ability = ABILITY_ILLUMINATE; }
    ASSUME(GetConfig(B_ILLUMINATE_EFFECT) >= GEN_9);
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Ability(ability); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_MEWTWO_MEGA_Y) { Level(102); Speed(1000);
            Moves(MOVE_SPACIAL_REND, MOVE_ROAR_OF_TIME, MOVE_SHADOW_FORCE, MOVE_DMS_PREROGATIVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_SPACIAL_REND, MOVE_DMS_PREROGATIVE); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Mewtwo avoids Authority when its own Ability is Contrary")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_MEWTWO) { Level(102); Speed(1000); Ability(ABILITY_CONTRARY);
            Moves(MOVE_SPACIAL_REND, MOVE_ROAR_OF_TIME, MOVE_SHADOW_FORCE, MOVE_DMS_PREROGATIVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_SPACIAL_REND, MOVE_DMS_PREROGATIVE); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Raze Kirlia burns a physical attacker")
{
    // No Guard: neither Guts nor Flare Boost, so burn is a real objective. The player is slower, so no
    // incoming hit lands before the burn (AR_Pressure beforeOnly == 0). FRAGILE: Z-move/gimmick choice.
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_MACHAMP) { HP(1000); Speed(1); Ability(ABILITY_NO_GUARD); Moves(MOVE_BODY_SLAM); }
        OPPONENT(SPECIES_RAZE_KIRLIA) { Level(100); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_DAZZLING_GLEAM, MOVE_WILL_O_WISP, MOVE_THUNDER_WAVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_BODY_SLAM); SCORE_GT(opponent, MOVE_WILL_O_WISP, MOVE_PSYCHIC); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Raze Kirlia does not burn a Guts attacker")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_MACHAMP) { HP(1000); Speed(1); Ability(ABILITY_GUTS); Moves(MOVE_BODY_SLAM); }
        OPPONENT(SPECIES_RAZE_KIRLIA) { Level(100); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_DAZZLING_GLEAM, MOVE_WILL_O_WISP, MOVE_THUNDER_WAVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_BODY_SLAM); SCORE_GT(opponent, MOVE_PSYCHIC, MOVE_WILL_O_WISP); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Raze Kirlia does not paralyze an already-statused target")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Status1(STATUS1_BURN); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_RAZE_KIRLIA) { Level(100); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_DAZZLING_GLEAM, MOVE_WILL_O_WISP, MOVE_THUNDER_WAVE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_PSYCHIC, MOVE_THUNDER_WAVE); }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Castoria revives a fainted ally when safe, but finishes first when it can")
{
    u32 playerHp;
    PARAMETRIZE { playerHp = 1000; }
    PARAMETRIZE { playerHp = 1; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(playerHp); MaxHP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Level(105); Speed(1000); Ability(ABILITY_MAGIC_GUARD);
            Moves(MOVE_REVIVAL_BLESSING_2, MOVE_CALM_MIND, MOVE_EARTH_POWER, MOVE_MOONBLAST); }
        OPPONENT(SPECIES_PICHU) { HP(0); Speed(1); }
    } WHEN {
        if (playerHp == 1000)
        {
            TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_REVIVAL_BLESSING_2, MOVE_EARTH_POWER); }
        }
        else
        {
            TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_EARTH_POWER, MOVE_REVIVAL_BLESSING_2); }
        }
    }
}

AI_SINGLE_BATTLE_TEST("Razeluxe Tactics Castoria does not Revivify with no fainted ally")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_RAZELUXE;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_RAZELUXE));
        sDynamicAiFunc = AI_RazeluxeTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { Level(105); Speed(1000); Ability(ABILITY_MAGIC_GUARD);
            Moves(MOVE_REVIVAL_BLESSING_2, MOVE_CALM_MIND, MOVE_EARTH_POWER, MOVE_MOONBLAST); }
        OPPONENT(SPECIES_PICHU) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_EARTH_POWER, MOVE_REVIVAL_BLESSING_2); }
    }
}
