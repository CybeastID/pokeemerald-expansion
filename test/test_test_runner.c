#include "global.h"
#include "test/test.h"
#include "test/battle.h"
#include "event_data.h"
#include "difficulty.h"

SINGLE_BATTLE_TEST("Test event data starts clean after another test estimates field settings")
{
    GIVEN {
        EXPECT_EQ(VarGet(TESTING_VAR_UNUSED_3), 0);
        EXPECT_EQ(VarGet(B_VAR_STARTING_STATUS), STARTING_STATUS_NONE);
        EXPECT_EQ(GetCurrentDifficultyLevel(), DIFFICULTY_NORMAL);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { ; }
    } SCENE {
        NOT MESSAGE("The battlefield is covered with grass!");
    }
}

SINGLE_BATTLE_TEST("Test event data flags start clean after another test estimates field settings")
{
    GIVEN {
        EXPECT_EQ(FlagGet(TESTING_FLAG_UNUSED_2), FALSE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { ; }
    }
}

SINGLE_BATTLE_TEST("Test event data can be set while estimating field settings")
{
    // Deliberately outside GIVEN: estimation invokes this on every runner,
    // including runners that will not execute this battle or its teardown.
    VarSet(TESTING_VAR_UNUSED_3, 1234);
    VarSet(B_VAR_STARTING_STATUS, STARTING_STATUS_GRASSY_TERRAIN);
    FlagSet(TESTING_FLAG_UNUSED_2);

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { ; }
    } SCENE {
        MESSAGE("The battlefield is covered with grass!");
    }
}

TEST("Tests resume after CRASH")
{
    KNOWN_CRASHING;
    void (*f)(void) = NULL;
    f();
}

MULTI_BATTLE_TEST("Forced Abilities are set correctly in multi battle tests")
{
    GIVEN {
        MULTI_PLAYER(SPECIES_WOBBUFFET);
        MULTI_PARTNER(SPECIES_WOBBUFFET);
        MULTI_PARTNER(SPECIES_NINETALES_ALOLA) { Ability(ABILITY_DRIZZLE); }
        MULTI_OPPONENT_A(SPECIES_WOBBUFFET);
        MULTI_OPPONENT_A(SPECIES_NINETALES_ALOLA) { Ability(ABILITY_SAND_STREAM); }
        MULTI_OPPONENT_B(SPECIES_WYNAUT);
        MULTI_OPPONENT_B(SPECIES_NINETALES_ALOLA) { Ability(ABILITY_DROUGHT); }
    } WHEN {
        TURN { SWITCH(opponentLeft, 1); SWITCH(playerRight, 4); SWITCH(opponentRight, 4); }
    } SCENE {
        ABILITY_POPUP(opponentLeft, ABILITY_SAND_STREAM);
        ABILITY_POPUP(playerRight, ABILITY_DRIZZLE);
        ABILITY_POPUP(opponentRight, ABILITY_DROUGHT);
    }
}
