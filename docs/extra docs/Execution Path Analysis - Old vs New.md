# Execution Path Analysis: Old vs New Code

**Repository:** `pokeemerald-expansion`  
**Branch:** `mypatch`  
**Baseline:** `68c13b3582` (HEAD — "Debilitation and BB reformat")  
**Working tree:** uncommitted changes (TX Rollback / Wishes / Reduction / Status Icons generalization)  

This document traces **actual function names and call sites** — how execution flows changed between the last committed state and the current working tree. No repository files were modified during this analysis.

---

## 1. Battle Status Icons — Single-Icon → Multi-Icon Registry

### Old execution path (committed at HEAD)

`src/battle_status_icons.c` handled exactly one icon: No Order Change.

**Init:**
```
InitNoOrderChangeIcons()
    └─ for each battler: sNoOrderChangeSpriteIds[battler] = MAX_SPRITES
```

**Per-battler refresh:**
```
RefreshNoOrderChangeIcon(u32 battler)
    └─ if sprite valid && callback == SpriteCB_NoOrderChange && data[0] == battler
           SpriteCB_NoOrderChange(&gSprites[spriteId]); return
    └─ else: load gfx/pal (TAG_NO_ORDER_CHANGE_ICON = 0xD7BE), CreateSprite(&sNoOrderChangeTemplate, ...)
    └─ sNoOrderChangeSpriteIds[battler] = spriteId; data[0] = battler
    └─ SpriteCB_NoOrderChange(&gSprites[spriteId])
```

**Sprite callback:**
```
SpriteCB_NoOrderChange(struct Sprite *sprite)
    └─ battler = sprite->data[0]
    └─ if !HasNoOrderChange(battler): DestroySprite(sprite); return
    └─ position relative to gHealthboxSpriteIds[battler] (x ± 40/104)
    └─ mirror healthbox.invisible
```

**Cleanup:** `FreeNoOrderChangeIcons()` — destroy sprites, free tiles/palette for TAG_NO_ORDER_CHANGE_ICON.

### New execution path (working tree)

`src/battle_status_icons.c` now manages **4 icons** via a data-driven registry `sIcons[BATTLE_STATUS_ICON_COUNT]`.

**Registry table:**
```c
[BATTLE_STATUS_ICON_GUTS] = {
    .isActive = HasGutsIcon,   // reads gBattleMons[battler].volatiles.kazuradropGuts
    .playerX = 36, .opponentX = 36, .y = -22,
},
[BATTLE_STATUS_ICON_INVINCIBLE] = {
    .isActive = HasInvincibleIcon,   // reads gBattleStruct->bugSpace.invincibleActive bit
    .onExpire = ExpireInvincibleIcon, // clears the bit
    .duration = 60, .flashInterval = 4,
},
[BATTLE_STATUS_ICON_MAGE_ARMOR] = {
    .isActive = HasMageArmor,
    .singlesOnly = TRUE, .hideWhenFainted = TRUE,
},
[BATTLE_STATUS_ICON_NO_ORDER_CHANGE] = {
    .isActive = HasNoOrderChange,
    .playerX = -32, .opponentX = 96, .y = 24,
},
```

**Init:**
```
InitBattleStatusIcons()
    └─ for each battler: for each icon: sIconSpriteIds[battler][icon] = MAX_SPRITES
```

**Per-icon refresh (core changed function):**
```
RefreshBattleStatusIcon(u32 battler, u32 icon)
    └─ if !IsIconActive(battler, icon): DestroyBattleStatusIcon(battler, icon); return
    └─ if sprite already valid for (battler, icon): UpdateIconPosition(&gSprites[spriteId]); return   // does NOT advance frame timer
    └─ load palette (info->palette), load sprite sheet (info->sheet)
    └─ spriteId = CreateSprite(&info->template, 0, 0, 0)
    └─ data[0] = battler; data[1] = icon
    └─ UpdateIconPosition(&gSprites[spriteId])
```

**Shared sprite callback:**
```
SpriteCB_BattleStatusIcon(struct Sprite *sprite)
    └─ battler = data[0]; icon = data[1]
    └─ if !IsIconActive(battler, icon): DestroyBattleStatusIcon(battler, icon); return
    └─ info = &sIcons[icon]
    └─ if info->duration && ++data[2] >= info->duration:
           if info->onExpire != NULL: info->onExpire(battler)   // e.g. ExpireInvincibleIcon clears invincibleActive bit
           DestroyBattleStatusIcon(battler, icon); return
    └─ UpdateIconPosition(sprite)
```

**UpdateIconPosition:**
```
UpdateIconPosition(struct Sprite *sprite)
    └─ battler = data[0]; icon = data[1]; healthbox = gHealthboxSpriteIds[battler]
    └─ if hideWhenFainted && !IsBattlerAlive(battler): invisible = TRUE; return
    └─ x = healthbox.x + (IsOnPlayerSide ? playerX : opponentX); y = healthbox.y + info->y
    └─ x2 = healthbox.x2; y2 = healthbox.y2
    └─ invisible = healthbox.invisible || (flashInterval && (data[2]/flashInterval)%2)
```

