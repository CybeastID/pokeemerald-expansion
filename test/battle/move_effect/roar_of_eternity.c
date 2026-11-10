#include "global.h"
#include "test/battle.h"

// Timeline these tests encode (battle_script_commands.c MOVE_EFFECT_TIMESTOP / recharge,
// battle_main.c end-of-turn countdown, battle_util.c CancelerTimestop / CancelerRecharge):
//   T1  Roar of Eternity hits: user rechargeTimer = 2; on a 50% proc target timestop = 3.
//       End of T1: rechargeTimer 2 -> 1, timestop 3 -> 2.
//   T2  User must recharge. A timestopped target's action resolves into nothing (silently).
//       End of T2: rechargeTimer 1 -> 0, timestop 2 -> 1 (colors restored).
//   T3  Both act again. timestop 1 still blocks a re-proc; end of T3 it reaches 0.
// So Time Stop lands exactly on the recharge turn: nobody acts on T2.

ASSUMPTIONS
{
    ASSUME(MoveHasAdditionalEffectSelf(MOVE_ROAR_OF_ETERNITY, MOVE_EFFECT_RECHARGE) == TRUE);
    ASSUME(MoveHasAdditionalEffect(MOVE_ROAR_OF_ETERNITY, MOVE_EFFECT_TIMESTOP) == TRUE);
    ASSUME(GetMovePower(MOVE_ROAR_OF_ETERNITY) == 150);
    ASSUME(GetMoveAccuracy(MOVE_ROAR_OF_ETERNITY) == 90);
}

// Shield Dust nulls every non-self additional effect (SetMoveEffect), so it removes the
// Timestop proc but leaves the self-targeted recharge alone. This isolates recharge.
SINGLE_BATTLE_TEST("Roar of Eternity makes the user unable to attack for exactly one turn")
{
    GIVEN {
        PLAYER(SPECIES_DIALGA_ORIGIN) { Moves(MOVE_ROAR_OF_ETERNITY, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_SHIELD_DUST); MaxHP(500); HP(500); SpDefense(500); }
    } WHEN {
        TURN { MOVE(player, MOVE_ROAR_OF_ETERNITY, hit: TRUE); }
        TURN { SKIP_TURN(player); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR_OF_ETERNITY, player);
        MESSAGE("Dialga must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
    }
}

SINGLE_BATTLE_TEST("Roar of Eternity inflicts Time Stop 50% of the time")
{
    PASSES_RANDOMLY(50, 100, RNG_SECONDARY_EFFECT_2);
    GIVEN {
        PLAYER(SPECIES_DIALGA_ORIGIN) { Moves(MOVE_ROAR_OF_ETERNITY, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(500); HP(500); SpDefense(500); }
    } WHEN {
        TURN { MOVE(player, MOVE_ROAR_OF_ETERNITY, hit: TRUE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        // On a proc, recharge and Time Stop bypass both action prompts on T2.
        // Playback reaches T3 before it ends, so both T1 and T2 have counted down.
        EXPECT_EQ((u32)opponent->volatiles.timestop, 1);
    }
}

// Both effects on the same hit: recharge still operates while the target is Time Stopped,
// and Time Stop covers exactly the recharge turn. Runs under the 50% proc; the non-proc
// outcome fails the T2 SKIP_TURN(opponent), which is what PASSES_RANDOMLY expects.
SINGLE_BATTLE_TEST("Roar of Eternity recharge and Time Stop resolve on the same turn")
{
    PASSES_RANDOMLY(50, 100, RNG_SECONDARY_EFFECT_2);
    GIVEN {
        PLAYER(SPECIES_DIALGA_ORIGIN) { Moves(MOVE_ROAR_OF_ETERNITY, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(500); HP(500); SpDefense(500); }
    } WHEN {
        TURN { MOVE(player, MOVE_ROAR_OF_ETERNITY, hit: TRUE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SKIP_TURN(player); SKIP_TURN(opponent); }
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROAR_OF_ETERNITY, player);
        MESSAGE("Dialga must recharge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.timestop, 0);
    }
}
