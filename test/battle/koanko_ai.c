#include "global.h"
#include "test/battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_break_bar.h"
#include "battle_koanko.h"
#include "battle_koanko_smeargle_diagnostics.h"
#include "battle_koanko_smeargle.h"
#include "battle_gimmick.h"
#include "data.h"
#include "pokemon.h"
#include "malloc.h"
#include "battle_tx_rollback.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "constants/battle_move_effects.h"

extern AiScoreFunc sDynamicAiFunc;

// Read authored production kits/levels/abilities/items/natures/EVs, not a
// separately maintained approximation. Controlled player stats set the scenario.
static void OriginalFixture(u32 index)
{
    const struct TrainerMon *mon = &GetTrainerPartyFromId(TRAINER_KOANKO)[index];
    AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
    OPPONENT(mon->species) {
        struct Pokemon *pokemon = gBattleTestRunnerState->data.currentMon;
        // No untagged rejection sampling in a test fixture. Nature = PID % 25.
        CreateMon(pokemon, mon->species, mon->lvl, 31, TRUE, mon->nature, OT_ID_PRESET, 0);
        static const u8 fields[] = {MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV,
            MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV};
        for (u32 i = 0; i < ARRAY_COUNT(fields); i++)
            SetMonData(pokemon, fields[i], &mon->ev[i]);
        CalculateMonStats(pokemon);
        Nature(mon->nature); Ability(mon->ability); Item(mon->heldItem);
        Moves(mon->moves[0], mon->moves[1], mon->moves[2], mon->moves[3]);
        if (mon->shouldUseDynamax)
        {
            DynamaxLevel(mon->dynamaxLevel);
            GigantamaxFactor(mon->gigantamaxFactor);
        }
        Speed(GetMonData(pokemon, MON_DATA_SPEED));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original all six take a secure KO over unnecessary utility")
{
    u32 index;
    PARAMETRIZE { index = 0; }
    PARAMETRIZE { index = 1; }
    PARAMETRIZE { index = 2; }
    PARAMETRIZE { index = 3; }
    PARAMETRIZE { index = 4; }
    PARAMETRIZE { index = 5; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_MACHAMP) { HP(1); Speed(1); Ability(ABILITY_NO_GUARD); Moves(MOVE_CELEBRATE); }
        OriginalFixture(index);
    } WHEN {
        if (index == 0) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_MOONBLAST); } }
        if (index == 1) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVES(opponent, MOVE_DAZZLING_GLEAM, MOVE_PSYCHIC, MOVE_MYSTICAL_FIRE); } }
        if (index == 2) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVES(opponent, MOVE_BLIZZARD, MOVE_FREEZE_DRY, MOVE_SHADOW_BALL); } }
        if (index == 3) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVES(opponent, MOVE_BLIZZARD, MOVE_FREEZE_DRY, MOVE_FLASH_CANNON); } }
        if (index == 4) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_KNOCK_OFF); } }
        if (index == 5) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_SEISMIC_TOSS); } }
    } SCENE {
        HP_BAR(player, hp: 0);
        MESSAGE("Machamp fainted!");
    } THEN {
        EXPECT_EQ(gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT], GetTrainerAIFlagsFromId(TRAINER_KOANKO) | AI_FLAG_DYNAMIC_FUNC);
        EXPECT_EQ(gBattleHistory->itemsNo, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Whimsicott avoids Prankster Dark and Grass immunity")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_SHIFTRY) { HP(1); Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(0);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_MOONBLAST); SCORE_GT(opponent, MOVE_MOONBLAST, MOVE_TAUNT); SCORE_GT(opponent, MOVE_MOONBLAST, MOVE_LEECH_SEED); }
    } SCENE { HP_BAR(player, hp: 0); }
}