**Destroy / wrappers:**
```
DestroyBattleStatusIcon(u32 battler, u32 icon)
    └─ spriteId = sIconSpriteIds[battler][icon]; if valid: DestroySprite(&gSprites[spriteId]); sIconSpriteIds[battler][icon] = MAX_SPRITES

RefreshBattlerStatusIcons(u32 battler)
    └─ for each icon: RefreshBattleStatusIcon(battler, icon)

ReshowBattleStatusIcons(void)
    └─ for each battler: RefreshBattlerStatusIcons(battler)   // after menus reset sprites

FreeBattleStatusIcons(void)
    └─ for each icon: for each battler: DestroyBattleStatusIcon(battler, icon); FreeSpriteTilesByTag; FreeSpritePaletteByTag
```

**New public entry points:**
```
ShowInvincibleBattleStatusIcon(u32 battler)
    └─ gBattleStruct->bugSpace.invincibleActive |= 1u << battler
    └─ RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_INVINCIBLE)

BS_ShowKazuGutsIcon(void)        // battle-script native
    └─ RefreshBattleStatusIcon(gBattlerAttacker, BATTLE_STATUS_ICON_GUTS)
BS_ShowKazuInvincibleIcon(void)  // battle-script native
    └─ ShowInvincibleBattleStatusIcon(gBattlerAttacker)
```


## 2. Kazuradrop Buff Icons — Removed, Calls Redirected

### Old execution path (committed at HEAD)

`src/kazuradrop_battle_ui.c` owned Guts and Invincible badges:

```
CreateKazuradropBuffIcon(u32 battler, u32 iconType)   // KA_BUFF_GUTS / KA_BUFF_INVINCIBLE
    └─ LoadKazuBuffIconGfx()      // loads TAG_KAZU_BUFF_PAL once
    └─ CreateSprite(&sSpriteTemplate_KazuGutsIcon / sSpriteTemplate_KazuInvincibleIcon, ...)
    └─ sGutsSpriteId[battler] = spriteId / sInvincibleSpriteId[battler] = spriteId

SpriteCb_KazuBuffIcon(struct Sprite *sprite)     // Guts: permanent, follows healthbox
SpriteCb_KazuInvincibleIcon(struct Sprite *sprite)  // flashes for 60 frames then self-destructs

DestroyKazuradropBuffIcon(u32 battler, u32 iconType)
DestroyAllKazuradropBuffIcons(void)

ReshowKazuradropBattleUI(void)
    └─ ReshowMageArmorIcons()            // from castoria_battle_ui.c
    └─ for each battler: reset sprite IDs
    └─ if gBattleMons[battler].volatiles.kazuradropGuts: CreateKazuradropBuffIcon(battler, KA_BUFF_GUTS)
    └─ if gBattleStruct->bugSpace.invincibleActive & (1u << battler): CreateKazuradropBuffIcon(battler, KA_BUFF_INVINCIBLE)
    └─ RefreshBugSpaceNumber(...)
```

### New execution path (working tree)

All Kazuradrop-specific buff icon code was **deleted** from `kazuradrop_battle_ui.c`. The same graphics now live in `battle_status_icons.c` and are triggered through the shared registry.

**Call site 1 — Explosion / Self-Destruct / Misty Explosion (`JumpIfMoveFailed`, `src/battle_script_commands.c`):**
```
// OLD: CreateKazuradropBuffIcon(gBattlerTarget, KA_BUFF_INVINCIBLE);
// NEW: ShowInvincibleBattleStatusIcon(gBattlerTarget);
```

**Call site 2 — Guts cleared on 10% HP cheat-death trigger (`MoveDamageDataHpUpdate`, `src/battle_script_commands.c`):**
```
// OLD: DestroyKazuradropBuffIcon(battler, KA_BUFF_GUTS);
// NEW: DestroyBattleStatusIcon(battler, BATTLE_STATUS_ICON_GUTS);
```

**Reshow path (`ReshowKazuradropBattleUI`, `src/kazuradrop_battle_ui.c`):**
```
// OLD: ReshowMageArmorIcons(); CreateKazuradropBuffIcon(...) for Guts/Invincible
// NEW: (no Guts/Invincible recreation here — refreshed via RefreshBattlerStatusIcons elsewhere)
```


## 3. Castoria Mage Armor — Moved to Shared Registry

### Old execution path (committed at HEAD)

`src/castoria_battle_ui.c` (now deleted) owned Mage Armor:

```
CreateMageArmorIcon(u32 battler)      // battle-script native BS_ShowMageArmorIcon called this
    └─ load MageArmor16 gfx/pal, CreateSprite, track sprite id
ReshowMageArmorIcons(void)            // called from ReshowKazuradropBattleUI
DestroyMageArmorIcon / FreeMageArmorIconGfx
```

