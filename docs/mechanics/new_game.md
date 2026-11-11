# Simplified new game

Implemented on `mypatch` before the planned upstream migration (2026-11-10).
The ROM builds; interactive fresh-save validation remains pending.

## Startup contract

New Game uses the existing gender prompt and player naming screen in
`src/main_menu.c`, then the normal `CB2_NewGame` / `NewGameInitData` path.
Birch's speech was already bypassed in this checkout; no menu change is needed.
The former `WarpToNewGamePostClockMeetRival` override actually warped to
AliceRoom, group 35/map 0/warp 1. Its replacement, `InitSimplifiedNewGame` in
`src/new_game.c`, is the only custom fresh-save initializer.

- Spawn: `MAP_LITTLEROOT_TOWN`, zero-based map coordinates **(10, 10)**,
  `WARP_ID_NONE`, on foot, facing south via the normal avatar reset.
  The tile is walkable grass (`0x3001`), without an object or coordinate event.
- Party: one **Treecko, level 5**, no held item, ordinary generated stats/moves,
  owned by the selected player. Change `NEW_GAME_STARTER_SPECIES` and
  `NEW_GAME_STARTER_LEVEL` in `src/new_game.c`. Torchic and Mudkip automatically
  select rival starter branches 1 and 2; Treecko selects branch 0. Other species
  use the Treecko rival branch and require deliberate rival-team design if used.
- Bag: **5 Poke Balls, 3 Potions**. Normal initial PC items remain unchanged.
- Money: normal **3,000**; coins 0; Running Shoes enabled.
- Menus: party and regional Pokedex available; starter recorded seen/caught.
  PokeNav, Match Call, National Dex, badges, HMs and later story rewards are not
  granted. They retain normal acquisition/progression.
- Whiteout: the selected gender's normal Littleroot bedroom heal location,
  including the engine's Mom respawn mapping when whiteout cutscenes apply.

The existing town layout sealed the Route 101 exit. Only tiles **(10, 1)** and
**(11, 1)** in `data/layouts/LittlerootTown/map.bin` change, from impassable
`0x044E` blocks to walkable `0x3001` grass (four bytes total). The existing north
connection, buildings, NPCs and custom encounters remain intact.

## Initialization order and state ownership

`NewGameInitData` keeps normal save clearing, Trainer ID generation, selected
name/gender, party/storage/bag initialization, berry/map defaults, difficulty,
custom-state resets and follower cleanup. After all resets it invokes the new
helper, which loads Littleroot's map header/coordinates, runs
`EventScript_InitSimplifiedNewGame` from `data/scripts/new_game.inc`, creates the
starter with the existing `ScriptGiveMon` interface, and adds starting items.
Loading the map first gives the starter the correct met location. Giving the
starter after party resets preserves its party count and Pokedex registration.
The script uses only immediate commands, including the normal
`SetUnlockedPokedexFlags` special for linking metadata; it has no waits/dialogue.

All progression state belongs to the ordinary save flags/variables. The only
new constants are compile-time starter defaults. No save structure, persistent
callback, battle script, or cleanup handler is introduced. Continue/load does
not invoke this initializer or give duplicate Pokemon/items.

The initializer mirrors the end of the opening Pokedex/Running Shoes handoff:

- `VAR_LITTLEROOT_INTRO_STATE = 7`: no truck, moving-in, clock or TV frame events.
  `FLAG_SET_WALL_CLOCK` also prevents manually examining the clock from starting
  the skipped clock/Mom sequence. Movers and both trucks are hidden.
- Both house-state variables are 2, with gender-specific Mom/rival-family
  visibility matching the truck's normal setup. No neighbor introduction is
  pending, and Hall of Fame / SS Ticket states remain untouched. Mom can heal.
- `VAR_LITTLEROOT_TOWN_STATE = 4`, Running Shoes flags set: no exit guard,
  rescue request or delayed shoe handoff. Mom outside remains hidden by the
  normal reset defaults.
- `VAR_LITTLEROOT_RIVAL_STATE = 4`, `VAR_BIRCH_LAB_STATE = 5`: bedroom meeting,
  starter naming/reward and Pokedex cutscenes cannot replay. Birch and rival are
  visible in the lab; both bedroom balls and bedroom rivals are hidden.
- `VAR_ROUTE101_STATE = 3`, rescue flag set, chase actors and bag hidden:
  Route 101 rescue triggers and map-name suppression are bypassed. The normal
  route boy is restored; later Birch roaming remains controlled by vanilla
  Petalburg progression.
