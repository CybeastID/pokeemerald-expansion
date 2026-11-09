# Code tour: what structurally changed in `dd509a37` + `68c13b35`

Scope: branch `mypatch` of `cybeastid/pokeemerald-expansion`, read at `68c13b35` (2026-11-07), comparing against `f55d1c21`. 68 files changed, +5,354 / -373.
Everything below was read from source. **Nothing was compiled or run** (no ARM toolchain in the review environment), and only pushed commits are visible, so anything local-only is invisible to this review. Line numbers are as of `68c13b35`.

---

## 0. The shape of the change in six lines

1. **Break Bars became an engine, not a Kazuradrop feature.** State moved onto the party slot (`PartyState`), logic into `src/battle_break_bar.c`, and the engine hooks the faint/HP/move-end flow instead of species checks.
2. **Trainers can now own bars.** A sparse rules table + a script-side override queue decide, at party creation, which opponent mons start with bars.
3. **Koanko is a two-team trainer** done *inside one trainer ID*: when team 1 is wiped, the victory check rewrites `gEnemyParty` with a C-defined second team. `TRAINER_KOANKO2` is gone.
4. **Arcanum is a self-contained virtual-slot layer** (slots 4–7) that rides on `AI_FLAG_DYNAMIC_FUNC`; Wish has three live outcomes.
5. **No Order Change** is a new 1-bit battler condition with ~16 enforcement points, plus a new status-icon sprite module.
6. **Cross-links:** Wish→Endurance reuses the Break Bar "saved layer" API; Wish→Debilitation sets No Order Change; Lunar Dance reads both; Koanko's conjure reuses trainer-bar rules.

---

## 1. Break Bar engine (`src/battle_break_bar.c`, `include/battle_break_bar.h`)

### 1.1 Data model: bars belong to the *party slot*, not the battler
`struct PartyState` (`include/battle.h`) gained:

| field | width | meaning |
|---|---|---|
| `breakBarsRemaining` | 3 bits | extra layers left (0–5) |
| `breakBarPending` | 1 bit | a layer emptied; transition not yet run |
| `breakBarItemUsed` | 1 bit | Break Charm already fired for this mon |
| `breakBarConfig` | pointer | static `BreakBarConfig` (phases, flags) |
| `breakBarSavedHp[5]` / `breakBarSavedMaxHp[5]` | u16 ×5 | covered HP pair per *saved* layer |
| `breakBarFillPixels` | u8 | visual-only fill progress |

Consequences: bars survive switching, replacements never inherit them, and `GetBreakBarOwner()` (`:54`) is just `GetBattlerPartyState(battler)` with null guards.

**`IsBattlerAlive` changed meaning** (`include/battle.h:1205–1212`): it now returns TRUE when `hp == 0` **and** `IsBreakBarPending(battler)`. This is the single most far-reaching edit in the commit: every "is it fainted?" check across the engine now treats a pending break as alive. Places that really mean "HP is zero" were patched individually (e.g. faint-EXP guards at `battle_util.c:1973`, `:2018`).

### 1.2 Public API (header)
- `ActivateBreakBar(battler, config)` (`:240`) registers a stack on the *current* HP bar. Refuses if: no owner, NULL config, not alive, `extraBars` 0 or >5, or a stack is already active. Does not heal.
- `TryBreakBar(battler, returnInstr)` (`:259`) the transition itself (see 1.3).
- `QueueBreakBar(battler)` (`:86`) marks pending if `hp == 0` and the owner can intercept.
- `ClampBreakBarPassiveDamage(battler, dmg)` (`:94`) floors passive damage at 1 HP; returns 0 if pending.
- `GrantSavedBreakBar(battler)` (`:103`) pushes a "saved" layer (Break Charm, Lunar Dance, Wish: Endurance).
- `TryResolveBreakBarsAfterMove()` (`:464`) scans all battlers for pending bars and runs `BattleScript_ResolveBreakBarsEnd2`.
- Render side: `RefreshBreakBarPalette` (`:150`), `ReshowBreakBars` (`:610`), `FreeBreakBarGraphics` (`:623`).
- Script natives: `BS_RefillBreakBar`, `BS_ResolveBreakBars`, `BS_ExhaustBreakBars`, `BS_TryGrantSavedBreakBar`, `BS_FillSavedBreakBar`, `BS_AnimateSavedBreakBar`, `BS_RestoreSavedBreakBar`, `BS_SyncSavedBreakBarHp`, `BS_RevealSavedBreakBar`, plus the Kazuradrop-only `BS_StartKazuraStackBreaker` / `BS_AnimateKazuraStackBreaker`.
- `BreakBarConfig { phases[5] {onBreakScript, afterBreakScript}; u8 extraBars; u8 flags; }`. Flags: `BREAK_BAR_IGNORE_WHILE_TRANSFORMED`, `BREAK_BAR_DEFER_INITIAL_DRAW`. Phase index 0 = purple (bottom extra), 4 = orange.

