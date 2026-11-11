#include "global.h"
#include "test/battle.h"
#include "castoria_name_theft.h"

SINGLE_BATTLE_TEST("Polymorph changes the target's species to a random non-banned species")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_POLYMORPH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLYMORPH, player);
//        MESSAGE("The enemy Caster transformed {B_DEF_NAME_WITH_PREFIX} into {B_BUFF1}!");
//       MESSAGE("“N'awwww, it's cuter like that! Even more so than I expected!”");
    } THEN {
        EXPECT_NE(opponent->species, SPECIES_WOBBUFFET);
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
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isSubLegendary, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isRestrictedLegendary, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isMythical, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isUltraBeast, FALSE);
        EXPECT_EQ((bool8)gSpeciesInfo[opponent->species].isParadox, FALSE);
        EXPECT_NE(opponent->species, SPECIES_SHEDINJA);
        EXPECT_LE(opponent->species, SPECIES_FALINKS_MEGA);
        EXPECT_EQ(player->species, SPECIES_CASTORIA);
    }
}
// Stage 03.1: the sampled best-scoring species used to be overwritten by a random fallback.
// Selection only runs when a player-side mon is polymorphed, so Castoria is the opponent here.

TEST("Polymorph best-sample picker returns the highest score")
{
    u32 scores[] = { 10, 55, 30, 54, 7 };
    EXPECT_EQ(PolymorphPickBestSample(scores, ARRAY_COUNT(scores)), 1);
}

TEST("Polymorph best-sample picker keeps the earliest of equal best scores")
{
    u32 scores[] = { 10, 55, 30, 55, 55 };
    EXPECT_EQ(PolymorphPickBestSample(scores, ARRAY_COUNT(scores)), 1);
}

TEST("Polymorph best-sample picker handles a single sample")
{
    u32 scores[] = { 0 };
    EXPECT_EQ(PolymorphPickBestSample(scores, 1), 0);
}

SINGLE_BATTLE_TEST("Polymorph on a player mon returns the best-scoring sample, not a random overwrite")
{
    // Injected draws (all inside the 168-273 hardware band for a threat-541 target). Without injection the
    // battle test runner returns one fixed species for every draw, which is outside the band: sampleCount 0.
    static const u16 draws[] = { SPECIES_CATERPIE, SPECIES_METAPOD, SPECIES_WEEDLE, SPECIES_KAKUNA, SPECIES_PIDGEY };
    GIVEN {
        Test_SetPolymorphDrawSpecies(draws, ARRAY_COUNT(draws));
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_WOBBUFFET) { HP(100); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_POLYMORPH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POLYMORPH, opponent);
    } THEN {
        const struct PolymorphSelectionDebug *dbg = Test_GetPolymorphSelectionDebug();
        EXPECT_GT(dbg->sampleCount, 0);   // the band produced candidates; otherwise this test proves nothing
        EXPECT_NE(dbg->bestSampled, SPECIES_NONE);
        EXPECT_EQ(dbg->chosen, dbg->bestSampled);
        EXPECT_EQ(player->species, dbg->chosen);
        bool32 fromDraws = FALSE;
        for (u32 i = 0; i < ARRAY_COUNT(draws); i++)
            if (dbg->chosen == draws[i])
                fromDraws = TRUE;
        EXPECT_EQ(fromDraws, TRUE);       // not a random fallback species
        Test_SetPolymorphDrawSpecies(NULL, 0);
    }
}

SINGLE_BATTLE_TEST("Polymorph on a player mon keeps the HP and species invariants after selection")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POLYMORPH) == EFFECT_POLYMORPH);
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(200); HP(100); }
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_POLYMORPH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_POLYMORPH); }
    } THEN {
        EXPECT_NE(player->species, SPECIES_WOBBUFFET);
        EXPECT_GT(player->hp, 0);
        EXPECT_LE(player->hp, player->maxHP);
        EXPECT_EQ(opponent->species, SPECIES_CASTORIA);
    }
}
