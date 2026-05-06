# Kazuradrop Implementation - Current Progress

## Completed Items (13/13)

- [x] Design discussion: Kazuradrop's lore, Bug Space mechanics, transformation phases
- [x] Finalized both movesets: Kazuradrop (pre-transformation) + Sakura Five (post-transformation)
- [x] Add move constants to moves.h (MELT_VIRUS, TRASH_CRUSH, INFINITE_GROWTH, CRACK_ICE)
- [x] Add move data entries to moves_info.h
- [x] Add MOVE_EFFECT_MELT_VIRUS and MOVE_EFFECT_TRASH_CRUSH to battle.h MoveEffect enum
- [x] Add EFFECT_INFINITE_GROWTH to battle_move_effects.h
- [x] Keep Kazuradrop's Dark/Fairy type (keeper's preference)
- [x] Add sKazuradropLevelUpLearnset to gen_9 level_up_learnsets with P_FAMILY_KAZURADROP guard
- [x] Update species data to use new learnset
- [x] Add damage clamp (10% HP) and transformation trigger in battle_script_commands.c
- [x] Add Infinite Growth end-of-turn handler in battle_end_turn.c
- [x] Add Melt Virus draining status handler in battle_end_turn.c
- [x] Add Trash & Crush OHKO logic with Bug Space tier interaction

## Files Modified

### include/constants/battle.h
- Added VOLATILE_MELT_VIRUS, VOLATILE_MELT_VIRUS_BY, VOLATILE_INFINITE_GROWTH to VOLATILE_DEFINITIONS
- MOVE_EFFECT_MELT_VIRUS and MOVE_EFFECT_TRASH_CRUSH already in MoveEffect enum

### include/constants/battle_end_turn.h
- Added ENDTURN_INFINITE_GROWTH and ENDTURN_MELT_VIRUS to EndTurnResolutionOrder

### src/battle_script_commands.c
- TryTriggerKazuradropTransformation() - clamps damage at 10% HP threshold
- ApplyKazuradropTransformation() - full restore, double max HP, accelerate decay, swap to Sakura Five moves
- MOVE_EFFECT_MELT_VIRUS case in SetMoveEffect() - applies meltVirus volatile status
- MOVE_EFFECT_TRASH_CRUSH case in SetMoveEffect() - OHKO at Bug Space OHKO tier+, 40% OHKO chance at MINIMIZE+ tier.

### src/battle_end_turn.c
- HandleEndTurnMeltVirus() - 1/8 HP damage (1/4 at Bug Space MINIMIZE+ tier)
- HandleEndTurnInfiniteGrowth() - +10% raw stats, +10% max HP, heal 1/16 HP each turn

## Remaining Work (Battle Scripts in data/battle_scripts_1.s)

The following battle script labels need to be defined (text/effect scripts, not animations):

1. **BattleScript_MoveEffectMeltVirus** - Applied when Melt Virus hits: print message + set volatile
2. **BattleScript_MoveEffectTrashCrush** - Applied when Trash & Crush hits: print message + OHKO effect
3. **BattleScript_MeltVirusTurnDmg** - End-of-turn Melt Virus damage: print damage message + apply damage
4. **BattleScript_InfiniteGrowthHeal** - Infinite Growth heal: print heal message + apply heal
5. **BattleScript_InfiniteGrowthStatUp** - Infinite Growth stat-up: print stat-up message

Reference: Check EFFECT_RAZE_AUTHORITY / DM_Authority in battle_scripts_1.s for how text/effect scripts are wired up.