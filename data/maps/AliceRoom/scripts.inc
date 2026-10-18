AliceRoom_MapScripts::
	.byte 0

AliceRoom_DebugMons_Proceed::
	callnative ZeroPlayerPartyMons
	giveitem ITEM_MEGA_RING, 1
	giveitem ITEM_TERA_ORB, 1
	giveitem ITEM_DYNAMAX_BAND, 1
	setflag FLAG_SYS_POKEMON_GET
	setflag B_FLAG_TERA_ORB_CHARGED
givemon SPECIES_XERNEAS, 100, item=ITEM_POWER_HERB, nature=NATURE_MODEST, abilityNum=0, hpEv=248, spAtkEv=252, spDefEv=8, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_MOONBLAST, move2=MOVE_GEOMANCY, move3=MOVE_PSYCHIC, move4=MOVE_THUNDERBOLT
givemon SPECIES_ANNIHILAPE, 100, item=ITEM_EXPERT_BELT, nature=NATURE_ADAMANT, abilityNum=2, hpEv=248, atkEv=252, spDefEv=8, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_RAGE_FIST, move2=MOVE_BULK_UP, move3=MOVE_DRAIN_PUNCH, move4=MOVE_POISON_JAB
givemon SPECIES_ARCHALUDON, 100, item=ITEM_ASSAULT_VEST, abilityNum=0, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_FLASH_CANNON, move2=MOVE_THUNDERBOLT, move3=MOVE_DRAGON_PULSE, move4=MOVE_BODY_PRESS
givemon SPECIES_LANDORUS_THERIAN, 100, item=ITEM_CHOICE_SCARF, nature=NATURE_JOLLY, abilityNum=0, gender=MON_MALE, atkEv=252, speedEv=252, spDefEv=4, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_EARTHQUAKE, move2=MOVE_U_TURN, move3=MOVE_BRICK_BREAK, move4=MOVE_ROCK_SLIDE
givemon SPECIES_CALYREX_ICE, 100, item=ITEM_LEFTOVERS, nature=NATURE_ADAMANT, abilityNum=0, hpEv=252, atkEv=252, spDefEv=4, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_GLACIAL_LANCE, move2=MOVE_SWORDS_DANCE, move3=MOVE_ZEN_HEADBUTT, move4=MOVE_HIGH_HORSEPOWER
givemon SPECIES_MAGEARNA_ORIGINAL, 100, item=ITEM_AIR_BALLOON, nature=NATURE_MODEST, abilityNum=0, hpEv=252, spAtkEv=252, spDefEv=4, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_DAZZLING_GLEAM, move2=MOVE_FLASH_CANNON, move3=MOVE_PSYCHIC, move4=MOVE_ICE_BEAM
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
	