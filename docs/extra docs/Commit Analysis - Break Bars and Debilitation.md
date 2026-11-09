# Commit Analysis: Break Bars & Debilitation

**Repository:** `pokeemerald-expansion`  
**Branch:** `mypatch`  
**Commits analyzed:** `dd509a377d` ("BREAK BARS!") and `68c13b3582` ("Debilitation and BB reformat")  
**Combined change:** 86 files, +5376 insertions, −395 deletions  

---

## 1. Commit `dd509a377d` — "BREAK BARS!" (2026-11-03)

**Scope:** 41 files, +2820 / −278 lines.  
**Purpose:** Introduce the **Break Bar** battle mechanic — a stackable extra-HP-bar layer that sits above a battler's base HP bar, must be broken before the base HP can be damaged, and triggers per-phase scripts when each layer breaks.

### 1.1 New files

| File | Lines | Purpose |
|------|-------|---------|
| `include/battle_break_bar.h` | 62 | Public API: `BreakBarConfig`, `BreakBarPhase`, `BreakBarColor` enum, and all break-bar function declarations |
| `src/battle_break_bar.c` | 631 | Core implementation: activation, break resolution, palette fading, HP refill, script hooks |
| `test/battle/break_bar.c` | 604 | Test suite for core break-bar behavior |
| `test/battle/break_bar_exit_hp.c` | 119 | Tests for break-bar exit-HP edge cases |
| `test/battle/break_bar_grants.c` | 259 | Tests for break-bar granting mechanics |
| `test/battle/arcanum_endurance.c` | 445 | Tests for Arcanum's Endurance mechanic (break-bar adjacent) |
| `docs/mechanics/break_bars.md` | 79 | Design documentation for break bars |
| `docs/mechanics/arcanum.md` | 69 | Design documentation for Arcanum mechanics |

### 1.2 Deleted files

None — `src/kazuradrop_battle_ui.c` and `include/kazuradrop_battle_ui.h` were **modified** (trimmed), not deleted. See Section 6.1 for details.

### 1.3 Modified files (key)

| File | +/- | Change summary |
|------|-----|----------------|
| `src/battle_script_commands.c` | +124 / −60 | New native battle-script commands for break bars (refill, resolve, kazura stack breaker, exhaust, saved-bar fill/restore/reveal/sync) |
| `src/battle_interface.c` | +68 / −22 | `DrawHealthbarOverlay`, `RefreshBreakBarPalette`, integration into `MoveBattleBarGraphically` |
| `src/battle_main.c` | +80 / −8 | Queue/resolve break bars at end of turn, on switch-in, on return-to-ball |
| `data/battle_scripts_1.s` | +99 / −14 | New battle scripts: `BattleScript_BreakBarReturn`, `BattleScript_BreakBarRefill`, `BattleScript_RestoreSavedBreakBar`, Kazura transform scripts |
| `include/battle.h` | +25 / −6 | Added break-bar fields to `PartyState` / `BattlerState` structs |
| `include/battle_interface.h` | +9 / −0 | Declared `DrawHealthbarOverlay`, `RefreshBreakBarPalette` |
| `include/battle_script_commands.h` | +8 / −0 | Declared new script command handlers |
| `include/constants/battle_script_commands.h` | +1 / −0 | New command ID constant |
| `include/constants/battle_string_ids.h` | +9 / −0 | New string IDs for break-bar messages |
| `include/constants/items.h` | +4 / −1 | New `ITEM_BREAK_CHARM` hold effect |
| `include/constants/hold_effects.h` | +1 / −0 | `HOLD_EFFECT_BREAK_CHARM` |
| `include/test/battle.h` | +7 / −0 | Test helper declarations |
| `include/test_runner.h` | +3 / −1 | Test runner registration |
| `src/battle_controllers.c` | +5 / −1 | Controller flag for break-bar execution |
| `src/battle_end_turn.c` | +5 / −0 | End-of-turn break-bar resolution |
| `src/battle_hold_effects.c` | +12 / −0 | `Break Charm` item effect on switch-in |
| `src/battle_message.c` | +3 / −0 | Break-bar message emission |
| `src/battle_util.c` | +20 / −0 | Utility helpers (`IsBreakBarPending`, `QueueBreakBar`, etc.) |
| `src/battle_util2.c` | +2 / −0 | Minor utility addition |
| `src/castoria_name_theft.c` | +19 / −4 | Removed Kazuradrop-specific debug printf, integrated break-bar palette refresh |
| `src/data/battle_move_effects.h` | +2 / −1 | Move effect flag adjustments |
| `src/data/hold_effects.h` | +5 / −0 | Break Charm hold-effect data |
| `src/data/items.h` | +14 / −0 | Break Charm item entry |
| `src/reshow_battle_screen.c` | +3 / −0 | Reshow-screen break-bar refresh |
| `test/battle/arcanum.c` | +6 / −1 | Arcanum test updates for break bars |
| `test/test_runner_battle.c` | +10 / −0 | Register new break-bar tests |
| `docs/SUMMARY.md` | +3 / −0 | Add break-bars and arcanum docs to sidebar |
| `docs/tutorials/ai_flags.md` | +61 / −39 | AI flag tutorial reworked |
### 1.4 Core data structures

