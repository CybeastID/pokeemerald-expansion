# Stage 07 — Independent AI Zacian reinforcement and return to singles

Status: **fully planned**; largest integration stage, tuning choices recorded below. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

Immediately after Eternamax's transform animation, add a temporary Zacian-Crowned to player-right, controlled by AI and owned separately from the real player party. Retire it on faint and remove the temporary formation before Dialga enters. This assistance is mandatory; party-slot substitution or Eternamax nerfs do not satisfy it.

## 2. Existing code verified

- `src/battle_tx_rollback.c:BS_TxRollbackCreate/Initialize/FinishEntry` demonstrates live 2→4 position expansion while preserving old choices/queue. It uses real enemy slot six, disables player-right, reserves enemy-right and keeps doubles until battle cleanup. It has no collapse-to-singles path and cannot be mirrored unchanged.
- `include/battle.h:GetBattlerMon`, `GetBattlerParty`, `GetBattlerPartyState` currently resolve exclusively by player/enemy side and gBattlerPartyIndexes. An out-of-range sentinel party index is unsafe; reusing a real player index aliases its data/state.
- `src/battle_controllers.c` common get/set data and sprite handlers use these accessors, making them useful routing points. Party masks iterate PARTY_SIZE; an isolated one-element allocation is unsafe if exposed as a party pointer.
- `src/battle_controller_player_partner.c` provides AI move/action handlers, but ChoosePokemon directly inspects gPlayerParty and partner trainer presentation. Ordinary INGAME_PARTNER/MULTI modes have real-party partition/whiteout assumptions. They are not separate ownership storage.
- `src/battle_ai_main.c` and `battle_ai_util.c` cache party knowledge by side/index. Ownership routing must include AI history; changing GetBattlerMon alone would still corrupt the player's cache.
- `src/battle_gimmick.c:GetActiveGimmick/SetActiveGimmick` also index side/party slot directly; normal trainer-consumption helpers share state with a same-trainer partner. An independent owner needs explicit routing/exclusion even when Zacian never chooses a gimmick, or it can inherit the player's active Tera/other gimmick.
- `src/battle_script_commands.c:NoAliveMonsForPlayer` counts the real player party; preserve this loss authority. CountAliveMonsForBattlerSide also reads side-indexed pending-bar state and must distinguish temporary owners. Player-party EXP/revival/selection paths have direct references requiring targeted handling.
- `src/reshow_battle_screen.c`, `battle_interface.c`, `battle_gfx_sfx_util.c` already consult TX position policy; present policies are summon-specific. Existing graphics cleanup initializes player-right sprite storage specially.
- `SPECIES_ZACIAN_CROWNED`, front/back assets and Intrepid Sword exist. `battle_util.c:EFFECT_DYNAMAX_DOUBLE_DMG` doubles base power only for active GIMMICK_DYNAMAX; species Eternamax alone does **not** qualify today. Setting Dynamax just for this would add unrelated HP/expiration mechanics.

## 3. Implementation design and order

### A. Separate owner storage before activating the encounter

1. Add a battle-owned temporary-ally record with lifecycle NONE/ENTERING/ACTIVE/FAINTED/DEPARTING, dedicated Pokemon party-compatible storage (six entries, only local index 0 populated), separate PartyState and AI owner cache, and original formation flags. Allocate in battle memory and release on every cleanup exit.
2. Add explicit actor ownership queries and route GetBattlerMon/GetBattlerParty/GetBattlerPartyState for the temporary battler to this storage. Its index 0 is local to its owner; it never means real gPlayerParty[0]. Keep GetSideParty and player menus referring to real party. Add a corresponding AI-owner accessor for active history/cache writes and exclude the ally from normal reserve selection.
3. Audit direct player/side-index reads in faint processing, controller transfer, raw data writes, AI history, moves/PP, item/revival targets, EXP participation, activeGimmick, itemLost/original-item tracking, once-per-owner ability flags and bar handling. Convert actor-owned accesses to the owner-aware API. Give Zacian separate NONE gimmick state and exclude it from player trainer-wide consumption sharing; player-left keeps all normal gimmick availability. Side weather/screens/targeting remain genuinely shared by battle side. Do not increase PARTY_SIZE or insert/save/restore a hidden seventh player member.
4. Extend the existing slot-policy seam into a small encounter-position policy that can represent Castoria's reserved opponent-right and Koanko's independent player-right. Keep TX wrappers/behavior compatible. A fainted ally slot is retired, never refillable from player reserves, revival items or Revival Blessing.

### B. Entry, action selection and departure

