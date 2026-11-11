#include "global.h"
#include "test/test.h"
#include "test/battle.h"
#include "battle_break_bar.h"
#include "event_data.h"

struct CleanupFixture
{
    struct BattleTest battle;
    bool8 declared;
    bool8 naturalEnd;
    bool8 parameterized;
    bool8 repeatedTrials;
};

// Cleanup only needs party-owned bar data, not healthbox initialization on the
// test body's small preserved stack.
static const struct BreakBarConfig sCleanupBar = {.extraBars = 1, .flags = BREAK_BAR_DEFER_INITIAL_DRAW};
static u32 sBodyCalls;
static u32 sBodyCallsAtThen;
static u32 sCleanupCalls;
static u32 sCleanupCallsAtThen;

static void CleanupSetUp(void *data)
{
    sBodyCalls = sBodyCallsAtThen = sCleanupCalls = sCleanupCallsAtThen = 0;
    gBattleTestRunner.setUp(data);
}

static void CleanupTearDown(void *data)
{
    const struct CleanupFixture *fixture = data;
    u32 expected = fixture->declared
        ? max(1, gBattleTestRunnerState->parameters) * max(1, gBattleTestRunnerState->trials) : 0;
    u32 bodyReentries = sBodyCalls - sBodyCallsAtThen;
    bool32 eventDataClean = VarGet(TESTING_VAR_UNUSED_5) == 0 && !FlagGet(TESTING_FLAG_UNUSED_5);
    gBattleTestRunner.tearDown(data);

    // A missing callback must fail too. Report without jumping out of teardown,
    // since the runner has already selected the next-test state at this point.
    if (sCleanupCalls != expected || bodyReentries != (fixture->declared ? 1 : 0)
     || !eventDataClean || gBattleStruct != NULL || gBattleResources != NULL)
    {
        gTestRunnerState.result = TEST_RESULT_FAIL;
        Test_MgbaPrintf(":L%s:%d: cleanup calls=%d expected=%d body reentries=%d clean event data=%d",
            gTestRunnerState.test->filename, __LINE__, sCleanupCalls, expected, bodyReentries, eventDataClean);
    }
}

// Delegate normal execution, RNG, and error handling unchanged; only observe
// setup/teardown so an AFTER_CLEANUP block that never runs cannot falsely pass.
static u32 CleanupEstimate(void *data) { return gBattleTestRunner.estimateCost(data); }
static void CleanupRun(void *data) { gBattleTestRunner.run(data); }
static bool32 CleanupProgress(void *data) { return gBattleTestRunner.checkProgress(data); }
static bool32 CleanupExit(void *data, enum TestResult result) { return gBattleTestRunner.handleExitWithResult(data, result); }
static u32 CleanupUniform(enum RandomTag tag, u32 lo, u32 hi, bool32 (*reject)(u32), void *caller)
{ return gBattleTestRunner.randomUniform(tag, lo, hi, reject, caller); }
static u32 CleanupWeighted(enum RandomTag tag, u32 sum, u32 n, const u16 *weights, void *caller)
{ return gBattleTestRunner.randomWeightedArray(tag, sum, n, weights, caller); }
static const void *CleanupElement(enum RandomTag tag, const void *array, size_t size, size_t count, void *caller)
{ return gBattleTestRunner.randomElementArray(tag, array, size, count, caller); }

static const struct TestRunner sCleanupRunner = {
    .estimateCost = CleanupEstimate, .setUp = CleanupSetUp, .run = CleanupRun,
    .tearDown = CleanupTearDown, .checkProgress = CleanupProgress, .handleExitWithResult = CleanupExit,
    .randomUniform = CleanupUniform, .randomWeightedArray = CleanupWeighted, .randomElementArray = CleanupElement,
};

static void CleanupBattle(void *results, const u32 i, struct BattlePokemon *player, struct BattlePokemon *opponent)
{
    const struct CleanupFixture *fixture = gTestRunnerState.test->data;
    ++sBodyCalls;
    VarSet(TESTING_VAR_UNUSED_5, 1234);
    FlagSet(TESTING_FLAG_UNUSED_5);

    if (fixture->parameterized)
    {
        PARAMETRIZE { ; }
        PARAMETRIZE { ; }
    }
    if (fixture->repeatedTrials)
        PASSES_RANDOMLY(1, 1);

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(30); Moves(MOVE_SEISMIC_TOSS); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(fixture->naturalEnd ? 1 : 300); MaxHP(300); }
    } WHEN {
        TURN { MOVE(player, MOVE_SEISMIC_TOSS); }
    } THEN {
        sBodyCallsAtThen = sBodyCalls;
        sCleanupCallsAtThen = sCleanupCalls;
        VarSet(TESTING_VAR_UNUSED_5, 0);
        FlagClear(TESTING_FLAG_UNUSED_5);
        if (fixture->declared)
        {
            EXPECT(ActivateBreakBar(B_POSITION_PLAYER_LEFT, &sCleanupBar));
            struct PartyState *owner = GetBattlerPartyState(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT));
            owner->breakBarSavedHp[0] = 25;
            owner->breakBarSavedMaxHp[0] = player->maxHP;
        }
    }
    if (fixture->declared)
    {
        AFTER_CLEANUP {
            ++sCleanupCalls;
            EXPECT_EQ(sCleanupCalls, sCleanupCallsAtThen + 1);
            EXPECT(gBattleStruct != NULL);
            EXPECT(gBattleResources != NULL);
            EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 55);
            EXPECT_EQ((u32)gBattleTestRunnerState->runGiven, FALSE);
            EXPECT_EQ((u32)gBattleTestRunnerState->runWhen, FALSE);
            EXPECT_EQ((u32)gBattleTestRunnerState->runScene, FALSE);
            EXPECT_EQ((u32)gBattleTestRunnerState->runThen, FALSE);
            EXPECT_EQ((u32)gBattleTestRunnerState->runFinally, FALSE);
            // A nested or repeated dispatch must not reenter the test body.
            TestRunner_Battle_AfterCleanup();
            VarSet(TESTING_VAR_UNUSED_5, 0);
            FlagClear(TESTING_FLAG_UNUSED_5);
        }
    }
}

#define CLEANUP_CASE(label, declare, natural, params, trials) \
    __attribute__((section(".tests"), used)) static const struct Test CAT(sCleanupTest, __LINE__) = { \
        .name = label, .filename = __FILE__, .sourceLine = __LINE__, .runner = &sCleanupRunner, \
        .data = (void *)&(const struct CleanupFixture){ \
            .battle = {.type = BATTLE_TEST_SINGLES, .function.singles = CleanupBattle}, \
            .declared = declare, .naturalEnd = natural, .parameterized = params, .repeatedTrials = trials}, \
    }

CLEANUP_CASE("After Cleanup ordinary completed tests do not reenter their bodies", FALSE, TRUE, FALSE, FALSE);
CLEANUP_CASE("After Cleanup ordinary recorded exits do not reenter their bodies", FALSE, FALSE, FALSE, FALSE);
CLEANUP_CASE("After Cleanup completed battles restore party HP before assertions", TRUE, TRUE, FALSE, FALSE);
CLEANUP_CASE("After Cleanup recorded exits restore party HP before assertions", TRUE, FALSE, FALSE, FALSE);
CLEANUP_CASE("After Cleanup dispatch resets between parameters", TRUE, FALSE, TRUE, FALSE);
CLEANUP_CASE("After Cleanup dispatch resets between random trials", TRUE, FALSE, FALSE, TRUE);