**`include/battle_break_bar.h`:**

```c
enum BreakBarColor {
    BREAK_BAR_RED,
    BREAK_BAR_PURPLE,
    BREAK_BAR_BLUE,
    BREAK_BAR_GREEN,
    BREAK_BAR_YELLOW,
    BREAK_BAR_ORANGE,
};

#define BREAK_BAR_MAX_EXTRA_BARS 5
#define BREAK_BAR_IGNORE_WHILE_TRANSFORMED (1 << 0)
#define BREAK_BAR_DEFER_INITIAL_DRAW     (1 << 1)

struct BreakBarPhase {
    const u8 *onBreakScript;   // runs when this layer breaks
    const u8 *afterBreakScript;// runs after the layer breaks (NULL = standard refill)
};

struct BreakBarConfig {
    struct BreakBarPhase phases[BREAK_BAR_MAX_EXTRA_BARS];
    u8 extraBars;              // 1..5
    u8 flags;
};
```

Per-battler state lives in `PartyState` (accessed via `GetBreakBarOwner(battler)`):
- `breakBarConfig` — pointer to the active `BreakBarConfig`
- `breakBarsRemaining` — how many extra bars are left
- `breakBarPending` — flag set when a break should resolve
- `breakBarSavedMaxHp[phase]` — saved max-HP values for "saved" break-bar layers

### 1.5 Break-bar codeflow (Commit 1)

```
ActivateBreakBar(battler, config)
    └─ Validates battler alive, config non-NULL, extraBars in 1..5, no existing bars
    └─ Stores config + extraBars count in battler's PartyState
    └─ If not DEFER_INITIAL_DRAW: RefreshBreakBarPalette + UpdateHealthboxAttribute

TryBreakBar(battler, returnInstr)   // called when HP reaches 0 while extra bars remain
    └─ Validates: owner exists, bars remaining > 0, battler not absent, HP == 0
    └─ If IGNORE_WHILE_TRANSFORMED && transformed: return FALSE
    └─ Decrements breakBarsRemaining
    └─ Pushes battle-script return address + BattleScript_BreakBarReturn
    └─ If afterBreakScript exists: pushes it
    └─ Sets gBattlescriptCurrInstr = onBreakScript (or BattleScript_BreakBarRefill if NULL)
    └─ Returns TRUE (battle script loops back to execute the phase script)

BS_ResolveBreakBars()   // native script command, called at end of turn / turn transition
    └─ If controller busy: return
    └─ For each battler: if IsBreakBarPending && TryBreakBar: return (stops script)
    └─ Otherwise advance to next instruction

BS_RefillBreakBar()     // standard "no afterBreakScript" behavior
    └─ Sets heal amount = maxHP (full refill of the broken layer)

BS_ExhaustBreakBars()   // exhausts all remaining break bars at once
BS_TryGrantSavedBreakBar()   // grants a saved break-bar layer
BS_FillSavedBreakBar()       // fills a saved layer to full
BS_AnimateSavedBreakBar()    // animation for saved bar
BS_RestoreSavedBreakBar()    // restores HP from saved layer
BS_SyncSavedBreakBarHp()     // syncs saved HP with current HP
BS_RevealSavedBreakBar()     // reveals a previously hidden saved bar

RefreshBreakBarPalette(battler, animate)
    └─ Fades healthbar palette between base HP color and break-bar fill color
    └─ 16-frame fade via Task_BreakBarPaletteFade

DrawHealthbarOverlay(battler, baseHp, baseMaxHp, overlayPixels)
    └─ Draws the extra break-bar segments on top of the base healthbar
    └─ Validates battler range, sprite IDs, pixel counts
```

**Where `TryBreakBar` is hooked in (Commit 1):**

1. **`Cmd_moveend`** — after a move ends, if attacker/target alive and not asleep/frozen, break bars resolve.
2. **`Cmd_returnatktoball`** — if break bar pending on attacker, run `TryBreakBar` before returning to ball.
3. **`Cmd_openpartyscreen`** — if break bar pending on battler, run `TryBreakBar` before opening party screen.
4. **`Cmd_returntoball`** — same pattern for return-to-ball.
5. **`Cmd_setatkhptozero`** — after setting attacker HP to 0, calls `QueueBreakBar(attacker)`.

**Kazuradrop removal:** All Kazuradrop-specific healthbar palette logic (hard-coded `SPECIES_KAZURADROP` checks in `MoveBattleBarGraphically`, `CreateBattlerHealthboxSprites`, debug `DebugPrintf` calls) was removed and replaced with the generic `RefreshBreakBarPalette` / `GetBreakBarCount` / `IsBreakBarBase` API.
---

## 2. Commit `68c13b3582` — "Debilitation and BB reformat" (2026-11-07)

**Scope:** 45 files, +2556 / −117 lines.  
**Purpose:** Add **Debilitation** (Arcanum mechanic), **Koanko** (new trainer/battle entity with conjuring), **No Order Change** visual indicator, and **trainer-assigned break bars** (replacing Kazuradrop-specific break bars with a data-driven system for any trainer).

### 2.1 New files

