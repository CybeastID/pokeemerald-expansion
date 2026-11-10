#include "global.h"
#include "battle.h"
#include "battle_koanko_smeargle_diagnostics.h"

#if KOANKO_SMEARGLE_DIAGNOSTICS
#include "battle_ai_util.h"
#include "battle_koanko_smeargle.h"
#include "battle_util.h"
#include "main.h"
#include "move.h"
#include "gba/isagbprint.h"
#include "constants/global.h"
#include "constants/characters.h"

// No default .bss arrays: all detailed observations live in EWRAM.
static EWRAM_DATA struct SmeargleDiagDecision sDecision = {0};
static EWRAM_DATA struct SmeargleDiagMove sCurrent = {0};
static EWRAM_DATA bool8 sEnabled = 0;
static EWRAM_DATA bool8 sLog = 0;
static EWRAM_DATA bool8 sContinuous = 0;
static EWRAM_DATA bool8 sInitialized = FALSE;
static EWRAM_DATA bool8 sCapturing = FALSE;
static EWRAM_DATA bool8 sCandidate = FALSE;
static EWRAM_DATA u8 sBattler = 0;
static EWRAM_DATA u32 sStartFrame = 0;
static const u16 sWatch[] = {MOVE_FOUL_PLAY, MOVE_BULLET_PUNCH, MOVE_ZIPPY_ZAP,
    MOVE_WATER_SHURIKEN, MOVE_ICE_SHARD, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_SHADOW_BALL};
extern const u8 gWireless_RSEtoASCIITable[];

static void InitKoankoSmeargleDiagnostics(void)
{
    if (sInitialized)
        return;

    sInitialized = TRUE;
    sEnabled = DEBUG_KOANKO_SMEARGLE_DIAGNOSTICS;
    sLog = DEBUG_KOANKO_SMEARGLE_DIAGNOSTICS;
    sContinuous = DEBUG_KOANKO_SMEARGLE_DIAGNOSTICS;
}

void KoankoSmeargleDiagEnable(bool32 log, bool32 continuous)
{
    InitKoankoSmeargleDiagnostics();
    sEnabled = TRUE;
    sLog = log;
    sContinuous = continuous;
    sCapturing = sCandidate = FALSE;
    memset(&sDecision, 0, sizeof(sDecision));
}

void KoankoSmeargleDiagDisable(void)
{
    InitKoankoSmeargleDiagnostics();
    sEnabled = sCapturing = sCandidate = FALSE;
}

const struct SmeargleDiagDecision *KoankoSmeargleDiagGet(void) { return &sDecision; }
const struct SmeargleDiagMove *KoankoSmeargleDiagCurrent(void) { return &sCurrent; }

bool32 KoankoSmeargleDiagWasEvaluated(u32 move)
{
    return move < MOVES_COUNT_GEN9 && (sDecision.evaluated[move / 8] & (1u << (move % 8))) != 0;
}

void KoankoSmeargleDiagBegin(u32 battler, bool32 sampled)
{
    InitKoankoSmeargleDiagnostics();
    if (!sEnabled || (!sContinuous && sDecision.complete))
        return;
    memset(&sDecision, 0, sizeof(sDecision));
    sBattler = battler;
    sCapturing = TRUE;
    sCandidate = FALSE;
    u32 target = GetOppositeBattler(battler);
    sDecision.flags = gAiThinkingStruct->aiFlags[battler];
    sDecision.turn = gBattleResults.battleTurnCounter;
    sDecision.sampled = sampled;
    sDecision.attackerSpecies = gBattleMons[battler].species;
    sDecision.targetSpecies = gBattleMons[target].species;
    sDecision.hp = gBattleMons[battler].hp;
    sDecision.maxHp = gBattleMons[battler].maxHP;
    sDecision.targetHp = gBattleMons[target].hp;
    sDecision.targetMaxHp = gBattleMons[target].maxHP;
    sDecision.targetStatus = gBattleMons[target].status1;
    memcpy(sDecision.targetTypes, gBattleMons[target].types, sizeof(sDecision.targetTypes));
    sDecision.attack = gBattleMons[battler].attack;
    sDecision.spAttack = gBattleMons[battler].spAttack;
    sDecision.targetAttack = gBattleMons[target].attack;
    sDecision.targetDefense = gBattleMons[target].defense;
    sDecision.targetSpDefense = gBattleMons[target].spDefense;
    sDecision.speed = gAiLogicData->speedStats[battler];
    sDecision.targetSpeed = gAiLogicData->speedStats[target];
    sDecision.ability = gAiLogicData->abilities[battler];
    sDecision.targetAbility = gAiLogicData->abilities[target];
    sDecision.item = gBattleMons[battler].item;
    sDecision.targetItem = gBattleMons[target].item;
    sDecision.field = gFieldStatuses;
    sDecision.weather = gBattleWeather;
    sStartFrame = gMain.vblankCounter1;
}