`src/castoria_name_theft.c` (at HEAD):
```
void BS_ShowMageArmorIcon(void)
{
    NATIVE_ARGS();
    CreateMageArmorIcon(gBattlerAttacker);   // <- direct call to castoria_battle_ui.c
    gBattlescriptCurrInstr = cmd->nextInstr;
}
```

### New execution path (working tree)

`castoria_battle_ui.c/.h` deleted. Mage Armor is now one entry in the shared `sIcons[]` table in `battle_status_icons.c`.

`src/castoria_name_theft.c` (working tree):
```
void BS_ShowMageArmorIcon(void)
{
    NATIVE_ARGS();
    RefreshBattleStatusIcon(gBattlerAttacker, BATTLE_STATUS_ICON_MAGE_ARMOR);   // <- shared registry
    gBattlescriptCurrInstr = cmd->nextInstr;
}
```

The `MageArmor16.png` graphic (in `graphics/special/CasterUI/`) was modified (268 → 220 bytes) and its gfx is now loaded via `sMageArmorGfx` INCBIN in `battle_status_icons.c`.


## 4. IsBattlerAlive / IsBreakBarPending — TxRollback Slot Policy Gating

### Old (committed at HEAD)

`include/battle.h`:
```c
static inline bool32 IsBattlerAlive(u32 battler)
{
    if (battler >= gBattlersCount)
        return FALSE;
    else if (gAbsentBattlerFlags & (1u << battler))
        return FALSE;
    else
        return gBattleMons[battler].hp != 0 || IsBreakBarPending(battler);
}

static inline bool32 IsBreakBarPending(u32 battler)
{
    return gBattleStruct != NULL && battler < gBattlersCount
        && gBattlerPartyIndexes[battler] < PARTY_SIZE
        && gBattleStruct->partyState[battler & BIT_SIDE][gBattlerPartyIndexes[battler]].breakBarPending;
}
```

### New (working tree)

```c
static inline bool32 IsBreakBarPending(u32 battler)
{
    return gBattleStruct != NULL && battler < gBattlersCount
        && TxRollbackGetSlotPolicy(battler) != TX_SLOT_DISABLED
        && TxRollbackGetSlotPolicy(battler) != TX_SLOT_RETIRED
        && gBattlerPartyIndexes[battler] < PARTY_SIZE
        && gBattleStruct->partyState[battler & BIT_SIDE][gBattlerPartyIndexes[battler]].breakBarPending;
}

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

**Execution impact:** `TxRollbackGetSlotPolicy(battler)` (from `src/battle_tx_rollback.c`) is now called on every `IsBattlerAlive` / `IsBreakBarPending` check. If the battler is in a `TX_SLOT_DISABLED` or `TX_SLOT_RETIRED` position, they are treated as dead/absent regardless of HP. This affects all code that calls `IsBattlerAlive` — spread moves, switch targeting, status icon visibility (`hideWhenFainted`), and more.


## 5. Passive HP / Healing Tracking — TxRollback Mirror Healing

### Old (committed at HEAD)

`src/battle_script_commands.c`, `PassiveDataHpUpdate`:
```c
static void PassiveDataHpUpdate(u32 battler, const u8 *nextInstr, bool32 instantDeath)
{
    s32 requested = gBattleStruct->passiveHpUpdate[battler];
    s32 change = instantDeath ? requested : ClampBreakBarPassiveDamage(battler, requested);
    if (IsBreakBarPending(battler) || (!instantDeath && requested != 0 && change == 0))
        { ... }
    gBattleStruct->passiveHpUpdate[battler] = 0;
    gBattlescriptCurrInstr = nextInstr;
}
```
No healing tracking — no `oldHp` capture, no `TxRollbackMirrorHealing` call.

### New (working tree)

```c
static void PassiveDataHpUpdate(u32 battler, const u8 *nextInstr, bool32 instantDeath)
{
    u32 oldHp = gBattleMons[battler].hp;
    bool32 healing = (gBattleStruct->healingHpUpdate & (1u << battler)) != 0;
    gBattleStruct->healingHpUpdate &= ~(1u << battler);
    s32 requested = gBattleStruct->passiveHpUpdate[battler];
    s32 change = instantDeath ? requested : ClampBreakBarPassiveDamage(battler, requested);
    if (IsBreakBarPending(battler) || (!instantDeath && requested != 0 && change == 0))
        { ... }
    gBattleStruct->passiveHpUpdate[battler] = 0;
    gBattlescriptCurrInstr = nextInstr;
    if (healing && !instantDeath)
        TxRollbackMirrorHealing(battler, oldHp, nextInstr);   // NEW
}
```

`MoveDamageDataHpUpdate` (also in `battle_script_commands.c`) similarly captures `oldHp` and calls `TxRollbackMirrorHealing(battler, oldHp, gBattlescriptCurrInstr)` when `moveDamage[battler] < 0` (healing via damage sign flip).

**`healingHpUpdate` field** (new in `BattleStruct`, `include/battle.h`):
```c
u8 healingHpUpdate; // Genuine recovery, as opposed to redistribution or layer changes.
```
Set in `Cmd_manipulatedamage` for `DMG_CHANGE_SIGN` and `DMG_BIG_ROOT`:
```c
case DMG_CHANGE_SIGN:
    gBattleStruct->passiveHpUpdate[gBattlerAttacker] *= -1;
    gBattleStruct->healingHpUpdate |= 1u << gBattlerAttacker;   // NEW
    break;
