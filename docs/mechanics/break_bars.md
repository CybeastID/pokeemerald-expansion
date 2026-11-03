# Break Bar reference

The shared API is in `include/battle_break_bar.h`, implemented by
`src/battle_break_bar.c`. Boss behavior opts in by registering a configuration;
the faint hook and healthbar renderer do not identify species or abilities.

## Layers and ownership

Red is the base HP bar. Up to five extra bars are stacked above it, in the order
purple, blue, green, yellow/gold, orange. A complete stack drains orange, gold,
green, blue, purple, then red. The remaining count is a three-bit field in the
existing per-party Pokemon state. Registration and depletion survive switching, and replacements do
not inherit the previous occupant's bars. Registered base bars stay red; ordinary
unregistered Pokemon retain their usual HP-dependent colors.

`ActivateBreakBar(battler, config)` marks the current HP bar as the top layer.
It does not heal, change max HP, or automatically register any species. An active
stack cannot be reactivated. Once its extra bars are depleted, a caller may
explicitly activate another stack. Configurations must have static/battle-long
lifetime and specify one to five extra bars.

`BREAK_BAR_DEFER_INITIAL_DRAW` registers the stack without its usual palette fade
or immediate healthbar redraw. A caller with a custom entry animation must finish
that presentation itself. Normal registrations keep their existing behavior.

## Phase scripts

`BreakBarConfig.phases` is indexed from the bottom extra layer: purple at index 0,
blue at 1, green at 2, yellow/gold at 3, orange at 4. Each entry may provide an
`onBreakScript` and an `afterBreakScript`. Both must return. A NULL `onBreakScript` uses the shared refill
script, restoring the next layer to the current max HP. Custom scripts can set
different HP values and perform the boss's phase effects instead.

Attack damage can empty an extra bar. At zero HP the bar becomes pending rather
than fainting. Later hits of the same move cannot damage the next layer. After the
whole move, `MOVEEND_BREAK_BARS` consumes the pending layer before move-switch
effects and before the next battler acts. It saves the original target and makes
the bar owner `BS_TARGET`; phase scripts return through the target-restoring
continuation. A slower owner takes its scheduled action after the transition.
Delayed attacks and instant-death effects in end-turn scripts resolve before the
next end-turn event.

Ordinary passive damage (including poison, weather, hazards, recoil, confusion,
abilities and items) stops at 1 HP while an extra bar is active. It cannot consume
a layer. Perish Song, Destiny Bond and deliberate self-KO bypass that floor; OHKO
attacks also break normally. The final red bar has no passive-damage protection.

Kazuradrop registers one extra purple bar when Bug Space first activates. Her
configuration runs the existing transformation and return scripts. Dialogue,
doubled max HP, full restore, moveset changes, status cure, accelerated decay, and
Guts remain Kazuradrop's policy. The configuration preserves her existing rule
that transformation disables her break interception.

## UI lifecycle

The module owns per-battler palette slots and the entry fade, so different stack
colors can coexist. Menu-return paths call `ReshowBreakBars`; battle cleanup calls
`FreeBreakBarGraphics`. Phase changes cancel obsolete fades before changing a
palette. Palette or task allocation failure does not write through an invalid
sprite, palette, or task index.

The five extra-layer palettes use two-color adaptations sampled from FGO's gauge
markers and quantized to GBA RGB. Green uses a dark teal/emerald body, yellow uses
muted gold, and orange has a pink/coral highlight. The bar tiles, frame, and red-base
colors are retained.

`GrantSavedBreakBar` saves the covered current/max HP pair in the owner's party
state, then adds a layer, up to five extras. Callers fill and animate that layer
after their move/item animation. On break the shared restore script reveals the
saved pair without overkill carryover. Added layers can sit above configured boss
phases; those original phase scripts remain intact. HP and max-HP controller
writes are serialized. The overlay uses the covered layer's color when stacking.

Wish for Endurance uses this API. Break Charm also uses it, stays held and grants
once per party member per battle, including when passed between holders. It
activates before entry hazards. Lunar Dance keeps full HP/status/PP restoration,
then adds a saved layer even to a healthy recipient; at the cap restoration still
works. Its user empties the current bar: a protected user resolves that break and
switches, while an unprotected user faints normally. Guts-on-Break remains separate.
