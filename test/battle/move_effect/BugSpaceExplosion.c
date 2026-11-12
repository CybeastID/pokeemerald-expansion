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
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(player->species, SPECIES_WYNAUT);
        EXPECT_GT(player->hp, 0);
        EXPECT_EQ(opponent->hp, opponent->maxHP);
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
        HP_BAR(player, hp: 0);
        MESSAGE("You can't use that move anymore!");
        MESSAGE("Wobbuffet fainted!");
        SEND_IN_MESSAGE("Wynaut");
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 0);
        EXPECT_EQ(opponent->hp, opponent->maxHP);
    }
}
