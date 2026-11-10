#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

// Successful Wishes dispatch to all active holders; delayed recovery does not dispatch.
//  - A successful Wish: heal 1/16 of current in-battle max HP, and ONE random non-HP raw
//    stat grows x1.10 (rounded). statStages are untouched.
//  - The delayed Wish recovery (end of the following turn) does not trigger it again.
//  - A failed Wish (Wish already pending) does not trigger it.
//  - Growth survives switching out and back in.

static u32 GrownStat(u32 value, u32 count)
{
    while (count--)
        value = (value * 11 + 5) / 10;
    return value;
}

static u32 BaseStat(u32 stat)
{
    static const u16 sMonData[NUM_STATS] = {0, MON_DATA_ATK, MON_DATA_DEF, MON_DATA_SPEED, MON_DATA_SPATK, MON_DATA_SPDEF};
    return GetMonData(&gPlayerParty[0], sMonData[stat]);
}

static u32 CurrentStat(struct BattlePokemon *mon, u32 stat)
{
    switch (stat)
    {
    case STAT_ATK:   return mon->attack;
    case STAT_DEF:   return mon->defense;
    case STAT_SPEED: return mon->speed;
    case STAT_SPATK: return mon->spAttack;
    default:         return mon->spDefense;
    }
}

static u32 TotalGrowth(void)
{
    u32 total = 0;
    for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
        total += gBattleStruct->partyState[B_SIDE_PLAYER][0].wishfulGrowth[stat];
    return total;
}

static bool32 RawStatsMatchGrowth(struct BattlePokemon *mon)
{
    for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
        if (CurrentStat(mon, stat) != GrownStat(BaseStat(stat), gBattleStruct->partyState[B_SIDE_PLAYER][0].wishfulGrowth[stat]))
            return FALSE;
    return TRUE;
}

SINGLE_BATTLE_TEST("Wishful Thinking: recovery ignores Heal Block and heals at least one HP")
{
    u32 maxHP;
    PARAMETRIZE { maxHP = 15; }
    PARAMETRIZE { maxHP = 320; }
    GIVEN {
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(maxHP); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_HEAL_BLOCK, MOVE_WISH); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_HEAL_BLOCK); }
        TURN { MOVE(opponent, MOVE_WISH); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_WISHFUL_THINKING);
        HP_BAR(player, damage: -max(1, maxHP / 16));
    } THEN {
        EXPECT_EQ(player->hp, 1 + max(1, maxHP / 16));
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT_EQ((u32)GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->breakBarsRemaining, 0);
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: a successful Wish heals 1/16 max HP and grows one stat once")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WISH, WITH_RNG(RNG_WISHFUL_THINKING, STAT_SPDEF)); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, player);
        MESSAGE("Wobbuffet's HP was restored.");
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Wobbuffet's Sp. Def grew with its wish!");
    } THEN {
        EXPECT_EQ(player->hp, 100 + 320 / 16);
        EXPECT_EQ(player->maxHP, 320);
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT(RawStatsMatchGrowth(player));
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: an opposing Wish immediately heals and grows the holder")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_WISH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_WISH, WITH_RNG(RNG_WISHFUL_THINKING, STAT_ATK)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, opponent);
        ABILITY_POPUP(player, ABILITY_WISHFUL_THINKING);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_WISH_HEAL, player);
        HP_BAR(player, damage: -20);
        MESSAGE("Wynaut's HP was restored.");
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Wynaut's Attack grew with its wish!");
    } THEN {
        EXPECT_EQ(player->hp, 120);
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT(RawStatsMatchGrowth(player));
        EXPECT_EQ(gWishFutureKnock.wishCounter[B_POSITION_OPPONENT_LEFT], 1);
        EXPECT_EQ(gWishFutureKnock.wishCounter[B_POSITION_PLAYER_LEFT], 0);
    }
}

