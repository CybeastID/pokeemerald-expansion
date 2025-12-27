CastersTerrarium_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Terrarium_On_Frame

Terrarium_On_Frame:
	map_script_2 VAR_CASTER_REVEAL, 2, Terrarium_On_Frame_1

Terrarium_On_Frame_1::
	fadedefaultbgm
	fadescreenspeed FADE_FROM_BLACK, 4
	addvar VAR_CASTER_REVEAL, 1
	return