// Start an actual production conjured party at a fresh-decision boundary.
// This is state setup, not a test scorer. Selection runs the registered real
// callback and ordinary flags. No flags are silently added to the trainer.
static void PrepareConjured(u32 index)
{
    gBattleStruct->koankoPhase = KOANKO_CONJURING;
    ConjureKoankoTeam();
    gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT] = index;
    PokemonToBattleMon(&gEnemyParty[index], &gBattleMons[B_POSITION_OPPONENT_LEFT]);
    KoankoOnSwitchIn(B_POSITION_OPPONENT_LEFT);
    KoankoSmeargleDiagDisable();
    // TESTING's intended-gimmick accessor replaces the production party mask.
    // Mirror the actual factory mask, without requesting activation or use.
    if (gBattleStruct->opponentMonCanTera & (1u << index))
        gBattleTestRunnerState->data.chosenGimmick[B_SIDE_OPPONENT][index] = GIMMICK_TERA;
    AssignUsableGimmicks();
    SetAiLogicDataForTurn(gAiLogicData);
    BattleAI_SetupAIData(0xF, B_POSITION_OPPONENT_LEFT);
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Alcremie Glaceon and Vanilluxe avoid failed support")
{
    u32 index;
    PARAMETRIZE { index = 1; }
    PARAMETRIZE { index = 2; }
    PARAMETRIZE { index = 3; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Ability(ABILITY_NO_GUARD); Status1(STATUS1_PARALYSIS); Moves(MOVE_TACKLE); }
        OriginalFixture(index);
    } WHEN {
        if (index == 1) { TURN { MOVE(player, MOVE_TACKLE); EXPECT_MOVES(opponent, MOVE_DAZZLING_GLEAM, MOVE_MYSTICAL_FIRE, MOVE_PSYCHIC); } }
        if (index == 2) { TURN { MOVE(player, MOVE_TACKLE); EXPECT_MOVES(opponent, MOVE_BLIZZARD, MOVE_FREEZE_DRY, MOVE_SHADOW_BALL); } }
        if (index == 3) { TURN { MOVE(player, MOVE_TACKLE); EXPECT_MOVES(opponent, MOVE_BLIZZARD, MOVE_FREEZE_DRY, MOVE_FLASH_CANNON); } }
    } SCENE { HP_BAR(player); }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Audino removes an item instead of failed status or Encore")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_STEELIX) { HP(1000); Speed(1); Item(ITEM_LEFTOVERS); Moves(MOVE_CELEBRATE); }
        OriginalFixture(4);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_KNOCK_OFF); } }
    SCENE { HP_BAR(player); MESSAGE("The opposing Audino knocked off Steelix's Leftovers!"); }
    THEN { EXPECT_EQ(player->item, ITEM_NONE); }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Blissey heals for survival instead of redundant hazards")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_STEELIX) { HP(1000); Speed(1); Moves(MOVE_STEALTH_ROCK); }
        OriginalFixture(5);
        SetMonData(&OPPONENT_PARTY[0], MON_DATA_HP, &(u16){100});
    } WHEN {
        TURN { MOVE(player, MOVE_STEALTH_ROCK); }
        TURN { MOVE(player, MOVE_STEALTH_ROCK); }
    } THEN {
        // Production recovery decisions use tagged RNG; observe useful recovery
        // over a short sequence without imposing a universal fixed opener.
        EXPECT_GT(opponent->hp, 100);
        opponent->hp = 100;
        PushHazardTypeToQueue(B_SIDE_PLAYER, HAZARDS_STEALTH_ROCK);
        SetAiLogicDataForTurn(gAiLogicData);
        BattleAI_SetupAIData(0xF, 1);
        EXPECT_EQ(opponent->moves[BattleAI_ChooseMoveIndex(1)], MOVE_SOFT_BOILED);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Alcremie chooses real G-Max Finale and heals instead of Max Guard")
{
    s16 healing;
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_GUZZLORD) { HP(2000); Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(1);
        SetMonData(&OPPONENT_PARTY[0], MON_DATA_HP, &(u16){100});
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_DAZZLING_GLEAM, gimmick: GIMMICK_DYNAMAX); } }
    SCENE {
        MESSAGE("The opposing Alcremie used G-Max Finale!");
        HP_BAR(player);
        HP_BAR(opponent, captureDamage: &healing);
    } THEN {
        EXPECT_LT(healing, 0);
        EXPECT_EQ(GetActiveGimmick(1), GIMMICK_DYNAMAX);
        EXPECT_EQ(gBattleStruct->gimmick.activated[1][GIMMICK_DYNAMAX], TRUE);
        EXPECT(!CanActivateGimmick(1, GIMMICK_DYNAMAX));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Glaceon Freeze-Dry selects coverage and matches actual damage")
{
    s16 damage;
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_KINGDRA) { HP(1000); Speed(1); Status1(STATUS1_PARALYSIS); Moves(MOVE_CELEBRATE); }
        OriginalFixture(2);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_FREEZE_DRY, criticalHit: FALSE); } }
    SCENE { ANIMATION(ANIM_TYPE_MOVE, MOVE_FREEZE_DRY, opponent); HP_BAR(player, captureDamage: &damage); }
    THEN {
        EXPECT_EQ(gAiLogicData->effectiveness[1][0][1], Q_4_12(4.0));
        EXPECT_GE(damage, gAiLogicData->simulatedDmg[1][0][1].minimum);
        EXPECT_LE(damage, gAiLogicData->simulatedDmg[1][0][1].maximum);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Original Audino avoids recasting its pending Wish")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_STEELIX) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE, MOVE_EARTHQUAKE); }
        OriginalFixture(4);
        SetMonData(&OPPONENT_PARTY[0], MON_DATA_HP, &(u16){100});
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_WISH); }
        TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVES(opponent, MOVE_KNOCK_OFF, MOVE_ENCORE); }
    } SCENE {
        MESSAGE("The opposing Audino used Wish!");
        NOT MESSAGE("The opposing Audino used Wish!");
        HP_BAR(opponent);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI TX ordinary Wish cannot credit unreachable injured reserves")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(1);
        EXPECT(TxRollbackHasWishes(1)); // Entry registers the solo combat profile.
        for (u32 i = 0; i < PARTY_SIZE; i++)
            if (i != 1)
                SetMonData(&gEnemyParty[i], MON_DATA_HP, &(u16){1});
        EXPECT(!ShouldUseWishAromatherapy(1, 0, MOVE_WISH));
        SetMonData(&gEnemyParty[1], MON_DATA_HP, &(u16){100});
        opponent->hp = 100;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT(ShouldUseWishAromatherapy(1, 0, MOVE_WISH));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Eternatus Arceus ordinary Recover values current actual max HP")
{
    u32 index;
    PARAMETRIZE { index = 3; }
    PARAMETRIZE { index = 5; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(index);
        opponent->hp = opponent->maxHP / 5;
        u32 hp = opponent->hp;
        SetAiLogicDataForTurn(gAiLogicData);
        BattleAI_SetupAIData(0xF, 1);
        EXPECT_EQ(opponent->moves[BattleAI_ChooseMoveIndex(1)], MOVE_RECOVER);
        EXPECT_EQ(opponent->hp, hp);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Cache conjured identities estimates and eligibility refresh without consumption")
{
    u32 index;
    PARAMETRIZE { index = 1; }
    PARAMETRIZE { index = 2; }
    PARAMETRIZE { index = 3; }
    PARAMETRIZE { index = 4; }
    PARAMETRIZE { index = 5; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(index);
        struct BattlePokemon before = *opponent;
        struct BattleGimmickData gimmick = gBattleStruct->gimmick;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            if (opponent->moves[slot] == MOVE_NONE)
                continue;
            uq4_12_t effectiveness;
            struct SimulatedDamage fresh = AI_CalcDamage(opponent->moves[slot], 1, 0, &effectiveness, USE_GIMMICK, NO_GIMMICK, AI_GetWeather());
            EXPECT_EQ(gAiLogicData->simulatedDmg[1][0][slot].median, fresh.median);
            EXPECT_EQ(gAiLogicData->effectiveness[1][0][slot], effectiveness);
        }
        EXPECT_EQ(memcmp(&before, opponent, sizeof(before)), 0);
        EXPECT_EQ(memcmp(&gimmick, &gBattleStruct->gimmick, sizeof(gimmick)), 0);
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_CONJURED_TEAM);
        EXPECT(gAiThinkingStruct->aiFlags[1] & AI_FLAG_SEQUENCE_SWITCHING);
        EXPECT(gAiThinkingStruct->aiFlags[1] & AI_FLAG_DYNAMIC_FUNC);
        EXPECT_EQ(gBattleStruct->gimmick.activated[1][GIMMICK_TERA], FALSE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Calyrex Eternatus Dialga Arceus avoid immune or failed options")
{
    u32 index;
    PARAMETRIZE { index = 2; }
    PARAMETRIZE { index = 3; }
    PARAMETRIZE { index = 4; }
    PARAMETRIZE { index = 5; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_TOGEKISS) { HP(1); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(index);
        opponent->statStages[STAT_ATK] = MAX_STAT_STAGE;
        SetAiLogicDataForTurn(gAiLogicData);
        BattleAI_SetupAIData(0xF, 1);
        u32 slot = BattleAI_ChooseMoveIndex(1);
        EXPECT_LT(slot, MAX_MON_MOVES);
        u32 move = opponent->moves[slot];
        EXPECT_NE(move, MOVE_SWORDS_DANCE);
        EXPECT_NE(move, MOVE_RECOVER);
        EXPECT_NE(move, MOVE_DYNAMAX_CANNON);
        EXPECT_NE(move, MOVE_ROAR_OF_ETERNITY);
        EXPECT_GT(gAiLogicData->simulatedDmg[1][0][slot].median, 0);
    }
}

static const struct BreakBarConfig sLayer = {.extraBars = 1, .flags = BREAK_BAR_DEFER_INITIAL_DRAW};
static const struct BreakBarConfig sTransformedLayer = {.extraBars = 1, .flags = BREAK_BAR_IGNORE_WHILE_TRANSFORMED};

// Pure outcome query fixture: no UI or execution is simulated here. Existing
// Break Bar battle regressions cover actual layer refill and hit truncation.
TEST("Koanko AI Resources distinguishes KO original break saved reveal and survival")
{
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gAiLogicData = AllocZeroed(sizeof(*gAiLogicData));
    gAiThinkingStruct = AllocZeroed(sizeof(*gAiThinkingStruct));
    gBattlersCount = 2;
    gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;
    gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;
    gBattlerPartyIndexes[0] = gBattlerPartyIndexes[1] = 0;
    gAbsentBattlerFlags = 0;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattleMons[0].species = SPECIES_WOBBUFFET;
    gBattleMons[0].hp = 10;
    gBattleMons[0].maxHP = 100;
    gBattleMons[1].moves[0] = MOVE_SEISMIC_TOSS;
    gAiLogicData->simulatedDmg[1][0][0].median = 100;
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_SEISMIC_TOSS, 100), AI_DAMAGE_KO);
    EXPECT(ActivateBreakBar(0, &sLayer));
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_SEISMIC_TOSS, 100), AI_DAMAGE_BREAK_LAYER);
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_DOUBLE_HIT, 1000), AI_DAMAGE_BREAK_LAYER);
    EXPECT_EQ(AI_GetReachableDamage(0, 1000), 10);
    EXPECT_EQ(AI_GetReachableDamage(0, -1), 0);
    EXPECT(!CanIndexMoveFaintTarget(1, 0, 0, AI_ATTACKING));
    EXPECT(!CanAIFaintTarget(1, 0, 2));
    struct PartyState *owner = GetBattlerPartyState(0);
    owner->breakBarSavedHp[0] = 37;
    owner->breakBarSavedMaxHp[0] = 100;
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_SEISMIC_TOSS, 100), AI_DAMAGE_REVEAL_SAVED_LAYER);
    EXPECT_EQ(owner->breakBarSavedHp[0], 37);
    owner->breakBarConfig = &sTransformedLayer;
    gBattleMons[0].volatiles.transformed = TRUE;
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_SEISMIC_TOSS, 100), AI_DAMAGE_KO);
    owner->breakBarsRemaining = 0;
    gBattleMons[0].hp = gBattleMons[0].maxHP;
    gAiLogicData->hpPercents[0] = 100;
    gAiLogicData->holdEffects[0] = HOLD_EFFECT_FOCUS_SASH;
    EXPECT_EQ(AI_GetDamageOutcome(1, 0, MOVE_SEISMIC_TOSS, 1000), AI_DAMAGE_SURVIVAL);
    Free(gAiThinkingStruct);
    Free(gAiLogicData);
    Free(gBattleStruct);
    gAiThinkingStruct = NULL;
    gAiLogicData = NULL;
    gBattleStruct = NULL;
}

