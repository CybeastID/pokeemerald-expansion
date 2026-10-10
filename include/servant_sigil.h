#ifndef GUARD_SERVANT_SIGIL_H
#define GUARD_SERVANT_SIGIL_H

// Servant-class sigils (Kazuradrop "Alter Ego", Castoria "Caster").
// Cosmetic chrome that rides the healthbox — created on send-out, not tied
// to any battle-script state. See src/servant_sigil.c.

void InitServantSigils(void);              // per-battle reset of the sprite-id array
void CreateServantSigil(u32 battler);      // spawn; no-ops unless the mon has a sigil
void DestroyServantSigil(u32 battler);     // one battler
void DestroyAllServantSigils(void);        // battle end / faint sweep
void ReshowServantSigils(void);            // menu return — reset + recreate all valid slots
void FreeServantSigilGfx(void);            // free all sigil tile/pal tags
bool32 HasServantSigil(u16 species);       // which species carry a sigil

#endif // GUARD_SERVANT_SIGIL_H
