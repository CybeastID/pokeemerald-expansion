#include "global.h"
#include "test/battle.h"

ASSUMPTIONS
{
    ASSUME(GetMoveEffect(MOVE_REVIVAL_BLESSING_2) == EFFECT_REVIVAL_BLESSING_2);
}

/* SINGLE_BATTLE_TEST("Revivify revives a chosen fainted party member for the player")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { HP(0); }
        PLAYER(SPECIES_WYNAUT) { HP(0); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_REVIVAL_BLESSING_2, partyIndex:2); }
    } SCENE {
        MESSAGE("Wobbuffet used Revivify!");
        MESSAGE("Wynaut was revived and is ready to fight again!");
    }
}
*/
SINGLE_BATTLE_TEST("Revivify revives a fainted party member for an opponent")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) {Level(105); Moves(MOVE_REVIVAL_BLESSING_2);}
        OPPONENT(SPECIES_PICHU) { HP(0); }
        OPPONENT(SPECIES_PIKACHU) { HP(0); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_REVIVAL_BLESSING_2, partyIndex:1); }
        TURN { MOVE(opponent, MOVE_REVIVAL_BLESSING_2, partyIndex:2); }
        TURN { SWITCH(opponent, 1); }
        
    } SCENE {
        // Turn 1
        MESSAGE("The opposing Caster? used Revivify!");
        MESSAGE("Pichu was revived and is ready to fight again!");
        // Turn 2
        MESSAGE("The opposing Caster? used Revivify!");
        MESSAGE("Pikachu was revived and is ready to fight again!");
        // Turn 3
        MESSAGE("2 withdrew Caster?!");
        MESSAGE("2 sent out Pichu!");
    }
}

/* AI_SINGLE_BATTLE_TEST("Revivify chosen by an opponent")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) {Level(105); Moves(MOVE_REVIVAL_BLESSING_2, MOVE_FIRE_BLAST, MOVE_GROWTH, MOVE_HYDRO_PUMP);}
        OPPONENT(SPECIES_PICHU) { HP(0); }
        OPPONENT(SPECIES_PIKACHU) { HP(0); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_REVIVAL_BLESSING_2, partyIndex:1); }
        TURN { EXPECT_MOVE(opponent, MOVE_REVIVAL_BLESSING_2, partyIndex:2); }
        TURN { EXPECT_SWITCH(opponent, 1); }
        
    } SCENE {
        // Turn 1
        MESSAGE("The opposing Castoria used Revivify!");
        MESSAGE("Pichu was revived and is ready to fight again!");
        // Turn 2
        MESSAGE("The opposing Castoria used Revivify!");
        MESSAGE("Pikachu was revived and is ready to fight again!");
    }
}

*/
