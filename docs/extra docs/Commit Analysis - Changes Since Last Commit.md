# Changes Since Last Commit (`68c13b3582` — "Debilitation and BB reformat")

**Repository:** `pokeemerald-expansion`  
**Branch:** `mypatch`  
**Baseline:** `68c13b3582` (HEAD)  
**Scope:** 52 tracked files modified (+1288 / −803), 2 tracked files deleted, 21 untracked new files  

---

## 1. Executive Summary

Since the last commit, the battle system has seen three major architectural additions plus several integrations:

1. **TX Rollback / Wishes / Reduction** — an entirely new battle sub-system with formation stages, wish outcomes (More Wishes, Reduction, Normality), fractional-HP stat scaling, and script-context save/restore for "phase" moves.
2. **Battle Status Icons generalization** — the No Order Change icon (from the last commit) was generalized into a multi-icon system that now also owns Guts, Invincible, and Mage Armor badges, replacing Kazuradrop's bespoke buff icons and Castoria's Mage Armor UI.
3. **Kazuradrop UI trim** — `kazuradrop_battle_ui.c` was heavily stripped (Guts/Invincible icons, Alter Ego sigil, buff palette removed), delegating those to `battle_status_icons.c`; `castoria_battle_ui.c/.h` were deleted entirely.


## 2. New Files (Untracked)

### 2.1 TX Rollback Core

| File | Purpose |
|------|---------|
| `include/battle_tx_rollback.h` | `TxRollbackStage` (NONE → DEFEATED), `TxRollbackSlotPolicy` (NORMAL/DISABLED/RESERVED/RETIRED), `TxRollbackWish` (REVERSAL/RECLAMATION/CALAMITY/RESTORATION/RECIPROCITY), `TxRollbackProfile`, `TxRollbackBattleData` |
| `src/battle_tx_rollback.c` | Core implementation: `TxRollbackGetSlotPolicy`, `TxRollbackCanPopulateSlot`, formation stages, ambush context save/restore, `sTxBreakBars` config (2 extra bars with Break Wish / Break Doom Desire after-scripts) |

### 2.2 Reduction System

| File | Purpose |
|------|---------|
| `include/battle_reduction.h` | `ReductionOccupant` (hpFraction, base/applied stats, lastHp, partyIndex, depth, scaled, powerTrick), `ReductionData` (active, grace, nextLoss, occupant array), full function API |
| `src/battle_reduction.c` | Fractional HP tracking (`HP_FRACTION_ONE = 1<<24`), `ObserveHp` for sub-HP remainder tracking across rescalings, `ReductionCanonicalHp/MaxHp`, `ReductionActivate`, `ReductionEndTurn`, `ReductionBeginMutation/EndMutation`, `ReductionNormalizeRequest`, `ReductionRefreshDisplay`, `ReductionFinishBattle` |

### 2.3 Wish / Phase System

| File | Purpose |
|------|---------|
| `src/battle_tx_wishes.c` | `TxScriptContext` (full battle state snapshot), `sPhaseContext` / `sRecoveryContext` EWRAM save/restore, `TxRollbackHasWishes`, `TxRollbackOwnsPartyMon`, wish script execution (`BattleScript_TxWishReversal/Reclamation/Calamity/Restoration/Reciprocity`) |

### 2.4 New Graphics

| File | Purpose |
|------|---------|
| `graphics/battle_interface/no_order_change_source.png` | Source art for No Order Change icon |
| `graphics/object_events/pics/people/koanko.png` | Koanko overworld sprite |
| `graphics/object_events/palettes/koanko.pal` | Koanko overworld palette |
| `graphics/pokemon/jirachi/tx_rollback/README.md` | Jirachi TX rollback asset notes |
| `graphics/pokemon/jirachi/tx_rollback/Sprite-0001.aseprite` | Aseprite source |
| `graphics/pokemon/jirachi/tx_rollback/Sprite-00012.aseprite` | Aseprite source |
| `graphics/pokemon/jirachi/tx_rollback/anim_front4x.png` | 4x animation frame |
| `graphics/pokemon/jirachi/tx_rollback/anim_frontCHECKLATER.png` | Animation frame (placeholder) |
| `graphics/pokemon/jirachi/tx_rollback/normal.pal` | Jirachi palette |