void KoankoSmeargleDiagCandidateBegin(u32 battler, u16 move)
{
    sCandidate = sCapturing && battler == sBattler;
    if (!sCandidate)
        return;
    memset(&sCurrent, 0, sizeof(sCurrent));
    sCurrent.move = move;
    sCurrent.order = ++sDecision.evaluations;
    sCurrent.eligible = KoankoSmeargleIsEligibleMove(move);
    if (move < MOVES_COUNT_GEN9)
        sDecision.evaluated[move / 8] |= 1u << (move % 8);
}

void KoankoSmeargleDiagPrepared(u32 battler, u16 move)
{
    if (!sCandidate || battler != sBattler)
        return;
    u32 target = GetOppositeBattler(battler);
    const struct SimulatedDamage *damage = &gAiLogicData->simulatedDmg[battler][target][0];
    sCurrent.initial = gAiThinkingStruct->score[0];
    sCurrent.usable = !(gAiLogicData->moveLimitations[battler] & 1);
    sCurrent.power = GetMovePower(move);
    sCurrent.priority = GetMovePriority(move); // overwritten by effective priority when CHECK_BAD_MOVE runs
    sCurrent.type = GetBattleMoveType(move);
    sCurrent.category = GetBattleMoveCategory(move);
    sCurrent.effectiveness = gAiLogicData->effectiveness[battler][target][0];
    sCurrent.accuracy = gAiLogicData->moveAccuracy[battler][target][0];
    sCurrent.damageMin = damage->minimum;
    sCurrent.damageMedian = damage->median;
    sCurrent.damageMax = damage->maximum;
    // Raw damage threshold, not a promise to bypass Sash/Sturdy/Endure.
    sCurrent.medianDamageKo = damage->median >= gBattleMons[target].hp;
}

void KoankoSmeargleDiagPriority(u32 battler, s32 priority)
{
    if (sCandidate && battler == sBattler)
        sCurrent.priority = priority;
}

void KoankoSmeargleDiagPass(u32 battler, u32 logic, s32 before, s32 after)
{
    if (!sCandidate || battler != sBattler)
        return;
    if (sCurrent.passCount < SMEARGLE_DIAG_PASSES)
        sCurrent.passes[sCurrent.passCount++] = (struct SmeargleDiagPass){before, after, logic};
    else
        sCurrent.omittedPasses++;
}

void KoankoSmeargleDiagViability(u32 battler, u32 part, s32 before, s32 after)
{
    if (sCandidate && battler == sBattler && part < ARRAY_COUNT(sCurrent.viability))
        sCurrent.viability[part] += after - before;
}

void KoankoSmeargleDiagCandidateEnd(u32 battler, s32 score)
{
    if (!sCandidate || battler != sBattler)
        return;
    sCurrent.score = score;
    sCurrent.positive = score > 0;
    sDecision.positives += sCurrent.positive;
    for (u32 i = 0; i < ARRAY_COUNT(sWatch); i++)
        if (sCurrent.move == sWatch[i])
            sDecision.watch[i] = sCurrent;
    u32 index = 0;
    while (index < sDecision.topCount && sDecision.top[index].score >= score)
        index++;
    if (index < SMEARGLE_DIAG_TOP)
    {
        if (sDecision.topCount < SMEARGLE_DIAG_TOP)
            sDecision.topCount++;
        for (u32 i = sDecision.topCount - 1; i > index; i--)
            sDecision.top[i] = sDecision.top[i - 1];
        sDecision.top[index] = sCurrent;
    }
    sCandidate = FALSE;
}

void KoankoSmeargleDiagBest(u16 move)
{
    if (sCapturing && sCurrent.move == move)
        sDecision.selected = sCurrent;
}

