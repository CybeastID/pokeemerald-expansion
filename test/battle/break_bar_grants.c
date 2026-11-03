#include "global.h"
#include "test/battle.h"
#include "battle_break_bar.h"

SINGLE_BATTLE_TEST("Break Bar final red layer can still faint from passive damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(15); HP(1); MaxHP(8); Item(ITEM_BREAK_CHARM); Status1(STATUS1_POISON); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(15); Level(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); SEND_OUT(player, 1); }
    } SCENE {
        HP_BAR(player, hp: 0);
        MESSAGE("Wobbuffet was hurt by its poisoning!");
        HP_BAR(player, hp: 0);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarsRemaining, 0);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm activates before entry hazards")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(15); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); HP(1); MaxHP(100); Item(ITEM_BREAK_CHARM); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(15); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPIKES); }
        TURN { SWITCH(player, 1); }
    } SCENE {
        MESSAGE("Wynaut's Break Charm formed an extra HP bar!");
        HP_BAR(player, hp: 88);
        MESSAGE("Wynaut was hurt by the spikes!");
    } THEN {
        EXPECT_EQ(player->hp, 88);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
        EXPECT_EQ(GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->breakBarSavedHp[0], 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm passed away and back cannot grant the same Pokemon twice")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(15); HP(13); MaxHP(100); Item(ITEM_BREAK_CHARM); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(15); Level(100); }
        OPPONENT(SPECIES_WYNAUT) { Speed(15); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(player, MOVE_TRICK); }
        TURN { SWITCH(opponent, 1); }
        TURN { SWITCH(opponent, 0); }
        TURN { MOVE(opponent, MOVE_TRICK); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_BREAK_CHARM);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm allows OHKO attacks to break a layer")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(15); HP(7); MaxHP(100); Item(ITEM_BREAK_CHARM); Level(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(15); Level(100); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_FISSURE); }
    } THEN {
        EXPECT_EQ(player->hp, 7);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm allows Destiny Bond to break a layer without fainting its owner")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(7); MaxHP(100); Item(ITEM_BREAK_CHARM); Level(100); Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); MaxHP(100); Speed(30); }
        OPPONENT(SPECIES_WYNAUT) { Speed(15); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_DESTINY_BOND); MOVE(player, MOVE_SEISMIC_TOSS); SEND_OUT(opponent, 1); }
    } SCENE {
        HP_BAR(opponent, hp: 0);
        MESSAGE("The opposing Wobbuffet took its attacker down with it!");
        HP_BAR(player, hp: 0);
        NOT MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(player->hp, 7);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm saves partial HP, stays held, and grants only once across switches")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(7); MaxHP(100); Item(ITEM_BREAK_CHARM); Speed(10); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(100); Speed(20); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
        TURN { SWITCH(player, 1); }
        TURN { SWITCH(player, 0); }
    } SCENE {
        MESSAGE("Wobbuffet's Break Charm formed an extra HP bar!");
        HP_BAR(player, hp: 0);
        NOT MESSAGE("Wobbuffet's Break Charm formed an extra HP bar!");
    } THEN {
        EXPECT_EQ(player->hp, 7);
        EXPECT_EQ(player->item, ITEM_BREAK_CHARM);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ((u32)GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->breakBarItemUsed, TRUE);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 0);
    }
}

SINGLE_BATTLE_TEST("Break Bar Lunar Dance faints an unprotected user and restores HP status and PP before granting a bar")
{
    GIVEN {
        PLAYER(SPECIES_GARDEVOIR) { Speed(30); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); HP(1); MaxHP(100); Status1(STATUS1_POISON); MovesWithPP({MOVE_SCRATCH, 1}); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 1); }
    } SCENE {
        HP_BAR(player, hp: 0);
        MESSAGE("Gardevoir fainted!");
        MESSAGE("Wynaut became cloaked in mystical moonlight!");
        HP_BAR(player, hp: 100);
        STATUS_ICON(player, none: TRUE);
        MESSAGE("Wynaut gained an extra HP bar!");
    } THEN {
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(player->status1, 0);
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_SCRATCH));
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
        EXPECT_EQ(GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->breakBarSavedHp[0], 100);
    }
}

SINGLE_BATTLE_TEST("Break Bar Lunar Dance consumes the user's bar before switching the surviving user")
{
    GIVEN {
        PLAYER(SPECIES_GARDEVOIR) { HP(23); MaxHP(100); Item(ITEM_BREAK_CHARM); Speed(30); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); HP(5); MaxHP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Gardevoir's Break Charm formed an extra HP bar!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_LUNAR_DANCE, player);
        HP_BAR(player, hp: 0);
        MESSAGE("Gardevoir went back to 1!");
        MESSAGE("Wynaut became cloaked in mystical moonlight!");
        HP_BAR(player, hp: 100);
        MESSAGE("Wynaut gained an extra HP bar!");
    } THEN {
        EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 23);
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarsRemaining, 0);
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarPending, FALSE);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 0);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Lunar Dance grants to a fully healthy recipient too")
{
    GIVEN {
        PLAYER(SPECIES_GARDEVOIR) { Speed(30); }
        PLAYER(SPECIES_WYNAUT) { Speed(15); HP(100); MaxHP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wynaut gained an extra HP bar!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Lunar Dance stacks with a recipient's Charm and restores each saved layer")
{
    GIVEN {
        PLAYER(SPECIES_GARDEVOIR) { Speed(30); }
        PLAYER(SPECIES_WYNAUT) { HP(17); MaxHP(100); Item(ITEM_BREAK_CHARM); Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(100); Speed(20); }
    } WHEN {
        TURN { MOVE(player, MOVE_LUNAR_DANCE); SEND_OUT(player, 1); }
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
        TURN { MOVE(opponent, MOVE_SEISMIC_TOSS); }
    } SCENE {
        MESSAGE("Wynaut gained an extra HP bar!");
        MESSAGE("Wynaut's Break Charm formed an extra HP bar!");
        HP_BAR(player, hp: 0);
        HP_BAR(player, hp: 0);
        NOT MESSAGE("Wynaut fainted!");
    } THEN {
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm protects from confusion self-damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(1); Item(ITEM_BREAK_CHARM); Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(30); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CONFUSE_RAY); MOVE(player, MOVE_CELEBRATE, WITH_RNG(RNG_CONFUSION, TRUE)); }
    } SCENE {
        MESSAGE("It hurt itself in its confusion!");
        NOT HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, 1);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm protects from contact ability and item damage")
{
    u32 ability, item;
    PARAMETRIZE { ability = ABILITY_ROUGH_SKIN; item = ITEM_NONE; }
    PARAMETRIZE { ability = ABILITY_RUN_AWAY; item = ITEM_ROCKY_HELMET; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(15); HP(1); MaxHP(1); Item(ITEM_BREAK_CHARM); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(15); HP(1000); MaxHP(1000); Ability(ability); Item(item); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent);
        NOT HP_BAR(player);
    } THEN {
        EXPECT_EQ(player->hp, 1);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 1);
    }
}

SINGLE_BATTLE_TEST("Break Bar Charm allows deliberate Explosion sacrifice to consume a bar")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(13); MaxHP(100); Item(ITEM_BREAK_CHARM); Speed(30); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Speed(10); }
    } WHEN {
        TURN { MOVE(player, MOVE_EXPLOSION); }
    } SCENE {
        HP_BAR(player, hp: 0);
        HP_BAR(opponent);
        NOT MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_EQ(player->hp, 13);
        EXPECT_EQ(GetBreakBarCount(B_POSITION_PLAYER_LEFT), 0);
    }
}