### 2.5 New Documentation

| File | Purpose |
|------|---------|
| `docs/mechanics/WISH_REDUCTION_IMPLEMENTATION.md` | Wish + Reduction implementation notes |
| `docs/mechanics/tx_rollback.md` | TX Rollback mechanic documentation |

### 2.6 New Tests

| File | Purpose |
|------|---------|
| `test/battle/after_cleanup.c` | Post-battle cleanup test |
| `test/battle/arcanum_more_wishes.c` | "More Wishes" outcome test |
| `test/battle/arcanum_normality.c` | Normality outcome test |
| `test/battle/arcanum_reduction.c` | Reduction outcome test |
| `test/battle/battle_status_icons.c` | Status icon system test |
| `test/battle/tx_wishes.c` | TX Rollback wish test |

## 3. Deleted Files (Tracked)

| File | Reason |
|------|--------|
| `src/castoria_battle_ui.c` (−163 lines) | Mage Armor icon UI moved into generalized `battle_status_icons.c` |
| `include/castoria_battle_ui.h` (−9 lines) | Declarations moved to `battle_status_icons.h` |

---

## 4. Major Structural Changes

### 4.1 `battle.h` — Struct & Inline Helpers

**New includes:**
```c
#include "battle_tx_rollback.h"   // before CastoriaBattleData
#include "battle_reduction.h"     // inside BattleStruct comment block
```

**`BattleStruct` gains two new fields:**
```c
struct ReductionData reduction;         // HP/stat reduction state
struct TxRollbackBattleData txRollback; // TX Rollback formation/wish state
u8 healingHpUpdate;                     // genuine recovery vs redistribution
```

**`PartyState` field renamed:**
```c
// Before:
u32 padding:3;
// After:
u32 breakBarsPeak:3;   // tracks peak break bars remaining
```

**`IsBreakBarPending` now gates on TxRollback slot policy:**
```c
static inline bool32 IsBreakBarPending(u32 battler)
{
    return gBattleStruct != NULL && battler < gBattlersCount
        && TxRollbackGetSlotPolicy(battler) != TX_SLOT_DISABLED
        && TxRollbackGetSlotPolicy(battler) != TX_SLOT_RETIRED
        && gBattlerPartyIndexes[battler] < PARTY_SIZE
        && gBattleStruct->partyState[battler & BIT_SIDE][gBattlerPartyIndexes[battler]].breakBarPending;
}
```

**`IsBattlerAlive` now returns FALSE for DISABLED/RETIRED TxRollback slots:**
```c
static inline bool32 IsBattlerAlive(u32 battler)
{
    if (battler >= gBattlersCount)
        return FALSE;
    else if (TxRollbackGetSlotPolicy(battler) == TX_SLOT_DISABLED
          || TxRollbackGetSlotPolicy(battler) == TX_SLOT_RETIRED)
        return FALSE;
    else if (gAbsentBattlerFlags & (1u << battler))
        return FALSE;
    else
        return gBattleMons[battler].hp != 0 || IsBreakBarPending(battler);
}
```

**Move slot evaluation gained TxRollback wish branches:**
- `IsArcanumAction` — if `TxRollbackHasWishes(battler)`, returns `gChosenMovePos == MAX_MON_MOVES`
- `GetBattlerMoveSlotCount` — if wishes active, returns `MAX_MON_MOVES + 1`
- `GetBattlerMoveAtSlot` — slot `MAX_MON_MOVES` maps to `MOVE_ARCANUM_WISH`

### 4.2 Battle Status Icons — From Single-Icon to Multi-Icon System

**Before (last commit):** `battle_status_icons.c/h` handled only `No Order Change` via a single sprite array `sNoOrderChangeSpriteIds[MAX_BATTLERS_COUNT]`.

**After:** Generalized into an icon registry pattern:

