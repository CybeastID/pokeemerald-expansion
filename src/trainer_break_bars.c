#include "global.h"
#include "battle_script_native.h"
#include "battle.h"
#include "battle_script_commands.h"
#include "battle_scripts.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "pokemon.h"
#include "trainer_break_bars.h"
#include "constants/trainers.h"

#include "data/trainer_break_bars.h"

// The target generates trainer parties before allocating BattleStruct. Keep
// only static configuration pointers until battle initialization consumes them.
static const struct BreakBarConfig *sPendingConfigs[MAX_BATTLE_TRAINERS][PARTY_SIZE];
static const struct TrainerBreakBarRules *sGenerationRules;

static u32 TrainerForParty(struct Pokemon *party)
{
    for (u32 trainer = 0; trainer < MAX_BATTLE_TRAINERS; trainer++)
        if (party == gParties[trainer])
            return trainer;
    return MAX_BATTLE_TRAINERS;
}

void TrainerBreakBarsBeginParty(struct Pokemon *party)
{
    u32 trainer = TrainerForParty(party);
    if (trainer < MAX_BATTLE_TRAINERS)
        memset(sPendingConfigs[trainer], 0, sizeof(sPendingConfigs[trainer]));
}

void TrainerBreakBarsRecordGeneratedMon(struct Pokemon *party, u32 partySlot, u32 species, u32 sourceEntry)
{
    u32 trainer = TrainerForParty(party);
    if (trainer >= MAX_BATTLE_TRAINERS || partySlot >= PARTY_SIZE)
        return;
    const struct BreakBarConfig *config = ResolveTrainerBreakBar(sGenerationRules, species, sourceEntry);
    if (config == NULL || config->extraBars == 0 || config->extraBars > BREAK_BAR_MAX_EXTRA_BARS)
        return;
    if (gBattleStruct != NULL)
    {
        struct PartyState *state = &gBattleStruct->partyState[trainer][partySlot];
        state->breakBarConfig = config;
        state->breakBarsRemaining = config->extraBars;
    }
    else
        sPendingConfigs[trainer][partySlot] = config;
}

void TrainerBreakBarsClearPending(void)
{
    memset(sPendingConfigs, 0, sizeof(sPendingConfigs));
    sGenerationRules = NULL;
}

void TrainerBreakBarsApplyPending(void)
{
    if (gBattleStruct != NULL && (gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        for (u32 trainer = 0; trainer < MAX_BATTLE_TRAINERS; trainer++)
            for (u32 slot = 0; slot < PARTY_SIZE; slot++)
            {
                const struct BreakBarConfig *config = sPendingConfigs[trainer][slot];
                if (config != NULL)
                {
                    struct PartyState *state = &gBattleStruct->partyState[trainer][slot];
                    state->breakBarConfig = config;
                    state->breakBarsRemaining = config->extraBars;
                }
            }
    TrainerBreakBarsClearPending();
}

u8 CreateNPCTrainerPartyWithBreakBars(struct Pokemon *party, const struct Trainer *trainer, bool32 firstTrainer, u32 battleTypeFlags, const struct TrainerBreakBarRules *rules)
{
    u32 savedBattleTypeFlags = gBattleTypeFlags;
    const struct TrainerBreakBarRules *savedRules = sGenerationRules;
    (void)firstTrainer; // Each target trainer owns a complete, independent party.
    gBattleTypeFlags = battleTypeFlags;
    sGenerationRules = rules;
    CreateNPCTrainerPartyFromTrainer(party, trainer);
    sGenerationRules = savedRules;
    gBattleTypeFlags = savedBattleTypeFlags;
    // This adapter also supports temporary generation buffers. The target
    // CalculatePartyCount takes a trainer identity, never a party pointer.
    u32 count = 0;
    while (count < PARTY_SIZE && GetMonData(&party[count], MON_DATA_SPECIES) != SPECIES_NONE)
        count++;
    return count;
}

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
