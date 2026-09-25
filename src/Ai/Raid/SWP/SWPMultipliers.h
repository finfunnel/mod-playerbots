/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_SWPMULTIPLIERS_H
#define PLAYERBOTS_SWPMULTIPLIERS_H

#include "Multiplier.h"

#include <unordered_map>

// While moving to/into a Specteral Rift, keep other movement actions from fighting it,
// except the emergency spread when standing in the blast radius.
class KalecgosPortalMovementMultiplier : public Multiplier
{
public:
    KalecgosPortalMovementMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kalecgos portal movement") {}
    float GetValue(Action* action) override;
};

// During Corruption Strike (incoming or landed on the spectral tank), boost healing actions
// on the victim and damp unnecessary movement for healers inside.
class KalecgosCorruptionStrikeHealMultiplier : public Multiplier
{
public:
    KalecgosCorruptionStrikeHealMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kalecgos corruption strike heal") {}
    float GetValue(Action* action) override;
};

// Outer-realm DPS hold: once the dragon ≤ 30% (and Sathrovarr alive > 1%), every
// AttackAction for an OUTSIDE non-tank is multiplied to 0. AttackStop() alone cannot
// hold the bots - the generic MeleeAction/AttackAction re-engages every tick, so the
// dragon keeps getting hit. Zeroing the action at the source is the only reliable stop
// (same pattern as Kara/BT/Gruul encounter multipliers).
class KalecgosOuterDpsHoldMultiplier : public Multiplier
{
public:
    KalecgosOuterDpsHoldMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kalecgos outer dps hold") {}
    float GetValue(Action* action) override;
};

// Brutallus: hold the shaman Bloodlust / Heroism for the first 15 seconds of the fight,
// then fire it at full priority. The opening window is soak setup + Burn spreading - the
// raid is not yet stacked on the boss, and a Bloodlust wasted on the pull is 40s of lost
// haste exactly when the tank-swap ladder starts getting tense. After 15s the camps are
// set, so the lust rides the real DPS window (guide: 6-min berserk is a pure DPS check).
class BrutallusBloodlustMultiplier : public Multiplier
{
public:
    BrutallusBloodlustMultiplier(PlayerbotAI* ai) : Multiplier(ai, "brutallus bloodlust") {}
    float GetValue(Action* action) override;

private:
    static constexpr uint32 BRUTALLUS_BLOODLUST_DELAY_MS = 15000; // hold first 15s of combat
    uint32 _fightStart = 0; // getMSTime() when Brutallus first entered combat (0 = not yet)
};

// M'uru P1 rush (user rule): as soon as the fight starts and M'uru is alive (P1),
// boost the shaman Bloodlust / Heroism so it fires at the pull and burns through the
// add waves + boss quickly. Once Entropius appears (P2) the lust is already on CD from
// the pull, so the multiplier goes neutral.
class MuruEntropiusRushMultiplier : public Multiplier
{
public:
    MuruEntropiusRushMultiplier(PlayerbotAI* ai) : Multiplier(ai, "muru entropius rush") {}
    float GetValue(Action* action) override;
};

// Kil'jaeden P3 rush (user rule): hold the shaman Bloodlust / Heroism while the fight is still
// in P1 (Hand of the Deceiver adds 25588) so it is not wasted on the hands, and through the easy
// P2 opener (100% -> 85%, before Thousand Souls). The moment the boss body is at 85% or below
// (P3: Darkness of a Thousand Souls enters the rotation, orbs double up, Sinister Reflection
// starts) boost the spell so the whole raid lusts through the first real pressure spike.
class KiljaedenP3BloodlustMultiplier : public Multiplier
{
public:
    KiljaedenP3BloodlustMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kiljaeden p3 bloodlust") {}
    float GetValue(Action* action) override;
};

// Kalecgos inner-realm burn (user rule): the OUTSIDE shaman opens Bloodlust/Heroism
// once at least 10 raid members are inside the spectral realm (Sathrovarr burn window).
// Triggered from the OUTER side so the buff covers everyone crossing the portal for the
// inside fight; the inside shaman would waste it on the short portal trip. Not active
// on M'uru/Entropius (that fight has its own rush multiplier and no spectral realm).
class KalecgosSpectralBloodlustMultiplier : public Multiplier
{
public:
    KalecgosSpectralBloodlustMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kalecgos spectral bloodlust") {}
    float GetValue(Action* action) override;
};

// Felmyst flight phase (P2): attacking the dragon is the LOWEST priority (it is 1-hit-locked
// in the air and not worth GCDs). Every attack-type action aimed at the boss gets its
// relevance crushed to ~0 so dodging / skeleton DPS / healing always win; only true idle time
// can spend a GCD on the boss. Attacks on skeletons / charmed allies keep full relevance.
class FelmystAirborneDpsHoldMultiplier : public Multiplier
{
public:
    FelmystAirborneDpsHoldMultiplier(PlayerbotAI* ai) : Multiplier(ai, "felmyst airborne dps hold") {}
    float GetValue(Action* action) override;
};

// Felmyst flight phase, right after a breath: for 10 seconds after the breath's SPEED_BURST
// (45495) aura disappears from the dragon (breath finished + it crossed to the next lane), the
// raid stands still - the lingering fog is still lethal, so everyone just keeps casting
// (skeleton DPS, healing) instead of running anywhere. Zero ALL generic MovementActions;
// spells/heals (non-movement) are untouched. Emergency dodges always stay allowed.
class FelmystPostBreathFreezeMultiplier : public Multiplier
{
public:
    FelmystPostBreathFreezeMultiplier(PlayerbotAI* ai) : Multiplier(ai, "felmyst post-breath freeze") {}
    float GetValue(Action* action) override;

private:
    // Suppress generic MovementActions but keep emergency escapes + casts/heals running.
    // allowSkeletonDps: during the 10s post-breath freeze (safe zone) skeleton DPS/pickup are
    // exempt so the raid thins the pack; while the breath is being guided they are NOT exempt.
    float FreezeIfGenericMovement(Action* action, bool allowSkeletonDps);

    std::unordered_map<uint32, uint32> _breathEndTimer; // instance -> getMSTime() when a breath ended
    std::unordered_map<uint32, bool> _wasBreathing;     // instance -> whether 45495 was up last tick
};

#endif
