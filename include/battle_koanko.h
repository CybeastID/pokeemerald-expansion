#ifndef GUARD_BATTLE_KOANKO_H
#define GUARD_BATTLE_KOANKO_H

enum KoankoPhase
{
    KOANKO_ORDINARY_TEAM,
    KOANKO_CONJURING,
    KOANKO_CONJURED_TEAM,
};

bool32 TryStartKoankoConjuring(const u8 *returnInstr);
void ConjureKoankoTeam(void);
void BS_ConjureKoankoTeam(void);
void KoankoOnSwitchIn(u32 battler);
s32 AI_KoankoTrainer(u32 battlerAtk, u32 battlerDef, u32 move, s32 score);
void KoankoSetupTrainerAi(void);

struct Pokemon;
struct ScriptContext;
void Script_StartKoankoSmeargleDebugBattle(struct ScriptContext *ctx);
bool32 KoankoTryCreateDebugSmeargleParty(struct Pokemon *party, u16 trainerNum, bool32 firstTrainer);

#endif