| File | Lines | Purpose |
|------|-------|---------|
| `src/battle_koanko.c` | 120 | Koanko battle logic: `TryStartKoankoConjuring`, `ConjureKoankoTeam`, `BS_ConjureKoankoTeam` |
| `include/battle_koanko.h` | 15 | Koanko phase enum + function declarations |
| `src/battle_status_icons.c` | 133 | No Order Change icon sprite system (FGO-style Order Change Seal) |
| `include/battle_status_icons.h` | 14 | `InitNoOrderChangeIcons`, `RefreshNoOrderChangeIcon`, `FreeNoOrderChangeIcons` |
| `src/trainer_break_bars.c` | 68 | Trainer break-bar initialization + `BS_ActivateBreakBar` script command |
| `include/trainer_break_bars.h` | 48 | `TrainerBreakBarRule`, `TrainerBreakBarRules`, resolution + initialization API |
| `src/data/trainer_break_bars.h` | 41 | Predefined break-bar profiles (1–5 extra bars), Kazuradrop config, Koanko rulesets |
| `src/data/koanko_party.h` | 106 | Koanko's party data (high-level legendary Pokémon) |
| `test/battle/arcanum_debilitation.c` | 673 | Debilitation mechanic tests |
| `test/battle/koanko.c` | 236 | Koanko battle tests |
| `test/battle/trainer_break_bars.c` | 317 | Trainer break-bar tests |
| `test/battle/kazuradrop_stack_breaker.c` | +3 | Kazuradrop stack breaker test update |

### 2.2 New graphics assets

| Asset | Type | Purpose |
|-------|------|---------|
| `graphics/trainers/front_pics/koanko.png` | Binary | Koanko trainer front sprite |
| `graphics/trainers/palettes/koanko.pal` | 19 lines | Koanko trainer palette |
| `graphics/pokemon/jirachi/tx_rollback/anim_front.png` | Binary | Jirachi TX rollback animation frame |
| `graphics/pokemon/jirachi/tx_rollback/normal2.pal` | 19 lines | Jirachi TX rollback palette |
| `graphics/battle_interface/no_order_change.png` | Binary | No Order Change icon (12x12 artwork in 16x16 sprite) |

### 2.3 Modified files (key)

| File | +/- | Change summary |
|------|-----|----------------|
| `include/script.h` | +9 / −0 | Added `trainerBreakBarRules` and `trainerBreakBarIds` arrays to `ScriptContext` (2 override slots) |
| `src/script.c` | +48 / −0 | `Script_SetTrainerBreakBars`, `Script_TakeTrainerBreakBarOverride`, `Script_ClearTrainerBreakBarOverrides` |
| `include/battle.h` | +4 / −0 | Added `HasNoOrderChange` / no-order-change battler state |
| `include/battle_main.h` | +2 / −0 | No-order-change main battle declarations |
| `include/battle_koanko.h` | +15 / −0 | New (see above) |
| `include/battle_status_icons.h` | +14 / −0 | New (see above) |
| `include/battle_util.h` | +1 / −0 | Utility declaration |
| `include/battle_script_commands.h` | +2 / −0 | `BS_ActivateBreakBar` declaration |
| `include/battle_string_ids.h` | +3 / −0 | Debilitation / no-order-change string IDs |
| `include/constants/opponents.h` | +3 / −0 | Koanko trainer ID |
| `include/castoria_name_theft.h` | +4 / −0 | Name-theft hook updates |
| `src/battle_koanko.c` | +120 / −0 | New (see above) |
| `src/battle_status_icons.c` | +133 / −0 | New (see above) |
| `src/trainer_break_bars.c` | +68 / −0 | New (see above) |
| `src/battle_main.c` | +28 / −0 | Debilitation + no-order-change integration into battle main loop |
| `src/battle_script_commands.c` | +38 / −10 | `BS_ActivateBreakBar`, debilitation-related command updates |
| `src/battle_interface.c` | +2 / −0 | Minor interface update |
| `src/battle_ai_switch_items.c` | +2 / −0 | AI considers break bars when switching items |
| `src/battle_ai_util.c` | +5 / −0 | AI utility for break bars |
| `src/battle_message.c` | +2 / −0 | Debilitation / break-bar messages |
| `src/battle_util.c` | +32 / −4 | `HasNoOrderChange`, `ClampBreakBarPassiveDamage` refactor |
| `src/battle_util2.c` | +5 / −0 | Utility additions |
| `src/castoria_name_theft.c` | +67 / −7 | Name theft integrates with debilitation / break bars |
| `src/data/trainers.party` | +/−79 | Koanko party data, trainer party rework |
| `src/reshow_battle_screen.c` | +3 / −0 | Reshow-screen no-order-change refresh |
| `test/battle/arcanum.c` | +8 / −0 | Arcanum debilitation test updates |
| `docs/mechanics/arcanum.md` | +21 / −2 | Debilitation docs |
| `docs/mechanics/break_bars.md` | +6 / −2 | Trainer break-bar docs |
| `docs/mechanics/no_order_change.md` | +51 / −0 | New No Order Change documentation |
| `docs/mechanics/trainer_break_bars.md` | +389 / −0 | Comprehensive trainer break-bar authoring guide |
| `asm/macros/battle_script.inc` | +14 / −0 | New battle script macro for break bars |
| `asm/macros/event.inc` | +7 / −0 | Event macro updates |
### 2.4 Trainer Break Bar system

