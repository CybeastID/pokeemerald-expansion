IntermediateWarpMap_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Warp_Map_On_Frame
	.byte 0

Warp_Map_On_Frame:
	map_script_2 VAR_TEMP_0, 0, WarpMapOnFrame_1

WarpMapOnFrame_1::
	warpwhitefade MAP_CASTERS_TERRARIUM, 6, 5
	return