case DMG_BIG_ROOT:
    gBattleStruct->passiveHpUpdate[gBattlerAttacker] = -1 * GetDrainedBigRootHp(...);
    gBattleStruct->healingHpUpdate |= 1u << gBattlerAttacker;   // NEW
    break;
```


## 6. Transform / Polymorph — Reduction Begin/End Mutation

### Old (committed at HEAD)

`src/castoria_name_theft.c`, `BS_PolymorphTarget`:
```c
void BS_PolymorphTarget(void)
{
    NATIVE_ARGS();
    ...
    u32 oldSpecies = gBattleMons[target].species;
    u32 oldHp = GetMonData(mon, MON_DATA_HP);
    u32 oldMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
    ...
    // no Reduction calls
}
```

`Cmd_transformdataexecution` (ordinary Transform move, `src/battle_script_commands.c`):
```c
// no ReductionBeginMutation call before transform
```

### New (working tree)

`src/castoria_name_theft.c`, `BS_PolymorphTarget`:
```c
void BS_PolymorphTarget(void)
{
    NATIVE_ARGS();
    ...
    // The snapshot source is always the unreduced party mon, never the overlay.
    u32 oldSpecies = GetMonData(mon, MON_DATA_SPECIES);
    u32 oldHp, oldMaxHp;
    ...
    ReductionBeginMutation(target);           // NEW — before reading HP
    oldHp = GetMonData(mon, MON_DATA_HP);
    oldMaxHp = GetMonData(mon, MON_DATA_MAX_HP);
    alreadyPolymorphed = GetBattlerPartyState(target)->polymorphed;
    ...
    ReductionEndMutation(target);             // NEW — after polymorph completes
}
```

`src/battle_script_commands.c`, `Cmd_transformdataexecution`:
```c
// NEW at start:
ReductionBeginMutation(gBattlerAttacker);
gBattleMons[gBattlerAttacker].volatiles.transformed = TRUE;
...
```

`src/battle_script_commands.c`, `Cmd_getexp`:
```c
if (gBattleMons[battler].volatiles.transformed)
{
    u16 hp = GetMonData(&gPlayerParty[*expMonId], MON_DATA_HP);
    ReductionBeginMutation(battler);          // NEW
    SetMonData(&gPlayerParty[*expMonId], MON_DATA_HP, &hp);
    ...
}
```

`src/battle_script_commands.c`, `Cmd_returntoball`:
```c
if (!cmd->changingForm)
{
    ReductionResetOccupant(battler);          // NEW
    TryBattleFormChange(battler, FORM_CHANGE_BATTLE_SWITCH);
}
```


## 7. Switch / Return-to-Ball — TxRollback Slot Policy Checks

### Old (committed at HEAD)

`src/battle_script_commands.c`, `CanBattlerSwitch`:
```c
bool32 CanBattlerSwitch(u32 battler)
{
    s32 i, lastMonId, battlerIn1, battlerIn2;
    ...
    if (IsDoubleBattle())
        battlerIn2 = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
    ...
}
```

`ChooseMonToSendOut`:
```c
static void ChooseMonToSendOut(u32 battler, u8 slotId)
{
    gBattleStruct->battlerPartyIndexes[battler] = gBattlerPartyIndexes[battler];
    ...
}
```

`Cmd_openpartyscreen`:
```c
// no TxRollback check before opening party screen
```

### New (working tree)

`CanBattlerSwitch`:
```c
bool32 CanBattlerSwitch(u32 battler)
{
    if (!TxRollbackCanPopulateSlot(battler))      // NEW — returns FALSE for disabled/retired slots
        return FALSE;
    ...
    if (IsDoubleBattle()
     && TxRollbackGetSlotPolicy(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)) != TX_SLOT_DISABLED)   // NEW
        battlerIn2 = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
    ...
}
```

`ChooseMonToSendOut`:
```c
static void ChooseMonToSendOut(u32 battler, u8 slotId)
{
    if (!TxRollbackCanPopulateSlot(battler))      // NEW — early return
        return;
    ...
}
```

`Cmd_openpartyscreen`:
```c
// NEW check after failInstr validation:
if (!TxRollbackCanPopulateSlot(battler))
{
    gAbsentBattlerFlags |= 1u << battler;
    gHitMarker &= ~HITMARKER_FAINTED(battler);
    gBattlescriptCurrInstr = failInstr;
    return;
}
```

`Cmd_returntoball`:
```c
if (!cmd->changingForm)
{
    ReductionResetOccupant(battler);      // NEW
    TryBattleFormChange(battler, FORM_CHANGE_BATTLE_SWITCH);
}
```


## 8. TX Rollback Creation — New Execution Path (entirely new code)

All functions below are in `src/battle_tx_rollback.c` (untracked new file). None of these existed at HEAD.

```
TxRollbackCanCreate(u32 caster)
    └─ returns gBattleStruct != NULL
    └─ && gBattleStruct->txRollback.stage == TX_ROLLBACK_NONE
    └─ && CanPrepareCreation(caster)

