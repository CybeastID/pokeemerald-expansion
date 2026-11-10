# Stage 03 — Smeargle Any Move through the ordinary AI pipeline

Status: **fully planned**, implementation pending. Snapshot: `mypatch`, 2026-11-09.

## 1. Objective and scope

At each fresh action choice, select the best of up to 60 distinct positive-scoring randomly drawn ordinary moves, using the existing AI and actual move slot 0. Preserve ordinary move execution and restrictions. Do not create a parallel attack executor or grant every Smeargle this behavior.

## 2. Existing code verified

- `src/data/koanko_party.h` already gives Koanko Smeargle only slot 0, provisionally Seismic Toss; remaining slots zero-initialize. No Any Move sampler exists in `battle_koanko.c` or the inspected AI selection paths.
- `src/battle_ai_main.c:BattleAI_SetupAIData`, `CalcBattlerAiMovesData`, `SetAiLogicDataForTurn`, `BattleAI_DoAIProcessing`, and `ChooseMoveOrAction_Singles` own limitations, damage/accuracy/effectiveness caches, enabled scoring passes and final ties. Four-slot caches are indexed by move position: changing a move without refreshing its caches produces invalid scores.
- `BattleAI_DoAIProcessing` stops evaluating nonpositive slots; unusable/absent candidates become score 0. A positive score is tactical acceptance, not a substitute for ordinary legality checks. The normal picker resolves equal scores randomly.
- `src/battle_util.c:CancelerChoiceLock` writes `choicedMove`; `CheckMoveLimitations` applies Encore/Choice/Gorilla Tactics restrictions; ordinary PP deduction also charges Pressure. `src/battle_script_commands.c:Cmd_trysetencore` writes Encore state after eligibility checks.
- `include/constants/moves.h` separates ordinary/custom/Z/Max move ranges and MOVE_NONE/MOVE_UNAVAILABLE. Boss custom moves already include Roar of Eternity and Arcanum actions. The ordinary legal pool must not use `MOVES_COUNT_ALL` indiscriminately.
- `test/battle/koanko.c` has an AI entry/selection test currently accepting Smeargle's placeholder Seismic Toss; it needs meaningful Any Move expectations.

## 3. Implementation design and order

1. Add a Koanko owner predicate for original conjured slot 0. Use encounter identity so ordinary Smeargle and copied species remain unchanged. Use this predicate for sampling, PP exemption and acquisition immunity.
2. Build a ROM-backed eligible move-ID pool or a compact runtime list from ordinary move definitions: exclude NONE, unavailable IDs, Struggle as a sampled candidate, Z/Max variants and all encounter-exclusive custom spells/moves. Do not score the full pool to construct eligibility. Keep pool membership separate from turn-specific legality.
3. In the AI module factor a single-candidate scoring entry point from existing setup/cache/scoring logic. Place the candidate in scratch slot 0, provide nonzero PP, recompute only affected candidate/target damage, accuracy, effectiveness and limitation data, then run the same enabled AI passes. Preserve relevant move-comparison behavior using the current one-slot moveset; restore scoring scratch/history before committing. Do not score all ordinary moves a second time or call the final random picker for each candidate.
4. Draw without replacement via partial Fisher–Yates over the eligible IDs with a new tagged RNG stream. For each drawn candidate run ordinary legality and the scoring entry point once. Retain positive scores until 60 accepted or all IDs drawn. Store only the best candidate/target/score and tie count; use reservoir tie selection for uniform best-score ties. No rejection loop may draw the same ID twice.
5. Commit the winner to real slot 0, refresh the winner's dependent AI caches without rescoring its tactical merit, and enter the standard action pipeline at slot 0. Synchronize temporary battle/party move and PP views wherever existing controllers read them; empty slots remain empty. Zero accepted candidates selects ordinary Struggle fallback, never MOVE_NONE or an infinite retry.
6. Exempt owned Smeargle's selected slot from PP depletion, including Pressure; keep displayed/controller PP valid. Fail Encore acquisition and suppress Choice/Gorilla Tactics lock assignment at their authoritative write sites, with AI eligibility using the same immunity predicate. Do not clear an inflicted lock after the fact.
7. Sample only for a fresh choice: recharge, charging/rampage continuation, forced continuation and an already committed action retain ordinary engine rules. A temporary move overwrite cannot evade recharge, sleep, paralysis, flinch, Taunt, Disable, Torment, Imprison or ordinary cancelers. Filter other boss-exclusive moves reachable indirectly by move-call helpers where needed; do not accidentally bypass the pool through Metronome/Assist.

Proposed files: Koanko owner helper/header; `battle_ai_main.c` and its header (single-candidate API and chooser); `battle_util.c` and `battle_script_commands.c` (PP/lock write gates); RNG enum; focused new `test/battle/koanko_smeargle.c`. Keep the already-correct party layout.

## 4. Dependencies

Stage 01 encounter identity/order. Existing ordinary AI, legality, move-caller exclusions, tagged RNG and action pipeline. This stage can be reviewed without TX or form implementation.

## 5. Edge cases and invariants

Uniform ID sampling, distinct positive accepted candidates, bounded exhaustion and stable tagged RNG for a fixed seed. Score 0 must not be counted; fewer than 60 eligible positives is valid. Scoring must not consume real PP, mutate real history, trigger moves, or change targets/actions for another battler. Preserve the selected move's dynamic type, target rules, recoil, recharge and ordinary secondary effects. No blanket immunity to all restrictions and no extra turn from resampling.

## 6. Test strategy

Implementation: `wsl make check TESTS='Koanko Smeargle'` (new tests use this prefix), then `TESTS=Koanko`, `TESTS=Encore`, and narrow existing Choice/Gorilla Tactics tests affected by edits.

Use injected small eligible pools/scoring observation under TESTING to prove every ID drawn at most once, stop at 60 positives, zeros skipped, pool exhaustion, all-zero/empty fallback, best-score winner and tie distribution. Compare candidate scoring with an equivalent ordinary one-slot AI fixture to detect stale caches. Count scoring calls to prevent an exhaustive second pass. Cover fixed seeds, Pressure, PP/controller display, Encore/Choice acquisition, Taunt/Disable, recharge/multi-turn moves and ordinary Smeargle unaffected. Measure worst-case pool exhaustion latency on emulator; avoid a per-candidate full party/AI rebuild.

## 7. Completion criteria

Chosen actions are standard slot-0 actions, satisfy the sampling contract, execute once, and maintain PP/legality invariants. The maximum work is bounded by eligible pool size, observed candidate scoring matches normal AI, and control never hangs on an empty usable pool.

## 8. Open questions

- **Default:** random uniform ties and Struggle on zero positives; neither is specified in detail by the handoff, and both follow ordinary engine expectations.
- **Pool policy to review:** exclude all custom encounter moves, Z/Max moves and indirect access to boss-exclusive actions; include ordinary species-signature moves when their ordinary runtime legality permits. Keep an explicit exclusion policy so new custom moves do not enter accidentally.
- Performance threshold should be recorded from emulator measurements; no unsupported timing claim is made here.
