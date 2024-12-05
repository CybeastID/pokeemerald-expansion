MysteryZone_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OnTransition
	.byte 0


OnTransition::
	compare VAR_CASTER_REVEAL, 0
	goto_if_eq OnTransition_1
	return

OnTransition_1:
	setflag FLAG_HIDES_CASTER
	return


CastersTrap::
	msgbox Casters_Text_0
	waitmessage
	closemessage
	setvar VAR_CASTER_REVEAL, 1
	clearflag FLAG_HIDES_CASTER
	warpteleport MAP_MYSTERY_ZONE, 2
	return


Casters_Text_0:
	.string "{COLOR LIGHT_GRAY} ??? {COLOR DARK_GRAY}used Teleport!$"