```c
enum BattleStatusIcon {
    BATTLE_STATUS_ICON_GUTS,
    BATTLE_STATUS_ICON_INVINCIBLE,
    BATTLE_STATUS_ICON_MAGE_ARMOR,
    BATTLE_STATUS_ICON_NO_ORDER_CHANGE,
    BATTLE_STATUS_ICON_COUNT,
};

struct BattleStatusIconInfo {
    struct SpriteSheet sheet;
    struct SpriteTemplate template;
    const struct SpritePalette *palette;
    bool32 (*isActive)(u32 battler);      // e.g. HasGutsIcon, HasInvincibleIcon
    void (*onExpire)(u32 battler);        // e.g. ExpireInvincibleIcon
    s16 playerX; s16 opponentX; s16 y;    // per-icon offsets
    u16 duration;                         // auto-expiry frames
    u8 flashInterval;                     // flash frames
    bool8 singlesOnly;
    bool8 hideWhenFainted;
};

static const struct BattleStatusIconInfo sIcons[BATTLE_STATUS_ICON_COUNT] = {
    [BATTLE_STATUS_ICON_GUTS] = { .isActive = HasGutsIcon, .playerX = 36, .opponentX = 36, .y = -22 },
    [BATTLE_STATUS_ICON_INVINCIBLE] = { .isActive = HasInvincibleIcon, .onExpire = ExpireInvincibleIcon,
                                        .duration = 60, .flashInterval = 4, ... },
    [BATTLE_STATUS_ICON_MAGE_ARMOR] = { .isActive = HasMageArmor, .singlesOnly = TRUE, .hideWhenFainted = TRUE, ... },
    [BATTLE_STATUS_ICON_NO_ORDER_CHANGE] = { .isActive = HasNoOrderChange, .playerX = -32, .opponentX = 96, .y = 24 },
};
```

New API: `InitBattleStatusIcons`, `RefreshBattleStatusIcon`, `RefreshBattlerStatusIcons`, `DestroyBattleStatusIcon`, `ReshowBattleStatusIcons`, `FreeBattleStatusIcons`, `LoadBattleStatusIconPalette`, `ShowInvincibleBattleStatusIcon`, `BS_ShowKazuGutsIcon`, `BS_ShowKazuInvincibleIcon`.

The Guts and Invincible badge graphics (`Guts8x8.4bpp`, `Invin8x8.4bpp`, `kazu_buff_icons.gbapal`) moved from `kazuradrop_battle_ui.c` into this module. Mage Armor graphics (`MageArmor16.4bpp`) moved from `castoria_battle_ui.c`.

### 4.3 Kazuradrop UI Trimmed

`src/kazuradrop_battle_ui.c` went from 1112 → 970 lines (−142). Removed:
- Guts/Invincible buff icon graphics, OAM data, sprite callbacks, state arrays
- Alter Ego sigil graphics and definitions
- Buff icon palette, `LoadKazuBuffIconGfx`, `CreateKazuradropBuffIcon`, `DestroyKazuradropBuffIcon`, `DestroyAllKazuradropBuffIcons`
- `tIconType`, `sKazuBuffIconsLoaded`, `sAlterEgoSpriteId`, `sGutsSpriteId`, `sInvincibleSpriteId`
- Buff icon positions (`POS_GUTS_X_OFFSET`, etc.)

Kept: Bug Space threshold panel, Kazuradrop dialogue box, number display. Now includes `battle_status_icons.h` instead of `castoria_battle_ui.h`.

`include/kazuradrop_battle_ui.h`: removed `CreateKazuradropBuffIcon`, `DestroyKazuradropBuffIcon`, `DestroyAllKazuradropBuffIcons`, `KA_BUFF_GUTS`, `KA_BUFF_INVINCIBLE`, Kazuradrop-specific palette functions.

### 4.4 Castoria Battle UI Deleted

Both `src/castoria_battle_ui.c` and `include/castoria_battle_ui.h` removed. Mage Armor icon responsibility moved entirely to `battle_status_icons.c`.

### 4.5 `castoria_name_theft.c` — Wish System Integration

Key changes:
- `#include "castoria_battle_ui.h"` → `#include "battle_status_icons.h"`
- `BS_ShowMageArmorIcon` now calls `RefreshBattleStatusIcon(gBattlerAttacker, BATTLE_STATUS_ICON_MAGE_ARMOR)` instead of `CreateMageArmorIcon`
- `BS_CastoriaTryWish` now routes to `BS_TxRollbackTryWish()` when `TxRollbackHasWishes(gBattlerAttacker)` is true
- New wish outcomes: `CASTORIA_WISH_MORE_WISHES` (triggers `TxRollbackRequestCreation`), `CASTORIA_WISH_REDUCTION` (`ReductionActivate()`), `CASTORIA_WISH_NORMALITY` (undoes Dynamax / form changes)
- `CastoriaCanWishFor` gains cases for `MORE_WISHES`, `REDUCTION`, `NORMALITY`
- `GetDebilitationTarget` renamed to `GetWishTarget`
- `IsArcanumSlotUsable` now checks `TxRollbackHasUsableWish(battler)` when wishes active

