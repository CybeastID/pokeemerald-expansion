#include "global.h"
#include "test/battle.h"
#include "castoria_name_theft.h"
#include "battle_ai_util.h"

// Script-native entry point; normally referenced only from assembly.
extern void BS_TryInstruct(void);

ASSUMPTIONS
{
    ASSUME(Test_GetCastoriaArcanumMove(0) == MOVE_FIREBALL);
    ASSUME(Test_GetCastoriaArcanumMove(1) == MOVE_MAGEARMOR);
}

SINGLE_BATTLE_TEST("Arcanum lookup bounds and boss-only access")
{
    GIVEN {
        PLAYER(SPECIES_CASTORIA);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 7), MOVE_POLYMORPH);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 8), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 255), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_PLAYER_LEFT, 4), MOVE_NONE);
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 0), MOVE_CELEBRATE);
        opponent->species = SPECIES_WOBBUFFET;
        EXPECT_EQ(GetBattlerMoveAtSlot(B_POSITION_OPPONENT_LEFT, 4), MOVE_NONE);
    }
}

SINGLE_BATTLE_TEST("Arcanum ignores exhausted regular PP when another regular move remains usable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { MovesWithPP({MOVE_TACKLE, 0}, {MOVE_SPLASH, 0}, {MOVE_GROWL, 0}, {MOVE_CELEBRATE, 10}); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 0); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Fireball!");
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_EQ(opponent->pp[2], 0);
        EXPECT_EQ(opponent->pp[3], 10);
    }
}

SINGLE_BATTLE_TEST("Arcanum Mage Armor cannot bypass Taunt")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(20); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_TACKLE, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TAUNT); MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(opponent, arcanumSlot: 1, allowed: FALSE); MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("The opposing Caster? fell for the taunt!");
        MESSAGE("The opposing Caster? used Tackle!");
        NOT MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("The opposing Caster? used Tackle!");
    }
}

SINGLE_BATTLE_TEST("Arcanum Instruct repeats the remembered regular move after a cast")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_SPLASH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_INSTRUCT); }
    } SCENE {
        MESSAGE("The opposing Caster? used Splash!");
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("Wobbuffet used Instruct!");
        MESSAGE("The opposing Caster? used Splash!");
    }
}

SINGLE_BATTLE_TEST("Arcanum Instruct fails on a missing remembered move with a virtual current slot")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        // Directly exercise a missing, Instruct-eligible remembered move.
        const u8 fail[] = {0};
        const struct __attribute__((packed)) {
            u8 opcode;
            void (*func)(void);
            const u8 *failInstr;
        } command = {0, BS_TryInstruct, fail};
        const u8 *savedScript = gBattlescriptCurrInstr;
        gBattlerAttacker = B_POSITION_PLAYER_LEFT;
        gBattlerTarget = B_POSITION_OPPONENT_LEFT;
        gLastPrintedMoves[gBattlerTarget] = MOVE_SPLASH;
        gCurrMovePos = 7;
        gBattlescriptCurrInstr = (const u8 *)&command;
        BS_TryInstruct();
        EXPECT(gBattlescriptCurrInstr == fail);
        EXPECT_EQ(gCurrMovePos, 7);
        gBattlescriptCurrInstr = savedScript;
    }
}

