#include "global.h"
#include "test/battle.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "data.h"
#include "event_data.h"
#include "malloc.h"
#include "script.h"
#include "trainer_break_bars.h"

static const struct TrainerBreakBarRule sRules[] =
{
    {TRAINER_BREAK_BAR_SPECIES, SPECIES_WOBBUFFET, &gTrainerBreakBarProfiles[1]},
    {TRAINER_BREAK_BAR_ENTRY, 2, &gTrainerBreakBarProfiles[4]},
    {TRAINER_BREAK_BAR_ENTRY, 3, NULL},
};
static const struct TrainerBreakBarRules sRuleSet = TRAINER_BREAK_BAR_RULES(sRules);

// Synthetic trainer rows exercise sparse authoring without configuring any
// additional trainer in the production table. Row 0 is deliberately omitted.
static const struct TrainerBreakBarRules *const sInlineTrainerRules[4][DIFFICULTY_COUNT] =
{
    [1] =
    {
        [DIFFICULTY_NORMAL] = INLINE_BREAK_BAR_RULES(
            {TRAINER_BREAK_BAR_SPECIES, SPECIES_WOBBUFFET, &gTrainerBreakBarProfiles[1]},
            {
                .selector = TRAINER_BREAK_BAR_ENTRY,
                .value = 2,
                .config = &(const struct BreakBarConfig)
                {
                    .extraBars = 2,
                    .flags = BREAK_BAR_DEFER_INITIAL_DRAW,
                    .phases =
                    {
                        [BREAK_BAR_BLUE - 1] = {.onBreakScript = BattleScript_BreakBarRefill},
                        [BREAK_BAR_PURPLE - 1] = {.afterBreakScript = BattleScript_BreakBarRefill},
                    },
                },
            },
            {TRAINER_BREAK_BAR_ENTRY, 3, NULL},
            {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, &gTrainerBreakBarProfiles[0]},
        ),
        [DIFFICULTY_HARD] = &gEmptyTrainerBreakBars,
    },
    [2] =
    {
        [DIFFICULTY_HARD] = INLINE_BREAK_BAR_RULES(
            {TRAINER_BREAK_BAR_ENTRY, 0, &gTrainerBreakBarProfiles[4]},
        ),
    },
    [3] =
    {
        [DIFFICULTY_NORMAL] = &sRuleSet,
        [DIFFICULTY_HARD] = INLINE_BREAK_BAR_RULES(
            {TRAINER_BREAK_BAR_SPECIES, SPECIES_PICHU, &gTrainerBreakBarProfiles[2]},
        ),
    },
};

static const struct TrainerBreakBarRules *const sDuplicateRules = INLINE_BREAK_BAR_RULES(
    {TRAINER_BREAK_BAR_ENTRY, 2, &gTrainerBreakBarProfiles[4]},
    {TRAINER_BREAK_BAR_SPECIES, SPECIES_WOBBUFFET, &gTrainerBreakBarProfiles[0]},
    {TRAINER_BREAK_BAR_ENTRY, 3, NULL},
    {TRAINER_BREAK_BAR_SPECIES, SPECIES_WOBBUFFET, &gTrainerBreakBarProfiles[1]},
    {TRAINER_BREAK_BAR_ENTRY, 2, NULL},
    {TRAINER_BREAK_BAR_ENTRY, 3, &gTrainerBreakBarProfiles[4]},
    {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, &gTrainerBreakBarProfiles[0]},
    {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, NULL},
);