AI_SINGLE_BATTLE_TEST("Koanko AI Resources player healing PP and revival remain legal and observable")
{
    u32 resource;
    PARAMETRIZE { resource = 0; }
    PARAMETRIZE { resource = 1; }
    PARAMETRIZE { resource = 2; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) {
            HP(resource == 0 ? 500 : 1000); MaxHP(1000); Speed(1);
            Defense(9999); SpDefense(9999);
            if (resource == 0) Status1(STATUS1_BURN);
            MovesWithPP({MOVE_TACKLE, 0}, {MOVE_CELEBRATE, 10});
        }
        if (resource == 2)
            PLAYER(SPECIES_WYNAUT) { HP(0); Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(0);
    } WHEN {
        if (resource == 0) { TURN { USE_ITEM(player, ITEM_FULL_RESTORE, partyIndex: 0); } }
        if (resource == 1) { TURN { USE_ITEM(player, ITEM_ETHER, partyIndex: 0, move: MOVE_TACKLE); } }
        if (resource == 2) { TURN { USE_ITEM(player, ITEM_MAX_REVIVE, partyIndex: 1); } }
    } THEN {
        if (resource == 0)
        {
            EXPECT_GT(player->hp, 500);
            EXPECT_EQ(player->status1, STATUS1_NONE);
        }
        if (resource == 1) EXPECT_EQ(player->pp[0], 10);
        if (resource == 2)
        {
            EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_HP), GetMonData(&gPlayerParty[1], MON_DATA_MAX_HP));
            EXPECT_EQ(CountUsablePartyMons(0), 1);
        }
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_EQ(gAiLogicData->hpPercents[0], 100 * player->hp / player->maxHP);
        if (resource == 1) EXPECT(!(gAiLogicData->moveLimitations[0] & 1));
        EXPECT_EQ(gBattleHistory->itemsNo, 0);
        EXPECT_EQ(TRAINER_BATTLE_PARAM.opponentA, TRAINER_KOANKO);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Smeargle a layer break retains progress without a victory bonus")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(10); MaxHP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(0);
        Test_SetKoankoSmeargleScorer(NULL);
        s32 victory = BattleAI_ScoreSlot0Candidate(1, MOVE_FUSION_BOLT);
        EXPECT(ActivateBreakBar(0, &sLayer));
        s32 progress = BattleAI_ScoreSlot0Candidate(1, MOVE_FUSION_BOLT);
        EXPECT_GT(victory, progress);
        EXPECT_GT(progress, AI_SCORE_DEFAULT);
        EXPECT_EQ(player->hp, 10);
        EXPECT_EQ(GetBreakBarCount(0), 1);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Cache clears removed and unusable move slots on next fresh choice")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_OMNISCIENT | AI_FLAG_CHECK_BAD_MOVE);
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_THUNDERBOLT, MOVE_FLAMETHROWER); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        EXPECT_GT(gAiLogicData->simulatedDmg[1][0][1].median, 0);
        opponent->moves[1] = MOVE_NONE;
        // Partial candidate refresh must not retain an estimate for the removed
        // real slot. A whole-turn memset alone would hide this regression.
        BattleAI_PrepareSlot0Move(1, MOVE_THUNDERBOLT);
        EXPECT_EQ(gAiLogicData->simulatedDmg[1][0][1].median, 0);
        EXPECT_EQ(gAiLogicData->moveAccuracy[1][0][1], 0);
        EXPECT_EQ(gAiLogicData->effectiveness[1][0][1], 0);
        opponent->pp[0] = 0;
        SetAiLogicDataForTurn(gAiLogicData);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            EXPECT_EQ(gAiLogicData->simulatedDmg[1][0][slot].median, 0);
            EXPECT_EQ(gAiLogicData->moveAccuracy[1][0][slot], 0);
            EXPECT_EQ(gAiLogicData->effectiveness[1][0][slot], 0);
        }
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Calyrex optional Fire Tera activation deferral and cache parity")
{
    bool32 activate;
    PARAMETRIZE { activate = TRUE; }
    PARAMETRIZE { activate = FALSE; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_GARCHOMP) { HP(1000); Attack(activate ? 300 : 600); SpAttack(300); Moves(activate ? MOVE_ERUPTION : MOVE_EARTHQUAKE, MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(2);
        EXPECT(CanActivateGimmick(1, GIMMICK_TERA));
        BattleAI_ChooseMoveIndex(1);
        EXPECT_EQ(IsAIUsingGimmick(1), activate);
        uq4_12_t effectiveness;
        struct SimulatedDamage fresh = AI_CalcDamage(MOVE_TERA_BLAST, 1, 0, &effectiveness,
            activate ? USE_GIMMICK : NO_GIMMICK, NO_GIMMICK, AI_GetWeather());
        EXPECT_EQ(gAiLogicData->simulatedDmg[1][0][2].median, fresh.median);
        EXPECT_EQ(gAiLogicData->effectiveness[1][0][2], effectiveness);
        EXPECT_EQ(GetActiveGimmick(1), GIMMICK_NONE);
        EXPECT_EQ(gBattleStruct->gimmick.activated[1][GIMMICK_TERA], FALSE);
        SetGimmickAsActivated(1, GIMMICK_TERA);
        EXPECT(!CanActivateGimmick(1, GIMMICK_TERA));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Dialga Time Stop credits probability only when the secondary applies")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(4);
        gAiThinkingStruct->movesetIndex = 0;
        s32 available = AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100);
        player->volatiles.timestop = 1;
        s32 blocked = AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100);
        EXPECT_EQ(available, blocked + 2);
        player->volatiles.timestop = 0;
        gAiLogicData->abilities[0] = ABILITY_SHIELD_DUST;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100), blocked);
        gAiLogicData->abilities[0] = ABILITY_NONE;
        gAiLogicData->holdEffects[0] = HOLD_EFFECT_COVERT_CLOAK;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100), blocked);
        gAiLogicData->holdEffects[0] = HOLD_EFFECT_NONE;
        player->volatiles.substitute = TRUE;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100), blocked);
        player->volatiles.substitute = FALSE;
        gAiLogicData->simulatedDmg[1][0][0].median = 0;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100), 100);
        gBattleStruct->koankoPhase = KOANKO_ORDINARY_TEAM;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_ROAR_OF_ETERNITY, 100), 100);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Cache simulation restores nesting dynamic context and early returns")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_OMNISCIENT);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        uq4_12_t effectiveness;
        struct SimulatedDamage baseline = AI_CalcDamage(MOVE_TERA_BLAST, 1, 0, &effectiveness, USE_GIMMICK, NO_GIMMICK, AI_GetWeather());
        for (u32 nested = 0; nested < 2; nested++)
        {
            gAiLogicData->aiCalcInProgress = nested;
            gBattleStruct->dynamicMoveType = TYPE_STEEL + 1;
            gBattleStruct->swapDamageCategory = TRUE;
            gMultiHitCounter = 5;
            struct SimulatedDamage simulated = AI_CalcDamage(MOVE_TERA_BLAST, 1, 0, &effectiveness, USE_GIMMICK, NO_GIMMICK, AI_GetWeather());
            EXPECT_EQ(simulated.median, baseline.median);
            EXPECT_EQ((u32)gAiLogicData->aiCalcInProgress, nested);
            EXPECT_EQ(gBattleStruct->dynamicMoveType, TYPE_STEEL + 1);
            EXPECT_EQ((u32)gBattleStruct->swapDamageCategory, TRUE);
            EXPECT_EQ(gMultiHitCounter, 5);
            AI_CalcDamage(MOVE_POLLEN_PUFF, 1, BATTLE_PARTNER(1), &effectiveness, USE_GIMMICK, NO_GIMMICK, AI_GetWeather());
            EXPECT_EQ((u32)gAiLogicData->aiCalcInProgress, nested);
            EXPECT_EQ(effectiveness, 0);
        }
        gAiLogicData->aiCalcInProgress = FALSE;
        gBattleStruct->dynamicMoveType = 0;
        gBattleStruct->swapDamageCategory = FALSE;
    }
}