### 1.3 Lifecycle of one break
1. **Damage empties a layer.** The HP write sets `hp = 0`, then calls `QueueBreakBar(battler)`:
   - direct move damage: `MoveDamageDataHpUpdate` → `battle_script_commands.c:2814`
   - passive: `PassiveDataHpUpdate` → `:2512`
   - self-KO: `Cmd_setatkhptozero` → `:10298`
   - faint gate: `Cmd_tryfaintmon` → `:4667` (`if (QueueBreakBar(battler)) { skip faint processing }`)
2. **Pending state blocks further damage.** `MoveDamageDataHpUpdate` returns early with `moveDamage = 0` while pending (`:2650`), which is why later hits of a multi-hit move can't hit the next layer. `Cmd_healthbarupdate` also skips all three cases when pending (`:2411–2427`).
3. **Resolution points** (all call `TryBreakBar` or the resolver script):
   - after the whole move: `HandleAction_ActionFinished` (`battle_util.c:965–966`) → `TryResolveBreakBarsAfterMove`
   - start of end-turn: `DoEndTurnEffects` (`battle_end_turn.c:1672`)
   - inline: `Cmd_returnatktoball` (`:7706`), `Cmd_returntoball` (`:8694`), the party-screen command (`:8190`), and the `callnative BS_ResolveBreakBars` in `BattleScript_EffectLunarDance`
4. **`TryBreakBar` mechanics** (`:259–294`): checks stack capacity (`savedTargetCount`, battle-script stack size), then `SaveBattlerTarget(gBattlerTarget)`, sets `gBattlerTarget = battler`, clears pending, **decrements `breakBarsRemaining`**, refreshes the palette, and pushes the script stack: `returnInstr`, then `BattleScript_BreakBarReturn` (`restoretarget; return`), then optionally `afterBreakScript`. It jumps to `onBreakScript` (or `BattleScript_BreakBarRefill`, or `BattleScript_RestoreSavedBreakBar` for saved layers).
   - Phase index used is `remainingBefore − 1`. A layer is "saved" iff `breakBarSavedMaxHp[phase] != 0`.
   - Saved layers skip *both* configured hooks.
5. **Refill** (`BattleScript_BreakBarRefill`): `BS_RefillBreakBar` → `SetHealAmount(target, maxHP)` + passive HP update.

### 1.4 Passive damage rules
- `ClampBreakBarPassiveDamage`: while a stack is active (and not transformed-ignored), damage is `min(damage, hp−1)`; the final red bar has no protection.
- **Bypass is path-based, not flag-based:** `PassiveDataHpUpdate(..., instantDeath=TRUE)` writes `hp = 0` without the clamp. The scripts using `INSTANT_DEATH_HP_UPDATE` are `BattleScript_FinalGambit`, `DestinyBondTakesLife`, `PerishSongTakesLife`, `BugSpacePassiveOHKO`. (In code the enum is `INSTANT_DEATH_HP_UPDATE`; a name like `HP_UPDATE_INSTANT_DEATH` does not exist in the tree.) Self-KO goes through `Cmd_setatkhptozero` (`:10298`), which also calls `QueueBreakBar`.

