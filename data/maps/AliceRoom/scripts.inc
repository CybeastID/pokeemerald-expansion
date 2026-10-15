AliceRoom_MapScripts::
	.byte 0

AliceRoom_DebugMons_Proceed::
	callnative ZeroPlayerPartyMons
	giveitem ITEM_MEGA_RING, 1
	giveitem ITEM_TERA_ORB, 1
	giveitem ITEM_DYNAMAX_BAND, 1
	setflag FLAG_SYS_POKEMON_GET
	setflag B_FLAG_TERA_ORB_CHARGED
givemon SPECIES_EMPOLEON, 100, item=ITEM_CHOICE_SPECS, nature=NATURE_CALM, abilityNum=0, hpEv=52, defEv=96, spAtkEv=252, spDefEv=108, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_FLASH_CANNON, move2=MOVE_BLIZZARD, move3=MOVE_SURF, move4=MOVE_GRASS_KNOT
givemon SPECIES_ZOROARK_HISUI, 100, item=ITEM_FOCUS_SASH, nature=NATURE_NAIVE, abilityNum=0, speedEv=252, spAtkEv=252, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_NASTY_PLOT, move2=MOVE_FLAMETHROWER, move3=MOVE_SHADOW_BALL, move4=MOVE_FOCUS_BLAST
givemon SPECIES_SALAMENCE, 100, item=ITEM_FOCUS_SASH, nature=NATURE_RELAXED, abilityNum=0, hpEv=112, defEv=252, spDefEv=144, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_IRON_DEFENSE, move2=MOVE_ROOST, move3=MOVE_TOXIC, move4=MOVE_PROTECT
givemon SPECIES_BOMBIRDIER, 100, item=ITEM_ASSAULT_VEST, nature=NATURE_NAUGHTY, abilityNum=0, hpEv=4, atkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_BRAVE_BIRD, move2=MOVE_DRILL_RUN, move3=MOVE_ROCK_SLIDE, move4=MOVE_SUCKER_PUNCH
givemon SPECIES_RESHIRAM, 100, item=ITEM_LIFE_ORB, nature=NATURE_RASH, abilityNum=0, hpEv=4, atkEv=252, spAtkEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_BLUE_FLARE, move2=MOVE_DRAGON_DANCE, move3=MOVE_OUTRAGE, move4=MOVE_FOCUS_BLAST
givemon SPECIES_ZAPDOS_GALAR, 100, item=ITEM_ASSAULT_VEST, nature=NATURE_ADAMANT, abilityNum=0, hpEv=4, atkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_THUNDEROUS_KICK, move2=MOVE_U_TURN, move3=MOVE_DRILL_PECK, move4=MOVE_KNOCK_OFF
	additem ITEM_MAX_REVIVE, 1
	additem ITEM_FULL_RESTORE, 3
	additem ITEM_MOOMOO_MILK, 4
	additem ITEM_SUPER_POTION, 10
	additem ITEM_HYPER_POTION, 3
	removeobject LOCALID_DEBUG_MONS
	setflag FLAG_LR_DEBUG_TEAM
	msgbox LittlerootTown_Text_TeamDelivered, MSGBOX_DEFAULT
	msgbox AliceRoom_Healing_Items, MSGBOX_DEFAULT
	releaseall
	end

AliceRoom_Alice_Fight::
	lock
	faceplayer
	goto_if_set FLAG_LR_DEBUG_TEAM, AliceRoom_Alice_Fight_PackageTaken
	msgbox AliceRoom_Alice_Fight_NoPackageText, MSGBOX_DEFAULT
	releaseall
	end

AliceRoom_Alice_Fight_PackageTaken:
	setrespawn HEAL_LOCATION_ALICE_ROOM
	setdynamicaifunc AI_AliceTrainer
	trainerbattle_single TRAINER_ALICE2, Text_Alice_Intro, Text_Alice_Defeated
	clearflag FLAG_KAZURADROP_ENTERED_BATTLE
	releaseall
	end

AliceRoom_Alice_Fight_NoPackageText:
	.string "I like your gusto, but you\n"
	.string "should grab your Pokemon first!$"	

AliceRoom_Healing_Items:
	.string "You also got some healing\n"
	.string "items.$"