DOUBLE_BATTLE_TEST("Wishful Thinking: both sides' holders activate once before the next move")
{
    GIVEN {
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Speed(30); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(40); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_WISH); }
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(160); HP(50); Speed(20); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_WISH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WISH, opponentLeft);
        ABILITY_POPUP(playerLeft, ABILITY_WISHFUL_THINKING);
        HP_BAR(playerLeft, damage: -20);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, playerLeft);
        ABILITY_POPUP(opponentRight, ABILITY_WISHFUL_THINKING);
        HP_BAR(opponentRight, damage: -10);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, playerRight);
    } THEN {
        u32 growth = 0;
        EXPECT_EQ(playerLeft->hp, 120);
        EXPECT_EQ(opponentRight->hp, 60);
        EXPECT_EQ(TotalGrowth(), 1);
        for (u32 stat = STAT_ATK; stat < NUM_STATS; stat++)
            growth += GetBattlerPartyState(B_POSITION_OPPONENT_RIGHT)->wishfulGrowth[stat];
        EXPECT_EQ(growth, 1);
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: full HP still grows each of the five raw stats")
{
    u32 stat;
    PARAMETRIZE { stat = STAT_ATK; }
    PARAMETRIZE { stat = STAT_DEF; }
    PARAMETRIZE { stat = STAT_SPEED; }
    PARAMETRIZE { stat = STAT_SPATK; }
    PARAMETRIZE { stat = STAT_SPDEF; }
    GIVEN {
        PLAYER(SPECIES_WYNAUT) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(320); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_WISH); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_WISH, WITH_RNG(RNG_WISHFUL_THINKING, stat)); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_WISHFUL_THINKING);
        NONE_OF {
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_WISH_HEAL, player);
            HP_BAR(player);
        }
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
    } THEN {
        EXPECT_EQ(player->hp, 320);
        EXPECT_EQ(gBattleStruct->passiveHpUpdate[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(gBattleStruct->healingHpUpdate & (1u << B_POSITION_PLAYER_LEFT), 0);
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT_EQ(GetBattlerPartyState(B_POSITION_PLAYER_LEFT)->wishfulGrowth[stat], 1);
        EXPECT(RawStatsMatchGrowth(player));
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: stat stages are not touched and the delayed Wish does not re-trigger it")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_WISH); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        // The opponent's delayed Wish lands at the end of turn 2; growth stays at 1.
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT_EQ(player->hp, 120);
        EXPECT_EQ(opponent->hp, 260);
        for (u32 stat = STAT_ATK; stat <= STAT_SPDEF; stat++)
            EXPECT_EQ(player->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: a failed Wish does not trigger it")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_WISH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_WISH); }
        // Wish is still pending at the start of turn 2, so the second one fails.
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_WISH); }
    } SCENE {
        MESSAGE("But it failed!");
    } THEN {
        EXPECT_EQ(TotalGrowth(), 1);
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: growth persists across switching out and back in")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WISH); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 0); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT(RawStatsMatchGrowth(player));
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: Wish without the ability heals nothing now and grows nothing")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SHADOW_TAG); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WISH); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->hp, 100);
        EXPECT_EQ(TotalGrowth(), 0);
    }
}

SINGLE_BATTLE_TEST("Wishful Thinking: repeated stat reloads never apply the multiplier twice")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_WISHFUL_THINKING); MaxHP(320); HP(100); Moves(MOVE_WISH, MOVE_CELEBRATE); }
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WISH); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 1); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { SWITCH(player, 0); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        // Switch-in path already ran once. Run the base-stat-copy path repeatedly on top of it.
        RecalcBattlerStats(B_POSITION_PLAYER_LEFT, &gPlayerParty[0], FALSE);
        RecalcBattlerStats(B_POSITION_PLAYER_LEFT, &gPlayerParty[0], FALSE);
        EXPECT_EQ(TotalGrowth(), 1);
        EXPECT(RawStatsMatchGrowth(player));
    }
}
