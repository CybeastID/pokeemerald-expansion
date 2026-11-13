// Faerie move-resolution policies. Included by battle_move_resolution.c.
static enum CancelerResult CancelerTimestop(struct BattleCalcValues *cv)
{
    // A timestopped battler's move resolves into nothing, with no message printed.
    if (gBattleMons[cv->battlerAtk].volatiles.timestop > 1)
    {
        gBattleStruct->unableToUseMove = TRUE;
        gBattlescriptCurrInstr = BattleScript_MoveEnd;
        return CANCELER_RESULT_FAILURE;
    }
    return CANCELER_RESULT_SUCCESS;
}

static u16 GetMoveBasePowerForBugSpace(u32 move)
{
    // For status moves, use 0 (they should not be blocked by BP threshold)
    if (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS)
        return 0;
    
    // Get the move's base power
    u16 power = GetMovePower(move);
    
    // Handle special cases where power is 0 but move is not status
    if (power == 0)
    {
        // Moves with variable power - use a default threshold value
        switch (move)
        {
        case MOVE_GYRO_BALL:
        case MOVE_HEAT_CRASH:
        case MOVE_HEAVY_SLAM:
        case MOVE_LOW_KICK:
        case MOVE_GRASS_KNOT:
        case MOVE_FLAIL:
        case MOVE_REVERSAL:
        case MOVE_WRING_OUT:
        case MOVE_HARD_PRESS:
            // Variable power moves - use a moderate value
            return 50;
        default:
            return 0;
        }
    }
    
    return power;
}

static bool32 IsMoveBlockedByBugSpace(u32 move)
{
    // If Bug Space is not active, don't block
    if (!gBattleStruct->bugSpace.active)
        return FALSE;
    
    // Only block moves from Kazuradrop's opponents (one-sided effect)
    if (GetBattlerSide(gBattlerAttacker) == GetBattlerSide(gBattleStruct->bugSpace.sourceBattler))
        return FALSE;
    
    // If threshold is 0 (floor reached), don't block (passive OHKO phase)
    // DEADCODE (NOT COMPILED): floor early-return, currently not needed. Kept for posterity.
    // if (gBattleStruct->bugSpace.bpThreshold == 0)
       // return FALSE;
    
    // Get move's base power
    u16 moveBP = GetMoveBasePowerForBugSpace(move);
    
    // Status moves (BP = 0) are not blocked
    if (moveBP == 0)
        return FALSE;
    
    // Check if move's BP >= threshold
    return moveBP >= gBattleStruct->bugSpace.bpThreshold;
}

static enum CancelerResult CancelerBugSpace(struct BattleCalcValues *cv)
{
    if (gBattleStruct->moveResultFlags[gBattlerTarget] & MOVE_RESULT_NO_EFFECT
     || !IsMoveBlockedByBugSpace(cv->move))
        return CANCELER_RESULT_SUCCESS;

    gBattleStruct->moveResultFlags[gBattlerTarget] |= MOVE_RESULT_NO_EFFECT;
    if (IsExplosionMove(cv->move))
        ShowInvincibleBattleStatusIcon(gBattlerTarget);
    gBattlerAbility = gBattleStruct->bugSpace.sourceBattler;
    if (gBattleStruct->bugSpace.currentTier == BUGSPACE_TIER_PASSIVE_OHKO)
        gBattlescriptCurrInstr = BattleScript_BugSpaceBlocked_POHKO;
    else if (!gBattleStruct->bugSpace.hasBlocked)
    {
        gBattlescriptCurrInstr = BattleScript_BugSpaceBlocked_First;
        gBattleStruct->bugSpace.hasBlocked = TRUE;
    }
    else
        gBattlescriptCurrInstr = BattleScript_BugSpaceBlocked_Repeat;
    return CANCELER_RESULT_FAILURE;
}

static bool32 TrashCrushInstantKnockout(struct BattleCalcValues *cv, enum BattlerId target)
{
    if (cv->moveEffect != EFFECT_TRASH_CRUSH)
        return FALSE;
    bool32 inBugSpace = gBattleStruct->bugSpace.active
        && GetBattlerSide(target) != GetBattlerSide(gBattleStruct->bugSpace.sourceBattler);
    if (inBugSpace && gBattleStruct->bugSpace.currentTier >= BUGSPACE_TIER_OHKO)
        return TRUE;
    u32 chance = gBattleMons[target].volatiles.minimize
        || (inBugSpace && gBattleStruct->bugSpace.currentTier >= BUGSPACE_TIER_MINIMIZE) ? 40 : 20;
    return RandomPercentage(RNG_TRASH_CRUSH_OHKO, chance);
}