**Data structures (`include/trainer_break_bars.h`):**

```c
enum TrainerBreakBarSelector {
    TRAINER_BREAK_BAR_SPECIES,   // match by exact species
    TRAINER_BREAK_BAR_ENTRY,     // match by zero-based source party/pool entry
};

struct TrainerBreakBarRule {
    enum TrainerBreakBarSelector selector;
    u16 value;                   // species or entry index
    const struct BreakBarConfig *config; // NULL explicitly disables
};

struct TrainerBreakBarRules {
    const struct TrainerBreakBarRule *rules;
    u16 count;
};
```

**Predefined profiles (`src/data/trainer_break_bars.h`):**

```c
const struct BreakBarConfig gTrainerBreakBarProfiles[BREAK_BAR_MAX_EXTRA_BARS] = {
    {.extraBars = 1},   // 1 extra bar
    {.extraBars = 2},   // 2 extra bars
    {.extraBars = 3},   // 3 extra bars
    {.extraBars = 4},   // 4 extra bars
    {.extraBars = 5},   // 5 extra bars
};

const struct BreakBarConfig gKazuradropBreakBarConfig = {
    .phases[BREAK_BAR_PURPLE - 1] = {
        .onBreakScript = BattleScript_KazuradropTransform,
        .afterBreakScript = BattleScript_KazuradropTransformReturn,
    },
    .extraBars = 1,
    .flags = BREAK_BAR_IGNORE_WHILE_TRANSFORMED,
};
```

**Trainer assignment (currently only Koanko):**

```c
static const struct TrainerBreakBarRules *const sTrainerBreakBars[TRAINERS_COUNT][DIFFICULTY_COUNT] = {
    [TRAINER_KOANKO] = {
        [DIFFICULTY_NORMAL] = &gKoankoInitialBreakBars,
    },
};
```

**Trainer break-bar codeflow:**

```
GetTrainerBreakBarRules(trainerId, difficulty)
    └─ Looks up sTrainerBreakBars[trainerId][difficulty]
    └─ Falls back to Normal difficulty if entry missing
    └─ Returns &gEmptyTrainerBreakBars if no rules defined

ResolveTrainerBreakBar(rules, species, sourceEntry)
    └─ Iterates rules; matches selector (SPECIES or ENTRY) against value
    └─ Returns matching config, or NULL if no match / explicit NULL config

InitializeTrainerBreakBar(partySlot, rules, species, sourceEntry)
    └─ Called during trainer party setup
    └─ Resolves config via ResolveTrainerBreakBar
    └─ If config non-NULL: ActivateBreakBar(battler, config)

BS_ActivateBreakBar()   // native battle script command
    └─ Reads trainer ID + species from script args
    └─ Calls InitializeTrainerBreakBar for the active battler
```

### 2.5 Koanko

**Phases (`include/battle_koanko.h`):**

```c
enum KoankoPhase {
    KOANKO_ORDINARY_TEAM,   // normal team
    KOANKO_CONJURING,       // in the middle of conjuring
    KOANKO_CONJURED_TEAM,   // conjured (legendary) team active
};
```

**Koanko codeflow:**

```
TryStartKoankoConjuring(returnInstr)
    └─ Checks if Koanko is in ORDINARY_TEAM phase
    └─ Sets phase to CONJURING
    └─ Pushes return instruction + conjuring script onto battle script stack
    └─ Returns TRUE

ConjureKoankoTeam()
    └─ Swaps Koanko's party from ordinary team to conjured (legendary) team
    └─ Sets phase to CONJURED_TEAM
    └─ Activates trainer break bars for the conjured team

BS_ConjureKoankoTeam()   // native script command
    └─ Entry point from battle scripts
```

### 2.6 No Order Change

**Visual indicator:** A 12x12 FGO-style "Order Change Seal" icon drawn next to each battler's healthbox (outside the panel: player side at x-40, opponent side at x+104).

**Codeflow (`src/battle_status_icons.c`):**

```
InitNoOrderChangeIcons()
    └─ Clears all sprite ID slots to MAX_SPRITES

RefreshNoOrderChangeIcon(battler)
    └─ If sprite exists and is valid: re-run SpriteCB_NoOrderChange
    └─ Else: if HasNoOrderChange(battler): load gfx/pal, create sprite, position it
    └─ Sprite follows healthbox position (tracks x2/y2 offsets)

SpriteCB_NoOrderChange(sprite)
    └─ If battler invalid or no no-order-change: destroy sprite
    └─ Position relative to owner's healthbox sprite
    └─ Mirror healthbox visibility

FreeNoOrderChangeIcons()
    └─ Destroys all sprites, frees tiles and palette
```

### 2.7 Debilitation

Debilitation is an Arcanum mechanic tested in `test/battle/arcanum_debilitation.c` (673 lines). It integrates with the break-bar system: when a break bar breaks, it can apply a debilitation status to the battler. The `castoria_name_theft.c` changes (+67/−7) wire name-theft mechanics into the debilitation/break-bar flow.

### 2.8 `script.h` changes

The `ScriptContext` struct gained two fields for per-script trainer break-bar overrides:

