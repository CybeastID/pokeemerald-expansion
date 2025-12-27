MysteryZone_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, MysteryZone_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, MysteryZone_On_Frame
	.byte 0

MysteryZone_On_Frame:
	map_script_2 VAR_CASTER_REVEAL, 1, MysteryZone_OnFrame_1
	.2byte 0

MysteryZone_OnFrame_1::
	delay 10
	msgbox CutieTrap_Text MSGBOX_NPC
	callnative StartPlayerShrinkAnimation
	playbgm MUS_ENCOUNTER_SUSPICIOUS FALSE
	delay 70
	msgbox MysteryZone_OnTransition_Text_0 MSGBOX_NPC
	fadescreen FADE_TO_BLACK
	fadeoutbgm 1
	delay 50
	msgbox CutieTrap2_Text MSGBOX_NPC
	playse SE_FALL
	warpsilent MAP_CASTERS_TERRARIUM, 1
	waitstate
	return
Dbug1_Text:
	.string "Debug: Player should be falling now.$"

CutieTrap_Text:
	.string "What a delightfully cute pet I've\n"
	.string "found! They'd look positively fetching\l"
	.string "in my terrarium!$"
	
MysteryZone_OnTransition::
	compare VAR_CASTER_REVEAL, 0
	goto_if_eq MysteryZone_OnTransition_1
	compare VAR_CASTER_REVEAL, 1
	goto_if_eq MysteryZone_OnTransition_4
	return

MysteryZone_OnTransition_1:
	setflag FLAG_HIDES_CASTER
	return

MysteryZone_OnTransition_4:
	
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


MysteryZone_OnTransition_Text_0:
	.string "Teehee~! A little cutie like you...\n"
	.string "you'll never escape me! Well...in you go,\l"
	.string "my cute little pet Trainer!$"

CutieTrap2_Text:
	.string "The mysterious girl's hand closed around\n"
	.string "you! What's happening...?!$"


CastersTrap_Text_0:
	.string "{COLOR LIGHT_GRAY} ??? {COLOR DARK_GRAY}used Teleport!$"

