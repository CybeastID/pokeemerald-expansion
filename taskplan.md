# Bug Fix Todo List

## 1. Fix Trash & Crush (OHKO move)
- [ ] Get OHKO working again (it's currently broken)
- [ ] Find and fix the infinite loop issue

## 2. Fix Infinite Growth
- [ ] Investigate why it's now dealing self-damage
- [ ] Restore proper functionality

## 3. Fix HP Animation at Transformation
- [ ] Currently only shows recovery to ~10% HP (incorrect)
- [ ] Visually stop damage at the 10% threshold — do not let it go beyond that point
- [ ] Ensure the animation properly reflects the intended HP behavior

## 4. Fix Melt Virus
- [ ] Make it properly drain HP from the target
- [ ] Make it properly heal Kazuradrop (currently only does damage, no healing)

## 5. Make It Pretty
- [ ] Proper text strings for the passive OHKO ("Kazuradrop purses her lips and exhales..."
(HP reduces to 0)
"Her breath blew your Pokemon away!"
(Faint call))
- [ ] Fix text strings for Infinite Growth
- [ ] Custom text string for Bug Space startup
- [ ] Strings on phase transitions
- [ ] Affine scale the opposing sprites as the threshold drops? (low priority)