CanPrepareCreation(u32 caster)   // static
    └─ gBattleStruct == NULL || caster >= gBattlersCount → FALSE
    └─ gBattlersCount != 2 || IsDoubleBattle() → FALSE   // only singles battles
    └─ !(gBattleTypeFlags & BATTLE_TYPE_TRAINER) → FALSE
    └─ link/frontier/safari → FALSE
    └─ gBattleStruct->castoria.nameResult != CASTORIA_NAME_FALSE → FALSE   // Castoria must have backed out / false name
    └─ gBattleStruct->castoria.wishUsed → FALSE
    └─ GetBattlerPosition(caster) != B_POSITION_OPPONENT_LEFT → FALSE   // Castoria must be opponent left
    └─ gBattleMons[caster].species != SPECIES_CASTORIA || !IsBattlerAlive(caster) → FALSE
    └─ GetMonData(&gEnemyParty[TX_PARTY_INDEX], MON_DATA_SPECIES) != SPECIES_NONE → FALSE   // slot must be empty
    └─ test runner: gBattleStruct->txRollback.profile != NULL
    └─ production: TRAINER_BATTLE_PARAM.opponentA == TRAINER_RAZELUXE && gBattlerPartyIndexes[caster] == 4

TxRollbackRequestCreation(u32 caster)   // called from BS_CastoriaTryWish via CASTORIA_WISH_MORE_WISHES
    └─ if !TxRollbackCanCreate(caster): return FALSE
    └─ data = &gBattleStruct->txRollback
    └─ if data->profile == NULL: data->profile = &sPrototypeProfile
    └─ data->caster = caster; data->casterPartyIndex = gBattlerPartyIndexes[caster]
    └─ data->partyIndex = TX_PARTY_INDEX  (PARTY_SIZE - 1, last slot)
    └─ data->savedTarget = gBattlerTarget; data->savedEffectBattler = gEffectBattler
    └─ data->savedScriptingBattler = gBattleScripting.battler
    └─ data->stage = TX_ROLLBACK_REQUESTED
    └─ return TRUE

BS_TxRollbackCreate(void)   // battle-script native, executes the creation
    └─ NATIVE_ARGS(const u8 *failInstr)
    └─ data = &gBattleStruct->txRollback
    └─ count free sprites; if stage != REQUESTED || freeSprites < 20 || !CanPrepareCreation(data->caster):
           data->stage = NONE; gBattlescriptCurrInstr = failInstr; return
    └─ CreateMon(&gEnemyParty[TX_PARTY_INDEX], SPECIES_TX_ROLLBACK, level, 31, TRUE, 0, OT_ID_PRESET, 0x12345678)
    └─ SetMonData nickname "TX Rollback"; set 4 moves (PSYCHIC, SWIFT, PROTECT, CELEBRATE)
    └─ gEnemyPartyCount = CalculatePartyCount(gEnemyParty); gAiPartyData->count[OPPONENT] = gEnemyPartyCount
    └─ gBattleTypeFlags |= BATTLE_TYPE_DOUBLE; gBattlersCount = MAX_BATTLERS_COUNT
    └─ data->stage = TX_ROLLBACK_ENTERING
    └─ for battler 2..3: zero battle structs, set positions, party indexes, controllers, AI flags
    └─ gAbsentBattlerFlags |= PLAYER_RIGHT; gAbsentBattlerFlags &= ~OPPONENT_RIGHT
    └─ gBattleScripting.battler = OPPONENT_RIGHT
    └─ BtlController_EmitGetMonData(OPPONENT_RIGHT, REQUEST_ALL_BATTLE, 1 << TX_PARTY_INDEX)
    └─ MarkBattlerForControllerExec(OPPONENT_RIGHT)
    └─ gBattlescriptCurrInstr = cmd->nextInstr
```

**Caller chain:** `BS_CastoriaTryWish` → `BS_TxRollbackTryWish()` → `BS_TxRollbackCreate()` (when `CASTORIA_WISH_MORE_WISHES`).


## 9. TX Rollback Wishes — Execution Path

`src/battle_tx_wishes.c` (untracked new file):

```
TxRollbackHasWishes(u32 battler)
    └─ gBattleStruct != NULL && battler < gBattlersCount
    └─ && gBattleStruct->txRollback.stage >= TX_ROLLBACK_AMBUSH_PENDING
    └─ && stage != TX_ROLLBACK_DEFEATED
    └─ && TxRollbackOwnsPartyMon(GetBattlerSide(battler), gBattlerPartyIndexes[battler])

TxRollbackHasUsableWish(u32 battler)
    └─ for wish in TX_WISH_REVERSAL .. TX_WISH_RECIPROCITY:
    └─   for target in 0..gBattlersCount:
    └─     if TxRollbackCanWishFor(battler, wish, target): return TRUE
    └─ return FALSE

