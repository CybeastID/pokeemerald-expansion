#include "global.h"              // u8/u16/u32/s16/bool32, INCBIN_U8/U16, ALIGNED, MAX_SPRITES
#include "battle.h"              // gBattleMons, gBattleStruct, GetBattlerVisualSpecies, MAX_BATTLERS_COUNT
#include "battle_interface.h"    // gHealthboxSpriteIds, TAG_KAZU_ALTEREGO, TAG_CASTER_SIGIL
#include "servant_sigil.h"
#include "sprite.h"              // Sprite/SpriteSheet/SpritePalette/SpriteTemplate/OamData,
                                 // CreateSprite, DestroySprite, LoadSpriteSheet, LoadSpritePalette,
                                 // GetSpriteTileStartByTag, FreeSpriteTilesByTag, ST_OAM_SQUARE
#include "palette.h"             // IndexOfSpritePaletteTag, LoadSpritePalette, FreeSpritePaletteByTag
#include "graphics.h"            // gDummySpriteAnimTable, gDummySpriteAffineAnimTable
#include "constants/species.h"   // SPECIES_KAZURADROP, SPECIES_CASTORIA



// Sprite data-field aliases (sigil sprite — only this file touches them,
// so the slots are ours to assign).
#define tBattler     data[0]
#define tTargetX     data[1]
#define tState       data[2]
// Healthbox sprite's data[5] holds its health-bar child sprite ID.
// This one is NOT free — it must match the slot battle_interface.c uses.
#define hMain_HealthBarSpriteId data[5]


static const u8 ALIGNED(4) sKazuSigilGfx[] =
    INCBIN_U8("graphics/special/kazfightui/Class16.4bpp");
static const u16 sKazuSigilPal[] =
    INCBIN_U16("graphics/special/kazfightui/Class16.gbapal");
static const struct SpriteSheet sSpriteSheet_KazuSigil =
    { sKazuSigilGfx, sizeof(sKazuSigilGfx), TAG_KAZU_ALTEREGO };
static const struct SpritePalette sSpritePalette_KazuSigil =
    { sKazuSigilPal, TAG_KAZU_ALTEREGO_PAL };

    static const u8 ALIGNED(4) sCasterSigilGfx[] =
    INCBIN_U8("graphics/special/CasterUI/Caster16.4bpp");
static const u16 sCasterSigilPal[] =
    INCBIN_U16("graphics/special/CasterUI/Caster16.gbapal");
static const struct SpriteSheet sSpriteSheet_CasterSigil =
    { sCasterSigilGfx, sizeof(sCasterSigilGfx), TAG_CASTER_SIGIL };
static const struct SpritePalette sSpritePalette_CasterSigil =
    { sCasterSigilPal, TAG_CASTER_SIGIL_PAL };

    static u8 sSigilSpriteId[MAX_BATTLERS_COUNT];

static const struct OamData sOamData_Sigil16x16 =
{
    .shape    = ST_OAM_SQUARE,
    .size     = 1,
    .priority = 0,
};

#define POS_SIGIL_X_OFFSET (25)
#define POS_SIGIL_Y_OFFSET (-6)

bool32 HasServantSigil(u16 species)
{
    return species == SPECIES_KAZURADROP || species == SPECIES_CASTORIA;
}

static void SpriteCb_ServantSigil(struct Sprite *sprite)
{
    u32 battler = sprite->tBattler;
    if (!HasServantSigil(GetBattlerVisualSpecies(battler)))
    {
        sprite->invisible = TRUE;
        return;
    }
    u8 healthboxId = gHealthboxSpriteIds[battler];
    if (healthboxId == MAX_SPRITES) { sprite->invisible = TRUE; return; }
    u8 barId = gSprites[healthboxId].hMain_HealthBarSpriteId;
    if (barId == MAX_SPRITES) { sprite->invisible = TRUE; return; }
    sprite->x  = gSprites[healthboxId].x + sprite->tTargetX;
    sprite->y  = gSprites[barId].y + sprite->tState;
    sprite->x2 = gSprites[healthboxId].x2;
    sprite->y2 = gSprites[healthboxId].y2;
    sprite->invisible = gSprites[healthboxId].invisible;
}