### 4.6 AI — Wish Evaluation (`battle_ai_main.c`)

Added:
- `ClearWishSelection(battler)` — resets wish candidate/outcome/target
- `CommitWishSelection(battler, outcome, partyIndex)` — commits wish to either TxRollback or Castoria state
- `ChooseArcanumForTarget(...)` — evaluates extra move slots (arcanum/wish moves) via dynamic AI func; regular moves win ties to conserve spell resources

`ChooseMoveOrAction_Singles` now calls `ClearWishSelection` instead of inline Castoria-only reset.

### 4.7 Battle Script Commands

- `CreateKazuradropBuffIcon(gBattlerTarget, KA_BUFF_INVINCIBLE)` → `ShowInvincibleBattleStatusIcon(gBattlerTarget)`
- `PassiveDataHpUpdate` now tracks `oldHp` and `healing` flag; after update calls `TxRollbackMirrorHealing(battler, oldHp, nextInstr)` when healing occurred
- `MoveDamageDataHpUpdate` — changes related to healing tracking

### 4.8 Battle Main

- `HandleEndTurn_FinishBattle` now calls `ReductionFinishBattle()`
- `TestRunner_Battle_AfterCleanup()` is now gated behind `TestRunner_Battle_HasPendingAfterCleanup()`
- New `#if TESTING` helper `Test_RestorePlayerBattleStats()` — calls `ReductionFinishBattle()`, undoes Dynamax, restores player break bar HP

### 4.9 Data / Scripts

- `data/battle_scripts_1.s` (+155 lines) — new battle scripts for TX Rollback ambush, wishes, break wish, doom desire, etc.
- `asm/macros/battle_script.inc` (+5) — new macros
- `src/data/pokemon/species_info/gen_3_families.h` (+87) — new species family data (likely for TX Rollback / Jirachi forms)

## 5. Codeflow: TX Rollback Wish Execution

```
Castoria tries to use a move (BS_CastoriaTryWish)
    └─ If TxRollbackHasWishes(attacker):
         └─ BS_TxRollbackTryWish()  // TX Rollback wish path
    └─ Else:
         └─ Existing Castoria wish outcomes (DOMINATION, ENDURANCE, DEBILITATION)

TX Rollback Wish (BS_TxRollbackTryWish)
    └─ CASTORIA_WISH_MORE_WISHES → TxRollbackRequestCreation(battler)
       │     (commits Wish use after animation; creates TX Rollback formation)
    └─ CASTORIA_WISH_REDUCTION → ReductionActivate()
    └─ CASTORIA_WISH_NORMALITY → undo Dynamax / form changes on target

TX Rollback Formation Active (gBattleStruct->txRollback.stage >= ENTERING)
    └─ Slot policy governs which battlers can act / be targeted
    └─ Move slot count extended to MAX_MON_MOVES + 1 (wish slot)
    └─ Slot MAX_MON_MOVES = MOVE_ARCANUM_WISH

Reduction Active
    └─ Fractional HP tracked via hpFraction (24-bit fixed point)
    └─ ObserveHp() retains sub-HP remainder across stat rescalings
    └─ Stats scaled per-occupant; base/applied/lastHp tracked
    └─ End-turn: ReductionEndTick() applies gradual effects
    └─ Battle end: ReductionFinishBattle() cleans up
```

## 6. Full File Inventory

