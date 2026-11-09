#ifndef GUARD_KAZURADROP_BATTLE_UI_H
#define GUARD_KAZURADROP_BATTLE_UI_H

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
// BS_callable wrappers
// ---------------------------------------------------------------

void BS_BugSpacePanelSlideIn(void);
void BS_BugSpacePanelSlideOut(void);
void BS_ShowKazuraDialogueBox(void);
void BS_RestoreTextbox(void);
void BS_KazuraGetLegendTier(void);
void BS_KazuraOmniboost(void);

// ---------------------------------------------------------------
// Initialization and cleanup
// ---------------------------------------------------------------

void InitKazuradropBattleUI(void);
void FreeKazuradropBattleUI(void);

// Called by reshow_battle_screen.c after menu open/return wipes sprites.
// Recreates the Bug Space panel and servant sigils.
void ReshowKazuradropBattleUI(void);

// ---------------------------------------------------------------
// Kazuradrop dialogue box (Moon Cell styled message window)
// ---------------------------------------------------------------

// Show her styled dialogue box with the given string, auto-dismisses after delay.
// Call from BattleScript via BS_ShowKazuraDialogue.
void ShowKazuraDialogueBox(const u8 *str);
void HideKazuraDialogueBox(void);




#endif // GUARD_KAZURADROP_BATTLE_UI_H
