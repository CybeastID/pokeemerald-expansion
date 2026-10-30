#ifndef GUARD_CASTORIA_NAME_THEFT_H
#define GUARD_CASTORIA_NAME_THEFT_H

#include "global.h"

// Result of the naming screen interaction
enum CastoriaNameResult
{
    CASTORIA_NAME_NONE = 0,         // Not asked yet (zero-init battle struct)
    CASTORIA_NAME_BACKED_OUT,       // Player hit B / left empty
    CASTORIA_NAME_FALSE,            // Entered a name, but not the player's true name
    CASTORIA_NAME_TRUE,             // Entered name matches gSaveBlock2Ptr->playerName
};

enum CastoriaWishOutcome
{
    CASTORIA_WISH_NONE,
    CASTORIA_WISH_DOMINATION,
    CASTORIA_WISH_REDUCTION,
    CASTORIA_WISH_MORE_WISHES,
    CASTORIA_WISH_ENDURANCE,
    CASTORIA_WISH_DEBILITATION,
    CASTORIA_WISH_NORMALITY,
};

#define CASTORIA_COMMAND_THIS_TURN        (1 << 0)
#define CASTORIA_COMMAND_LAST_TURN        (1 << 1)
#define CASTORIA_FORCED_MOVE_SLOT_SHIFT   2
#define CASTORIA_FORCED_MOVE_SLOT_MASK    (3 << CASTORIA_FORCED_MOVE_SLOT_SHIFT)
#define CASTORIA_MAXMOVE_EXPIRED        (1 << 4)
#define CASTORIA_COMMAND_UI_LOCKED        (1 << 5)


// ---------------------------------------------------------------
// callnative entry point from BattleScript
//   Presents a two-choice battle textbox:
//     Top: player's real name (CASTORIA_NAME_TRUE)
//     Bottom: "Francis Xavier" (CASTORIA_NAME_FALSE)
//     B: backs out (CASTORIA_NAME_BACKED_OUT)
// ---------------------------------------------------------------
void BS_DoCastoriaNameChoice(void);
bool32 TryCastoriaNameTheft(u32 battler, bool32 viaCall);
void BS_PolymorphTarget(void);
bool32 TryRestorePolymorphedMon(u32 partyIndex);
// ---------------------------------------------------------------
// True Name Command — AI decision + worst-move scoring
// ---------------------------------------------------------------

// Called at turn start. Castoria decides whether to spend a
// charge to force the player's worst move this turn.
bool32 CastoriaDecideUseCommand(u32 battler);

// Scores all 4 of target's moves from Castoria's perspective.
// Returns the move slot index (0-3) with the LOWEST score.
u8 CastoriaFindWorstMove(u32 castoriaBattler, u32 targetBattler);

// Apply stat boosts + volatiles based on name-theft result.
void BS_ApplyCastoriaNameResult(void);
void BS_ShowMageArmorIcon(void);

// callnative: prepares the worst move choice in gBattleTextBuff1
void BS_CastoriaPrepareWorstMove(void);

// callnative: decrements castoriaTrueNameCharges by 1
void BS_CastoriaDecTrueNameCharges(void);

// callnative: jumps to the correct BattleScript branch based on nameResult
void BS_JumpCastoriaNameResult(void);

// ---------------------------------------------------------------
// Arcanum — extra magical move slots for Castoria
// ---------------------------------------------------------------

// Populates CastoriaBattleData.arcanumMoves[] with the default spell pool.
void CastoriaInitArcanum(u32 battler);

// Availability of logical spell slots (4-7), independent of ordinary PP.
bool32 IsArcanumSlotUsable(u32 battler, u32 slot);
bool32 HasUsableArcanumMove(u32 battler);
bool32 HasMageArmor(u32 battler);

// Only implemented, route-eligible outcomes can be selected or executed.
bool32 CastoriaCanWishFor(u32 battler, u32 outcome);
u32 CastoriaGetAvailableWishOutcome(u32 battler);
void BS_CastoriaTryWish(void);

#if TESTING
u16 Test_GetCastoriaArcanumMove(u32 slot);
#endif

// Returns the move ID for any logical slot (0-3 normal, 4-7 arcanum).
// Returns MOVE_NONE for invalid slots or non-Castoria battlers.
u16 CastoriaGetMoveAtSlot(u32 battler, u8 slot);

#endif // GUARD_CASTORIA_NAME_THEFT_H
