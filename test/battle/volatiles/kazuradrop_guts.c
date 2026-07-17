#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Kazuradrop Guts (cheat death) expires after one full turn")
{
    u32 turns;
    u32 expectedGuts;

    // Kazuradrop sets kazuradropGuts = 2 on transformation.
    // battle_end_turn.c decrements it once per full turn.
    // Observed in the test harness:
    // - After end of transformation turn, kazuradropGuts reaches 1.
    // - It appears to stay at 1 after the next full TURN {}, and reaches 0 one turn later.
    PARAMETRIZE { turns = 0; expectedGuts = 1; } // after end of transformation turn
    PARAMETRIZE { turns = 1; expectedGuts = 0; } // after one extra full turn (observed)
    PARAMETRIZE { turns = 2; expectedGuts = 0; } // after two extra full turns (observed)

    GIVEN {
        // Bug Space must be active; in existing tests this is done via Kazuradrop's ability.
        PLAYER(SPECIES_REGISTEEL) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(300); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); }
    } WHEN {
        // Deal damage large enough to force the <10% HP clamp -> transformation -> guts icon + kazuradropGuts=2.
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); 
            MOVE(opponent, MOVE_CELEBRATE); // dummy move to keep the turn flow going while we test the timer
        }
        
        u32 count;
        for (count = 0; count < turns; count++)
            TURN { MOVE(player, MOVE_CELEBRATE);
             MOVE(opponent, MOVE_CELEBRATE); // dummy move to keep the turn flow going while we test the timer
            };
    } THEN {
        u32 guts = gBattleMons[B_POSITION_OPPONENT_LEFT].volatiles.kazuradropGuts;
        EXPECT_EQ(guts, expectedGuts);
    }
}

SINGLE_BATTLE_TEST("Kazuradrop Guts snaps internal HP to 50% immediately on cheat death")
{
    GIVEN {
        // Make damage large enough that turn 2 would normally KO if not snapped.
        PLAYER(SPECIES_REGISTEEL) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(500); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); }

    } WHEN {
        // Turn 1: trigger Bug Space transformation + start kazuradropGuts window.
        WHEN {
    TURN { MOVE(player, MOVE_PETAL_BLIZZARD); MOVE(opponent, MOVE_INFINITE_GROWTH); 
    

    }
        

        }
         // Turn 2: use damage that should KO; Kazuradrop Guts should snap HP to maxHP/2 instead.
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD);
             MOVE(opponent, MOVE_TRASH_CRUSH); // dummy move to keep the turn flow going while we test the snap
        }
    } SCENE {
    } THEN {
        u32 snapHp = gBattleMons[B_POSITION_OPPONENT_LEFT].hp;
        u32 halfMaxHp = gBattleMons[B_POSITION_OPPONENT_LEFT].maxHP / 2;
        EXPECT_EQ(snapHp, halfMaxHp);
    }
   // KNOWN_FAILING;
}

SINGLE_BATTLE_TEST("Kazuradrop Guts snaps HP bar to 50% immediately on cheat death (visual)")
{
    // Initial Kazuradrop MaxHP is 20. Transformation doubles max HP to 40.
    // Cheat-death snap sets HP to maxHP / 2 => 20.
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { Moves(MOVE_PETAL_BLIZZARD, MOVE_CELEBRATE); Attack(500); }
        OPPONENT(SPECIES_KAZURADROP) { Ability(ABILITY_BUG_SPACE); Level(5); Defense(1); SpDefense(1); MaxHP(20); HP(20); }
    } WHEN {
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD);
             MOVE(opponent, MOVE_INFINITE_GROWTH); // Moves that wont affect her HP bar.
        }
   
        TURN { MOVE(player, MOVE_PETAL_BLIZZARD); 
             MOVE(opponent, MOVE_TRASH_CRUSH); // Moves that wont hurt my bar.
        }
    } SCENE {
        HP_BAR(opponent, hp: 20);
    }
    // KNOWN_FAILING; // HP is not visually updating correctly at this exact time.
}
