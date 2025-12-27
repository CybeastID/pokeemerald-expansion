CastersTerrarium_MapScripts::
	.byte 0

Terrarium_On_Frame:
	map_script_2 VAR_CASTER_REVEAL, 2, Terrarium_On_Frame_1

Terrarium_On_Frame_1::
	fadescreenspeed FADE_FROM_BLACK, 4
	addvar VAR_CASTER_REVEAL, 1
	return

Terrarium_On_Transition:
	setrespawn HEAL_LOCATION_CASTERS_TERRARIUM



Terrarium_Nurse_EventScript::
	compare VAR_CASTER_REVEAL, 1
	call_if_eq Terrarium_Nurse_EventScript_FirstVisit
	setvar VAR_0x800B, LOCALID_CASTERS_NURSE
	goto EventScript_TerrariumPkmnCenterNurse
	waitmessage
	release
	end

Terrarium_Nurse_EventScript_FirstVisit::
	msgbox CastersTerrarium_Text_WelcomeToCastersTerrarium, MSGBOX_NPC
	addvar VAR_CASTER_REVEAL, 1
	return

CastersTerrarium_Text_WelcomeToCastersTerrarium:
	.string "Another newbie...? That girl...\n"
	.string "Well...welcome to the Pokemon Center.\p"
	.string "I'm the Nurse Joy she scooped up to helm\n" 
	.string "the place.$"

EventScript_TerrariumPkmnCenterNurse::
	lock
	faceplayer
	setvar VAR_0x8004, 0
	msgbox Terrarium_WouldYouLikeToRestYourPkmn, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, TerrariumEventScript_PkmnCenterNurse_HealPkmn
	goto_if_eq VAR_RESULT, NO, TerrariumEventScript_PkmnCenterNurse_Goodbye
	end

Terrarium_WouldYouLikeToRestYourPkmn:
	.string "It never hurts to be careful...\n"
	.string "Would you like me to heal your POKéMON?$"

TerrariumEventScript_PkmnCenterNurse_HealPkmn::
	incrementgamestat GAME_STAT_USED_POKECENTER
	waitmessage
	call EventScript_PkmnCenterNurse_TakeAndHealPkmn
	msgbox Terrarium_YourPkmnAreHealed 
	waitmessage
	release
	end

TerrariumEventScript_PkmnCenterNurse_Goodbye::
	msgbox Terrarium_VisitUsAgain, MSGBOX_NPC
	release
	end

Terrarium_VisitUsAgain:
	.string "I hope to see you again...\n"
	.string "And if I don't...\p"
	.string "Well. I'll just assume you managed to\n"
	.string "leave this 'fairy wonderland'.$"

Terrarium_YourPkmnAreHealed:
	.string "Your POKéMON are fully healed!\n"
	.string "...I miss my old job at the POKéMON\l"
	.string "CENTER...$"