TEST("Trainer Break Bar entry rules override species, including explicit disabling")
{
    EXPECT_EQ(ResolveTrainerBreakBar(&sRuleSet, SPECIES_WOBBUFFET, 0), &gTrainerBreakBarProfiles[1]);
    EXPECT_EQ(ResolveTrainerBreakBar(&sRuleSet, SPECIES_WOBBUFFET, 1), &gTrainerBreakBarProfiles[1]);
    EXPECT_EQ(ResolveTrainerBreakBar(&sRuleSet, SPECIES_WOBBUFFET, 2), &gTrainerBreakBarProfiles[4]);
    EXPECT_EQ(ResolveTrainerBreakBar(&sRuleSet, SPECIES_WOBBUFFET, 3), NULL);
    EXPECT_EQ(ResolveTrainerBreakBar(&sRuleSet, SPECIES_PICHU, 0), NULL);
    EXPECT_EQ(ResolveTrainerBreakBar(&gEmptyTrainerBreakBars, SPECIES_WOBBUFFET, 0), NULL);
}

TEST("Trainer Break Bar inline rules retain counts, named profiles, custom phases and NULL overrides")
{
    const struct TrainerBreakBarRules *rules = sInlineTrainerRules[1][DIFFICULTY_NORMAL];
    const struct BreakBarConfig *config = ResolveTrainerBreakBar(rules, SPECIES_WOBBUFFET, 2);
    EXPECT_EQ(rules->count, 4);
    EXPECT_EQ(ResolveTrainerBreakBar(rules, SPECIES_WOBBUFFET, 0), &gTrainerBreakBarProfiles[1]);
    EXPECT_EQ(ResolveTrainerBreakBar(rules, SPECIES_JIRACHI, 0), &gTrainerBreakBarProfiles[0]);
    EXPECT_EQ(ResolveTrainerBreakBar(rules, SPECIES_WOBBUFFET, 3), NULL);
    EXPECT_EQ(ResolveTrainerBreakBar(rules, SPECIES_PICHU, 0), NULL);
    EXPECT_EQ(ResolveTrainerBreakBar(NULL, SPECIES_WOBBUFFET, 0), NULL);
    EXPECT_EQ(config, rules->rules[1].config);
    EXPECT_EQ(config->extraBars, 2);
    EXPECT_EQ(config->flags, BREAK_BAR_DEFER_INITIAL_DRAW);
    EXPECT_EQ(config->phases[BREAK_BAR_BLUE - 1].onBreakScript, BattleScript_BreakBarRefill);
    EXPECT_EQ(config->phases[BREAK_BAR_BLUE - 1].afterBreakScript, NULL);
    EXPECT_EQ(config->phases[BREAK_BAR_PURPLE - 1].onBreakScript, NULL);
    EXPECT_EQ(config->phases[BREAK_BAR_PURPLE - 1].afterBreakScript, BattleScript_BreakBarRefill);
}

TEST("Trainer Break Bar first entry and last species matches retain precedence regardless of rule order")
{
    EXPECT_EQ(ResolveTrainerBreakBar(sDuplicateRules, SPECIES_WOBBUFFET, 2), &gTrainerBreakBarProfiles[4]);
    EXPECT_EQ(ResolveTrainerBreakBar(sDuplicateRules, SPECIES_WOBBUFFET, 3), NULL);
    EXPECT_EQ(ResolveTrainerBreakBar(sDuplicateRules, SPECIES_WOBBUFFET, 0), &gTrainerBreakBarProfiles[1]);
    EXPECT_EQ(ResolveTrainerBreakBar(sDuplicateRules, SPECIES_JIRACHI, 0), NULL);
}

TEST("Trainer Break Bar difficulty lookup inherits Normal and validates trainer IDs")
{
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_KOANKO, DIFFICULTY_NORMAL), &gKoankoInitialBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_KOANKO, DIFFICULTY_HARD), &gKoankoInitialBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_KOANKO, DIFFICULTY_EASY), &gKoankoInitialBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_ROXANNE_1, DIFFICULTY_NORMAL), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_ROXANNE_1, DIFFICULTY_HARD), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_NONE, DIFFICULTY_NORMAL), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_SECRET_BASE, DIFFICULTY_NORMAL), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINERS_COUNT, DIFFICULTY_NORMAL), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_KOANKO, DIFFICULTY_COUNT), &gEmptyTrainerBreakBars);
    EXPECT_EQ(GetTrainerBreakBarRules(TRAINER_KOANKO, (enum DifficultyLevel)-1), &gEmptyTrainerBreakBars);
    EXPECT_EQ(gKoankoInitialBreakBars.count, 0);
    EXPECT_EQ(gKoankoConjuredBreakBars.count, 0);
    EXPECT_NE(&gKoankoInitialBreakBars, &gKoankoConjuredBreakBars);
}