SINGLE_BATTLE_TEST("Arcanum AI damage reads handle virtual and invalid slots without changing battlers")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_TACKLE, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 1); }
    } THEN {
        u32 attacker = B_POSITION_OPPONENT_LEFT;
        u32 defender = B_POSITION_PLAYER_LEFT;
        u16 species = opponent->species;
        u16 hp = opponent->hp;
        u32 damage = AI_GetDamage(attacker, defender, 4, AI_ATTACKING, gAiLogicData);
        EXPECT_GT(damage, 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 5, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 8, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamage(attacker, defender, 255, AI_ATTACKING, gAiLogicData), 0);
        EXPECT_EQ(AI_GetDamageForMove(attacker, defender, MOVE_FIREBALL, AI_ATTACKING, gAiLogicData), damage);
        EXPECT_EQ(opponent->species, species);
        EXPECT_EQ(opponent->hp, hp);
        gAiLogicData->simulatedDmg[attacker][defender][0] = (struct SimulatedDamage){ .minimum = 11, .median = 22, .maximum = 33 };
        gAiThinkingStruct->aiFlags[attacker] = 0;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 22);
        EXPECT_EQ(AI_GetDamageForMove(attacker, defender, MOVE_TACKLE, AI_ATTACKING, gAiLogicData), 22);
        gAiThinkingStruct->aiFlags[attacker] = AI_FLAG_RISKY;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 33);
        gAiThinkingStruct->aiFlags[attacker] = AI_FLAG_CONSERVATIVE;
        EXPECT_EQ(AI_GetDamage(attacker, defender, 0, AI_ATTACKING, gAiLogicData), 11);
        gAiThinkingStruct->aiFlags[attacker] = 0;
    }
}

SINGLE_BATTLE_TEST("Arcanum casts from virtual slots without consuming regular PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA) { Moves(MOVE_CELEBRATE, MOVE_SPLASH, MOVE_TACKLE, MOVE_FIREBALL); }
    } WHEN {
        TURN { MOVE(opponent, arcanumSlot: 0); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Fireball!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FIREBALL, opponent);
        HP_BAR(player);
    } THEN {
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(opponent->pp[1], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponent->pp[2], GetMovePP(MOVE_TACKLE));
        EXPECT_EQ(opponent->pp[3], GetMovePP(MOVE_FIREBALL));
    }
}

SINGLE_BATTLE_TEST("Arcanum spell IDs in regular slots still say used")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CASTORIA);
    } WHEN {
        TURN { MOVE(opponent, MOVE_FIREBALL); }
    } SCENE {
        MESSAGE("The opposing Caster? used Fireball!");
        HP_BAR(player);
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Encore and enforces its lock")
{
    GIVEN {
        WITH_CONFIG(CONFIG_ENCORE_TARGET, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_ENCORE); }
        TURN { MOVE(opponent, arcanumSlot: 1, allowed: FALSE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? used Celebrate!");
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("Wobbuffet used Encore!");
        MESSAGE("The opposing Caster? must do an encore!");
        MESSAGE("The opposing Caster? used Celebrate!");
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Disable")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_DISABLE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("The opposing Caster?'s Celebrate was disabled!");
    }
}

SINGLE_BATTLE_TEST("Arcanum preserves the regular move for Mimic")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_SPLASH, MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_MIMIC); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("Wobbuffet learned Splash!");
    }
}

SINGLE_BATTLE_TEST("Arcanum failed casts preserve regular history")
{
    GIVEN {
        WITH_CONFIG(CONFIG_ENCORE_TARGET, GEN_5);
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_CASTORIA) { Speed(20); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, arcanumSlot: 1); }
        TURN { MOVE(opponent, arcanumSlot: 1); MOVE(player, MOVE_ENCORE); }
        TURN { MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("The opposing Caster? cast Mage Armor!");
        MESSAGE("But it failed!");
        MESSAGE("The opposing Caster? must do an encore!");
        MESSAGE("The opposing Caster? used Celebrate!");
    }
}

SINGLE_BATTLE_TEST("Arcanum KO under Grudge leaves regular PP intact")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(20); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(20); }
        OPPONENT(SPECIES_CASTORIA) { Speed(10); Moves(MOVE_CELEBRATE, MOVE_SPLASH, MOVE_TACKLE, MOVE_FIREBALL); }
    } WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, arcanumSlot: 0); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Grudge!");
        MESSAGE("The opposing Caster? cast Fireball!");
        MESSAGE("Wobbuffet fainted!");
        NOT MESSAGE("The opposing Caster?'s Fireball lost all its PP due to the grudge!");
    } THEN {
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_CELEBRATE));
        EXPECT_EQ(opponent->pp[1], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponent->pp[2], GetMovePP(MOVE_TACKLE));
        EXPECT_EQ(opponent->pp[3], GetMovePP(MOVE_FIREBALL));
    }
}