TxRollbackCanWishFor(u32 battler, u32 wish, u32 target)
    └─ if !TxRollbackHasWishes(battler) || !IsBattlerAlive(battler) || IsBreakBarPending(battler): return FALSE
    └─ switch (wish):
         TX_WISH_REVERSAL: any player-side battler with stat stage > default → TRUE
         TX_WISH_CALAMITY: any player-side battler with perishSong volatile → TRUE
         TX_WISH_RECLAMATION: GetBenefits(battler, entries) != 0  (stat stages, substitute, aqua ring, ingrain, side statuses)
         TX_WISH_RESTORATION: IsOriginalAlly(target) && !IsBreakBarPending(target)
                              && GetBreakBarCount(target) < breakBarsPeak && < BREAK_BAR_MAX_EXTRA_BARS
         TX_WISH_RECIPROCITY: !gBattleStruct->txRollback.reciprocity
```

**`GetBenefits(u32 tx, struct TxBenefitEntry *entries)`** (static):
- Iterates player-side alive battlers
- For each stat ATK..SPDEF: if player stat > default and tx stat < max → entry
- Volatiles: substitute, aquaRing, root(ingrain) → entries
- Side benefits (first player only): reflect, lightscreen, aurora veil, tailwind, safeguard, mist → entries

**`IsOriginalAlly(u32 battler)`** (static):
```c
if (battler >= gBattlersCount || IsOnPlayerSide(battler) || !IsBattlerAlive(battler)) return FALSE;
return gBattlerPartyIndexes[battler] == gBattleStruct->txRollback.casterPartyIndex
    || TxRollbackOwnsPartyMon(GetBattlerSide(battler), gBattlerPartyIndexes[battler]);
```


## 10. Castoria Wish System — Routing Changes

### Old (committed at HEAD)

`src/castoria_name_theft.c`, `BS_CastoriaTryWish`:
```c
void BS_CastoriaTryWish(void)
{
    // no TxRollback check
    NATIVE_ARGS(const u8 *failInstr);
    u32 battler = gBattlerAttacker;
    u32 outcome = gBattleStruct->castoria.wishOutcome;
    switch (outcome)
    {
    case CASTORIA_WISH_DOMINATION: ... set castoriaTrueNameCharges = 3
    case CASTORIA_WISH_ENDURANCE: ... apply debilitation to target
    case CASTORIA_WISH_DEBILITATION: { u32 targetBattler = GetDebilitationTarget(battler); ... }
    }
}
```

`CastoriaCanWishFor`:
```c
switch (outcome)
{
case CASTORIA_WISH_DOMINATION: return nameResult == TRUE && charges == 0;
case CASTORIA_WISH_ENDURANCE: return ... && GetDebilitationTarget(battler) < gBattlersCount;
case CASTORIA_WISH_DEBILITATION: return ... && GetDebilitationTarget(battler) < gBattlersCount;
default: return FALSE;   // More Wishes, Reduction, Normality all returned FALSE
}
```

### New (working tree)

`BS_CastoriaTryWish`:
```c
void BS_CastoriaTryWish(void)
{
    if (TxRollbackHasWishes(gBattlerAttacker))
    {
        BS_TxRollbackTryWish();     // NEW — delegates to TX Rollback wish system
        return;
    }
    NATIVE_ARGS(const u8 *failInstr);
    u32 battler = gBattlerAttacker;
    u32 outcome = gBattleStruct->castoria.wishOutcome;
    switch (outcome)
    {
    case CASTORIA_WISH_MORE_WISHES:     // NEW
        if (!TxRollbackRequestCreation(battler)) { gBattlescriptCurrInstr = cmd->failInstr; return; }
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CASTORIA_WISH_MORE_WISHES;
        gBattlescriptCurrInstr = cmd->nextInstr;
        return;   // Creation commits Wish use after animation
    case CASTORIA_WISH_REDUCTION:       // NEW
        ReductionActivate();            // NEW — from battle_reduction.c
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CASTORIA_WISH_REDUCTION;
        break;
    case CASTORIA_WISH_DOMINATION: ... (unchanged)
    case CASTORIA_WISH_ENDURANCE: ... (unchanged)
    case CASTORIA_WISH_DEBILITATION: { u32 targetBattler = GetWishTarget(battler); ... }   // renamed
    case CASTORIA_WISH_NORMALITY:       // NEW — undo Dynamax / form changes
        { u32 target = GetWishTarget(battler);
          if GetActiveGimmick(target) == GIMMICK_DYNAMAX: UndoDynamax(target); ... }
    }
}
```

`CastoriaCanWishFor`:
```c
switch (outcome)
{
case CASTORIA_WISH_MORE_WISHES:   return TxRollbackCanCreate(battler);           // NEW
case CASTORIA_WISH_DOMINATION: return nameResult == TRUE && charges == 0;   (unchanged)
case CASTORIA_WISH_REDUCTION:     return ... && GetBreakBarCount(battler) == 0;  // NEW
case CASTORIA_WISH_ENDURANCE:     return ... && GetWishTarget(battler) < gBattlersCount;   // renamed func
case CASTORIA_WISH_NORMALITY:     return ... && GetWishTarget(battler) < gBattlersCount;   // NEW
case CASTORIA_WISH_DEBILITATION:  return ... && GetWishTarget(battler) < gBattlersCount;   // renamed
default: return FALSE;
}
```

`GetDebilitationTarget` renamed to `GetWishTarget` (same body, new name).


## 11. AI Wish Evaluation — New Code Path

### Old (committed at HEAD)

`src/battle_ai_main.c`, `ChooseMoveOrAction_Singles`:
```c
if (GetBattlerMoveSlotCount(battler) > MAX_MON_MOVES)
{
    gBattleStruct->castoria.wishCandidate = CASTORIA_WISH_NONE;
    gBattleStruct->castoria.wishOutcome = CASTORIA_WISH_NONE;
}
```
Then inline evaluation of extra move slots (arcanum moves) with dynamic AI func.

### New (working tree)

Added functions in `src/battle_ai_main.c`:

```
ClearWishSelection(u32 battler)
    └─ if TxRollbackHasWishes(battler):
           txRollback.wishCandidate = TX_WISH_NONE; wishOutcome = TX_WISH_NONE; wishCandidateTarget = MAX_BATTLERS_COUNT; wishTargetPartyIndex = PARTY_SIZE
    └─ else if GetBattlerMoveSlotCount(battler) > MAX_MON_MOVES:
           castoria.wishCandidate = CASTORIA_WISH_NONE; castoria.wishOutcome = CASTORIA_WISH_NONE

