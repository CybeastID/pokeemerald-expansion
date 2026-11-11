# Trainer Break Bars and Koanko

Trainer assignments live in `src/data/trainer_break_bars.h`; `trainers.party`,
trainerproc and saved Pokemon data need no new fields. The existing
[Break Bar engine](break_bars.md) owns depletion, switching, palettes and phases.

## Trainer defaults

Add a block only for a trainer that needs explicit assignments inside the
existing `sTrainerBreakBars[trainerId][difficulty]` initializer. Write its rules
directly with `INLINE_BREAK_BAR_RULES`; no separate array, ruleset wrapper or
registration entry is needed:

```c
[TRAINER_ALICE] =
{
    [DIFFICULTY_NORMAL] = INLINE_BREAK_BAR_RULES(
        // Every Jirachi selected from this trainer's party/pool gets two extras.
        {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, &gTrainerBreakBarProfiles[1]},
        // Source entry 0 gets five extras, regardless of selected party position.
        {TRAINER_BREAK_BAR_ENTRY, 0, &gTrainerBreakBarProfiles[4]},
        // Source entry 3 gets no configured bars, even if it is a Jirachi.
        {TRAINER_BREAK_BAR_ENTRY, 3, NULL},
    ),
},
```

The table is sparsely authored with designated initializers. Do not add blocks
for ordinary trainers: unspecified trainers and difficulties initialize to NULL.
Keep each configured trainer's assignments in one block, normally using only
`DIFFICULTY_NORMAL`. Add another difficulty inside that same block only when
needed. An explicit difficulty ruleset replaces the whole Normal ruleset rather
than merging with it.

Species IDs are exact, including forms. Entry indices are zero-based and refer
to the source party/pool, including a copied pool. Entry rules beat species
rules. Use at most one rule for each selector/value. Missing difficulty rulesets
inherit Normal; an explicit `&gEmptyTrainerBreakBars` disables that fallback.
For example, `[DIFFICULTY_HARD] = &gEmptyTrainerBreakBars` inside a trainer's
block disables its Normal assignments on Hard. A trainer with no applicable
ruleset gets the shared empty ruleset.

Use `INLINE_BREAK_BAR_RULES` in file-scope initializers only, including the
existing static table. Its compound-literal ruleset, rule array and nested
inline configs then have static lifetime; the helper derives the rule count.
For an explicit empty ruleset, use `&gEmptyTrainerBreakBars` instead of an empty
macro invocation. The existing `TRAINER_BREAK_BAR_RULES(array)` initializer
remains available for named arrays and exported rulesets.

Profiles `[0]` through `[4]` provide one through five extra full HP bars. Custom
named or inline `BreakBarConfig` objects can provide phase scripts. A NULL
`onBreakScript` uses the normal refill; use `afterBreakScript` for effects after
that refill.
Both scripts must return. Assignment happens at party creation, before entry
effects or healthbox drawing; switching and form changes never reassign bars.

## Lookup and runtime integration

`src/trainer_break_bars.c` includes the shared data header. The table's full
dimensions are `[TRAINERS_COUNT][DIFFICULTY_COUNT]`, but only explicitly
configured trainers have designated initializer blocks. The current production
table contains only Koanko's Normal registration, pointing to the exported,
empty `gKoankoInitialBreakBars`. The examples in this document are not active
assignments; the refactor adds no new bars to any trainer.

The existing party-creation flow uses the configuration in this order:

1. `CreateNPCTrainerParty` first consumes a matching `settrainerbreakbars`
   override. A non-NULL override, including the shared empty ruleset, replaces
   the trainer defaults for that initial generation.
2. Without an override, `GetTrainerBreakBarRules` validates the trainer ID and
   calls the private `SelectTrainerBreakBarRules` helper for its table row.
   Selection uses the requested difficulty, then Normal, then the shared empty
   ruleset. Invalid difficulty values return the shared empty ruleset directly.
3. `ResolveTrainerBreakBar` resolves the selected source entry and exact species.
   Any matching entry rule takes precedence over species rules, even when its
   config is NULL. If duplicate entry rules exist, the first match wins; if no
   entry matches, the last matching species rule wins, including NULL. Prefer
   unique selector/value pairs when authoring new data.
