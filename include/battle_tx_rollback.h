#ifndef GUARD_BATTLE_TX_ROLLBACK_H
#define GUARD_BATTLE_TX_ROLLBACK_H

#include "global.h"

enum TxRollbackStage
{
    TX_ROLLBACK_NONE,
    TX_ROLLBACK_REQUESTED,
    TX_ROLLBACK_ENTERING,
    TX_ROLLBACK_AMBUSH_PENDING,
    TX_ROLLBACK_AMBUSH_RUNNING,
    TX_ROLLBACK_ACTIVE,
    TX_ROLLBACK_DEFEATED,
};

enum TxRollbackSlotPolicy
{
    TX_SLOT_NORMAL,
    TX_SLOT_DISABLED,
    TX_SLOT_RESERVED,
    TX_SLOT_RETIRED,
};

enum TxRollbackCombatProfile
{
    TX_PROFILE_NONE,
    TX_PROFILE_CASTORIA_SUMMON,
    TX_PROFILE_KOANKO_SOLO,
};

enum TxRollbackWish
{
    TX_WISH_NONE,
    TX_WISH_REVERSAL,
    TX_WISH_RECLAMATION,
    TX_WISH_CALAMITY,
    TX_WISH_RESTORATION,
    TX_WISH_RECIPROCITY,
    TX_WISH_REDUCTION,
};

// Battle-only maxHP view. Party HP is synchronized as a proportion of the
// original maximum; reloading a controller must not repeatedly round/heal it.
struct TxRollbackHealth
{
    u32 hpFraction;
    u16 originalMaxHp;
    u16 appliedMaxHp;
    u16 lastHp;
    u8 percent;
};

struct TxRollbackProfile
{
    // A script subroutine: it must return, never end an ordinary action.
    const u8 *ambushScript;
};

struct TxRollbackBattleData
{
    const struct TxRollbackProfile *profile;
    struct TxRollbackHealth health;
    u8 combatProfile;
    u8 ownerTrainer;
    bool8 ownerDefeated;
    u8 stage;
    u8 caster;
    u8 partyIndex;
    u8 casterPartyIndex;
    u8 wishCandidate;
    u8 wishOutcome;
    u8 wishCandidateTarget; // Battler nominated during scoring.
    u8 wishTargetPartyIndex; // Committed identity; replacements cannot inherit it.
    bool8 reciprocity;
    u8 savedTarget;
    u8 savedEffectBattler;
    u8 savedScriptingBattler;
    u8 savedAttacker;
    u8 savedActionFunc;
};

bool32 TxRollbackCanCreate(u32 caster);
bool32 TxRollbackRequestCreation(u32 caster);
enum TxRollbackSlotPolicy TxRollbackGetSlotPolicy(u32 battler);
bool32 TxRollbackCanPopulateSlot(u32 battler);
bool32 TxRollbackCanRevive(enum BattleTrainer trainer, u32 partyIndex);
bool32 TxRollbackOwnsPartyMon(enum BattleTrainer trainer, u32 partyIndex);
bool32 TxRollbackIsSummonedPartyMon(enum BattleTrainer trainer, u32 partyIndex);
u32 TxRollbackGetOwnerBattler(void);
void TxRollbackRegisterKoanko(u32 battler);
void TxRollbackOnFaint(u32 battler);
bool32 TxRollbackTryAmbush(void);
void BS_TxRollbackCreate(void);
void BS_TxRollbackInitialize(void);
void BS_TxRollbackFinishEntry(void);
void BS_TxRollbackCallAmbush(void);
void BS_TxRollbackFinishAmbush(void);

bool32 TxRollbackHasWishes(u32 battler);
bool32 TxRollbackCanWishFor(u32 battler, u32 wish, u32 target);
bool32 TxRollbackHasUsableWish(u32 battler);
u32 TxRollbackGetWishCount(u32 battler);
u32 TxRollbackGetWishAtIndex(u32 battler, u32 index);
void BS_TxRollbackTryWish(void);
void BS_WishfulThinkingTry(void);
void BS_TxRollbackEndurance(void);
void BS_TxRollbackBeginPhase(void);
void BS_TxRollbackEndPhase(void);
void BS_TxRollbackReduction(void);
void BS_TxRollbackPrepareDoomDesire(void);
void TxRollbackMirrorHealing(u32 battler, u32 oldHp, const u8 *returnInstr);
void BS_TxRollbackBeginReciprocity(void);
void BS_TxRollbackEndReciprocity(void);

void TxRollbackInitializeHealth(u32 battler);
bool32 TxRollbackHasHealthOverlay(u32 battler);
void TxRollbackOnStatsCopied(u32 battler);
void TxRollbackNormalizeRequest(u32 battler, u32 request, u32 monMask, void *data);
void TxRollbackFinishBattle(void);
void BS_TxRollbackDeteriorate(void);

#endif
