AliceRoom_MapScripts::
	.byte 0

AliceRoom_DebugMons_Proceed::
	callnative ZeroPlayerPartyMons
	giveitem ITEM_MEGA_RING, 1
	giveitem ITEM_TERA_ORB, 1
	setflag FLAG_SYS_POKEMON_GET
givemon SPECIES_PIDGEOT, 100, item=ITEM_CHOICE_SPECS, nature=NATURE_TIMID, abilityNum=1, speedEv=252, spAtkEv=252, hpEv=4, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_HURRICANE, move2=MOVE_HEAT_WAVE, move3=MOVE_U_TURN, move4=MOVE_SLEEP_TALK, teraType=TYPE_NORMAL
givemon SPECIES_CHARIZARD, 100, item=ITEM_CHOICE_SPECS, nature=NATURE_TIMID, abilityNum=0, hpEv=4, spAtkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_FIRE_BLAST, move2=MOVE_AIR_SLASH, move3=MOVE_FOCUS_BLAST, move4=MOVE_FLAMETHROWER, teraType=TYPE_FIRE
givemon SPECIES_RAICHU, 100, item=ITEM_LIFE_ORB, nature=NATURE_TIMID, abilityNum=1, hpEv=4, spAtkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_THUNDERBOLT, move2=MOVE_VOLT_SWITCH, move3=MOVE_FOCUS_BLAST, move4=MOVE_NASTY_PLOT, teraType=TYPE_ELECTRIC
givemon SPECIES_GYARADOS, 100, item=ITEM_LUM_BERRY, nature=NATURE_JOLLY, abilityNum=1, hpEv=4, atkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_DRAGON_DANCE, move2=MOVE_WATERFALL, move3=MOVE_CRUNCH, move4=MOVE_ICE_FANG, teraType=TYPE_WATER
givemon SPECIES_GOLEM, 100, item=ITEM_CHOICE_BAND, nature=NATURE_ADAMANT, abilityNum=0, hpEv=252, atkEv=252, spDefEv=4, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_STONE_EDGE, move2=MOVE_EARTHQUAKE, move3=MOVE_DOUBLE_EDGE, move4=MOVE_FIRE_PUNCH, teraType=TYPE_ROCK
givemon SPECIES_VICTREEBEL, 100, item=ITEM_BLACK_SLUDGE, nature=NATURE_MODEST, abilityNum=1, hpEv=4, spAtkEv=252, speedEv=252, hpIv=31, atkIv=31, defIv=31, speedIv=31, spAtkIv=31, spDefIv=31, move1=MOVE_LEAF_STORM, move2=MOVE_SLUDGE_BOMB, move3=MOVE_SLEEP_POWDER, move4=MOVE_WEATHER_BALL, teraType=TYPE_GRASS
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