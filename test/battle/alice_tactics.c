#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_setup.h"
#include "data.h"

// "Alice Tactics": fixtures for the trainer-gated TRAINER_ALICE2 policy (AR_AliceMoves) in
// src/battle_ai_main.c. AUTHORED WITHOUT A BUILD: expected scores come from the proposal's arithmetic
// (base 100; best damage +3; secure KO +8; layer break/reveal +3; setup +6..; status while a finish
// exists -18). Assertions are relational (SCORE_GT between two moves) with wide margins.
// Production kits come from src/data/trainers.party (TRAINER_ALICE2).
//
// NOT COVERED YET: Kazuradrop (opener, emergency slot, actor-bit move swap, post-transform choices),
// Umbreon pending-Wish/Protect timing, Gothitelle Rest budget, Palkia Draco Meteor comparison,
// Hoopa Nasty Plot/Protect-bypass, callback isolation (Alice3 / unrelated trainers).

extern AiScoreFunc sDynamicAiFunc;
s32 AI_AliceTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);


AI_SINGLE_BATTLE_TEST("Alice Tactics Espeon takes a secure KO over Calm Mind")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_ESPEON) { Ability(ABILITY_MAGIC_BOUNCE); Item(ITEM_LIFE_ORB); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_DAZZLING_GLEAM, MOVE_CALM_MIND); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE);
               EXPECT_MOVES(opponent, MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_DAZZLING_GLEAM);
               SCORE_GT(opponent, MOVE_PSYCHIC, MOVE_CALM_MIND); }
    } SCENE { HP_BAR(player, hp: 0); }
}

AI_SINGLE_BATTLE_TEST("Alice Tactics Espeon sets up Calm Mind when it is safe and an attack pays off")
{
    // Player only Celebrates: no incoming damage, no KO available. Expected: Calm Mind 100 + 6 (AR_Setup)
    // vs Psychic 100 + 3 (best damage). FRAGILE if ordinary passes add to Psychic.
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_ESPEON) { Ability(ABILITY_MAGIC_BOUNCE); Item(ITEM_LIFE_ORB); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_DAZZLING_GLEAM, MOVE_CALM_MIND); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_CALM_MIND, MOVE_PSYCHIC); }
    }
}

AI_SINGLE_BATTLE_TEST("Alice Tactics Espeon does not set up into a lethal special attacker")
{
    // Gengar's maximum-roll Shadow Ball is more than half of Espeon's HP: AR_Setup returns -10.
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_GENGAR) { HP(1000); Speed(1); SpAttack(999); Moves(MOVE_SHADOW_BALL); }
        OPPONENT(SPECIES_ESPEON) { Ability(ABILITY_MAGIC_BOUNCE); Item(ITEM_LIFE_ORB); Speed(1000);
            Moves(MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_DAZZLING_GLEAM, MOVE_CALM_MIND); }
    } WHEN {
        TURN { MOVE(player, MOVE_SHADOW_BALL); SCORE_GT(opponent, MOVE_PSYCHIC, MOVE_CALM_MIND); }
    }
}

AI_SINGLE_BATTLE_TEST("Alice Tactics Umbreon finishes with Foul Play instead of Toxic")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_UMBREON) { Ability(ABILITY_SYNCHRONIZE); Item(ITEM_LEFTOVERS); Speed(1000);
            Moves(MOVE_FOUL_PLAY, MOVE_WISH, MOVE_PROTECT, MOVE_TOXIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_FOUL_PLAY);
               SCORE_GT(opponent, MOVE_FOUL_PLAY, MOVE_TOXIC); }
    } SCENE { HP_BAR(player, hp: 0); }
}

AI_SINGLE_BATTLE_TEST("Alice Tactics Umbreon does not Toxic a Poison Heal target")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_BRELOOM) { HP(1000); Speed(1); Ability(ABILITY_POISON_HEAL); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_UMBREON) { Ability(ABILITY_SYNCHRONIZE); Item(ITEM_LEFTOVERS); Speed(1000);
            Moves(MOVE_FOUL_PLAY, MOVE_WISH, MOVE_PROTECT, MOVE_TOXIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_FOUL_PLAY, MOVE_TOXIC); }
    }
}

AI_SINGLE_BATTLE_TEST("Alice Tactics Umbreon does not Wish at full HP with no wounded reserve")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_ALICE2;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_ALICE2));
        sDynamicAiFunc = AI_AliceTrainer;
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_UMBREON) { Ability(ABILITY_SYNCHRONIZE); Item(ITEM_LEFTOVERS); Speed(1000);
            Moves(MOVE_FOUL_PLAY, MOVE_WISH, MOVE_PROTECT, MOVE_TOXIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); SCORE_GT(opponent, MOVE_FOUL_PLAY, MOVE_WISH); }
    }
}