CommitWishSelection(u32 battler, u8 outcome, u8 partyIndex)
    └─ if TxRollbackHasWishes(battler):
           txRollback.wishOutcome = outcome; txRollback.wishTargetPartyIndex = partyIndex
    └─ else: castoria.wishOutcome = outcome

ChooseArcanumForTarget(u32 battler, u32 opposingBattler, s32 normalScore, s32 *bestScore, u8 *wishOutcome, u8 *wishTarget)
    └─ tx = TxRollbackHasWishes(battler)
    └─ clear wish candidate/outcome (tx or castoria)
    └─ if AI_FLAG_DYNAMIC_FUNC && sDynamicAiFunc && GetBattlerMoveSlotCount(battler) > MAX_MON_MOVES:
           for i in MAX_MON_MOVES .. GetBattlerMoveSlotCount(battler):
               if !IsArcanumSlotUsable(battler, i): continue
               move = CastoriaGetMoveAtSlot(battler, i)
               score = RunDynamicAiFunc(battler, opposingBattler, move, AI_SCORE_DEFAULT, TRUE)
               if score > bestArcanumScore: record slot, outcome, target
           if bestArcanumSlot != 0xFF && bestArcanumScore > bestNormalScore:
               *bestScore = bestArcanumScore; *wishOutcome = bestWishOutcome; return bestArcanumSlot
    └─ return MAX_MON_MOVES + CASTORIA_ARCANUM_COUNT   // sentinel: no arcanum chosen
```

`ChooseMoveOrAction_Singles` now calls `ClearWishSelection(battler)` instead of the inline Castoria-only reset, and uses `ChooseArcanumForTarget` for extra-slot evaluation.


## 12. Reduction System — New Execution Path (entirely new code)

All functions in `src/battle_reduction.c` and `src/battle_reduction.h` (untracked new files). `ReductionData` is now embedded in `BattleStruct`.

```
ReductionActivate(void)
    └─ gBattleStruct->reduction.active = TRUE
    └─ gBattleStruct->reduction.grace = TRUE
    └─ gBattleStruct->reduction.nextLoss = 0

ReductionIsActive(void)   → gBattleStruct != NULL && gBattleStruct->reduction.active
ReductionIsScaled(u32 battler)   → occupant != NULL && occupant->scaled && occupant->partyIndex == gBattlerPartyIndexes[battler]

ReductionBeginMutation(u32 battler)    // called before polymorph/transform reads HP
    └─ (marks occupant as in-mutation; HP reads use unreduced party data)

ReductionEndMutation(u32 battler)      // called after polymorph/transform completes
    └─ (finalizes mutation state)

ReductionCanonicalHp(u32 battler)      // returns fractional HP, not raw gBattleMons[battler].hp
    └─ if !ReductionIsScaled(battler): return gBattleMons[battler].hp
    └─ ObserveHp(battler, gBattleMons[battler].hp)   // track sub-HP remainder
    └─ return FractionHp(state->hpFraction, state->base[STAT_HP])

ReductionCanonicalMaxHp(u32 battler)   // similar, returns scaled max HP

ObserveHp(u32 battler, u32 hp)   // static — retains sub-HP remainder across rescalings
    └─ if maxHp changed: rescale base[STAT_HP], reset hpFraction
    └─ if hp == 0: hpFraction = 0
    └─ elif hp >= maxHp: hpFraction = HP_FRACTION_ONE (1<<24)
    └─ else: hpFraction += (hp - lastHp) * HP_FRACTION_ONE / maxHp; clamp [1, MAX-1]
    └─ lastHp = hp

