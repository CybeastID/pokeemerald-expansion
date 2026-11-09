#include "global.h"
#include "test/battle.h"
#include "battle_interface.h"
#include "battle_status_icons.h"
#include "palette.h"
#include "sprite.h"

static void ActivateIcon(u32 battler, u32 icon)
{
    switch (icon)
    {
    case BATTLE_STATUS_ICON_GUTS:
        gBattleMons[battler].volatiles.kazuradropGuts = 1;
        break;
    case BATTLE_STATUS_ICON_INVINCIBLE:
        ShowInvincibleBattleStatusIcon(battler);
        break;
    case BATTLE_STATUS_ICON_MAGE_ARMOR:
        gBattleStruct->castoria.mageArmorPartyMask = 1u << gBattlerPartyIndexes[battler];
        break;
    case BATTLE_STATUS_ICON_NO_ORDER_CHANGE:
        gBattleStruct->battlerState[battler].noOrderChange = TRUE;
        break;
    }
    RefreshBattleStatusIcon(battler, icon);
}

SINGLE_BATTLE_TEST("Battle status icons follow healthbox slides and visibility without duplicate sprites")
{
    u32 icon;
    PARAMETRIZE { icon = BATTLE_STATUS_ICON_GUTS; }
    PARAMETRIZE { icon = BATTLE_STATUS_ICON_INVINCIBLE; }
    PARAMETRIZE { icon = BATTLE_STATUS_ICON_MAGE_ARMOR; }
    PARAMETRIZE { icon = BATTLE_STATUS_ICON_NO_ORDER_CHANGE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        ActivateIcon(battler, icon);
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, icon);
        EXPECT_LT(spriteId, MAX_SPRITES);
        if (spriteId < MAX_SPRITES)
        {
            struct Sprite *sprite = &gSprites[spriteId];
            u32 healthboxId = gHealthboxSpriteIds[battler];
            struct Sprite *healthbox = &gSprites[healthboxId];
            s16 offsetX = sprite->x - healthbox->x;
            s16 offsetY = sprite->y - healthbox->y;
            RefreshBattleStatusIcon(battler, icon);
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, icon), spriteId);
            healthbox->x += 7;
            healthbox->y -= 3;
            healthbox->x2 = 13;
            healthbox->y2 = -4;
            sprite->callback(sprite);
            EXPECT_EQ(sprite->x - healthbox->x, offsetX);
            EXPECT_EQ(sprite->y - healthbox->y, offsetY);
            EXPECT_EQ(sprite->x2, 13);
            EXPECT_EQ(sprite->y2, -4);
            healthbox->invisible = TRUE;
            sprite->callback(sprite);
            EXPECT_EQ((u32)sprite->invisible, TRUE);
            healthbox->invisible = FALSE;
            gHealthboxSpriteIds[battler] = MAX_SPRITES;
            sprite->callback(sprite);
            EXPECT_EQ((u32)sprite->invisible, TRUE);
            gHealthboxSpriteIds[battler] = healthboxId;
            // Rebuilding the UI refreshes positions without restarting the timer.
            ReshowBattleStatusIcons();
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, icon), spriteId);
        }
    }
}

SINGLE_BATTLE_TEST("Battle status icons share the buff palette and reload resources without clearing conditions")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        for (u32 icon = 0; icon < BATTLE_STATUS_ICON_COUNT; icon++)
        {
            ActivateIcon(battler, icon);
            u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, icon);
            EXPECT_LT(spriteId, MAX_SPRITES);
            if (spriteId < MAX_SPRITES)
            {
                u32 paletteTag = gSprites[spriteId].template->paletteTag;
                EXPECT_EQ(paletteTag, icon == BATTLE_STATUS_ICON_NO_ORDER_CHANGE
                    ? TAG_BATTLE_STATUS_NO_ORDER_CHANGE : TAG_BATTLE_STATUS_ICON_PAL);
            }
        }
        FreeBattleStatusIcons();
        EXPECT_EQ(IndexOfSpritePaletteTag(TAG_BATTLE_STATUS_ICON_PAL), 0xFF);
        EXPECT_EQ(IndexOfSpritePaletteTag(TAG_BATTLE_STATUS_NO_ORDER_CHANGE), 0xFF);
        EXPECT_EQ((u32)opponent->volatiles.kazuradropGuts, 1);
        EXPECT_NE(gBattleStruct->castoria.mageArmorPartyMask, 0);
        EXPECT_EQ((u32)gBattleStruct->battlerState[battler].noOrderChange, TRUE);
        // Match the menu lifecycle: reset the tracking, then restore from battle state.
        InitBattleStatusIcons();
        ReshowBattleStatusIcons();
        for (u32 icon = 0; icon < BATTLE_STATUS_ICON_COUNT; icon++)
        {
            EXPECT_LT(Test_GetBattleStatusIconSpriteId(battler, icon), MAX_SPRITES);
        }
        EXPECT_NE(IndexOfSpritePaletteTag(TAG_BATTLE_STATUS_ICON_PAL), 0xFF);
    }
}

