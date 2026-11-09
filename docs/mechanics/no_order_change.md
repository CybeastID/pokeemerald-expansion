# No Order Change

No Order Change prevents an affected Pokemon from leaving its active position.
It is stored independently of nonvolatile and volatile ailments in one existing
padding bit of `BattlerState`. `HasNoOrderChange(battler)` exposes the condition.
Arcanum Wish for Debilitation applies it alongside six nonvolatile ailments.

## Indicator

The FGO Order Change Seal icon appears below the affected Pokemon's healthbox,
independently of the ordinary ailment badge. It follows the panel's movement and
visibility in singles and doubles. Status cures and bar breaks leave it visible;
clearing No Order Change removes the sprite. Its trailing-corner placement is
staggered to clear the next healthbox in doubles. Healthbox reconstruction after menus
recreates it from battle state, and battle cleanup frees its tiles and palette.

The user supplied `bufficon_554.png`; the original is retained as
`graphics/battle_interface/no_order_change_source.png`. The game uses the reduced
12x12 artwork centered with two transparent pixels of padding on each side in
the 16x16, 16-color `no_order_change.png` in the same directory.

## Switching

- Manual switching is blocked even for Ghost types and Shed Shell holders.
- U-turn, Volt Switch and Flip Turn retain their damage but do not switch.
- Parting Shot retains its stat drops; Chilly Reception retains its weather.
- Baton Pass, Teleport and Shed Tail fail in the attack canceler.
- Roar and Whirlwind fail to remove an affected target. Dragon Tail and Circle
  Throw retain their damage but cannot remove it.
- Red Card checks the attacker it would remove, not the holder. Eject Button,
  Eject Pack, Emergency Exit and Wimp Out cannot withdraw an affected holder.
  Blocked switching items are not consumed.
- Ally Switch fails if either partner is affected.

The actual withdrawal paths enforce the condition, including paths that ignore
ordinary escape prevention. `CanBattlerSwitch` remains a party-availability check
so mandatory faint replacement is not blocked.

## Lifetime and sacrifices

Ordinary status cures, including Full Restore, do not clear No Order Change.
Breaking an extra HP bar also preserves it. Genuine faint cleanup and switch-in
clear it; allocation of battle state starts it at zero.

Lunar Dance fails in the attack canceler if the affected user has any remaining
extra bar or a pending break. On the final red bar, it can cause a genuine faint
and permit replacement normally.

Explosion, Self-Destruct and Memento remain usable. Healing Wish also retains its
sacrifice and stored healing behavior: if a bar absorbs the sacrifice, the user
remains trapped. Its older immediate party-screen path cannot withdraw that
survivor. Healing Wish does not inherit Lunar Dance's bar grant or early failure.
