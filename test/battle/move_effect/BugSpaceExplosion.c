#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Bug Space blocks Explosion but still causes user to faint without printing bad message.")
{
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { Moves(MOVE_EXPLOSION); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_KAZURADROP) { Level(105); Ability(ABILITY_BUG_SPACE); }
        OPPONENT(SPECIES_GOTHITELLE) { HP(0); }
    } WHEN {
        TURN { MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Registeel used Explosion!");
        MESSAGE("You can't use that move anymore!");
        MESSAGE("Registeel fainted!");
        SEND_IN_MESSAGE("Wynaut");
        NONE_OF {
            MESSAGE("2 is about to send out Gothitelle. Will you switch your Pokémon?");
        }
    } THEN {
        EXPECT_EQ(gBattleMons[B_POSITION_PLAYER_LEFT].hp, 0);
    }
}
SINGLE_BATTLE_TEST("Bug Space blocks Explosion but still causes the user to faint")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_EXPLOSION); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { MOVE(player, MOVE_EXPLOSION); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Explosion!");
        MESSAGE("You can't use that move anymore!");
        HP_BAR(player, hp: 0);
        MESSAGE("Wobbuffet fainted!");
        SEND_IN_MESSAGE("Wynaut");
    }
}