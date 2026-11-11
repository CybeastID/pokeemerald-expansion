#ifndef GUARD_TRAINER_BREAK_BARS_H
#define GUARD_TRAINER_BREAK_BARS_H

#include "battle_break_bar.h"
#include "constants/difficulty.h"

enum TrainerBreakBarSelector
{
    TRAINER_BREAK_BAR_SPECIES,
    TRAINER_BREAK_BAR_ENTRY,
};

struct TrainerBreakBarRule
{
    enum TrainerBreakBarSelector selector;
    u16 value; // Exact species or zero-based source party/pool entry.
    const struct BreakBarConfig *config; // NULL explicitly disables a match.
};

struct TrainerBreakBarRules
{
    const struct TrainerBreakBarRule *rules;
    u16 count;
};

#define TRAINER_BREAK_BAR_RULES(array) { .rules = (array), .count = ARRAY_COUNT(array) }

// Use in file-scope initializers so the ruleset, rules and any inline configs
// have static lifetime. Use &gEmptyTrainerBreakBars for an explicit empty set.
#define INLINE_BREAK_BAR_RULES(...) \
    (&(const struct TrainerBreakBarRules) TRAINER_BREAK_BAR_RULES(((const struct TrainerBreakBarRule[]) { __VA_ARGS__ })))

extern const struct BreakBarConfig gTrainerBreakBarProfiles[BREAK_BAR_MAX_EXTRA_BARS];
extern const struct BreakBarConfig gKazuradropBreakBarConfig;
extern const struct TrainerBreakBarRules gKoankoInitialBreakBars;
extern const struct TrainerBreakBarRules gKoankoConjuredBreakBars;
extern const struct TrainerBreakBarRules gEmptyTrainerBreakBars;

const struct TrainerBreakBarRules *GetTrainerBreakBarRules(u16 trainerId, enum DifficultyLevel difficulty);
const struct BreakBarConfig *ResolveTrainerBreakBar(const struct TrainerBreakBarRules *rules, u32 species, u32 sourceEntry);
void InitializeTrainerBreakBar(u32 partySlot, const struct TrainerBreakBarRules *rules, u32 species, u32 sourceEntry);
void TrainerBreakBarsBeginParty(struct Pokemon *party);
void TrainerBreakBarsRecordGeneratedMon(struct Pokemon *party, u32 partySlot, u32 species, u32 sourceEntry);
void TrainerBreakBarsApplyPending(void);
void TrainerBreakBarsClearPending(void);
void BS_ActivateBreakBar(void);

#if TESTING
const struct TrainerBreakBarRules *Test_SelectTrainerBreakBarRules(const struct TrainerBreakBarRules *const rules[DIFFICULTY_COUNT], enum DifficultyLevel difficulty);
#endif

#endif