TEST("Trainer Break Bar sparse difficulty rows inherit Normal unless explicitly replaced or empty")
{
    const struct TrainerBreakBarRules *normal = sInlineTrainerRules[1][DIFFICULTY_NORMAL];
    const struct TrainerBreakBarRules *hard = sInlineTrainerRules[2][DIFFICULTY_HARD];
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], DIFFICULTY_NORMAL), normal);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], DIFFICULTY_EASY), normal);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], DIFFICULTY_HARD), &gEmptyTrainerBreakBars);
    EXPECT_EQ(ResolveTrainerBreakBar(normal, SPECIES_WOBBUFFET, 0), &gTrainerBreakBarProfiles[1]);
    EXPECT_EQ(ResolveTrainerBreakBar(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], DIFFICULTY_HARD), SPECIES_WOBBUFFET, 0), NULL);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[2], DIFFICULTY_HARD), hard);
    EXPECT_EQ(ResolveTrainerBreakBar(hard, SPECIES_PICHU, 0), &gTrainerBreakBarProfiles[4]);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[2], DIFFICULTY_NORMAL), &gEmptyTrainerBreakBars);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[2], DIFFICULTY_EASY), &gEmptyTrainerBreakBars);
    const struct TrainerBreakBarRules *replacement = Test_SelectTrainerBreakBarRules(sInlineTrainerRules[3], DIFFICULTY_HARD);
    EXPECT_EQ(replacement, sInlineTrainerRules[3][DIFFICULTY_HARD]);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[3], DIFFICULTY_EASY), &sRuleSet);
    EXPECT_EQ(ResolveTrainerBreakBar(replacement, SPECIES_PICHU, 0), &gTrainerBreakBarProfiles[2]);
    EXPECT_EQ(ResolveTrainerBreakBar(replacement, SPECIES_WOBBUFFET, 0), NULL);
    for (u32 difficulty = 0; difficulty < DIFFICULTY_COUNT; difficulty++)
        EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[0], difficulty), &gEmptyTrainerBreakBars);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], DIFFICULTY_COUNT), &gEmptyTrainerBreakBars);
    EXPECT_EQ(Test_SelectTrainerBreakBarRules(sInlineTrainerRules[1], (enum DifficultyLevel)-1), &gEmptyTrainerBreakBars);
}

static const struct TrainerMon sPool[] =
{
    {.species = SPECIES_WOBBUFFET, .lvl = 10},
    {.species = SPECIES_PICHU, .lvl = 10},
    {.species = SPECIES_GEODUDE, .lvl = 10},
};
static const struct TrainerBreakBarRule sPoolRules[] =
{
    {TRAINER_BREAK_BAR_ENTRY, 0, &gTrainerBreakBarProfiles[0]},
    {TRAINER_BREAK_BAR_ENTRY, 1, &gTrainerBreakBarProfiles[1]},
    {TRAINER_BREAK_BAR_ENTRY, 2, &gTrainerBreakBarProfiles[2]},
};
static const struct TrainerBreakBarRules sPoolRuleSet = TRAINER_BREAK_BAR_RULES(sPoolRules);

