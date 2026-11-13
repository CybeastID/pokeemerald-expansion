#ifndef GUARD_BATTLE_SCRIPT_NATIVE_H
#define GUARD_BATTLE_SCRIPT_NATIVE_H

// Match the target bytecode's packed callnative argument layout. Custom
// modules share this helper without exporting the engine's command table.
#define FAERIE_CMD_ARGS(...) const struct __attribute__((packed)) { u8 opcode; RECURSIVELY(R_FOR_EACH(APPEND_SEMICOLON, __VA_ARGS__)) const u8 nextInstr[0]; } *const cmd UNUSED = (const void *)gBattlescriptCurrInstr
#define NATIVE_ARGS(...) FAERIE_CMD_ARGS(void (*func)(void), ##__VA_ARGS__)

#endif // GUARD_BATTLE_SCRIPT_NATIVE_H