### 1.5 Rendering
- One palette slot **per battler** (`TAG_BREAK_BAR_*_PAL` = `0xD7B5`, `0xD7BB`, `0xD7BC`, `0xD7BD`) so differently colored stacks coexist in doubles.
- `BuildBreakBarPalette` copies `hpbar_purple.gbapal` and overwrites **entries 10 and 11** (the green fill pair) with the layer's two colors.
- `Task_BreakBarPaletteFade` lerps from the normal ball-display palette over 16 frames; guarded against task exhaustion and stale sprites.
- Bar tile choice is in `battle_interface.c:2174–2180`: any active extra layer → green tile (color from palette); `IsBreakBarBase` (registered, no extras left) → red; otherwise the usual HP-percent colors.
- `ReshowBreakBars` is called from `reshow_battle_screen.c:173,260`; `FreeBreakBarGraphics` from `battle_util2.c:66`.

### 1.6 Saved layers (`GrantSavedBreakBar`)
Stores current `hp`/`maxHP` into slot `layer = breakBarsRemaining`, then increments. On break, `BS_RestoreSavedBreakBar` (`:567`) writes the saved pair back **and zeroes that slot** (so no stale "saved" flag can leak into a later configured phase at the same index). Callers:
- Break Charm: `battle_hold_effects.c:1092` (once per party member; `breakBarItemUsed`)
- Lunar Dance (via script)
- Wish: Endurance: `castoria_name_theft.c` Wish switch (~`:614`)

### 1.7 What stayed Kazuradrop-specific
- `gKazuradropBreakBarConfig` (`src/data/trainer_break_bars.h`) with `BREAK_BAR_IGNORE_WHILE_TRANSFORMED`; activated by her Bug Space switch-in (`battle_util.c:4766`) if she isn't transformed and has no config.
- `BS_StartKazuraStackBreaker` / `BS_AnimateKazuraStackBreaker` remain in `battle_break_bar.c` by design.
- `src/kazuradrop_battle_ui.c` lost 142 lines (break-bar UI moved out); the file still exists.

---

## 2. Trainer-assigned bars (`src/trainer_break_bars.c`, `src/data/trainer_break_bars.h`)

