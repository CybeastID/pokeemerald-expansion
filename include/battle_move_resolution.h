#ifndef GUARD_BATTLE_MOVE_RESOLUTION_H
#define GUARD_BATTLE_MOVE_RESOLUTION_H

#include "constants/battle_move_resolution.h"

enum CancelerResult DoAttackCanceler(void);
enum MoveEndResult DoMoveEnd(enum MoveEndState endMode, enum MoveEndState endState);
enum MoveResult DoStatChange(void);

void SetFutureSightAttack(enum BattlerId attacker, enum BattlerId target, enum Move move);

bool32 IsStatChangeMove(enum Move move);
#endif // GUARD_BATTLE_MOVE_RESOLUTION_H
