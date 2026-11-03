#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_break_bar.h"
#include "battle.h"

extern AiScoreFunc sDynamicAiFunc;

// Test-only registration: the existing Break Bar tests use the same
// dynamic-AI callback to install a stack after battle initialization.
static const struct BreakBarConfig sStackBreakerFiveBars = {.extraBars = 5};
static bool32 sStackBreakerSetupDone;

static s32 StackBreakerTestSetup(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    if (!sStackBreakerSetupDone)
    {
        u32 target = B_POSITION_PLAYER_LEFT;
        if (GetBattlerPartyState(target)->breakBarConfig == NULL)
            ActivateBreakBar(target, &sStackBreakerFiveBars);

        // Force the existing end-of-turn Bug Space passive path rather than
        // simulating six independent attacks or invoking the native manually.
        gBattleStruct->bugSpace.active = TRUE;
        gBattleStruct->bugSpace.currentTier = BUGSPACE_TIER_PASSIVE_OHKO;
        gBattleStruct->bugSpace.sourceBattler = B_POSITION_OPPONENT_LEFT;
        sStackBreakerSetupDone = TRUE;
    }
    return score;
}

AI_SINGLE_BATTLE_TEST("Kazuradrop Stack Breaker consumes five extra bars and defeats the target")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        sStackBreakerSetupDone = FALSE;
        sDynamicAiFunc = StackBreakerTestSetup;
        PLAYER(SPECIES_WOBBUFFET) { HP(100); MaxHP(100); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Moves(MOVE_CELEBRATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarsRemaining, 0);
        EXPECT_EQ(gBattleResults.playerFaintCounter, 1);
    }
}
