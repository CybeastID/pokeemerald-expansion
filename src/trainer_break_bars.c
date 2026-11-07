#include "global.h"
#include "battle.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "trainer_break_bars.h"
#include "constants/trainers.h"

#include "data/trainer_break_bars.h"

static const struct TrainerBreakBarRules *SelectTrainerBreakBarRules(const struct TrainerBreakBarRules *const rules[DIFFICULTY_COUNT], enum DifficultyLevel difficulty)
{
    if ((u32)difficulty >= DIFFICULTY_COUNT)
        return &gEmptyTrainerBreakBars;
    if (rules[difficulty] != NULL)
        return rules[difficulty];
    return rules[DIFFICULTY_NORMAL] ?: &gEmptyTrainerBreakBars;
}

const struct TrainerBreakBarRules *GetTrainerBreakBarRules(u16 trainerId, enum DifficultyLevel difficulty)
{
    if (trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT)
        return &gEmptyTrainerBreakBars;
    return SelectTrainerBreakBarRules(sTrainerBreakBars[trainerId], difficulty);
}

#if TESTING
const struct TrainerBreakBarRules *Test_SelectTrainerBreakBarRules(const struct TrainerBreakBarRules *const rules[DIFFICULTY_COUNT], enum DifficultyLevel difficulty)
{
    return SelectTrainerBreakBarRules(rules, difficulty);
}
#endif

const struct BreakBarConfig *ResolveTrainerBreakBar(const struct TrainerBreakBarRules *rules, u32 species, u32 sourceEntry)
{
    const struct BreakBarConfig *speciesConfig = NULL;
    if (rules == NULL || rules->rules == NULL)
        return NULL;
    for (u32 i = 0; i < rules->count; i++)
    {
        const struct TrainerBreakBarRule *rule = &rules->rules[i];
        if (rule->selector == TRAINER_BREAK_BAR_ENTRY && rule->value == sourceEntry)
            return rule->config;
        if (rule->selector == TRAINER_BREAK_BAR_SPECIES && rule->value == species)
            speciesConfig = rule->config;
    }
    return speciesConfig;
}

void InitializeTrainerBreakBar(u32 partySlot, const struct TrainerBreakBarRules *rules, u32 species, u32 sourceEntry)
{
    const struct BreakBarConfig *config = ResolveTrainerBreakBar(rules, species, sourceEntry);
    if (gBattleStruct == NULL || partySlot >= PARTY_SIZE || config == NULL
     || config->extraBars == 0 || config->extraBars > BREAK_BAR_MAX_EXTRA_BARS)
        return;
    // Party creation precedes battler/healthbox creation. Register without
    // touching graphics; the first healthbox draw reads this party-owned state.
    struct PartyState *state = &gBattleStruct->partyState[B_SIDE_OPPONENT][partySlot];
    state->breakBarConfig = config;
    state->breakBarsRemaining = config->extraBars;
}

void BS_ActivateBreakBar(void)
{
    NATIVE_ARGS(u8 battler, const struct BreakBarConfig *config, const u8 *failInstr);
    u32 battler = GetBattlerForBattleScript(cmd->battler);
    gBattlescriptCurrInstr = ActivateBreakBar(battler, cmd->config) ? cmd->nextInstr : cmd->failInstr;
}
