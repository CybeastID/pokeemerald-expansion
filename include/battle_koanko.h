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

#endif