- `VAR_OLDALE_RIVAL_STATE = 2`, `VAR_OLDALE_TOWN_STATE = 1`, adventure flag set:
  no rival return-to-lab trigger or footprints-man west-exit block.
- The Route 103 rival is hidden and its defeated/progression flag is set as a
  **skipped introductory battle**, not a verified battle victory. Rival-left,
  met-rival-Mom, received-Pokedex and cable-club-tutorial state 1 mirror the
  completed opening. `VAR_STARTER_MON` agrees with the configured Hoenn starter
  for later rival battles. No later trainer victory flags are set.

The exact flag list is in `EventScript_InitSimplifiedNewGame`. Flags not changed
there retain `InitEventData` / `EventScript_ResetAllMapFlags` defaults. In
particular Petalburg's Norman/Wally sequence, Rustboro/Devon, gyms and the rest
of Hoenn have not been completed.

## Preserved encounters and upstream maintenance

AliceRoom's map, item-ball team script, `AliceRoom_Alice_Fight`, `TRAINER_ALICE2`,
respawn behavior and battle mechanics are unchanged. It remains selectable with
the existing non-release overworld debug warp: hold R + Start, Utilities,
Warp, group **35**, map **0**, warp **1**. Take its team package before speaking
to Alice. This room and AlicesRoom already form an isolated warp pair; this
patch does not add a new ordinary-world entrance. Littleroot's separate Alice
NPC (`LittlerootTown_Alice_Fight`, `TRAINER_ALICE3`) also remains unchanged at
(9, 4), as do the optional debug team ball and other custom warps.

The upstream integration is limited to the existing fresh-save override in
`src/new_game.c`, an immediate event script alongside existing reset scripts,
and the two exit tiles. No overworld/menu/battle algorithm changes are required.
This avoids a competing startup hook and numeric map IDs. Textual merge exposure
is small; semantic review must still recheck reset ordering, starter creation,
heal locations and vanilla intro states during the upstream migration. See
`upstream_modifications.md`. No upgrade or Stage 05 work is included.

## Validation and stable snapshot

- Executed `make -j4` through WSL: **passed**, producing `pokeemerald.gba`.
  Log: repository-root `simplified-new-game-build.log`. Warnings about missing
  final newlines in unchanged LittlerootTown, CastersTerrarium and AliceRoom
  scripts were reported. No build errors.
- Source/asset inspection confirms the spawn tile, four-byte exit change,
  Route 101 connection and ordinary route entrances, menu flag predicates,
  starter OT/Pokedex registration path, gender-specific healing, intro trigger
  states, and preserved Alice scripts. `git diff --check` passed for this patch.
- No gameplay test suite was run: battle mechanics are unchanged. There is no
  applicable `TESTS=` filter for interactive New Game/save/map validation.
- mGBA is installed, but automatic approval review rejected computer-use access
  to that app. **None of the requested emulator checks is claimed verified.**

Rebuild in WSL from repository root with `make -j4`. Keep `src/new_game.c`,
`data/scripts/new_game.inc`, `data/layouts/LittlerootTown/map.bin` and their map /
engine dependencies stable while validating this ROM. The build includes the
pre-existing unrelated working-tree changes; those were not edited for this task.
An isolated copy is prepared at `tmp/simplified-new-game/fresh-start.gba` with
no save copied from the normal ROM. Use an unused ROM/save basename when
repeating; do not reuse an old battery save or save state.

Pending emulator checks (repeat house/respawn checks for both genders):

1. New Game -> gender/name -> outdoor (10, 10), one level-5 Treecko, correct OT,
   inventory and party/Pokedex menu entries, no opening dialogue.
2. Enter/leave both houses, both bedrooms and Birch's lab. Examine the clock;
   heal with Mom. No delayed intro, duplicate starter or Pokedex scene.
3. Walk north into Route 101, encounter/catch wild Pokemon, and battle normally.
   Return to Littleroot and leave again. Check an early whiteout returns safely.
4. Save in town, reload with Continue, then save/reload on Route 101. Party,
   items, name, gender, money and position persist without repeated grants.
5. Reach Oldale, Route 103 and Route 102 without introductory blockers; continue
   to Petalburg's normal Norman/Wally event and toward Rustboro. Check later
   rival teams agree with the configured starter.
6. On a separate disposable save, use the existing warp described above to
   enter AliceRoom, take its package and start Alice's battle. The package
   replaces the party, so do this separately from normal progression checks.