// Deliberately injected scores: picker arithmetic regression, NOT tactical AI.
static u32 sBoundaryMode;
static s32 BoundaryScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    s32 desired = move == MOVE_TACKLE ? 300 : move == MOVE_SCRATCH ? 280 : move == MOVE_POUND ? -1 : 0;
    if (sBoundaryMode == 1)
        desired = move == MOVE_SCRATCH ? -1 : -10;
    else if (sBoundaryMode == 2)
        desired = move == MOVE_SCRATCH ? 0 : -10;
    else if (sBoundaryMode == 3)
        desired = move == MOVE_SCRATCH ? -1 : -10;
    if (IsBattlerAlly(battlerAtk, battlerDef))
        desired = -1000;
    else if (sBoundaryMode == 4)
        desired = INT32_MIN;
    if (sBoundaryMode == 4)
    {
        SET_SCORE(battlerAtk, gAiThinkingStruct->movesetIndex, desired);
        return desired;
    }
    ADJUST_SCORE(desired - score);
    return score;
}

AI_SINGLE_BATTLE_TEST("Koanko AI Score signed maximum exceeds 255 without narrowing")
{
    u32 mode;
    PARAMETRIZE { mode = 0; }
    PARAMETRIZE { mode = 1; }
    PARAMETRIZE { mode = 2; }
    PARAMETRIZE { mode = 3; }
    GIVEN {
        sBoundaryMode = mode;
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetDynamicAiFunc(BoundaryScore);
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) {
            if (mode == 3)
                MovesWithPP({MOVE_TACKLE, 0}, {MOVE_SCRATCH, 10}, {MOVE_POUND, 0}, {MOVE_CELEBRATE, 0});
            else
                Moves(MOVE_TACKLE, MOVE_SCRATCH, MOVE_POUND, MOVE_CELEBRATE);
        }
    } WHEN {
        if (mode == 0) { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_TACKLE); } }
        else { TURN { MOVE(player, MOVE_CELEBRATE); EXPECT_MOVE(opponent, MOVE_SCRATCH); } }
    } SCENE { HP_BAR(player); }
}

