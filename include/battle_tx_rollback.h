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

enum TxRollbackWish
{
    TX_WISH_NONE,
    TX_WISH_REVERSAL,
    TX_WISH_RECLAMATION,
    TX_WISH_CALAMITY,
    TX_WISH_RESTORATION,
    TX_WISH_RECIPROCITY,
};

struct TxRollbackProfile
{
    // A script subroutine: it must return, never end an ordinary action.
    const u8 *ambushScript;
};

struct TxRollbackBattleData
{
    const struct TxRollbackProfile *profile;
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
bool32 TxRollbackCanRevive(u32 side, u32 partyIndex);
bool32 TxRollbackOwnsPartyMon(u32 side, u32 partyIndex);
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
void BS_TxRollbackTryWish(void);
void BS_TxRollbackEndurance(void);
void BS_TxRollbackBeginPhase(void);
void BS_TxRollbackEndPhase(void);
void BS_TxRollbackReduction(void);
void BS_TxRollbackPrepareDoomDesire(void);
void TxRollbackMirrorHealing(u32 battler, u32 oldHp, const u8 *returnInstr);
void BS_TxRollbackBeginReciprocity(void);
void BS_TxRollbackEndReciprocity(void);

#endif