TEST("Trainer Break Bar assignment follows selected pool entries into either trainer's party offset")
{
    struct Trainer trainer = {.party = sPool, .partySize = 2, .poolSize = ARRAY_COUNT(sPool)};
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    for (u32 offset = 0; offset <= PARTY_SIZE / 2; offset += PARTY_SIZE / 2)
    {
        for (u32 trial = 0; trial < 8; trial++)
        {
            memset(gBattleStruct->partyState, 0, sizeof(gBattleStruct->partyState));
            CreateNPCTrainerPartyWithBreakBars(&gEnemyParty[offset], &trainer, offset == 0,
                BATTLE_TYPE_TRAINER | BATTLE_TYPE_TWO_OPPONENTS, &sPoolRuleSet);
            for (u32 slot = offset; slot < offset + trainer.partySize; slot++)
            {
                u32 species = GetMonData(&gEnemyParty[slot], MON_DATA_SPECIES);
                u32 source = species == SPECIES_WOBBUFFET ? 0 : species == SPECIES_PICHU ? 1 : 2;
                EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][slot].breakBarConfig, &gTrainerBreakBarProfiles[source]);
                EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_OPPONENT][slot].breakBarsRemaining, source + 1);
            }
            EXPECT_EQ(gBattleStruct->partyState[B_SIDE_PLAYER][0].breakBarConfig, NULL);
        }
    }
    Free(gBattleStruct);
    gBattleStruct = NULL;
}

static void SetOverride(struct ScriptContext *ctx, u16 trainerId, const struct TrainerBreakBarRules *rules)
{
    const struct __attribute__((packed))
    {
        u16 trainerId;
        const struct TrainerBreakBarRules *rules;
    } args = {trainerId, rules};
    ctx->scriptPtr = (const u8 *)&args;
    Script_SetTrainerBreakBars(ctx);
    ctx->scriptPtr = NULL;
}

TEST("Trainer Break Bar script overrides replace defaults and are consumed once by trainer ID")
{
    struct ScriptContext ctx;
    InitScriptContext(&ctx, NULL, NULL);
    SetOverride(&ctx, TRAINER_KOANKO, &sRuleSet);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    SetOverride(&ctx, TRAINER_KOANKO, &gEmptyTrainerBreakBars);
    ScriptContext_ContinueScript(&ctx);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_ROXANNE_1), NULL);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), &gEmptyTrainerBreakBars);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), NULL);
    ScriptContext_Init();
}

TEST("Trainer Break Bar script overrides assign inline configs and explicit empty replacements")
{
    struct ScriptContext ctx;
    const struct TrainerBreakBarRules *rules = sInlineTrainerRules[1][DIFFICULTY_NORMAL];
    InitScriptContext(&ctx, NULL, NULL);
    SetOverride(&ctx, TRAINER_KOANKO, rules);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    SetOverride(&ctx, TRAINER_ROXANNE_1, rules);
    SetOverride(&ctx, TRAINER_ROXANNE_1, &gEmptyTrainerBreakBars);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    ScriptContext_ContinueScript(&ctx);
    const struct TrainerBreakBarRules *configured = Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO);
    const struct TrainerBreakBarRules *disabled = Script_TakeTrainerBreakBarOverride(TRAINER_ROXANNE_1);
    EXPECT_EQ(configured, rules);
    EXPECT_EQ(disabled, &gEmptyTrainerBreakBars);
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    InitializeTrainerBreakBar(0, configured, SPECIES_WOBBUFFET, 2);
    InitializeTrainerBreakBar(1, configured, SPECIES_WOBBUFFET, 3);
    InitializeTrainerBreakBar(2, disabled, SPECIES_WOBBUFFET, 2);
    EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarConfig, rules->rules[1].config);
    EXPECT_EQ((u32)gBattleStruct->partyState[B_SIDE_OPPONENT][0].breakBarsRemaining, 2);
    EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][1].breakBarConfig, NULL);
    EXPECT_EQ(gBattleStruct->partyState[B_SIDE_OPPONENT][2].breakBarConfig, NULL);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), NULL);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_ROXANNE_1), NULL);
    Free(gBattleStruct);
    gBattleStruct = NULL;
    ScriptContext_Init();
}

