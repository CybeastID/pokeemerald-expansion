MysteryZone_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, On_Frame
	.byte 0

On_Frame:
	map_script_2 VAR_CASTER_REVEAL, 1, OnFrame_1
	.2byte 0

OnFrame_1::
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	msgbox OnTransition_Text_0
	waitmessage
	warpwhitefade MAP_CASTERS_TERRARIUM, 6, 5
	addvar VAR_CASTER_REVEAL, 1
	return

	
OnTransition::
	compare VAR_CASTER_REVEAL, 0
	goto_if_eq OnTransition_1
	compare VAR_CASTER_REVEAL, 1
	goto_if_eq OnTransition_4
	return

OnTransition_1:
	setflag FLAG_HIDES_CASTER
	return

OnTransition_4:
	
	return


CastersTrap::
	msgbox CastersTrap_Text_0
	waitmessage
	closemessage
	setvar VAR_CASTER_REVEAL, 1
	clearflag FLAG_HIDES_CASTER
	warpteleport MAP_MYSTERY_ZONE, 2
	fadescreen FADE_FROM_BLACK
	return


OnTransition_Text_0:
	.string "What a delightfully cute pet I've\n"
	.string "found! They'd look positively fetching\l"
	.string "in my terrarium! Well...in you go, my\n"
	.string "cute little humant!$"


CastersTrap_Text_0:
	.string "{COLOR LIGHT_GRAY} ??? {COLOR DARK_GRAY}used Teleport!$"