AI_DOUBLE_BATTLE_TEST("Koanko AI Score negative legal choices do not target self absent slots or allies")
{
    u32 mode;
    PARAMETRIZE { mode = 1; }
    PARAMETRIZE { mode = 4; }
    GIVEN {
        sBoundaryMode = mode;
        AI_FLAGS(AI_FLAG_DYNAMIC_FUNC);
        SetDynamicAiFunc(BoundaryScore);
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE, MOVE_SCRATCH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE, MOVE_SCRATCH); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_CELEBRATE);
            MOVE(playerRight, MOVE_CELEBRATE);
            if (mode == 1)
            {
                EXPECT_MOVE(opponentLeft, MOVE_SCRATCH);
                EXPECT_MOVE(opponentRight, MOVE_SCRATCH);
            }
            else
            {
                EXPECT_MOVES(opponentLeft, MOVE_TACKLE, MOVE_SCRATCH);
                EXPECT_MOVES(opponentRight, MOVE_TACKLE, MOVE_SCRATCH);
            }
        }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, opponentLeft->maxHP);
        EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
        EXPECT_LT(playerLeft->hp + playerRight->hp, playerLeft->maxHP + playerRight->maxHP);
        EXPECT_EQ(GetBattlerSide(gAiBattleData->chosenTarget[1]), B_SIDE_PLAYER);
        EXPECT_EQ(GetBattlerSide(gAiBattleData->chosenTarget[3]), B_SIDE_PLAYER);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko AI Topology fresh refresh preserves custom flags and committed choices")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_SEQUENCE_SWITCHING | AI_FLAG_DYNAMIC_FUNC);
        SetDynamicAiFunc(NULL);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        gAiBattleData->chosenMoveIndex[1] = 2;
        gAiBattleData->chosenTarget[1] = 0;
        gChosenMoveByBattler[1] = MOVE_CELEBRATE;
        u64 flags = gAiThinkingStruct->aiFlags[1];
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_EQ(gAiThinkingStruct->aiFlags[1], flags | AI_FLAG_DOUBLE_BATTLE);
        gBattleTypeFlags &= ~BATTLE_TYPE_DOUBLE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_EQ(gAiThinkingStruct->aiFlags[1], flags);
        EXPECT_EQ(gAiBattleData->chosenMoveIndex[1], 2);
        EXPECT_EQ(gAiBattleData->chosenTarget[1], 0);
        EXPECT_EQ(gChosenMoveByBattler[1], MOVE_CELEBRATE);
        // This does not create Stage07's independent ally or controller.
        EXPECT(!BattlerHasAi(B_POSITION_PLAYER_LEFT));
        // No usable moves: the fresh doubles picker still returns a bounded
        // ordinary slot/real enemy for the executor's Struggle handling.
        opponent->pp[0] = 0;
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
        SetAiLogicDataForTurn(gAiLogicData);
        BattleAI_SetupAIData(0xF, 1);
        EXPECT_LT(BattleAI_ChooseMoveIndex(1), MAX_MON_MOVES);
        EXPECT_EQ(gAiBattleData->chosenTarget[1], 0);
        gBattleTypeFlags &= ~BATTLE_TYPE_DOUBLE;
    }
}

