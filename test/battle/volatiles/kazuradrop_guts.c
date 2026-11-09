#include "global.h"
#include "test/battle.h"
#include "event_data.h"

SINGLE_BATTLE_TEST("Kazuradrop Guts (cheat death) expires after one full turn")
{
    u32 turns;
    u32 expectedGuts;

    // Kazuradrop sets kazuradropGuts = 2 on transformation.
    // battle_end_turn.c decrements it once per full turn.
    PARAMETRIZE { turns = 0; expectedGuts = 1; } // after end of transformation turn
    PARAMETRIZE { turns = 1; expectedGuts = 0; } // after one extra full turn
    PARAMETRIZE { turns = 2; expectedGuts = 0; } // remains expired

    GIVEN {
        // Bug Space must be active; in existing tests this is done via Kazuradrop's ability.
        // Keep both hits lethal without overflowing signed damage under terrain boosts.
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(100); Speed(100); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TRASH_CRUSH, MOVE_INFINITE_GROWTH, MOVE_CRACK_ICE); }
    } WHEN {
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
        for (u32 count = 0; count < turns; count++)
            TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
    } THEN {
        u32 guts = gBattleMons[B_POSITION_OPPONENT_LEFT].volatiles.kazuradropGuts;
        EXPECT_EQ(guts, expectedGuts);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Guts snaps internal HP to 50% immediately on cheat death")
{
    u16 species;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; }
    PARAMETRIZE { species = SPECIES_REGISTEEL; }
    PARAMETRIZE { species = SPECIES_XERNEAS; }
    GIVEN {
        // Make damage large enough that turn 2 would normally KO if not snapped.
        PLAYER(species) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(100); Speed(100); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TRASH_CRUSH, MOVE_INFINITE_GROWTH, MOVE_CRACK_ICE); }

    } WHEN {
        // Stable slots across the phase change; no Infinite Growth healing.
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
    } SCENE {
        MESSAGE("The Alter Ego's Guts activated!");
    } THEN {
        u32 snapHp = gBattleMons[B_POSITION_OPPONENT_LEFT].hp;
        u32 halfMaxHp = gBattleMons[B_POSITION_OPPONENT_LEFT].maxHP / 2;
        EXPECT_EQ(snapHp, halfMaxHp);
        EXPECT_EQ(opponent->maxHP, 40);
        EXPECT_EQ((u32)opponent->volatiles.kazuradropGuts, 0);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Guts snaps HP bar to 50% immediately on cheat death (visual)")
{
    u16 species;
    u16 terrain;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; terrain = STARTING_STATUS_NONE; }
    PARAMETRIZE { species = SPECIES_REGISTEEL; terrain = STARTING_STATUS_NONE; }
    PARAMETRIZE { species = SPECIES_XERNEAS; terrain = STARTING_STATUS_NONE; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; terrain = STARTING_STATUS_GRASSY_TERRAIN; }
    PARAMETRIZE { species = SPECIES_REGISTEEL; terrain = STARTING_STATUS_GRASSY_TERRAIN; }
    PARAMETRIZE { species = SPECIES_XERNEAS; terrain = STARTING_STATUS_GRASSY_TERRAIN; }
    // Initial Kazuradrop MaxHP is 20. Transformation doubles max HP to 40.
    // Cheat-death snap sets HP to maxHP / 2 => 20.
    GIVEN {
        VarSet(B_VAR_STARTING_STATUS, terrain);
        PLAYER(species) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(100); Speed(100); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TRASH_CRUSH, MOVE_INFINITE_GROWTH, MOVE_CRACK_ICE); }
    } WHEN {
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_TRASH_CRUSH, hit: FALSE); }
    } SCENE {
        HP_BAR(opponent, hp: 20);
        MESSAGE("The Alter Ego's Guts activated!");
    }
}
