AliceRoom_MapScripts::
	.byte 0

AliceRoom_DebugMons_Proceed::
	callnative ZeroPlayerPartyMons
	giveitem ITEM_MEGA_RING, 1
	giveitem ITEM_TERA_ORB, 1
	giveitem ITEM_DYNAMAX_BAND, 1
	setflag FLAG_SYS_POKEMON_GET
givemon SPECIES_CHANDELURE, 100, item=ITEM_CHANDELURITE, nature=NATURE_TIMID, abilityNum=0, hpEv=4, speedEv=252, spAtkEv=252, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_CALM_MIND, move2=MOVE_FLAMETHROWER, move3=MOVE_SHADOW_BALL, move4=MOVE_WILL_O_WISP, teraType=TYPE_GRASS
givemon SPECIES_HYDRAPPLE, 100, item=ITEM_LEFTOVERS, nature=NATURE_MODEST, abilityNum=1, hpEv=248, spAtkEv=252, spDefEv=8, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_SUBSTITUTE, move2=MOVE_NASTY_PLOT, move3=MOVE_GIGA_DRAIN, move4=MOVE_DRACO_METEOR, teraType=TYPE_STEEL
givemon SPECIES_GALVANTULA, 100, item=ITEM_FOCUS_SASH, nature=NATURE_TIMID, abilityNum=0, hpEv=4, speedEv=252, spAtkEv=252, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_THUNDER_WAVE, move2=MOVE_STICKY_WEB, move3=MOVE_VOLT_SWITCH, move4=MOVE_THUNDERBOLT, teraType=TYPE_GHOST
givemon SPECIES_ALCREMIE, 100, item=ITEM_LEFTOVERS, nature=NATURE_MODEST, abilityNum=2, hpEv=252, spAtkEv=252, spDefEv=4, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_PROTECT, move2=MOVE_CALM_MIND, move3=MOVE_STORED_POWER, move4=MOVE_ALLURING_VOICE, teraType=TYPE_STEEL
givemon SPECIES_KARTANA, 100, item=ITEM_ASSAULT_VEST, nature=NATURE_JOLLY, abilityNum=0, gender=MON_GENDERLESS, atkEv=4, speedEv=252, spDefEv=252, move1=MOVE_KNOCK_OFF, move2=MOVE_LEAF_BLADE, move3=MOVE_SMART_STRIKE, move4=MOVE_SACRED_SWORD, teraType=TYPE_WATER
givemon SPECIES_ARCHEOPS, 100, item=ITEM_HEAVY_DUTY_BOOTS, nature=NATURE_JOLLY, abilityNum=0, atkEv=252, speedEv=252, spDefEv=4, move1=MOVE_EARTHQUAKE, move2=MOVE_KNOCK_OFF, move3=MOVE_DRAGON_CLAW, move4=MOVE_STONE_EDGE, teraType=TYPE_FIRE	
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