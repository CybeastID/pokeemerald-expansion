.set LOCALID_RAZELUXE, 1

Test_MapScripts::
	.byte 0

Test_EventScript_Razeluxe::
	lock
	faceplayer
	msgbox Razeluxe_Text, MSGBOX_DEFAULT
	closemessage
	release
	end

Razeluxe_Text:
	.string "This is a test.\n"
	.string "How did you get here?$"
	
Test_EventScript_RazeluxeBattle::
	trainerbattle_single TRAINER_RAZELUXE, Test_Text_RazeluxeIntro, Test_Text_RazeluxeDefeated
	msgbox Test_Text_RazeluxePostBattle, MSGBOX_AUTOCLOSE
	end