5. At stage 06's completed-animation continuation, validate encounter/owner, idle required controllers and resource headroom, then expand to four positions with only DOUBLE added. Opponent-right remains disabled; player-right uses owner-aware AI partner controller handlers. Do not enable MULTI/INGAME_PARTNER merely for AI and thereby split the player's party or truncate loss checking.
6. Create Crowned Zacian with Intrepid Sword and Behemoth Blade/Sacred Sword/Play Rough/Swords Dance in its dedicated storage. Use ordinary entry presentation/ability handling, show a back sprite and independent healthbox, reposition player-left for doubles. Entry must happen before the next selection; preserve the current action cursor and never grant Zacian an attack in an already-chosen queue. It joins ordinary AI next turn.
7. AI can choose moves/targets only, cannot switch, use the player's inventory, consume a player gimmick or prompt a party menu. Use normal scoring with a narrow pressure/setup adjustment: direct useful damage/KO preferred, Behemoth Blade receives its actual anti-Eternamax merit, Swords Dance only when its payoff beats immediate pressure and does not loop at high stages. No fixed move rotation.
8. Recommended anti-Eternamax policy: extend the damage-doubling predicate for the Koanko-owned genuine Eternamax target only, sharing it between damage and AI estimates. Apply the established anti-Dynamax move effect without setting Dynamax state; ordinary Dynamax targets retain existing behavior. This is a proposed encounter rule, not current behavior.
9. Zacian faint runs normal faint animation and effects, retires its owner/position, cancels its pending action and allows the real player to continue. If the real party is exhausted, loss occurs even with Zacian alive; it cannot sustain the encounter alone. Simultaneous faints must preserve normal player-left replacement.
10. On Eternamax's **genuine final faint**, request departure before opponent replacement sends Dialga. At a controller/script-safe barrier retire Zacian, finish its required faint/departure effects, clear both temporary positions and their queued actions/targets, free healthbox/sprite/palette/shadow resources, restore singles layout/flags/count, and then resume ordinary Dialga replacement. Translate any pending action/end-turn cursor so existing left-side actions resolve once; do not shrink loop bounds halfway through an unchecked iteration. Pending delayed effects targeting retained left positions remain valid; retired-right positional effects expire unused.
11. Cleanup is idempotent if Zacian already fainted, Eternamax and Zacian faint together, hazards/status/Destiny Bond cause the final faint, the player loses first, or battle cleanup occurs during presentation. No extra ordinary turn for departure. Retire Reduction's ally occupant before freeing its backing mon so cleanup cannot restore into invalid/player storage.

Proposed files: new narrowly scoped temporary-ally module/header; battle state/accessors; common controllers plus AI partner handlers; active AI owner/history accessors; faint/replacement/EXP/revival gates; existing layout/reshow graphics modules; Koanko transition scripts and focused ally tests. This is a necessary ownership extension, not a generic multi-trainer rewrite.

## 4. Dependencies

Stages 01 and 06; stage 04's side-oriented Reduction/normalization must support the ally without aliasing player data. Integrate final AI with stage 05 encounter dispatch. Ordinary Castoria TX topology must remain unchanged.

## 5. Edge cases and invariants

Real six-player party bytes, count, order, PP, items, bar state and AI history cannot be replaced or used as scratch. Zacian is not owned/capturable/revivable and cannot gain EXP or survive encounter cleanup. Genuine side effects may affect it, including the continuing player-side Reduction; its baseline/HP normalization uses its own storage. It has no normal reserves. Player bag/menu actions remain available for the real party. Singles must be restored **before Dialga entry**, not merely next turn or battle end.

## 6. Test strategy

New prefix `Koanko Zacian`; targeted commands `wsl make check TESTS='Koanko Zacian'`, `TESTS='Koanko Eternatus'`, `TESTS='Intrepid Sword'`, `TESTS='Arcanum Wish More Wishes'`, `TESTS='Arcanum Wish Reduction'` and affected ordinary doubles/partner test-name groups.

First unit-test owner routing with distinct real-party canaries and controller read/write masks. Then test live expansion after animation, no player-right prompt, independent move/PP/status/AI history, one Intrepid Sword entry, no inventory/gimmick use, no inherited player Tera/other active gimmick, item/ability-history isolation, damage-doubling predicate/AI consistency, useful attacks versus repeated setup, ally faint/no replacement/no revive, true player loss with surviving ally, simultaneous KO in all orders, player reserve replacement, continuing Reduction, menu reconstruction and resource exhaustion. Finally assert count=2/layout singles and both right slots clean before Dialga send-out, including Zacian already dead and delayed/faint-triggered cleanup cases. Run repeated encounter→ordinary battle emulator checks for leaked tasks/palettes.

## 7. Completion criteria

Zacian enters at the exact required boundary, acts through standard AI execution, uses no real party slot, can faint without causing a false loss, and departs cleanly before Dialga. Player ownership and Castoria summon behavior remain intact. Genuine Eternamax stats remain unchanged.

## 8. Open questions

- **Tuning not supplied:** Zacian level, EVs, nature and held item. Recommend matching current Eternatus level (103), 31 IVs, zero EVs, neutral nature, no item as a reproducible baseline; approve tuning before balance acceptance. The mandatory form/ability/kit are settled.
- **Damage rule to approve:** encounter-specific anti-Eternamax doubling above. Current code does not provide it for a non-Dynamaxed form. Do not silently claim canonical form stats imply qualification.
- **Entry hazards default:** independent reinforcement uses ordinary switch-in hazards/abilities unless explicitly exempted; unlike Castoria's creation, no exemption is specified. If hazards immediately faint Zacian, it still entered at the required time and retires normally.
- Ownership/layout feasibility is supported by inspected seams, but runtime proof is a stage gate; no claim that this is a plug-and-play mirror of TX is made.
