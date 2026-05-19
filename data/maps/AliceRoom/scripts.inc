AliceRoom_MapScripts::
	.byte 0

AliceRoom_DebugMons_Proceed::
	callnative ZeroPlayerPartyMons
	giveitem ITEM_MEGA_RING, 1
	giveitem ITEM_TERA_ORB, 1
	setflag FLAG_SYS_POKEMON_GET
givemon SPECIES_AEGISLASH, 100, item=ITEM_IRON_PLATE, nature=NATURE_CAREFUL, hpEv=252, atkEv=4, spDefEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_IRON_HEAD, move2=MOVE_IRON_DEFENSE, move3=MOVE_SWORDS_DANCE, teraType=TYPE_STEEL
givemon SPECIES_ARMAROUGE, 100, item=ITEM_CHOICE_SPECS, nature=NATURE_MODEST, hpEv=4, defEv=252, spAtkEv=128, hpIv=31, atkIv=0, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_ARMOR_CANNON, teraType=TYPE_FIRE
givemon SPECIES_METAGROSS, 100, item=ITEM_METAGROSSITE, nature=NATURE_NAUGHTY, atkEv=120, speedEv=252, spDefEv=96, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_HYPER_BEAM, move2=MOVE_METEOR_MASH, teraType=TYPE_STEEL
givemon SPECIES_REGISTEEL, 100, item=ITEM_IRON_PLATE, nature=NATURE_BRAVE, gender=MON_GENDERLESS, hpEv=252, atkEv=252, spAtkEv=4, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_EXPLOSION, move2=MOVE_HEAVY_SLAM, move3=MOVE_ZAP_CANNON, move4=MOVE_AMNESIA, teraType=TYPE_STEEL
givemon SPECIES_DIALGA, 100, item=ITEM_ADAMANT_ORB, nature=NATURE_NAIVE, gender=MON_GENDERLESS, atkEv=4, spAtkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_ROAR_OF_TIME, move2=MOVE_DRAGON_TAIL, move3=MOVE_FLASH_CANNON, move4=MOVE_HEAVY_SLAM, teraType=TYPE_STEEL
givemon SPECIES_ARCHALUDON, 100, item=ITEM_LIFE_ORB, nature=NATURE_NAIVE, gender=MON_GENDERLESS, atkEv=252, spAtkEv=4, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_BODY_PRESS, move2=MOVE_EARTHQUAKE, move3=MOVE_AURA_SPHERE, move4=MOVE_IRON_HEAD, teraType=TYPE_STEEL
	giveitem ITEM_REVIVE, 5
	giveitem ITEM_MAX_REVIVE, 1
	giveitem ITEM_FULL_RESTORE, 3
	giveitem ITEM_MOOMOO_MILK, 4
	giveitem ITEM_SUPER_POTION, 10
	giveitem ITEM_HYPER_POTION, 3
	removeobject LOCALID_DEBUG_MONS
	msgbox LittlerootTown_Text_TeamDelivered, MSGBOX_DEFAULT
	releaseall
	end

AliceRoom_Alice_Fight::
	setrespawn HEAL_LOCATION_ALICE_ROOM
	setdynamicaifunc AI_AliceTrainer
	trainerbattle_single TRAINER_ALICE2, Text_Alice_Intro, Text_Alice_Defeated