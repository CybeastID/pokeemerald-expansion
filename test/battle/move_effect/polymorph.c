#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Polymorph changes the target's species to a random non-banned species")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_CASTORIA) { HP(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_POLYMORPH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLYMORPH, player);
//        MESSAGE("The enemy Caster transformed {B_DEF_NAME_WITH_PREFIX} into {B_BUFF1}!");
//       MESSAGE("“N'awwww, it's cuter like that! Even more so than I expected!”");
    } THEN {
        EXPECT_NE(player->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(player->species, SPECIES_CASTORIA);
    }
}

SINGLE_BATTLE_TEST("Polymorph fails on a Mega-Evolved target")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_GARDEVOIR) { Item(ITEM_GARDEVOIRITE); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_POLYMORPH); MOVE(opponent, MOVE_CELEBRATE, gimmick: GIMMICK_MEGA); }
    } SCENE {
        MESSAGE("The opposing Gardevoir used Celebrate!");
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(opponent->species, SPECIES_GARDEVOIR_MEGA);
        EXPECT_EQ(player->species, SPECIES_CASTORIA);
    }
}

SINGLE_BATTLE_TEST("Polymorph HP remains valid after species change")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(200); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_POLYMORPH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLYMORPH, player);
    } THEN {
        EXPECT_GT(opponent->hp, 0);
        EXPECT_LE(opponent->hp, opponent->maxHP);
        EXPECT_NE(opponent->species, SPECIES_WOBBUFFET);
        EXPECT_EQ(player->species, SPECIES_CASTORIA);
    }
}

SINGLE_BATTLE_TEST("Polymorph never produces a Legendary, Mythical, Ultra Beast, or Paradox species. Or Shedinja")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_POLYMORPH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLYMORPH, player);
    } THEN {
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isLegendary, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isMythical, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isUltraBeast, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isParadox, FALSE);
        EXPECT_NE(opponent->species, SPECIES_SHEDINJA);
        EXPECT_LE(opponent->species, SPECIES_FALINKS_MEGA);
        EXPECT_EQ(player->species, SPECIES_CASTORIA);
    }
}