SINGLE_BATTLE_TEST("Battle status icons expire Invincible after sixty frames regardless of healthbox refreshes")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        ShowInvincibleBattleStatusIcon(battler);
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_INVINCIBLE);
        EXPECT_LT(spriteId, MAX_SPRITES);
        if (spriteId < MAX_SPRITES)
        {
            for (u32 frame = 0; frame < 59; frame++)
            {
                RefreshBattlerStatusIcons(battler);
                RefreshBattlerStatusIcons(battler);
                gSprites[spriteId].callback(&gSprites[spriteId]);
            }
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_INVINCIBLE), spriteId);
            gSprites[spriteId].callback(&gSprites[spriteId]);
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_INVINCIBLE), MAX_SPRITES);
            EXPECT_EQ(gBattleStruct->bugSpace.invincibleActive & (1u << battler), 0);
            ReshowBattleStatusIcons();
            EXPECT_EQ(Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_INVINCIBLE), MAX_SPRITES);
        }
    }
}

SINGLE_BATTLE_TEST("Battle status icons do not destroy unrelated sprites that reuse their IDs")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        u32 battler = B_POSITION_OPPONENT_LEFT;
        ActivateIcon(battler, BATTLE_STATUS_ICON_GUTS);
        u32 spriteId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_GUTS);
        EXPECT_LT(spriteId, MAX_SPRITES);
        if (spriteId < MAX_SPRITES)
        {
            DestroySprite(&gSprites[spriteId]);
            u32 replacement = CreateSprite(&gDummySpriteTemplate, 0, 0, 0);
            EXPECT_EQ(replacement, spriteId);
            DestroyBattleStatusIcon(battler, BATTLE_STATUS_ICON_GUTS);
            if (replacement < MAX_SPRITES)
            {
                EXPECT_EQ((u32)gSprites[replacement].inUse, TRUE);
                RefreshBattleStatusIcon(battler, BATTLE_STATUS_ICON_GUTS);
                EXPECT_NE(Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_GUTS), replacement);
                FreeBattleStatusIcons();
                EXPECT_EQ((u32)gSprites[replacement].inUse, TRUE);
                DestroySprite(&gSprites[replacement]);
            }
        }
    }
}

DOUBLE_BATTLE_TEST("Battle status icons place No Order Change below each panel without clipping or overlapping the next panel")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_CELEBRATE); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
    } THEN {
        for (u32 battler = 0; battler < MAX_BATTLERS_COUNT; battler++)
        {
            ActivateIcon(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
            u32 iconId = Test_GetBattleStatusIconSpriteId(battler, BATTLE_STATUS_ICON_NO_ORDER_CHANGE);
            EXPECT_LT(iconId, MAX_SPRITES);
            if (iconId >= MAX_SPRITES)
                continue;
            struct Sprite *icon = &gSprites[iconId];
            struct Sprite *panel = &gSprites[gHealthboxSpriteIds[battler]];
            EXPECT_GE(icon->y - 8, panel->y + 16);
            EXPECT_GE(icon->x - 8, 0);
            EXPECT_LE(icon->x + 8, DISPLAY_WIDTH);
            EXPECT_LE(icon->y + 8, DISPLAY_HEIGHT);
        }
        struct Sprite *upperPlayer = &gSprites[Test_GetBattleStatusIconSpriteId(B_POSITION_PLAYER_LEFT, BATTLE_STATUS_ICON_NO_ORDER_CHANGE)];
        struct Sprite *lowerPlayerPanel = &gSprites[gHealthboxSpriteIds[B_POSITION_PLAYER_RIGHT]];
        EXPECT_LE(upperPlayer->x + 8, lowerPlayerPanel->x - 32);
        struct Sprite *upperOpponent = &gSprites[Test_GetBattleStatusIconSpriteId(B_POSITION_OPPONENT_LEFT, BATTLE_STATUS_ICON_NO_ORDER_CHANGE)];
        struct Sprite *lowerOpponentPanel = &gSprites[gHealthboxSpriteIds[B_POSITION_OPPONENT_RIGHT]];
        EXPECT_GE(upperOpponent->x - 8, lowerOpponentPanel->x + 96);
    }
}