4. `InitializeTrainerBreakBar` registers the resolved profile and extra-bar
   count in the opponent's party state during party creation. It skips missing
   configs and invalid bar counts and does not draw graphics. The existing
   Break Bar engine still owns depletion, refills, phase scripts and rendering.

`INLINE_BREAK_BAR_RULES` only builds const configuration data: it wraps an
anonymous rule array with the existing pointer/count structure and returns its
address. It does not register a battler or execute scripts. Named profiles,
exported rulesets and the original array initializer remain compatible, and
there are no new fields in trainer data, saved Pokemon or script commands.

## Custom phase scripts

### Worked example: Alice's second Pokemon, two extra bars

This example means two extra bars plus the ordinary red bar: the player breaks
blue first, purple second, then red to faint the Pokemon. The first break runs
script X; the second break uses the normal refill and then runs script Y.

Declare X and Y in `include/battle_scripts.h` (or before the table in
`src/data/trainer_break_bars.h`):

```c
extern const u8 BattleScript_AliceBarX[];
extern const u8 BattleScript_AliceBarY[];
```

Add or replace Alice's block inside `sTrainerBreakBars`. Keep the profile and
its script references directly alongside the assignment:

```c
[TRAINER_ALICE] =
{
    [DIFFICULTY_NORMAL] = INLINE_BREAK_BAR_RULES(
        {
            .selector = TRAINER_BREAK_BAR_ENTRY,
            .value = 1, // Source entries are zero-based: the second Pokemon.
            .config = &(const struct BreakBarConfig)
            {
                .extraBars = 2,
                .phases =
                {
                    [BREAK_BAR_BLUE - 1] = // First extra bar broken.
                    {
                        .onBreakScript = BattleScript_AliceBarX,
                    },
                    [BREAK_BAR_PURPLE - 1] = // Second extra bar broken.
                    {
                        .afterBreakScript = BattleScript_AliceBarY,
                    },
                },
            },
        },
    ),
},
```

Use the trainer ID actually used by the encounter: `TRAINER_ALICE`,
`TRAINER_ALICE2`, and `TRAINER_ALICE3` are separate entries. This rule follows
the second source entry if a party pool changes the selected party order; on a
fixed team it is simply Pokemon 2. In the current `TRAINER_ALICE3` team that entry
is Kazuradrop; assigning a custom profile there prevents Bug Space from granting
its default profile, so retain her transformation hooks in the custom profile
if that behavior is wanted.

Finally, define X and Y in `data/battle_scripts_1.s`:

```asm
BattleScript_AliceBarX::
    @ Put X's custom effects here.
    @ X replaces automatic healing, so explicitly refill the next layer.
    call BattleScript_BreakBarRefill
    return

BattleScript_AliceBarY::
    @ Put Y's custom effects here. The normal refill has already happened.
    return
```

No changes to `trainers.party` or an overworld event script are required for this
permanent trainer assignment. The examples' comments are placeholders for your
effects; they do not add effects themselves.

### Another example: a named profile for Koanko

Define a custom profile in `src/data/trainer_break_bars.h` and point a trainer
rule at it. For example, to give Koanko's conjured Jirachi one purple extra bar:

```c
extern const u8 BattleScript_KoankoJirachiBreak[];
extern const u8 BattleScript_KoankoJirachiAfterBreak[];

static const struct BreakBarConfig sKoankoJirachiBars =
{
    .extraBars = 1,
    .phases[BREAK_BAR_PURPLE - 1] =
    {
        .onBreakScript = BattleScript_KoankoJirachiBreak,
        .afterBreakScript = BattleScript_KoankoJirachiAfterBreak,
    },
};

// Replace the existing empty definition; do not add a second definition.
const struct TrainerBreakBarRules gKoankoConjuredBreakBars =
    TRAINER_BREAK_BAR_RULES(((const struct TrainerBreakBarRule[])
    {
        {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, &sKoankoJirachiBars},
    }));
```

The script declarations must precede the profile. They can instead be added to
`include/battle_scripts.h`, which is included by the source compiling the shared
data file. Named profiles can also be referenced directly from another trainer's
`INLINE_BREAK_BAR_RULES` block. Koanko's initial and conjured rulesets remain
distinct exported objects grouped together; her conjured ruleset is applied by
her team-rewrite script rather than the defaults table.