// Isolate the trainer's conditional delta using real production kits/caches.
// The existing Koanko AI cases additionally observe full-pipeline execution.
static s32 TacticsScore(u32 move)
{
    for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        if (gBattleMons[1].moves[slot] == move)
        {
            gAiThinkingStruct->movesetIndex = slot;
            return AI_KoankoTrainer(1, 0, move, AI_SCORE_DEFAULT);
        }
    return 0;
}

static u32 TacticsChoice(void)
{
    SetAiLogicDataForTurn(gAiLogicData);
    BattleAI_SetupAIData(0xF, 1);
    return gBattleMons[1].moves[BattleAI_ChooseMoveIndex(1)];
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics original six register the real callback without scripted setup")
{
    u32 index;
    PARAMETRIZE { index = 0; }
    PARAMETRIZE { index = 1; }
    PARAMETRIZE { index = 2; }
    PARAMETRIZE { index = 3; }
    PARAMETRIZE { index = 4; }
    PARAMETRIZE { index = 5; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Defense(9999); SpDefense(9999); Moves(MOVE_CELEBRATE); }
        OriginalFixture(index);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        EXPECT_EQ(sDynamicAiFunc, AI_KoankoTrainer);
        EXPECT(gAiThinkingStruct->aiFlags[1] & AI_FLAG_DYNAMIC_FUNC);
        EXPECT_EQ(gBattleStruct->koankoPhase, KOANKO_ORDINARY_TEAM);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Whimsicott uses fresh disruption and avoids duplicate speed setup")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TACKLE); }
        OriginalFixture(0);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        gAiLogicData->speedStats[1] = 500;
        gAiLogicData->speedStats[0] = 999;
        gSideTimers[B_SIDE_OPPONENT].tailwindTimer = 0;
        s32 fresh = TacticsScore(MOVE_TAILWIND);
        gSideTimers[B_SIDE_OPPONENT].tailwindTimer = 3;
        EXPECT_GT(fresh, TacticsScore(MOVE_TAILWIND));
        gDisableStructs[0].tauntTimer = 0;
        s32 taunt = TacticsScore(MOVE_TAUNT);
        gDisableStructs[0].tauntTimer = 3;
        EXPECT_GT(taunt, TacticsScore(MOVE_TAUNT));
        gBattleMons[0].volatiles.leechSeed = FALSE;
        s32 seed = TacticsScore(MOVE_LEECH_SEED);
        gBattleMons[0].volatiles.leechSeed = TRUE;
        EXPECT_GT(seed, TacticsScore(MOVE_LEECH_SEED));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Alcremie distinguishes Max Guard from healing and special suppression")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); SpAttack(1); Moves(MOVE_CELEBRATE, MOVE_WATER_GUN); }
        OriginalFixture(1);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        opponent->hp = opponent->maxHP / 5;
        SetActiveGimmick(1, GIMMICK_DYNAMAX);
        s32 guard = TacticsScore(MOVE_RECOVER);
        SetActiveGimmick(1, GIMMICK_NONE);
        gBattleStruct->gimmick.usableGimmick[1] = GIMMICK_NONE;
        EXPECT_GT(TacticsScore(MOVE_RECOVER), guard);
        s32 suppress = TacticsScore(MOVE_MYSTICAL_FIRE);
        player->moves[1] = MOVE_TACKLE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(suppress, TacticsScore(MOVE_MYSTICAL_FIRE));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Glaceon uses actual Water coverage and avoids repeated Yawn")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(2);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        player->volatiles.yawn = FALSE;
        player->status1 = STATUS1_NONE;
        s32 sleep = TacticsScore(MOVE_YAWN);
        player->volatiles.yawn = TRUE;
        EXPECT_GT(sleep, TacticsScore(MOVE_YAWN));
        player->status1 = STATUS1_PARALYSIS;
        s32 neutral = TacticsScore(MOVE_FREEZE_DRY);
        player->types[0] = TYPE_WATER;
        player->types[1] = TYPE_WATER;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(TacticsScore(MOVE_FREEZE_DRY), neutral);
        EXPECT_EQ(TacticsChoice(), MOVE_FREEZE_DRY);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Vanilluxe revalues Blizzard when snow is replaced")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Status1(STATUS1_PARALYSIS); Moves(MOVE_TACKLE); }
        OriginalFixture(3);
    } WHEN { TURN { MOVE(player, MOVE_TACKLE); } }
    THEN {
        gBattleWeather = B_WEATHER_HAIL;
        SetAiLogicDataForTurn(gAiLogicData);
        s32 snow = TacticsScore(MOVE_BLIZZARD);
        gBattleWeather = B_WEATHER_SUN;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(snow, TacticsScore(MOVE_BLIZZARD));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Audino values reachable Wish Encore and removable items")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TACKLE); }
        OriginalFixture(4);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        opponent->hp = opponent->maxHP / 3;
        gWishFutureKnock.wishCounter[1] = 0;
        s32 wish = TacticsScore(MOVE_WISH);
        gWishFutureKnock.wishCounter[1] = 2;
        EXPECT_GT(wish, TacticsScore(MOVE_WISH));
        gAiLogicData->lastUsedMove[0] = MOVE_CELEBRATE;
        gDisableStructs[0].encoreTimer = 0;
        s32 encore = TacticsScore(MOVE_ENCORE);
        gDisableStructs[0].encoreTimer = 3;
        EXPECT_GT(encore, TacticsScore(MOVE_ENCORE));
        player->item = ITEM_LEFTOVERS;
        SetAiLogicDataForTurn(gAiLogicData);
        s32 item = TacticsScore(MOVE_KNOCK_OFF);
        player->item = ITEM_NONE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(item, TacticsScore(MOVE_KNOCK_OFF));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Blissey sustains and suppresses redundant attrition")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        PLAYER(SPECIES_WYNAUT) { Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(5);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        opponent->hp = opponent->maxHP / 5;
        s32 recovery = TacticsScore(MOVE_SOFT_BOILED);
        opponent->volatiles.healBlock = TRUE;
        EXPECT_GT(recovery, TacticsScore(MOVE_SOFT_BOILED));
        player->status1 = STATUS1_NONE;
        s32 toxic = TacticsScore(MOVE_TOXIC);
        player->status1 = STATUS1_POISON;
        EXPECT_GT(toxic, TacticsScore(MOVE_TOXIC));
        IsHazardOnSideAndClear(B_SIDE_PLAYER, HAZARDS_STEALTH_ROCK);
        s32 rocks = TacticsScore(MOVE_STEALTH_ROCK);
        PushHazardTypeToQueue(B_SIDE_PLAYER, HAZARDS_STEALTH_ROCK);
        EXPECT_GT(rocks, TacticsScore(MOVE_STEALTH_ROCK));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics TX coordinates delayed damage Wish and one Protect")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Attack(50); Moves(MOVE_CELEBRATE, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(1);
        opponent->hp = opponent->maxHP / 3;
        gWishFutureKnock.wishCounter[1] = 0;
        s32 wish = TacticsScore(MOVE_WISH);
        gWishFutureKnock.wishCounter[1] = 2;
        EXPECT_GT(wish, TacticsScore(MOVE_WISH));
        gDisableStructs[1].protectUses = 0;
        s32 protect = TacticsScore(MOVE_PROTECT);
        gDisableStructs[1].protectUses = 1;
        EXPECT_GT(protect, TacticsScore(MOVE_PROTECT));
        gWishFutureKnock.futureSightCounter[0] = 0;
        s32 doom = TacticsScore(MOVE_DOOM_DESIRE);
        gWishFutureKnock.futureSightCounter[0] = 2;
        EXPECT_GT(doom, TacticsScore(MOVE_DOOM_DESIRE));
        EXPECT(TxRollbackHasWishes(1));
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Calyrex invests only in useful uncapped setup")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(2);
        opponent->statStages[STAT_ATK] = DEFAULT_STAT_STAGE;
        SetAiLogicDataForTurn(gAiLogicData);
        s32 setup = TacticsScore(MOVE_SWORDS_DANCE);
        opponent->statStages[STAT_ATK] = MAX_STAT_STAGE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(setup, TacticsScore(MOVE_SWORDS_DANCE));
        opponent->statStages[STAT_ATK] = DEFAULT_STAT_STAGE;
        player->hp = 1;
        EXPECT_LT(TacticsScore(MOVE_SWORDS_DANCE), AI_SCORE_DEFAULT);
        EXPECT_NE(TacticsChoice(), MOVE_SWORDS_DANCE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Shadow Calyrex reads a real special kit without inventing a transition")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(2);
        // Mechanism fixture only: Stage06 still owns actual form/kit reload.
        CreateMon(&gEnemyParty[2], SPECIES_CALYREX_SHADOW, 102, 31, TRUE, 0, OT_ID_PRESET, 0);
        SetMonMoveSlot(&gEnemyParty[2], MOVE_ASTRAL_BARRAGE, 0);
        SetMonMoveSlot(&gEnemyParty[2], MOVE_PSYSHOCK, 1);
        SetMonMoveSlot(&gEnemyParty[2], MOVE_NASTY_PLOT, 2);
        SetMonMoveSlot(&gEnemyParty[2], MOVE_NONE, 3);
        PokemonToBattleMon(&gEnemyParty[2], opponent);
        SetAiLogicDataForTurn(gAiLogicData);
        s32 setup = TacticsScore(MOVE_NASTY_PLOT);
        opponent->statStages[STAT_SPATK] = MAX_STAT_STAGE;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(setup, TacticsScore(MOVE_NASTY_PLOT));
        EXPECT_EQ(opponent->species, SPECIES_CALYREX_SHADOW);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Eternatus forms use current recovery and avoid comparable recharge")
{
    bool32 eternamax;
    PARAMETRIZE { eternamax = FALSE; }
    PARAMETRIZE { eternamax = TRUE; }
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(3);
        if (eternamax)
        {
            u16 form = SPECIES_ETERNATUS_ETERNAMAX;
            SetMonData(&gEnemyParty[3], MON_DATA_SPECIES, &form);
            CalculateMonStats(&gEnemyParty[3]);
            PokemonToBattleMon(&gEnemyParty[3], opponent);
        }
        opponent->hp = opponent->maxHP / 5;
        s32 recovery = TacticsScore(MOVE_RECOVER);
        opponent->hp = opponent->maxHP;
        EXPECT_GT(recovery, TacticsScore(MOVE_RECOVER));
        opponent->moves[0] = MOVE_ETERNABEAM;
        opponent->pp[0] = 5;
        player->types[0] = player->types[1] = TYPE_STEEL;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(TacticsScore(MOVE_FLAMETHROWER), TacticsScore(MOVE_ETERNABEAM));
        EXPECT_EQ(GetActiveGimmick(1), GIMMICK_NONE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Dialga prefers coverage when recharge has a comparable alternative")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(4);
        player->types[0] = player->types[1] = TYPE_STEEL;
        player->volatiles.timestop = 1;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(TacticsScore(MOVE_EARTH_POWER), TacticsScore(MOVE_ROAR_OF_ETERNITY));
        EXPECT_EQ(TacticsChoice(), MOVE_EARTH_POWER);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics Arceus uses actual coverage over immune Judgment and unnecessary recovery")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(5);
        player->types[0] = TYPE_GHOST;
        player->types[1] = TYPE_DRAGON;
        player->hp = 1;
        opponent->hp = opponent->maxHP / 5;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(TacticsScore(MOVE_ICE_BEAM), TacticsScore(MOVE_JUDGMENT));
        EXPECT_EQ(TacticsChoice(), MOVE_ICE_BEAM);
        EXPECT_EQ(opponent->ability, ABILITY_MULTITYPE);
    }
}

AI_SINGLE_BATTLE_TEST("Koanko Tactics ownership and callback precedence do not commandeer other actors")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Speed(1); Moves(MOVE_CELEBRATE); }
        OriginalFixture(0);
    } WHEN { TURN { MOVE(player, MOVE_CELEBRATE); } }
    THEN {
        EXPECT_EQ(AI_KoankoTrainer(0, 1, MOVE_CELEBRATE, 103), 103);
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_ROXANNE_1;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_TAUNT, 103), 103);
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_KOANKO;
        gBattleStruct->koankoPhase = KOANKO_CONJURED_TEAM;
        EXPECT_EQ(AI_KoankoTrainer(1, 0, MOVE_TAUNT, 103), 103);
        SetDynamicAiFunc(BoundaryScore);
        BattleAI_SetupFlags();
        EXPECT_EQ(sDynamicAiFunc, BoundaryScore);
        // A player-side Zacian is not inferred as an encounter owner by species.
        player->species = SPECIES_ZACIAN_CROWNED;
        EXPECT_EQ(AI_KoankoTrainer(0, 1, MOVE_SWORDS_DANCE, 103), 103);
    }
}