// Regular-slot regressions kept here so TESTS=Arcanum is sufficient.

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge depletes all PP of the move that fainted the target")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); };
    } WHEN {
        TURN {
            MOVE(player, MOVE_GRUDGE);
            MOVE(opponent, MOVE_SCRATCH);
            SEND_OUT(player, 1);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponent);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge does not deplete PP of a Z-Move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_NORMALIUM_Z); Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); };
    } WHEN {
        TURN {
            MOVE(player, MOVE_GRUDGE);
            MOVE(opponent, MOVE_SCRATCH, gimmick: GIMMICK_Z_MOVE);
            SEND_OUT(player, 1);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_ZMOVE_ACTIVATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BREAKNECK_BLITZ, opponent);
        MESSAGE("Wobbuffet fainted!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_GT(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge depletes all PP from a Max Move's base move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) {  Moves(MOVE_CELEBRATE, MOVE_SCRATCH, MOVE_POUND, MOVE_SURF); Item(ITEM_LAGGING_TAIL); }
    } WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, MOVE_SCRATCH, gimmick: GIMMICK_DYNAMAX); SEND_OUT(player, 1); }
    } SCENE {
        MESSAGE("Wobbuffet used Grudge!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        MESSAGE("The opposing Wobbuffet used Max Strike!");
        MESSAGE("Wobbuffet fainted!");
        MESSAGE("The opposing Wobbuffet's Scratch lost all its PP due to the grudge!");
    } THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_EQ(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/grudge.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Grudge does not activate for Struggle")
{
    GIVEN {
        PLAYER (SPECIES_WOBBUFFET) { HP(1); }
        PLAYER (SPECIES_WOBBUFFET);
        OPPONENT (SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE, MOVE_STRUGGLE, MOVE_POUND, MOVE_SURF); };
    }
    WHEN {
        TURN { MOVE(player, MOVE_GRUDGE); MOVE(opponent, MOVE_STRUGGLE); SEND_OUT(player, 1); }
    }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GRUDGE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_STRUGGLE, opponent);
        MESSAGE("Wobbuffet fainted!");
        NOT MESSAGE("The opposing Wobbuffet's Struggle lost all its PP due to the grudge!");
    }
    THEN {
        EXPECT_GT(opponent->pp[0], 0);
        EXPECT_GT(opponent->pp[1], 0);
        EXPECT_GT(opponent->pp[2], 0);
        EXPECT_GT(opponent->pp[3], 0);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore forces consecutive move uses for 3 turns: Encore used before move")
{
    struct BattlePokemon *encoreUser = NULL;
    struct BattlePokemon *encoreTarget = NULL;
    u32 speedPlayer, speedOpponent;
    PARAMETRIZE { encoreUser = opponent; encoreTarget = player; speedPlayer = 10; speedOpponent = 20; }
    PARAMETRIZE { encoreUser = player; encoreTarget = opponent; speedPlayer = 20; speedOpponent = 10; }
    GIVEN {
        WITH_CONFIG(CONFIG_ENCORE_TARGET, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Speed(speedPlayer); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(speedOpponent); }
    } WHEN {
        TURN { MOVE(encoreUser, MOVE_CELEBRATE); MOVE(encoreTarget, MOVE_CELEBRATE); }
        TURN { MOVE(encoreUser, MOVE_ENCORE); MOVE(encoreTarget, MOVE_CELEBRATE); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { MOVE(encoreTarget, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, encoreTarget);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore forces consecutive move uses for 3 turns for player: Encore used after move")
{
    struct BattlePokemon *encoreUser = NULL;
    struct BattlePokemon *encoreTarget = NULL;
    u32 speedPlayer, speedOpponent;
    PARAMETRIZE { encoreUser = opponent; encoreTarget = player; speedPlayer = 20; speedOpponent = 10; }
    PARAMETRIZE { encoreUser = player; encoreTarget = opponent; speedPlayer = 10; speedOpponent = 20; }
    GIVEN {
        WITH_CONFIG(CONFIG_ENCORE_TARGET, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Speed(speedPlayer); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(speedOpponent); }
    } WHEN {
        TURN { MOVE(encoreTarget, MOVE_CELEBRATE); MOVE(encoreUser, MOVE_ENCORE); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { FORCED_MOVE(encoreTarget); }
        TURN { MOVE(encoreTarget, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, encoreUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, encoreTarget);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, encoreTarget);
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore has no effect if no previous move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_ENCORE); MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet used Encore!");
        MESSAGE("But it failed!");
    }
}

// Copied from test/battle/move_effect/encore.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Encore overrides the chosen move if it occurs first")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(opponent, MOVE_ENCORE); MOVE(player, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ENCORE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
}

// Copied from test/battle/move_effect/mimic.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Mimic doesn't error when the last move used by the target resolves to MOVE_UNAVAILABLE")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {MOVE(opponent, MOVE_TRANSFORM); MOVE(player, MOVE_MIMIC); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TRANSFORM, opponent);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_MIMIC, player);
    } THEN {
        EXPECT_EQ(gLastMoves[1], MOVE_UNAVAILABLE); // This test depends on the current implementation of Transform, if this changes, the test should be changed
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
SINGLE_BATTLE_TEST("Arcanum regression: Instruct causes the target to use its last used move again")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); MOVE(opponent, MOVE_INSTRUCT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, opponent);
        MESSAGE("Wobbuffet followed the opposing Wynaut's instructions!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if target hasn't made a move")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); }
    } SCENE {
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if move is banned by Instruct")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(IsMoveInstructBanned(MOVE_BIDE));
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_BIDE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_BIDE); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BIDE, playerRight);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called move targets the target of the move picked on its last use")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_FAKE_OUT); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        HP_BAR(opponentLeft);
        NOT HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        HP_BAR(opponentLeft);
        NOT HP_BAR(opponentRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct doesn't bypass sleep")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_SPORE) == EFFECT_NON_VOLATILE_STATUS);
        ASSUME(GetMoveNonVolatileStatus(MOVE_SPORE) == MOVE_EFFECT_SLEEP);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(opponentLeft, MOVE_SPORE, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPORE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        MESSAGE("Wobbuffet is fast asleep.");
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct fails if target doesn't know the last move it used")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(IsDanceMove(MOVE_DRAGON_DANCE));
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_ORICORIO) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_DRAGON_DANCE); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, opponentLeft);
        ABILITY_POPUP(playerRight, ABILITY_DANCER);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, playerRight);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_DRAGON_DANCE, playerRight);
        }
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called move fails if it can only be used on the first turn but consumes PP")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_FAKE_OUT) == EFFECT_FIRST_TURN_ONLY);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_FAKE_OUT); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_FAKE_OUT, target: opponentLeft); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FAKE_OUT, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_FAKE_OUT, playerRight);
    } THEN {
        EXPECT_EQ(playerRight->pp[3], GetMovePP(MOVE_FAKE_OUT) - 2);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called status move fails if taunted")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_TAUNT) == EFFECT_TAUNT);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_GROWL); MOVE(opponentLeft, MOVE_TAUNT, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, playerRight);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentRight);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TAUNT, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NONE_OF {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWL, playerRight);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentRight);
            ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, opponentLeft);
        }
    } THEN {
        EXPECT_EQ(playerRight->pp[3], GetMovePP(MOVE_GROWL) - 1);
    }
}

// Copied from test/battle/move_effect/instruct.c; keep the original test unchanged.
DOUBLE_BATTLE_TEST("Arcanum regression: Instruct-called moves fail if disabled")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_INSTRUCT) == EFFECT_INSTRUCT);
        ASSUME(GetMoveEffect(MOVE_DISABLE) == EFFECT_DISABLE);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SCRATCH, MOVE_POUND, MOVE_SCRATCH, MOVE_GROWL); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SCRATCH, target: opponentLeft); MOVE(opponentLeft, MOVE_DISABLE, target: playerRight); MOVE(playerLeft, MOVE_INSTRUCT, target: playerRight); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_DISABLE, opponentLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_INSTRUCT, playerLeft);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
    } THEN {
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SCRATCH) - 1);
    }
}