Add the corresponding global labels to `data/battle_scripts_1.s`:

```asm
BattleScript_KoankoJirachiBreak::
    @ Custom phase changes can go here.

    @ A custom onBreakScript replaces the automatic refill.
    @ Explicitly call this if the new layer should refill to full HP.
    call BattleScript_BreakBarRefill
    return

BattleScript_KoankoJirachiAfterBreak::
    @ Add dialogue, buffs, animations, etc. after the break script here.
    return
```

These are example hooks; the comments add no effects by themselves. The sequence
is `onBreakScript`, then `afterBreakScript`, then the engine's continuation.
Leaving `onBreakScript` NULL uses the standard full refill. When supplying your
own `onBreakScript`, it must restore usable HP itself, either by calling the
shared refill script or by performing custom healing. `afterBreakScript` runs
after that script completes; it does not automatically heal. For a full refill
followed by special effects, specify only `afterBreakScript`.

During both hooks, `BS_TARGET` is the Pokemon whose bar broke. `BS_ATTACKER`
retains its existing meaning and is not automatically changed to the bar owner.
The remaining bar count and palette have already advanced to the next layer.
The engine restores the original target after the hooks return.

Both hooks must finish with `return`, rather than `end` or `end2`. Phase indices
identify the extra bar being broken: purple is `[0]`, blue `[1]`, green `[2]`,
yellow `[3]`, and orange `[4]`. With two extras, blue's hooks run first, then
purple's. The final red-bar faint runs neither hook.

Saved layers added by Break Charm, Lunar Dance, or another `GrantSavedBreakBar`
caller restore their covered HP/max-HP pair and skip both configured hooks.
The original configured layers beneath them still use their phase scripts.

## Kazuradrop: ability-activated profile

Kazuradrop's `gKazuradropBreakBarConfig` lives in the same shared data file:

```c
const struct BreakBarConfig gKazuradropBreakBarConfig =
{
    .phases[BREAK_BAR_PURPLE - 1] =
    {
        .onBreakScript = BattleScript_KazuradropTransform,
        .afterBreakScript = BattleScript_KazuradropTransformReturn,
    },
    .extraBars = 1,
    .flags = BREAK_BAR_IGNORE_WHILE_TRANSFORMED,
};
```

This is an existing definition, not a second definition to paste into the file.
Her Bug Space ability hook in `src/battle_util.c` activates it through
`ActivateBreakBar(battler, &gKazuradropBreakBarConfig)` when Bug Space first
starts. The existing guards require an untransformed battler with no profile
already registered. The profile is not assigned to Alice's trainers at party
creation, so its timing and availability to player and wild battlers remain
unchanged.

Her `onBreakScript` performs the existing transformation and custom healing;
it does not need the generic refill. Her `afterBreakScript` runs the existing
post-transformation continuation. Dialogue, doubled max HP, full restoration,
moveset changes, status cure, accelerated decay and Guts retain their existing
behavior. The flag disables break interception while transformed. Sharing the
profile changes where the configuration is authored, not what triggers it.

## Script interfaces

Before `trainerbattle`, an overworld script can replace that encounter's initial
assignments. Unlike anonymous inline defaults, rulesets referenced by assembly
must have exported C names. For example, define this at file scope:

```c
const struct TrainerBreakBarRules gBossRules =
    TRAINER_BREAK_BAR_RULES(((const struct TrainerBreakBarRule[])
    {
        {TRAINER_BREAK_BAR_SPECIES, SPECIES_JIRACHI, &gTrainerBreakBarProfiles[1]},
        {TRAINER_BREAK_BAR_ENTRY, 3, NULL},
    }));
```

Then reference that existing symbol from the event script:

```asm
settrainerbreakbars TRAINER_KOANKO, gBossRules
```

Pass `gEmptyTrainerBreakBars` to disable trainer defaults. Rulesets must be
exported C symbols with static lifetime. Up to two distinct trainers may be
overridden in one script, supporting two-opponent encounters; repeating a trainer
replaces its pending override. `VAR_RESULT` is TRUE on success, FALSE for invalid
IDs, NULL rulesets or a full override list. Overrides are consumed by matching
initial party generation. Unused overrides clear when the script stops, finishes
or resets, after initial battle setup, and at battle cleanup. Waiting for a
message or another asynchronous command preserves them.

