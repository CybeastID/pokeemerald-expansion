#include "global.h"
#include "test/battle.h"
#include "event_data.h"
#include "constants/flags.h"

SINGLE_BATTLE_TEST("Kazuradrop Bug Space popup belongs to the boss after player Intimidate")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_INTIMIDATE); Speed(20); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Speed(10); }
    } WHEN {
        TURN { }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        ABILITY_POPUP(opponent, ABILITY_BUG_SPACE);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Bug Space popup belongs to the boss on a later sendout")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_INTIMIDATE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { SWITCH(opponent, 1); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        ABILITY_POPUP(opponent, ABILITY_BUG_SPACE);
    }
}

DOUBLE_BATTLE_TEST("Kazuradrop Bug Space popup uses the boss position in doubles")
{
    GIVEN {
        PLAYER(SPECIES_GYARADOS) { Ability(ABILITY_INTIMIDATE); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(80); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Speed(70); }
    } WHEN {
        TURN { }
    } SCENE {
        ABILITY_POPUP(playerLeft, ABILITY_INTIMIDATE);
        ABILITY_POPUP(opponentRight, ABILITY_BUG_SPACE);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause uses the existing classification and boost tiers")
{
    u16 species;
    u32 tier;
    PARAMETRIZE { species = SPECIES_WOBBUFFET; tier = 0; }
    PARAMETRIZE { species = SPECIES_GARCHOMP; tier = 0; }
    PARAMETRIZE { species = SPECIES_IRON_VALIANT; tier = 0; }
    PARAMETRIZE { species = SPECIES_MEW; tier = 1; }
    PARAMETRIZE { species = SPECIES_NIHILEGO; tier = 1; }
    PARAMETRIZE { species = SPECIES_ARTICUNO; tier = 1; }
    PARAMETRIZE { species = SPECIES_XERNEAS; tier = 2; }
    PARAMETRIZE { species = SPECIES_CALYREX_ICE; tier = 2; }
    GIVEN {
        PLAYER(species);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { }
    } THEN {
        EXPECT_EQ(GetKazuraLegendTier(B_POSITION_PLAYER_LEFT), tier);
        EXPECT_EQ(FlagGet(FLAG_KAZU_LEGENDARY_PUNISH), tier != 0);
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + tier);
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE + tier);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + tier);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + tier);
        EXPECT_EQ(opponent->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE + tier);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause triggers on a later legendary and only once")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_XERNEAS);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 2); }
        TURN { SWITCH(player, 0); }
        TURN { SWITCH(player, 1); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("Kazuradrop's parameters are rapidly increasing!");
        HP_BAR(opponent);
    } THEN {
        EXPECT(FlagGet(FLAG_KAZU_LEGENDARY_PUNISH));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponent->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause triggers when the boss enters against an existing legendary")
{
    GIVEN {
        PLAYER(SPECIES_XERNEAS);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { SWITCH(opponent, 1); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_BUG_SPACE);
        MESSAGE("Kazuradrop's parameters are rapidly increasing!");
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT(FlagGet(FLAG_KAZU_LEGENDARY_PUNISH));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_DEF], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(opponent->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE + 2);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause also follows a legendary switch-in ability")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_LANDORUS_THERIAN) { Ability(ABILITY_INTIMIDATE); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { SWITCH(player, 1); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_BUG_SPACE);
        ABILITY_POPUP(player, ABILITY_INTIMIDATE);
        MESSAGE("Kazuradrop's parameters are rapidly increasing!");
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponent);
    } THEN {
        EXPECT(FlagGet(FLAG_KAZU_LEGENDARY_PUNISH));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause clears a stale flag on activation")
{
    GIVEN {
        FLAG_SET(FLAG_KAZU_LEGENDARY_PUNISH);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { }
    } THEN {
        EXPECT(!FlagGet(FLAG_KAZU_LEGENDARY_PUNISH));
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Legendary Clause judges Mega forms by their base species")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); }
    } WHEN {
        TURN { }
    } THEN {
        u16 species = SPECIES_LATIOS_MEGA;
        SetMonData(&gParties[B_TRAINER_PLAYER][gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT]], MON_DATA_SPECIES, &species);
        EXPECT_EQ(GetKazuraLegendTier(B_POSITION_PLAYER_LEFT), 1);
        species = SPECIES_GARCHOMP_MEGA;
        SetMonData(&gParties[B_TRAINER_PLAYER][gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT]], MON_DATA_SPECIES, &species);
        EXPECT_EQ(GetKazuraLegendTier(B_POSITION_PLAYER_LEFT), 0);
    }
}