```c
struct ScriptContext {
    // ... existing fields ...
    const struct TrainerBreakBarRules *trainerBreakBarRules[TRAINER_BREAK_BAR_OVERRIDE_COUNT]; // = 2
    u16 trainerBreakBarIds[TRAINER_BREAK_BAR_OVERRIDE_COUNT];
};
```

New script functions:
- `Script_SetTrainerBreakBars(ctx)` — assign break-bar rules to the current script context
- `Script_TakeTrainerBreakBarOverride(trainerId)` — consume and return an override
- `Script_ClearTrainerBreakBarOverrides(ctx)` — clear all overrides
---

## 3. Combined codeflow overview

### 3.1 Break Bar lifecycle (both commits)

```
Trainer party setup (or script)
    └─ InitializeTrainerBreakBar() → ActivateBreakBar(battler, config)
    └─ Stores BreakBarConfig in PartyState, sets breakBarsRemaining

Battle turn proceeds normally

Battler HP reaches 0 while breakBarsRemaining > 0
    └─ QueueBreakBar(battler) sets breakBarPending = TRUE
    └─ At next script opportunity (TryBreakBar called from moveend/returntoball/etc.):
         └─ TryBreakBar(battler, returnInstr)
         └─ Decrements breakBarsRemaining
         └─ Pushes onBreakScript (or BattleScript_BreakBarRefill)
         └─ Battle script executes phase script
         └─ onBreakScript typically: show message, apply effects, optionally BS_ResolveBreakBars

All extra bars broken
    └─ Base HP bar becomes vulnerable to damage
    └─ If afterBreakScript configured: it runs after the final break

Battle end / switch / faint
    └─ ReshowBreakBars() / FreeBreakBarGraphics() cleans up
```

### 3.2 Where break bars intersect other systems

| System | Integration point |
|--------|-------------------|
| **Hold effects** | `HOLD_EFFECT_BREAK_CHARM` — on switch-in, `ItemBattleEffects` can trigger break-bar logic |
| **AI** | `battle_ai_switch_items.c`, `battle_ai_util.c` — AI accounts for break bars when choosing items/switches |
| **Name theft** | `castoria_name_theft.c` — name theft now integrates with break-bar palette refresh and debilitation |
| **No Order Change** | `battle_status_icons.c` — icon drawn per-battler, refreshed on healthbox updates |
| **Koanko conjuring** | `battle_koanko.c` — when Koanko conjures, trainer break bars are (re)initialized for the new team |

### 3.3 Key design decisions