void KoankoSmeargleDiagEnd(u32 pool, u16 chosen, s32 bestScore, u32 ties)
{
    if (!sCapturing)
        return;
    sDecision.elapsedFrames = gMain.vblankCounter1 - sStartFrame;
    sDecision.poolSize = pool;
    sDecision.chosen = chosen;
    sDecision.bestScore = bestScore;
    sDecision.ties = ties;
    sDecision.complete = TRUE;
    sCapturing = sCandidate = FALSE;
    if (sLog)
        KoankoSmeargleDiagPrint();
}

static void MoveName(char *dest, u16 move)
{
    const u8 *src = GetMoveName(move);
    u32 i = 0;
    while (i < MOVE_NAME_LENGTH && src[i] != EOS)
    {
        dest[i] = gWireless_RSEtoASCIITable[src[i]];
        i++;
    }
    dest[i] = 0;
}

static void PrintMove(const char *kind, const struct SmeargleDiagMove *record)
{
    char name[MOVE_NAME_LENGTH + 1];
    MoveName(name, record->move);
    DebugPrintf("SMEAR %s %s id=%u order=%u score=%ld init=%ld prio=%d type=%u cat=%u bp=%u", kind, name,
        record->move, record->order, (long)record->score, (long)record->initial, record->priority, record->type, record->category, record->power);
    DebugPrintf("SMEAR damage=%u/%u/%u effQ12=%u acc=%u eligible=%u usable=%u positive=%u medianDamageKO=%u", record->damageMin,
        record->damageMedian, record->damageMax, record->effectiveness, record->accuracy, record->eligible, record->usable, record->positive, record->medianDamageKo);
    for (u32 i = 0; i < record->passCount; i++)
        DebugPrintf("SMEAR pass=%u aggregate %ld -> %ld delta=%ld", record->passes[i].logic,
            (long)record->passes[i].before, (long)record->passes[i].after, (long)(record->passes[i].after - record->passes[i].before));
    DebugPrintf("SMEAR viability aggregates effect=%ld additional=%ld item=%ld omittedPasses=%u", (long)record->viability[0],
        (long)record->viability[1], (long)record->viability[2], record->omittedPasses);
}

void KoankoSmeargleDiagPrint(void)
{
    DebugPrintf("SMEAR decision turn=%u sampled=%u pool=%u eval=%u positive=%u best=%ld chosen=%u ties=%u frames=%lu",
        sDecision.turn, sDecision.sampled, sDecision.poolSize, sDecision.evaluations, sDecision.positives,
        (long)sDecision.bestScore, sDecision.chosen, sDecision.ties, (unsigned long)sDecision.elapsedFrames);
    DebugPrintf("SMEAR species=%u target=%u hp=%u/%u targetHp=%u/%u targetPct=%u flags=%08lx:%08lx", sDecision.attackerSpecies,
        sDecision.targetSpecies, sDecision.hp, sDecision.maxHp, sDecision.targetHp, sDecision.targetMaxHp,
        sDecision.targetMaxHp ? 100 * sDecision.targetHp / sDecision.targetMaxHp : 0,
        (unsigned long)(sDecision.flags >> 32), (unsigned long)sDecision.flags);
    DebugPrintf("SMEAR atk=%u spa=%u targetAtk=%u def=%u spd=%u speed=%u/%u abilities=%u/%u items=%u/%u",
        sDecision.attack, sDecision.spAttack, sDecision.targetAttack, sDecision.targetDefense, sDecision.targetSpDefense,
        sDecision.speed, sDecision.targetSpeed, sDecision.ability, sDecision.targetAbility, sDecision.item, sDecision.targetItem);
    DebugPrintf("SMEAR targetTypes=%u/%u/%u status=%lu field=%lu weather=%lu", sDecision.targetTypes[0], sDecision.targetTypes[1],
        sDecision.targetTypes[2], (unsigned long)sDecision.targetStatus, (unsigned long)sDecision.field, (unsigned long)sDecision.weather);
    if (sDecision.selected.move != MOVE_NONE)
        PrintMove("SELECTED", &sDecision.selected);
    for (u32 i = 0; i < sDecision.topCount; i++)
        PrintMove("TOP", &sDecision.top[i]);
    for (u32 i = 0; i < ARRAY_COUNT(sWatch); i++)
    {
        if (sDecision.watch[i].move != MOVE_NONE)
            PrintMove("WATCH", &sDecision.watch[i]);
        else
        {
            char name[MOVE_NAME_LENGTH + 1];
            MoveName(name, sWatch[i]);
            DebugPrintf("SMEAR WATCH %s id=%u NOT_EVALUATED (no score inferred)", name, sWatch[i]);
        }
    }
}
#endif