- **Rule model:** `TrainerBreakBarRule {selector (SPECIES|ENTRY), value, config}`; a `TrainerBreakBarRules` is `{rules*, count}`. `INLINE_BREAK_BAR_RULES(...)` builds both as compound literals (file-scope only → static lifetime).
- **Table:** `sTrainerBreakBars[TRAINERS_COUNT][DIFFICULTY_COUNT]`, sparse via designated initializers. Only `TRAINER_KOANKO → &gKoankoInitialBreakBars` (an empty set) is populated today.
- **Selection order:** script override → difficulty row → `DIFFICULTY_NORMAL` row → `gEmptyTrainerBreakBars` (`SelectTrainerBreakBarRules`, `:11`). Explicit empty set opts out of the Normal fallback.
- **Rule precedence** (`ResolveTrainerBreakBar`, `:34`): first matching **ENTRY** rule wins (even if its config is NULL); otherwise the **last** matching SPECIES rule wins.
- **Assignment point:** `CreateNPCTrainerPartyWithBreakBars` (`battle_main.c:1891`) calls `InitializeTrainerBreakBar(partyOffset + i, ...)` right after `CalculateMonStats`. It writes only `breakBarConfig` and `breakBarsRemaining` into `partyState[B_SIDE_OPPONENT][slot]`: no graphics (healthboxes don't exist yet). The old `CreateNPCTrainerPartyFromTrainer` is now a thin wrapper passing `rules = NULL`.
- `partyOffset` is 0 for `gEnemyParty`, 3 for the second-opponent half, −1 otherwise. The same offset now also fixes the `opponentMonCanDynamax` / `opponentMonCanTera` bit positions for the second half.
- **Script override queue:** `settrainerbreakbars trainer, ruleset` (`asm/macros/event.inc:2124`) → `Script_SetTrainerBreakBars` (`script.c:649`). Up to two trainers per script context (`TRAINER_BREAK_BAR_OVERRIDE_COUNT`); `Script_TakeTrainerBreakBarOverride` (`:668`) consumes by trainer ID during `CreateNPCTrainerParty`; `Script_ClearTrainerBreakBarOverrides` (`:683`) runs at battle init (`battle_main.c:579`) and cleanup (`battle_util2.c:60`).
- **Battle-script side:** `activatebreakbar battler, config, failInstr` → `BS_ActivateBreakBar`.

---

## 3. Koanko: one trainer ID, two teams

- **Trigger:** `Cmd_checkteamslost` (`battle_script_commands.c:5425–5428`): when `NoAliveMonsForOpponent()` and outcome is still 0, `TryStartKoankoConjuring(gBattlescriptCurrInstr)` runs *before* `B_OUTCOME_WON` is set. If it returns TRUE the command returns and the script continues in the conjure script.
- **Guards** (`battle_koanko.c:25–36`): phase must be `KOANKO_ORDINARY_TEAM`; trainer battle; none of double/multi/two-opponent/in-game-partner/link/Frontier/Trainer Hill/secret base/e-Reader; recorded battles only allowed under the test runner; `opponentA == TRAINER_KOANKO`; no outcome; no controller exec pending; player not wiped; script stack has room for 2 pushes. The phase flag is committed immediately so it can only fire once.
- **Script:** `BattleScript_KoankoConjureTeam` (`battle_scripts_1.s:9346`): `call BattleScript_TrainerASlideMsgRet` (shows "You thought that was all?…"), `callnative BS_ConjureKoankoTeam`, `return`.
- **`ConjureKoankoTeam`** (`:47`), in order:
  1. Zero opponent-side `partyState`, `itemLost`, `activeGimmick`, `gAiPartyData->mons`, knock-off mask, `givenExpMons`, `opponentMonCanTera/Dynamax`, `arenaLostOpponentMons`.
  2. Copy `TRAINER_KOANKO`'s `Trainer`, swap in `sKoankoConjuredParty` (from `src/data/koanko_party.h`), `poolSize = 0`, and run `CreateNPCTrainerPartyWithBreakBars(gEnemyParty, &trainer, TRUE, BATTLE_TYPE_TRAINER, &gKoankoConjuredBreakBars)`.
  3. Per slot: `TryFormChange(..., FORM_CHANGE_BEGIN_BATTLE)`, record `originalItem`; recompute AI party count with `CalculateEnemyPartyCount()`.
  4. Wipe a long list of battler-0-opponent state: `battlerState`, disable/special/protect structs, illusion, move/ability/item history, Dynamax, Z-move, gimmick toggles, `monToSwitchIntoId`, `AI_monToSwitchIntoId = 0` (forces Smeargle to lead), choiced/changed items, trace, move-result/damage, AI `shouldSwitch`, absent flag; mark any pending action for that battler `B_ACTION_FINISHED`.
  5. Set phase `KOANKO_CONJURED_TEAM`.
  - Deliberately **not** touched: the fainted `BattlePokemon` and hit marker. The normal replacement path then loads the new mon, sprite and switch-in effects.
- **Why it works without a second trainer ID:** identity (`TRAINER_KOANKO`, AI flags, rewards, defeat flag) never changes; only the party array does. `TRAINER_KOANKO2` was deleted from `trainers.party` and `opponents.h` (`TRAINERS_COUNT` 860 → 859). The second team now lives in C (`TrainerMon` array, includes Dynamax level 10 on all six; Tera Fire Calyrex; Steelium Z Jirachi).
- **Bars on the conjured team:** `gKoankoConjuredBreakBars` (currently empty) is passed explicitly, so the first team's rules can't leak into reused slots.
- **Not implemented per the author's doc:** doubles/link/Frontier variants, a Valkyrie 13th mon.

---

## 4. Arcanum (`src/castoria_name_theft.c` + AI hooks)

### 4.1 State (`struct CastoriaBattleData`, `battle.h:282`)
`nameResult`, `useCommand`, `timer`, `arcanumMoves[CASTORIA_ARCANUM_COUNT]`, `mageArmorPartyMask`, `wishCandidate`, `wishOutcome`, `wishUsed`. **`CASTORIA_ARCANUM_COUNT` is 4** (`battle.h:280`), pool = Fireball, Revivify (`MOVE_REVIVAL_BLESSING_2`), `MOVE_ARCANUM_WISH`, Polymorph (`castoria_name_theft.c:408`). Magic Missile is gone.

### 4.2 Virtual slots end to end
- Slot accessors are `static inline` in `battle.h` (`GetBattlerMoveSlotCount`, `GetBattlerMoveAtSlot`, `GetChosenMoveFromPosition`, `IsArcanumAction` ~`:1319–1346`); `CastoriaGetMoveAtSlot` is a thin wrapper.
- AI → controller: `battle_controller_opponent.c:458` resolves slots ≥ 4 through `CastoriaGetMoveAtSlot`; the wild-random path picks from `MAX_MON_MOVES + CASTORIA_ARCANUM_COUNT` (`:490–495`).
- PP: **`CancelerPPDeduction` now guards with `movePosition < MAX_MON_MOVES`** (`battle_util.c:2628+`), with a comment that MP cost isn't implemented. This closes the unbounded `pp[]` write that earlier notes flagged.
- Other PP/message sites are gated by `IsArcanumAction` (`battle_script_commands.c:6889–7534`, `battle_message.c:2352`, `battle_util.c:2623,2660`).
- `IsArcanumSlotUsable` (`:436`) re-implements the ordinary move-legality checks (Encore, Disable, Torment, Taunt, Imprison, Choice, Assault Vest, Gravity, Heal Block, Throat Chop, etc.) for virtual slots, and rejects Wish unless some outcome is available.

### 4.3 Special AI path
- Gate: Razeluxe's trainer entry gained `AI_FLAG_DYNAMIC_FUNC` in `68c13b35` (`trainers.party` diff). In `ChooseMoveOrAction_Singles` (`battle_ai_main.c:812–840`), if that flag is set, a dynamic func is registered, and the battler has >4 slots, every usable spell is scored via `RunDynamicAiFunc(..., AI_SCORE_DEFAULT, isArcanum=TRUE)`.
- **A spell wins only if its score is strictly greater than the best ordinary score** (`:837`); ordinary moves win ties.
- `BattleAI_IsScoringArcanum()` (`:7376`) is a context flag restored after each callback.
- `AI_RazeluxeTrainer` (`:7246`): Wish → score = baseline if `CastoriaGetAvailableWishOutcome` ≠ NONE else 0, and it stashes the candidate in `wishCandidate`; Revivify → 0 if nobody to revive, `INT32_MAX` if Castoria has Perish Song; Polymorph → `INT32_MAX − 1` if the target actually has Perish Song; everything else baseline. Non-Castoria battlers get 0 for spell candidates.
- `AI_AliceTrainer` explicitly declines spell candidates (`:6938`).
- The chosen Wish outcome is committed only if the Wish slot wins (`wishOutcome = bestWishOutcome`, `:838`).

### 4.4 Wish execution
`BattleScript_EffectArcanumWish` (`battle_scripts_1.s:9259`): `attackcanceler`, `trycastoriawish`, animation, then branch on `cMULTISTRING_CHOOSER`. `BS_CastoriaTryWish` (`:591`) rechecks `IsArcanumAction`, move IDs, and `CastoriaCanWishFor(outcome)` *after* the canceler, then:
- **Domination** (true name, 0 Command Seals): sets `castoriaTrueNameCharges = 3`.
- **Endurance** (true or fake name, no active extra bar): `GrantSavedBreakBar`, then the script fills/animates the purple layer.
- **Debilitation** (any resolved route + a living target): ORs `SLEEP` (random duration if not already asleep) | `POISON | BURN | FREEZE | PARALYSIS | FROSTBITE` into `status1` directly (bypassing all status prevention), sets `battlerState.noOrderChange`, emits a status update, then `BS_CastoriaCancelDebilitationMoves` interrupts multi-turn moves (Sky Drop path uses a special return script).
- `wishUsed = TRUE` is set only on success; it's shared across all outcomes and never reset by switching.

---

## 5. No Order Change + status icon

- **Storage:** `BattlerState.noOrderChange:1` (`battle.h:636`); accessor `HasNoOrderChange` (`battle_util.c:5996`). Cleared in `SwitchInClearSetData` (`battle_main.c:3163`) and `FaintClearSetData` (`:3321`).
- **Enforcement points (verified):** manual switch select `battle_main.c:4422`; `jumpifcantswitch` (`:8005`, skipped when `ignoreEscapePrevention`: those callers enforce at the real withdrawal); party-screen command (`:8192`); forced-switch/Roar family (`:11152`); Emergency Exit/Wimp Out (`:1113`); Eject Button (`:6056`) and Eject Pack (`:6071`, `battle_util.c:10807`); Red Card (`:7178`); Ally Switch (`:16203`); Teleport/Baton Pass/Shed Tail cancelers and Lunar Dance (`battle_util.c:2865–2870`; Lunar Dance fails only if the user has a pending/remaining extra bar).
- **AI respects it:** `AI_CanBattlerEscape` and `IsBattlerTrapped` (`battle_ai_util.c:507,519`), `CanBattlerConsiderSwitch` (`battle_ai_switch_items.c:1096`).
- **Icon module** (`src/battle_status_icons.c`): 16×16 OBJ sprite, tag `0xD7BE` (unique; no collision with the break bar tags). Created lazily by `RefreshNoOrderChangeIcon`, which `UpdateStatusIconInHealthbox` calls (`battle_interface.c:1861`). Its callback follows the healthbox's x/y/visibility each frame, parked at `-40` (player) or `+104` (opponent) offset; self-destroys when the flag clears. `InitNoOrderChangeIcons` runs at battle init and on reshow (`reshow_battle_screen.c:77,222`, `battle_util2.c:24`); `FreeNoOrderChangeIcons` at cleanup (`battle_util2.c:68`).

---

## 6. Doc-vs-code mismatches and risks (all verified in source)

1. **`MOVEEND_BREAK_BARS` doesn't exist.** `docs/mechanics/break_bars.md` and the comment at `battle_script_commands.c:2532` describe a move-end step, but the real resolution is `HandleAction_ActionFinished` + end-turn + inline `returntoball`/`returnatktoball`/party-screen/Lunar Dance. Behavior is fine; the doc and comment are misleading.
2. **Endurance and Debilitation are unreachable in a real fight.** `CastoriaGetAvailableWishOutcome` returns only Domination (`:583`), and `AI_RazeluxeTrainer` scores Wish 0 if that returns NONE. Both are exercised only by tests that nominate them through their own Special AI. Docs state this; worth knowing when you play-test.
3. **Wish may rarely win even for Domination.** It returns the untouched baseline (100) and must strictly beat the best ordinary score. I did not measure how often ordinary scores sit below 100, so treat this as "needs a playtest/log", not a confirmed bug.
4. **Razeluxe's special AI was only switched on in `68c13b35`** (`AI_FLAG_DYNAMIC_FUNC` added to his trainer entry). Inference from the diff: before that commit the spell-scoring block would not have run for his fight.
5. **Debilitation writes six `STATUS1` bits at once** and the party-struct concern you noted is real to check: `noOrderChange` blocks *retreat*, but I did not verify what happens when the battle ends (status is written back to the party mon) or on faint. Needs a test or a look at the party-menu status display.
6. **TX Rollback has art but no code.** The two Jirachi frames/palette are committed under `graphics/pokemon/jirachi/tx_rollback/`, but nothing in `src/include/data/asm` references it. More Wishes / Reduction / Normality are reserved enum values with no implementation; MP costs are not implemented.
7. **`IsBattlerAlive` now lies about HP==0** while a bar is pending. Any future code that really means "HP is zero" (faint-style checks) must use `hp == 0` explicitly. This is the most likely source of future surprises.
8. **Known failing test:** `kazuradrop_stack_breaker.c:35` is `KNOWN_FAILING` (custom game-over leaves `AnimTask_ShakePlatforms` active at teardown).
9. **Fixed since your earlier notes:** PP `pp[]` unbounded write for slots 4+ (now guarded); `CASTORIA_ARCANUM_COUNT` is 4, not 5; the saved-layer slot is zeroed on restore.

---

## 7. How to navigate it (greps)

```bash
# every break bar entry point outside the module
grep -nE "QueueBreakBar|TryBreakBar|ClampBreakBarPassiveDamage|IsBreakBarPending|TryResolveBreakBarsAfterMove|GrantSavedBreakBar|ActivateBreakBar" src/*.c | grep -v "^src/battle_break_bar.c"

# every No Order Change enforcement point
grep -n "HasNoOrderChange" src/*.c

# Arcanum slot consumers
grep -n "IsArcanumAction\|CastoriaGetMoveAtSlot\|CASTORIA_ARCANUM_COUNT" src/*.c include/*.h

# Koanko conjure hook and the party-creation seam
grep -n "TryStartKoankoConjuring\|CreateNPCTrainerPartyWithBreakBars\|Script_TakeTrainerBreakBarOverride" src/*.c
```