static const struct SpriteTemplate sSpriteTemplate_KazuSigil =
{
    .tileTag     = TAG_KAZU_ALTEREGO,
    .paletteTag  = TAG_KAZU_ALTEREGO_PAL,
    .oam         = &sOamData_Sigil16x16,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_ServantSigil,
};
static const struct SpriteTemplate sSpriteTemplate_CasterSigil =
{
    .tileTag     = TAG_CASTER_SIGIL,
    .paletteTag  = TAG_CASTER_SIGIL_PAL,
    .oam         = &sOamData_Sigil16x16,
    .anims       = gDummySpriteAnimTable,
    .images      = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback    = SpriteCb_ServantSigil,
};

void CreateServantSigil(u32 battler)
{
    DebugPrintf("SIGIL ENTER b=%d species=%d", battler, GetBattlerVisualSpecies(battler));
    if (!HasServantSigil(GetBattlerVisualSpecies(battler)))
        return;
    if (sSigilSpriteId[battler] != MAX_SPRITES)
        return;

    bool32 isCaster = GetBattlerVisualSpecies(battler) == SPECIES_CASTORIA;
    u32 tileTag = isCaster ? TAG_CASTER_SIGIL     : TAG_KAZU_ALTEREGO;
    u32 palTag  = isCaster ? TAG_CASTER_SIGIL_PAL : TAG_KAZU_ALTEREGO_PAL;
    const struct SpriteTemplate *tmpl = isCaster ? &sSpriteTemplate_CasterSigil
                                                 : &sSpriteTemplate_KazuSigil;

    u8 healthboxId = gHealthboxSpriteIds[battler];
    if (healthboxId == MAX_SPRITES)
        return;
    u8 barId = gSprites[healthboxId].hMain_HealthBarSpriteId;
    if (barId == MAX_SPRITES)
        return;

    if (IndexOfSpritePaletteTag(palTag) == 0xFF)
        LoadSpritePalette(isCaster ? &sSpritePalette_CasterSigil : &sSpritePalette_KazuSigil);
    if (GetSpriteTileStartByTag(tileTag) == 0xFFFF)
        LoadSpriteSheet(isCaster ? &sSpriteSheet_CasterSigil : &sSpriteSheet_KazuSigil);

    s16 x = gSprites[healthboxId].x + POS_SIGIL_X_OFFSET;
    s16 y = gSprites[barId].y      + POS_SIGIL_Y_OFFSET;
    DebugPrintf("SIGIL create b=%d species=%d isCaster=%d tile=%04x tileStart=%04x pal=%04x",
    battler, GetBattlerVisualSpecies(battler), isCaster, tileTag,
    GetSpriteTileStartByTag(tileTag), palTag);
    u32 spriteId = CreateSprite(tmpl, x, y, 0);
    DebugPrintf("SIGIL created spriteId=%d", spriteId);
    if (spriteId == MAX_SPRITES)
        return;
    gSprites[spriteId].tBattler = battler;
    gSprites[spriteId].tTargetX = POS_SIGIL_X_OFFSET;
    gSprites[spriteId].tState   = POS_SIGIL_Y_OFFSET;
    gSprites[spriteId].x2 = gSprites[healthboxId].x2;
    gSprites[spriteId].y2 = gSprites[healthboxId].y2;
    sSigilSpriteId[battler] = spriteId;
}

void DestroyServantSigil(u32 battler)
{
    u8 spriteId = sSigilSpriteId[battler];
    if (spriteId != MAX_SPRITES)
    {
        DestroySprite(&gSprites[spriteId]);
        sSigilSpriteId[battler] = MAX_SPRITES;
    }
}

void DestroyAllServantSigils(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        DestroyServantSigil(battler);
}

void ReshowServantSigils(void)
{
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        sSigilSpriteId[battler] = MAX_SPRITES;
    for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        CreateServantSigil(battler);
}

void InitServantSigils(void)
{
    for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
        sSigilSpriteId[i] = MAX_SPRITES;
}

void FreeServantSigilGfx(void)
{
    FreeSpritePaletteByTag(TAG_CASTER_SIGIL_PAL);
    FreeSpriteTilesByTag(TAG_CASTER_SIGIL);
    FreeSpritePaletteByTag(TAG_KAZU_ALTEREGO_PAL);
    FreeSpriteTilesByTag(TAG_KAZU_ALTEREGO);
}
