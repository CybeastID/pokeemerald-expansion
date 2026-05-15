
//{{BLOCK(gameover)

//======================================================================
//
//	gameover, 256x256@4, 
//	+ palette 16 entries, not compressed
//	+ 103 tiles (t|f reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 32 + 3296 + 2048 = 5376
//
//	Time-stamp: 2026-05-14, 21:32:14
//	Exported by Cearn's GBA Image Transmogrifier, v0.9.2
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_GAMEOVER_H
#define GRIT_GAMEOVER_H

#define gameoverTilesLen 3296
extern const unsigned int gameoverTiles[824];

#define gameoverMapLen 2048
extern const unsigned short gameoverMap[1024];

#define gameoverPalLen 32
extern const unsigned short gameoverPal[16];

#endif // GRIT_GAMEOVER_H

//}}BLOCK(gameover)
