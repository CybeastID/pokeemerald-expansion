#ifndef GUARD_CASTORIA_NAME_THEFT_H
#define GUARD_CASTORIA_NAME_THEFT_H

#include "global.h"

// Result of the naming screen interaction
enum CastoriaNameResult
{
    CASTORIA_NAME_BACKED_OUT = 0,   // Player hit B / left empty
    CASTORIA_NAME_FALSE,            // Entered a name, but not the player's true name
    CASTORIA_NAME_TRUE,             // Entered name matches gSaveBlock2Ptr->playerName
};

#define CASTORIA_COMMAND_THIS_TURN        (1 << 0)
#define CASTORIA_COMMAND_LAST_TURN        (1 << 1)
#define CASTORIA_FORCED_MOVE_SLOT_SHIFT   2
#define CASTORIA_FORCED_MOVE_SLOT_MASK    (3 << CASTORIA_FORCED_MOVE_SLOT_SHIFT)
#define CASTORIA_MAXMOVE_EXPIRED        (1 << 4)


// ---------------------------------------------------------------
// callnative entry point from BattleScript
//   Presents a two-choice battle textbox:
//     Top: player's real name (CASTORIA_NAME_TRUE)
//     Bottom: "Francis Xavier" (CASTORIA_NAME_FALSE)
//     B: backs out (CASTORIA_NAME_BACKED_OUT)
// ---------------------------------------------------------------
void BS_DoCastoriaNameChoice(void);

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

// callnative: prepares the worst move choice in gBattleTextBuff1
void BS_CastoriaPrepareWorstMove(void);

// callnative: decrements castoriaTrueNameCharges by 1
void BS_CastoriaDecTrueNameCharges(void);

// callnative: jumps to the correct BattleScript branch based on nameResult
void BS_JumpCastoriaNameResult(void);

#endif // GUARD_CASTORIA_NAME_THEFT_H