1. **Per-battler palette slots:** `sBreakBarPaletteTags[MAX_BATTLERS_COUNT]` gives each battler its own palette tag, so differently colored stacks can coexist in doubles/triples.
2. **Saved break bars:** `breakBarSavedMaxHp[phase]` allows a break-bar layer to "save" its HP state and be restored later (used by Kazuradrop and potentially other mechanics).
3. **Deferred initial draw:** `BREAK_BAR_DEFER_INITIAL_DRAW` lets callers provide their own entry animation instead of the default palette flash.
4. **Ignore while transformed:** `BREAK_BAR_IGNORE_WHILE_TRANSFORMED` prevents break bars from triggering on Pokémon that have transformed (e.g., Kazuradrop's transform).
5. **Kazuradrop migration:** The old Kazuradrop-specific healthbar UI was deleted entirely; Kazuradrop now uses the generic `gKazuradropBreakBarConfig` with `BattleScript_KazuradropTransform` / `BattleScript_KazuradropTransformReturn`.

---

## 4. File inventory

### New source files (8)
```
include/battle_break_bar.h
include/battle_koanko.h
include/battle_status_icons.h
include/trainer_break_bars.h
src/battle_break_bar.c
src/battle_koanko.c
src/battle_status_icons.c
src/trainer_break_bars.c
```

### New data files (2)
```
src/data/koanko_party.h
src/data/trainer_break_bars.h
```

### New test files (7)
```
test/battle/arcanum_debilitation.c
test/battle/arcanum_endurance.c
test/battle/break_bar.c
test/battle/break_bar_exit_hp.c
test/battle/break_bar_grants.c
test/battle/koanko.c
test/battle/trainer_break_bars.c
```

### New graphics (5)
```
graphics/battle_interface/no_order_change.png
graphics/pokemon/jirachi/tx_rollback/anim_front.png
graphics/pokemon/jirachi/tx_rollback/normal2.pal
graphics/trainers/front_pics/koanko.png
graphics/trainers/palettes/koanko.pal
```

### New documentation (4)
```
docs/mechanics/break_bars.md
docs/mechanics/no_order_change.md
docs/mechanics/trainer_break_bars.md
```

### Modified documentation (3)
```
docs/mechanics/arcanum.md
docs/mechanics/break_bars.md
docs/tutorials/ai_flags.md
docs/SUMMARY.md
```

### Deleted

None — `src/kazuradrop_battle_ui.c` and `include/kazuradrop_battle_ui.h` were modified (trimmed), not deleted.

---

## 5. Summary

Together, these two commits transform the battle system:

1. **Break bars** (commit 1) generalize Kazuradrop's existing single-bit break bar into a stackable, multi-layer HP shield mechanic with per-phase scripts, palette fading, saved-bar support, and a full native-script command set — now data-driven and available to any battler via `PartyState` fields. Kazuradrop's species-specific HP bar palette override was removed and replaced by the generic API.

2. **Debilitation + trainer break bars + Koanko + No Order Change** (commit 2) extend break bars to arbitrary trainers via data-driven rules, add a new trainer entity (Koanko) with conjuring and legendary teams, add a visual "No Order Change" status icon, and integrate debilitation into the Arcanum mechanic set.


---

## 6. Old Behavior vs New Behavior (Structural Comparison)

### 6.1 Kazuradrop-specific Battle UI → Generic Break Bar System

| Aspect | Before (HEAD~2) | After (HEAD) |
|--------|-----------------|--------------|
| **Files** | `src/kazuradrop_battle_ui.c` (1112 lines), `include/kazuradrop_battle_ui.h` (85 lines) | `src/kazuradrop_battle_ui.c` (970 lines), `include/kazuradrop_battle_ui.h` (78 lines) (+ `src/battle_break_bar.c`, `include/battle_break_bar.h`) |
| **Scope** | Kazuradrop-only break bar — a single bit in `gBattleStruct->bugSpace` plus hard-coded `SPECIES_KAZURADROP` checks in `battle_interface.c`. Kazuradrop already had a break bar; these commits did not invent it. | Generic break bar system — any battler can have 1–5 extra HP bars via `PartyState` break-bar fields. `kazuradrop_battle_ui.c/.h` kept (trimmed) for Bug Space panel, buff icons, and dialogue box; the HP bar palette override code was removed and replaced by the generic API. |
| **UI elements** | Bug Space threshold panel, Guts/Invincible buff icons, Kazuradrop dialogue box, purple→red HP bar palette override. | Bug Space threshold panel, Guts/Invincible buff icons, Kazuradrop dialogue box (kept in `kazuradrop_battle_ui.c`); break bar segments on healthbar via `DrawHealthbarOverlay`; per-battler palette fading via `RefreshBreakBarPalette`. |
| **HP bar palette** | `LoadKazuradropHpBarPalette()`, `SetKazuradropHpBarPalette()`, `StartKazuHpBarPurpleFade()` — Kazuradrop-specific purple→red switching. | Removed from Kazuradrop UI; replaced by generic `sBreakBarFillColors[][2]` indexed by `BreakBarColor` enum in `battle_break_bar.c`; per-battler palette tags `sBreakBarPaletteTags[MAX_BATTLERS_COUNT]`. |
| **Battle script commands** | `BS_ShowKazuGutsIcon`, `BS_ShowKazuInvincibleIcon`, `BS_BugSpacePanelSlideIn/Out`, `BS_ShowKazuraDialogueBox`, `BS_KazuraOmniboost`, etc. (kept in Kazuradrop UI). | `BS_RefillBreakBar`, `BS_ResolveBreakBars`, `BS_StartKazuraStackBreaker`, `BS_ExhaustBreakBars`, `BS_TryGrantSavedBreakBar`, `BS_ActivateBreakBar`, etc. (new in `battle_script_commands.c`). |
| **Cleanup** | `DestroyBugSpacePanel`, `DestroyAllKazuradropBuffIcons`, `FreeKazuradropBattleIconGfx`, `ReshowKazuradropBattleUI`. | `FreeBreakBarGraphics`, `ReshowBreakBars` (new; Kazuradrop cleanup functions kept). |

**Key structural shift:** Kazuradrop already had a single-bit break bar in `gBattleStruct->bugSpace`. These commits generalized it into a data-driven multi-bar system: any battler can now have 1–5 extra HP bars, each with its own `onBreakScript`/`afterBreakScript`, driven by `BreakBarConfig` stored in `PartyState`. Kazuradrop's species-specific HP bar palette override was removed in favor of the generic API.
The two commits are tightly coupled: commit 1 builds the foundation (`battle_break_bar.c/.h`, script commands, core hooks), and commit 2 builds the trainer-facing layer (`trainer_break_bars.{c,h}`, `battle_koanko.{c,h}`, `battle_status_icons.{c,h}`, party data, graphics, docs).

### 6.2 `IsBattlerAlive` — Battler with 0 HP and a Pending Break Bar

**Before (HEAD~2) — `include/battle.h`:**

```c
static inline bool32 IsBattlerAlive(u32 battler)
{
    if (gBattleMons[battler].hp == 0)
        return FALSE;
    else if (battler >= gBattlersCount)
        return FALSE;
    else if (gAbsentBattlerFlags & (1u << battler))
        return FALSE;
    else
        return TRUE;
}
```

There was **no** general `IsBreakBarPending` check. Instead, Kazuradrop's break bar was handled by a **species-gated catch in `Cmd_tryfaintmon`** — when Kazuradrop's HP reached 0, the faint-mon command checked `gBattleMons[battler].species == SPECIES_KAZURADROP` and suppressed the faint if Kazuradrop's break bar bit was set. All other Pokémon fainted immediately at `hp == 0`.

**After (HEAD):**

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
```

A battler with `hp == 0` is still considered "alive" if `IsBreakBarPending(battler)` is true — i.e., a break bar is queued to resolve. HP reaching zero no longer immediately removes the battler; the break-bar layer must be exhausted first. This replaces the old species-gated `SPECIES_KAZURADROP` check in `Cmd_tryfaintmon` with a general mechanism that works for any battler.

### 6.3 `PartyState` and `BattlerState` Struct Fields

**Before (HEAD~2):**

```c
struct BattlerState {
    // ... existing fields ...
    u32 padding:3;  // 3 bits of padding
    // End of Word
};

struct PartyState {
    // ... existing fields ...
    u32 padding:8;  // 8 bits of padding
    u16 usedHeldItem;
    // no break-bar fields at all
};
```

**After (HEAD):**

```c
struct BattlerState {
    // ... existing fields ...
    u32 noOrderChange:1;   // No Order Change visual indicator active
    u32 padding:2;         // reduced from 3 to 2
    // End of Word
};

struct PartyState {
    // ... existing fields ...
    u32 breakBarsRemaining:3;   // how many extra break bars left (0..5)
    u32 breakBarPending:1;      // break bar queued to resolve
    u32 breakBarItemUsed:1;     // Break Charm item used flag
    u32 padding:3;              // reduced from 8 to 3
    u16 usedHeldItem;
    const struct BreakBarConfig *breakBarConfig;
    u16 breakBarSavedHp[BREAK_BAR_MAX_EXTRA_BARS];
    u16 breakBarSavedMaxHp[BREAK_BAR_MAX_EXTRA_BARS];
    u8 breakBarFillPixels;
};
```

Also added to `BattleStruct`:
```c
u8 koankoPhase; // enum KoankoPhase; keeps the original trainer identity.
```

### 6.4 `ScriptContext` — Trainer Break Bar Override Slots

**Before (HEAD~2):**

```c
struct ScriptContext {
    ScrCmdFunc *cmdTable;
    ScrCmdFunc *cmdTableEnd;
    u32 data[4];
    // no trainer break-bar fields
};
```

**After (HEAD):**

```c
struct ScriptContext {
    ScrCmdFunc *cmdTable;
    ScrCmdFunc *cmdTableEnd;
    u32 data[4];
    const struct TrainerBreakBarRules *trainerBreakBarRules[TRAINER_BREAK_BAR_OVERRIDE_COUNT]; // = 2
    u16 trainerBreakBarIds[TRAINER_BREAK_BAR_OVERRIDE_COUNT];
};
```

Three new functions:
- `Script_SetTrainerBreakBars(ctx)` — assign break-bar rules to the current script context
- `Script_TakeTrainerBreakBarOverride(trainerId)` — consume and return an override
- `Script_ClearTrainerBreakBarOverrides(ctx)` — clear all overrides

Called from `CB2_InitBattleInternal` via `Script_ClearTrainerBreakBarOverrides()`.

### 6.5 `CreateNPCTrainerPartyFromTrainer` — Break Bar Initialization in Party Setup

**Before (HEAD~2):**

```c
u8 CreateNPCTrainerPartyFromTrainer(struct Pokemon *party, const struct Trainer *trainer,
                                    bool32 firstTrainer, u32 battleTypeFlags)
{
    // ... sets up party, dynamax, tera, stats ...
    // No break-bar initialization.
    return trainer->partySize;
}
```

**After (HEAD):** Split into two functions:

```c
// New internal function with break-bar support
u8 CreateNPCTrainerPartyWithBreakBars(struct Pokemon *party, const struct Trainer *trainer,
                                      bool32 firstTrainer, u32 battleTypeFlags,
                                      const struct TrainerBreakBarRules *rules)
{
    s32 partyOffset = ...;  // which enemy party slot this trainer occupies
    // ... existing setup ...
    if (partyOffset >= 0 && rules != NULL)
        InitializeTrainerBreakBar(partyOffset + i, rules, partyData[monIndex].species, monIndex);
    // ...
    return trainer->partySize;
}

// Thin wrapper preserving the old signature
u8 CreateNPCTrainerPartyFromTrainer(struct Pokemon *party, const struct Trainer *trainer,
                                    bool32 firstTrainer, u32 battleTypeFlags)
{
    return CreateNPCTrainerPartyWithBreakBars(party, trainer, firstTrainer, battleTypeFlags, NULL);
}
```

**Structural change:** Trainer parties can now be created with break-bar rules. The old signature is preserved as a wrapper passing `NULL` rules (no break bars). New callers pass a `TrainerBreakBarRules*` to assign break bars during party creation. Also fixed a side-channel bug where `opponentMonCanDynamax` / `opponentMonCanTera` bit shifts didn't account for `partyOffset` in double-battle partner slots.

### 6.6 `MoveBattleBarGraphically` — Kazuradrop Hardcoding Removed

**Before (HEAD~2) — `src/battle_interface.c`:**

```c
// Inside MoveBattleBarGraphically:
DebugPrintf("MBBG battler=%d species=%d isKazu=%d", battler, ...);

if (gBattleMons[battler].species == SPECIES_KAZURADROP)
{
    u8 healthbarSpriteId = gSprites[...].hMain_HealthBarSpriteId;
    u8 curPalNum = gSprites[healthbarSpriteId].oam.paletteNum;
    u8 kazuPalSlot = IndexOfSpritePaletteTag(TAG_KAZU_HP_BAR_PAL);
    DebugPrintf("MBBG kazu healthbarSpriteId=%d curPalNum=%d kazuPalSlot=%d match=%d", ...);
    if (curPalNum == kazuPalSlot)
        barElementId = HEALTHBOX_GFX_HP_BAR_GREEN;
    else if (gBattleStruct->bugSpace.moveSwapActive & (1u << battler))
        barElementId = HEALTHBOX_GFX_HP_BAR_RED;
    else
        barElementId = HEALTHBOX_GFX_HP_BAR_GREEN;
}
```

`CreateBattlerHealthboxSprites` had a Kazuradrop-specific `DebugPrintf` that was commented out, and `B_HEALTHBAR_PIXELS` was a `#define` (now a hardcoded constant used inline).

**After (HEAD):**

```c
RefreshBreakBarPalette(battler, FALSE);
if (GetBreakBarCount(battler) != 0)
    barElementId = HEALTHBOX_GFX_HP_BAR_GREEN;
else if (IsBreakBarBase(battler))
    barElementId = HEALTHBOX_GFX_HP_BAR_RED;
else if (filledPixelsCount > (B_HEALTHBAR_PIXELS * 50 / 100))
    barElementId = HEALTHBOX_GFX_HP_BAR_GREEN;
// ... same HP thresholds as before ...
```

### 6.7 Battle Script Commands Added

**Before (HEAD~2):** No break-bar-related script commands existed. When a battler's HP reached 0, they fainted immediately.

**After (HEAD):** 14+ new native script command handlers in `src/battle_script_commands.c`:

| Command | Purpose |
|---------|---------|
| `BS_RefillBreakBar` | Heals the broken layer back to full HP (standard no-after-script behavior) |
| `BS_ResolveBreakBars` | Resolves all pending break bars across battlers |
| `BS_StartKazuraStackBreaker` | Starts Kazuradrop's stack-breaker animation |
| `BS_AnimateKazuraStackBreaker` | Animates the stack breaker |
| `BS_ExhaustBreakBars` | Exhausts all remaining break bars at once |
| `BS_TryGrantSavedBreakBar` | Grants a saved break-bar layer |
| `BS_FillSavedBreakBar` | Fills a saved layer to full HP |
| `BS_AnimateSavedBreakBar` | Animates a saved break bar appearing |
| `BS_RestoreSavedBreakBar` | Restores HP from a saved break-bar layer |
| `BS_SyncSavedBreakBarHp` | Syncs saved HP with current HP |
| `BS_RevealSavedBreakBar` | Reveals a previously hidden saved bar |
| `BS_ActivateBreakBar` | Activates break bars for a trainer (trainer-facing) |

New battle scripts in `data/battle_scripts_1.s`: `BattleScript_BreakBarReturn`, `BattleScript_BreakBarRefill`, `BattleScript_RestoreSavedBreakBar`, `BattleScript_KazuradropTransform`, `BattleScript_KazuradropTransformReturn`.

### 6.8 `TryBreakBar` Hook Points

**Before (HEAD~2):** No `TryBreakBar` existed. HP reaching 0 meant immediate faint.

**After (HEAD):** `TryBreakBar(battler, returnInstr)` is called from five locations:

| Hook | File | Trigger |
|------|------|---------|
| `Cmd_moveend` | `src/battle_script_commands.c` | After a move ends, if alive and not asleep/frozen |
| `Cmd_returnatktoball` | `src/battle_script_commands.c` | Before returning attacker to ball |
| `Cmd_openpartyscreen` | `src/battle_script_commands.c` | Before opening the party screen |
| `Cmd_returntoball` | `src/battle_script_commands.c` | Before returning to ball |
| `Cmd_setatkhptozero` | `src/battle_script_commands.c` | After setting attacker HP to 0; calls `QueueBreakBar(attacker)` |

### 6.9 New Systems with No Pre-Existing Equivalent

| System | Files | Old behavior | New behavior |
|--------|-------|-------------|--------------|
| **Trainer Break Bars** | `src/trainer_break_bars.c/.h`, `src/data/trainer_break_bars.h` | None — only Kazuradrop had break bars, hard-coded | Data-driven `TrainerBreakBarRule`/`TrainerBreakBarRules` matching by species or party entry; predefined 1–5 bar profiles; Koanko assigned on Normal difficulty |
| **Koanko** | `src/battle_koanko.c/.h`, `src/data/koanko_party.h` | None | `KOANKO_ORDINARY_TEAM` → `KOANKO_CONJURING` → `KOANKO_CONJURED_TEAM`; `TryStartKoankoConjuring` / `ConjureKoankoTeam` / `BS_ConjureKoankoTeam` |
| **No Order Change** | `src/battle_status_icons.c/.h` | None | 12x12 Order Change Seal icon per-battler next to healthbox; `InitNoOrderChangeIcons` / `RefreshNoOrderChangeIcon` / `FreeNoOrderChangeIcons` |
| **Debilitation** | `test/battle/arcanum_debilitation.c` + `castoria_name_theft.c` | None | Debilitation mechanic integrated with break bars; name theft wires into debilitation/break-bar flow |