### Modified tracked files (52)
```
.gitignore
asm/macros/battle_script.inc            (+5)
data/battle_scripts_1.s                 (+155)
docs/mechanics/arcanum.md               (+63/-15)
docs/mechanics/no_order_change.md       (+5/-2)
docs/tutorials/how_to_testing_system.md (+15)
graphics/special/CasterUI/MageArmor16.png (binary)
include/battle.h                        (+22)
include/battle_gfx_sfx_util.h           (+1)
include/battle_interface.h              (+8/-1)
include/battle_status_icons.h           (+26/-4)
include/castoria_battle_ui.h            (−9, deleted)
include/castoria_name_theft.h           (+12/-4)
include/constants/battle_end_turn.h     (+1)
include/constants/battle_string_ids.h   (+14)
include/constants/species.h             (+2)
include/event_data.h                    (+4)
include/kazuradrop_battle_ui.h          (+10/-24)
include/pokemon.h                       (+2)
include/random.h                        (+2)
include/test/battle.h                   (+11/-2)
include/test_runner.h                   (+3)
src/battle_ai_main.c                    (+149/-64)
src/battle_anim_mons.c                  (+3)
src/battle_break_bar.c                  (+8/-5)
src/battle_controllers.c                (+14/-4)
src/battle_dynamax.c                    (+8)
src/battle_end_turn.c                   (+4/-2)
src/battle_gfx_sfx_util.c               (+12)
src/battle_interface.c                  (+25/-15)
src/battle_main.c                       (+58/-18)
src/battle_message.c                    (+11)
src/battle_script_commands.c            (+103/-36)
src/battle_status_icons.c               (+300/-158)
src/battle_util.c                       (+14/-4)
src/battle_util2.c                      (+11/-4)
src/castoria_battle_ui.c                (−163, deleted)
src/castoria_name_theft.c               (+118/-32)
src/data/graphics/pokemon.h             (+2)
src/data/pokemon/species_info/gen_3_families.h (+87)
src/event_data.c                        (+6)
src/kazuradrop_battle_ui.c              (+428/−428, net ~0)
src/pokemon.c                           (+6/-4)
src/reshow_battle_screen.c              (+25/-6)
test/battle/arcanum.c                   (+3/-2)
test/battle/arcanum_debilitation.c      (+16/-16)
test/battle/move_effect/BugSpaceExplosion.c (+12/-2)
test/battle/move_effect/polymorph.c     (+8/-4)
test/battle/volatiles/kazuradrop_guts.c (+19/-10)
test/test_runner.c                      (+6)
test/test_runner_battle.c               (+37/-8)
test/test_test_runner.c                 (+46)
```

### Untracked new files (21)
```
docs/mechanics/WISH_REDUCTION_IMPLEMENTATION.md
docs/mechanics/tx_rollback.md
graphics/battle_interface/no_order_change_source.png
graphics/object_events/palettes/koanko.pal
graphics/object_events/pics/people/koanko.png
graphics/pokemon/jirachi/tx_rollback/README.md
graphics/pokemon/jirachi/tx_rollback/Sprite-0001.aseprite
graphics/pokemon/jirachi/tx_rollback/Sprite-00012.aseprite
graphics/pokemon/jirachi/tx_rollback/anim_front4x.png
graphics/pokemon/jirachi/tx_rollback/anim_frontCHECKLATER.png
graphics/pokemon/jirachi/tx_rollback/normal.pal
include/battle_reduction.h
include/battle_tx_rollback.h
src/battle_reduction.c
src/battle_tx_rollback.c
src/battle_tx_wishes.c
test/battle/after_cleanup.c
test/battle/arcanum_more_wishes.c
test/battle/arcanum_normality.c
test/battle/arcanum_reduction.c
test/battle/battle_status_icons.c
test/battle/tx_wishes.c
```

---

## 7. Summary

Since `68c13b3582`, the codebase has added a complete **TX Rollback / Wishes / Reduction** battle sub-system. This introduces formation-based battler slots (RESERVED/RETIRED/DISABLED), wish outcomes (More Wishes, Reduction, Normality, Reversal, Reclamation, Calamity, Restoration, Reciprocity), fractional-HP stat scaling with sub-HP remainder tracking, and full script-context save/restore for "phase" moves. The Battle Status Icons system was generalized from a single No Order Change badge into a multi-icon registry now owning Guts, Invincible, and Mage Armor. Kazuradrop's bespoke buff UI was trimmed, Castoria's Mage Armor UI was deleted, and Castoria's wish system was extended to route through TX Rollback when active. All changes are uncommitted in the working tree.
Supporting changes span `battle.h` struct fields, AI wish evaluation, break bar peak tracking, Castoria/Archanum wish outcomes, new graphics, and new test files.