FractionHp(u32 fraction, u32 maxHp)   // static — (fraction * maxHp + 0.5) >> 24, clamped [1, maxHp]

ReductionEndTurn(u32 battler)   // called each turn while active
ReductionNormalizeRequest(u32 battler, u32 request, u32 monMask, void *data)   // stat request normalization
ReductionRefreshDisplay(u32 battler)   // update healthbox to canonical HP
ReductionFinishBattle(void)   // called at HandleEndTurn_FinishBattle and Test_RestorePlayerBattleStats
```

**Call sites:**
- `src/battle_main.c`, `HandleEndTurn_FinishBattle`: `ReductionFinishBattle();` — NEW call at end of battle
- `src/battle_main.c` (TESTING): `Test_RestorePlayerBattleStats()` calls `ReductionFinishBattle()`
- `src/castoria_name_theft.c`, `BS_PolymorphTarget`: `ReductionBeginMutation(target)` / `ReductionEndMutation(target)`
- `src/battle_script_commands.c`, `Cmd_transformdataexecution`: `ReductionBeginMutation(gBattlerAttacker)`
- `src/battle_script_commands.c`, `Cmd_getexp`: `ReductionBeginMutation(battler)`
- `src/battle_script_commands.c`, `Cmd_returntoball`: `ReductionResetOccupant(battler)`
- `src/castoria_name_theft.c`, `BS_CastoriaTryWish` (CASTORIA_WISH_REDUCTION): `ReductionActivate()`


## 13. Battle Main — End-of-Battle Changes

### Old (committed at HEAD)

`src/battle_main.c`, `HandleEndTurn_FinishBattle`:
```c
RestorePlayerBreakBarHp();
if (gTestRunnerEnabled)
    TestRunner_Battle_AfterCleanup();
```

### New (working tree)

```c
ReductionFinishBattle();                                    // NEW
// Undo Dynamax HP multiplier before recalculating stats.
for (battler = 0; battler < gBattlersCount; ++battler) { ... }
RestorePlayerBreakBarHp();
if (gTestRunnerEnabled && TestRunner_Battle_HasPendingAfterCleanup())   // NEW gate
    TestRunner_Battle_AfterCleanup();
```

New TESTING-only helper:
```c
#if TESTING
void Test_RestorePlayerBattleStats(void)
{
    ReductionFinishBattle();
    for (u32 battler = 0; battler < gBattlersCount; battler++)
        if (GetActiveGimmick(battler) == GIMMICK_DYNAMAX)
            UndoDynamax(battler);
    RestorePlayerBreakBarHp();
}
#endif
```

---

## 14. Summary of Execution Path Changes

| Area | Old call → New call |
|------|-------------------|
| Status icon refresh | `RefreshNoOrderChangeIcon(battler)` → `RefreshBattleStatusIcon(battler, icon)` / `RefreshBattlerStatusIcons(battler)` |
| Guts icon create | `CreateKazuradropBuffIcon(battler, KA_BUFF_GUTS)` → `RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_GUTS)` |
| Invincible icon create | `CreateKazuradropBuffIcon(battler, KA_BUFF_INVINCIBLE)` → `ShowInvincibleBattleStatusIcon(battler)` |
| Destroy Guts icon | `DestroyKazuradropBuffIcon(battler, KA_BUFF_GUTS)` → `DestroyBattleStatusIcon(battler, BATTLE_STATUS_ICON_GUTS)` |
| Mage Armor icon | `CreateMageArmorIcon(battler)` → `RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_MAGE_ARMOR)` |
| IsBattlerAlive | no TxRollback gate → `TxRollbackGetSlotPolicy` DISABLED/RETIRED check |
| IsBreakBarPending | no slot policy → `TxRollbackGetSlotPolicy` DISABLED/RETIRED check |
| Passive HP update | no healing tracking → captures `oldHp`, calls `TxRollbackMirrorHealing` on healing |
| Polymorph target | no Reduction calls → `ReductionBeginMutation` / `ReductionEndMutation` |
| Transform | no Reduction call → `ReductionBeginMutation(gBattlerAttacker)` |
| Return-to-ball | no Reduction call → `ReductionResetOccupant(battler)` |
| CanBattlerSwitch | no slot check → `TxRollbackCanPopulateSlot(battler)` |
| ChooseMonToSendOut | no slot check → `TxRollbackCanPopulateSlot(battler)` |
| Castoria TryWish | direct Castoria outcomes → routes to `BS_TxRollbackTryWish()` when wishes active |
| End of battle | `TestRunner_Battle_AfterCleanup()` unconditional → `ReductionFinishBattle()` + gated test runner |
| Wish outcomes | More Wishes/Reduction/Normality all returned FALSE → now implemented (TxRollbackRequestCreation, ReductionActivate, Normality undo) |