TEST("Trainer Break Bar unused script overrides clear on stop, reset, and battle cleanup")
{
    struct ScriptContext ctx;
    InitScriptContext(&ctx, NULL, NULL);
    SetOverride(&ctx, TRAINER_KOANKO, sInlineTrainerRules[1][DIFFICULTY_NORMAL]);
    StopScript(&ctx);
    EXPECT_EQ(ctx.trainerBreakBarRules[0], NULL);
    SetOverride(&ctx, TRAINER_KOANKO, sInlineTrainerRules[1][DIFFICULTY_NORMAL]);
    InitScriptContext(&ctx, NULL, NULL);
    EXPECT_EQ(ctx.trainerBreakBarRules[0], NULL);
    SetOverride(&ctx, TRAINER_KOANKO, sInlineTrainerRules[1][DIFFICULTY_NORMAL]);
    ScriptContext_ContinueScript(&ctx);
    Script_ClearTrainerBreakBarOverrides();
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), NULL);
    SetOverride(&ctx, TRAINER_KOANKO, sInlineTrainerRules[1][DIFFICULTY_NORMAL]);
    ScriptContext_ContinueScript(&ctx);
    ScriptContext_Init();
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), NULL);
}

TEST("Trainer Break Bar overrides keep two opponents independent and reject an overflowing queue")
{
    struct ScriptContext ctx;
    InitScriptContext(&ctx, NULL, NULL);
    SetOverride(&ctx, TRAINER_KOANKO, &sRuleSet);
    SetOverride(&ctx, TRAINER_ROXANNE_1, &gEmptyTrainerBreakBars);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    SetOverride(&ctx, TRAINER_ROXANNE_2, &sPoolRuleSet);
    EXPECT_EQ(gSpecialVar_Result, FALSE);
    ScriptContext_ContinueScript(&ctx);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_ROXANNE_1), &gEmptyTrainerBreakBars);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_KOANKO), &sRuleSet);
    EXPECT_EQ(Script_TakeTrainerBreakBarOverride(TRAINER_ROXANNE_2), NULL);
    ScriptContext_Init();
}

TEST("Trainer Break Bar battle-script activation preserves active stacks and custom profiles")
{
    static const struct BreakBarConfig config =
    {
        .extraBars = 2,
        .flags = BREAK_BAR_DEFER_INITIAL_DRAW,
        .phases[0].onBreakScript = BattleScript_BreakBarRefill,
    };
    static const u8 fail[] = {0};
    const struct __attribute__((packed))
    {
        u8 opcode;
        void (*func)(void);
        u8 battler;
        const struct BreakBarConfig *config;
        const u8 *fail;
    } command = {0, BS_ActivateBreakBar, BS_OPPONENT1, &config, fail};
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gBattlersCount = 2;
    gBattlerPositions[B_POSITION_OPPONENT_LEFT] = B_POSITION_OPPONENT_LEFT;
    gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT] = 0;
    gBattleMons[B_POSITION_OPPONENT_LEFT].hp = 15;
    gAbsentBattlerFlags = 0;
    gBattlescriptCurrInstr = (const u8 *)&command;
    BS_ActivateBreakBar();
    EXPECT_EQ(gBattlescriptCurrInstr, (const u8 *)(&command + 1));
    EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 2);
    EXPECT_EQ(GetBattlerPartyState(B_POSITION_OPPONENT_LEFT)->breakBarConfig, &config);
    EXPECT_EQ(gBattleMons[B_POSITION_OPPONENT_LEFT].hp, 15);
    gBattlescriptCurrInstr = (const u8 *)&command;
    BS_ActivateBreakBar();
    EXPECT_EQ(gBattlescriptCurrInstr, fail);
    EXPECT_EQ(GetBreakBarCount(B_POSITION_OPPONENT_LEFT), 2);
    Free(gBattleStruct);
    gBattleStruct = NULL;
}
