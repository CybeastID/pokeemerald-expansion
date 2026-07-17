#ifndef GUARD_KAZURADROP_BATTLE_UI_H
#define GUARD_KAZURADROP_BATTLE_UI_H

// Icon types for Kazuradrop's HP bar
enum KazuradropBuffIconType
{
    KA_BUFF_GUTS = 0,     // Guts (cheat death) active on Kazuradrop
    KA_BUFF_INVINCIBLE,   // Invincibility from BugSpace vs Explosion
};

// ---------------------------------------------------------------
// Bug Space threshold panel (number display, slides in/out)
// ---------------------------------------------------------------

// Call on Kazuradrop sendout — creates panel and slides it in
void CreateBugSpacePanel(u32 battler);

// Call whenever the threshold value changes (end-of-turn decay, etc.)
void RefreshBugSpaceNumber(u32 battler);

// Call when a move is blocked by Bug Space — flashes the number
void TriggerBugSpaceNumberFlash(u32 battler);

// Call at transformation — panel slides out
void TriggerBugSpacePanelTransformSlide(u32 battler);

// Call on faint / battle end
void DestroyBugSpacePanel(u32 battler);

// Call at transformation — panel slides back in
void TriggerBugSpacePanelSlideIn(u32 battler);

// ---------------------------------------------------------------
// Buff icons (pop in/out, displayed right of the panel)
// ---------------------------------------------------------------

void CreateKazuradropBuffIcon(u32 battler, u32 iconType);
void DestroyKazuradropBuffIcon(u32 battler, u32 iconType);
void DestroyAllKazuradropBuffIcons(void);

// ---------------------------------------------------------------
// BS_callable wrappers
// ---------------------------------------------------------------

void BS_ShowKazuGutsIcon(void);
void BS_ShowKazuInvincibleIcon(void);
void BS_BugSpacePanelSlideIn(void);
void BS_BugSpacePanelSlideOut(void);
void BS_ShowKazuraDialogueBox(void);
void BS_RestoreTextbox(void);
// ---------------------------------------------------------------
// Initialization and cleanup
// ---------------------------------------------------------------

void InitKazuradropBattleIcons(void);
void FreeKazuradropBattleIconGfx(void);

// Called by reshow_battle_screen.c after menu open/return wipes sprites.
// Recreates the Bug Space panel + Kazuradrop icons based on gBattleStruct->bugSpace flags.
void ReshowKazuradropBattleUI(void);

// ---------------------------------------------------------------
// Kazuradrop dialogue box (Moon Cell styled message window)
// ---------------------------------------------------------------

// Show her styled dialogue box with the given string, auto-dismisses after delay.
// Call from BattleScript via BS_ShowKazuraDialogue.
void ShowKazuraDialogueBox(const u8 *str);
void HideKazuraDialogueBox(void);


#endif // GUARD_KAZURADROP_BATTLE_UI_H