Inside a battle script:

```asm
activatebreakbar BS_TARGET, gCustomBreakBarConfig, BattleScript_ActivationFailed
```

The command uses `ActivateBreakBar`: it does not heal, accepts one through five
extra bars, and branches on failure (including an existing active stack).
Default assignment does not repeat after depletion; explicit scripted activation
may create a new stack after all extras have been broken.

## Wild encounters

The Break Bar engine works on wild battlers too, including the same phase
scripts. Scripted encounters can assign a profile with `setwildbreakbars` before
`setwildbattle` and `dowildbattle`; see [Break Bar reference](break_bars.md#scripted-wild-encounters-and-capture).
There is no automatic wild-species/map rules table. `setwildbattle` alone does
not attach a profile.

An existing C ability/encounter hook can activate a static profile once the wild
battler's battle data is loaded and it is alive:

```c
ActivateBreakBar(battler, &gWildBossBars);
```

An existing battle-script entry point can use the same exported profile:

```asm
activatebreakbar BS_OPPONENT1, gWildBossBars, BattleScript_NoWildBars
```

Here `gWildBossBars` is a `BreakBarConfig` you define and export, and
`BattleScript_NoWildBars` is the caller's failure label. This remains useful for
activation during an encounter; the overworld setter registers a profile before
the initial healthbox is shown.

## Koanko

Normal singles encounters with `TRAINER_KOANKO` automatically continue into a
second six-Pokemon team. The conjured party is in `src/data/koanko_party.h` and
retains its former levels, items, moves, EVs, IVs, abilities and Tera setting.
It uses the original trainer's presentation, AI settings, rewards and defeat flag.
No second trainer ID is needed.

After the first team's final faint and EXP processing, the victory check runs a
Trainer Slide, rewrites the enemy party and returns to ordinary replacement.
Smeargle leads; entry abilities and hazards run normally. Player and field state,
spent trainer inventory and trainer-wide gimmick consumption carry over. Reused
enemy slots receive fresh per-Pokemon state, including Break Bars, item use,
form tracking and AI history. Each defeated opponent remains an EXP source;
victory and trainer completion happen only after the second team.

`gKoankoInitialBreakBars` and `gKoankoConjuredBreakBars` are independent rulesets,
both initially empty. The conjuring script applies the second explicitly, so
initial encounter overrides cannot accidentally give the new team the old team's
profiles. Ordinary slots can therefore be reused without inheriting old bars.

Conjuring requires a usable player party. Simultaneous faints with a surviving
reserve allow normal replacement; exhausting the player ends the encounter.
Link, recorded playback, Frontier, partner and doubles variants do not conjure.
The automated battle runner can exercise the same sequence through its recorded
fixtures. Valkyrie and further teams are not implemented.

## Regression checks

Run these commands from the repository root in WSL:

```sh
make -j8 check TESTS=test/battle/trainer_break_bars.c
make -j8 check TESTS=test/battle/wild_break_bars.c
make -j8 check TESTS=Break
make -j8 check TESTS=Koanko
make -j8 check TESTS=Kazuradrop
make -j8
```

The trainer tests exercise inline configs and phase pointers, automatic counts,
named profiles, entry/species precedence, NULL overrides, duplicate ordering,
unconfigured trainers, invalid IDs/difficulties, and explicit empty rulesets.
Synthetic difficulty rows test inheritance and whole-ruleset replacement without
adding production trainer fixtures. `Test_SelectTrainerBreakBarRules` exposes
the same private selection helper only under `#if TESTING`; it is absent from
normal builds. Script tests cover assignment from inline configs, empty
replacements, independent opponents, queue limits, one-time consumption and
override cleanup. Existing tests continue to cover party-pool source indices
and battle-script activation.

The broader groups exercise the engine, Koanko's replacement team and
Kazuradrop's ability-triggered profile. The Stack Breaker test is currently
marked `KNOWN_FAILING` because its custom game-over leaves
`AnimTask_ShakePlatforms` active at teardown; that existing exception is
separate from trainer assignment lookup.