AI_DOUBLE_BATTLE_TEST("Koanko Tactics recovery considers a fast second opponent without reading commands")
{
    gBattleTestRunnerState->data.recordedBattle.opponentA = TRAINER_KOANKO;
    GIVEN {
        AI_FLAGS(GetTrainerAIFlagsFromId(TRAINER_KOANKO));
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Attack(1); Speed(1); Moves(MOVE_CELEBRATE, MOVE_TACKLE); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); Attack(999); Speed(1000); Moves(MOVE_CELEBRATE, MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(500); Moves(MOVE_CELEBRATE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(500); Moves(MOVE_CELEBRATE); }
    } WHEN { TURN { MOVE(playerLeft, MOVE_CELEBRATE); MOVE(playerRight, MOVE_CELEBRATE); } }
    THEN {
        PrepareConjured(3);
        gAbsentBattlerFlags |= 1u << 3;
        opponentLeft->hp = 100;
        SetAiLogicDataForTurn(gAiLogicData);
        s32 tooLate = TacticsScore(MOVE_RECOVER);
        EXPECT_LT(tooLate, AI_SCORE_DEFAULT);
        playerRight->speed = 1;
        SetAiLogicDataForTurn(gAiLogicData);
        EXPECT_GT(TacticsScore(MOVE_RECOVER), tooLate);
    }
}
