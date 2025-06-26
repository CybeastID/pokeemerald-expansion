MysteryZone_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OnTransition
	.byte 0

OnFrame::
	compare VAR_CASTER_REVEAL, 1
	goto_if_eq OnFrame_1
	return

OnFrame_1::
	playbgm MUS_ENCOUNTER_GIRL, FALSE
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
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	return


OnTransition_Text_0:
	.string "What a delightfully cute pet I've\l"
	.string "found! They'd look positively fetching\l"
	.string "in my terrarium!$"


CastersTrap_Text_0:
	.string "{COLOR LIGHT_GRAY} ??? {COLOR DARK_GRAY}used Teleport!$"

CasterTrap_2::
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	msgbox OnTransition_Text_0

