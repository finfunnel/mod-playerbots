/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SWPMultipliers.h"
#include "AttackAction.h"
#include "GameObject.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "Playerbots.h"
#include "ReachTargetActions.h"
#include "SWPActions.h"
#include "SWPTriggers.h"

#include "Timer.h"

float KalecgosPortalMovementMultiplier::GetValue(Action* action)
{
    // Only matters while a rift is up for this bot
    if (bot->HasAura(SPELL_SPECTRAL_REALM) || bot->HasAura(SPELL_SPECTRAL_EXHAUSTION))
        return 1.0f;

    GameObject* rift = bot->FindNearestGameObject(GO_SPECTRAL_RIFT, 30.0f);
    if (!rift)
        return 1.0f;

    // Do not let generic movement (repositioning, follow) fight the portal click-through,
    // but never block the emergency spread when inside the blast radius, and never block
    // the enter action itself (it needs to reach the 3y click range through this zone).
    if (dynamic_cast<MovementAction const*>(action) &&
        !dynamic_cast<KalecgosSpreadFromRiftAction const*>(action) &&
        !dynamic_cast<KalecgosEnterSpectralRiftAction const*>(action) &&
        bot->GetExactDist2d(rift) < 8.0f)
        return 0.0f;

    return 1.0f;
}

float KalecgosCorruptionStrikeHealMultiplier::GetValue(Action* action)
{
    Unit* sath = AI_VALUE2(Unit*, "find target", "sathrovarr");
    if (!sath)
        return 1.0f;

    bool corruptionStrike = sath->FindCurrentSpellBySpellId(SPELL_CORRUPTION_STRIKE);
    Unit* victim = sath->GetVictim();
    if (!corruptionStrike && !(victim && victim->HasAura(SPELL_CORRUPTION_STRIKE)))
        return 1.0f;

    // Healers: quiet movement so healing GCDs flow; everyone else untouched
    if (botAI->IsHeal(bot) && dynamic_cast<MovementAction const*>(action))
        return 0.0f;

    return 1.0f;
}

float KalecgosOuterDpsHoldMultiplier::GetValue(Action* action)
{
    // Only outside bots matter (inside bots must keep killing Sathrovarr)
    if (bot->HasAura(SPELL_SPECTRAL_REALM))
        return 1.0f;

    // Tank keeps the dragon stuck on him - never hold a tank
    if (botAI->IsTank(bot))
        return 1.0f;

    Unit* kalecgos = bot->FindNearestCreature(NPC_KALECGOS_DRAGON, 100.0f, true);
    if (!kalecgos || !kalecgos->HealthBelowPct(KALECGOS_BALANCE_THRESHOLD_PCT))
        return 1.0f;

    // Once Sathrovarr is banished (<=1%) the scripted kill fires and the dragon goes
    // down with him - release the hold so it gets burned out too.
    Unit* sath = bot->FindNearestCreature(NPC_SATHROVARR, 100.0f, true);
    if (!sath || !sath->IsAlive() || sath->HealthBelowPct(1))
        return 1.0f;

    // Hold: zero every action that would hit the forestalled dragon body.
    // AttackStop() alone is not enough - generic MeleeAction re-engages every tick,
    // and ranged DPS casts arrive as CastSpell/Reach* actions, not just AttackAction.
    // Same target-condition pattern as ICC's sphère hold multiplier.
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (currentTarget && currentTarget == kalecgos)
    {
        if (dynamic_cast<AttackAction const*>(action) ||
            dynamic_cast<ReachMeleeAction const*>(action) ||
            dynamic_cast<ReachSpellAction const*>(action) ||
            dynamic_cast<ReachTargetAction const*>(action) ||
            dynamic_cast<CastSpellAction const*>(action))
            return 0.0f;
    }
    else if (dynamic_cast<AttackAction const*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}

float BrutallusBloodlustMultiplier::GetValue(Action* action)
{
    // Only shaman's Bloodlust / Heroism matters here
    std::string const& name = action->getName();
    if (name != "bloodlust" && name != "heroism")
        return 1.0f;

    // Only for Brutallus
    Unit* brutallus = bot->FindNearestCreature(NPC_BRUTALLUS, 100.0f, true);
    if (!brutallus || !brutallus->IsInCombat())
        return 1.0f;

    // Hold the first 15s of the fight (soak setup / Burn spread window), then fire at
    // full priority. 10x outranks the generic 30.0-triggered use near low boss health.
    // Single-tick gate: record once when combat flips on, and let time decide.
    if (_fightStart == 0)
        _fightStart = getMSTime();
    else if (getMSTimeDiff(_fightStart, getMSTime()) < BRUTALLUS_BLOODLUST_DELAY_MS)
        return 1.0f;

    return 10.0f;
}

float MuruEntropiusRushMultiplier::GetValue(Action* action)
{
    // Only shaman's Bloodlust / Heroism matters here
    std::string const& name = action->getName();
    if (name != "bloodlust" && name != "heroism")
        return 1.0f;

    // User rule (start of fight): open Bloodlust/Heroism at the pull to burn through P1
    // (M'uru + his add waves) as fast as possible. Entropius never exists during P1
    // (scripted spawn only after M'uru dies), so "M'uru alive" is a clean P1 gate.
    Unit* entropius = bot->FindNearestCreature(NPC_ENTROPIUS, 100.0f, true);
    if (entropius)
        return 1.0f; // P2 already: lust was spent at the pull, let it cool down / ride out

    Unit* muru = bot->FindNearestCreature(NPC_MURU, 100.0f, true);
    if (muru && muru->IsAlive())
        return 10.0f; // P1: fire it immediately to speed through the adds and the boss

    // Not in the M'uru fight: stay neutral
    return 1.0f;
}

float KiljaedenP3BloodlustMultiplier::GetValue(Action* action)
{
    // Only shaman's Bloodlust / Heroism matters here
    std::string const& name = action->getName();
    if (name != "bloodlust" && name != "heroism")
        return 1.0f;

    // P1 (Hand of the Deceiver adds, boss body not yet up): hold the lust - do not spend
    // it on the hands. The body only spawns once all three hands die.
    Creature* kiljaeden = bot->FindNearestCreature(NPC_KILJAEDEN_SWP, 100.0f, true);
    if (!kiljaeden || !kiljaeden->IsAlive())
        return 0.0f;

    // P2 body phase (100% -> 85%): still hold - it is the lowest-pressure window (no
    // Thousand Souls yet; just Soul Flay / Lightning / the first Shield Orb).
    if (kiljaeden->HealthAbovePct(85))
        return 0.0f;

    // P3 (<=85%): Darkness of a Thousand Souls enters rotation, shield orbs double up and
    // Sinister Reflection follows - the first real pressure spike. Bloodlust/Heroism 40s
    // covers exactly this window, pushing the raid through it (user rule: P3开嗜血).
    return 10.0f;
}

float KalecgosSpectralBloodlustMultiplier::GetValue(Action* action)
{
    // Only shaman's Bloodlust / Heroism matters here
    std::string const& name = action->getName();
    if (name != "bloodlust" && name != "heroism")
        return 1.0f;

    // Only for Kalecgos. If Entropius/M'uru is up this fight has its own rush logic
    // (MuruEntropiusRushMultiplier) and no spectral realm - stay neutral.
    Unit* kalecgos = bot->FindNearestCreature(NPC_KALECGOS_DRAGON, 100.0f, true);
    if (!kalecgos)
        return 1.0f;

    // Count raid members currently in the spectral realm. "Inside" = has the realm
    // aura. The user rule: open once >= 10 people are through the portal. Any shaman
    // (outer or inner) fires it - Bloodlust/Heroism covers the whole raid through the
    // transmogrified buff, so whoever lands the trigger grants it everywhere.
    Group* group = bot->GetGroup();
    if (!group)
        return 1.0f;

    uint32 inside = 0;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member->IsAlive() && member->HasAura(SPELL_SPECTRAL_REALM))
            ++inside;
    }

    if (inside < KALECGOS_BLOODLUST_INSIDE_COUNT)
        return 1.0f;

    // Threshold met: fire Bloodlust/Heroism immediately (10x priority vs normal use,
    // which only fires near low boss health anyway).
    return 10.0f;
}

float FelmystAirborneDpsHoldMultiplier::GetValue(Action* action)
{
    // Only applies while Felmyst is airborne (P2 flight phase). isTargetableForAttack() does
    // NOT work here: P2 the dragon is SetInvincibility(true) (a private BossAI flag) but keeps
    // no NON_ATTACKABLE/NOT_SELECTABLE unit flags, so isTargetableForAttack() stays true. The
    // reliable signal is IsFlying() (MOVEMENTFLAG_DISABLE_GRAVITY, set on takeoff, cleared on
    // landing in boss_felmyst.cpp).
    Unit* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    if (!felmyst || !felmyst->IsFlying())
        return 1.0f;

    // Crush the priority of every attack-type action aimed at the dragon itself - down to an
    // almost-zero (but non-zero) relevance. The user rule: P2 打boss优先级最低 - the boss is
    // 1-hit-locked and not worth GCDs, but if there is literally nothing else to do a bot may
    // still poke it. Zeroing (0.0f) would hard-ban it; 0.01 keeps it at the very bottom of the
    // queue so dodging / skeleton DPS / healing always win, and only absolute idle feeds it.
    // Attacks on other targets (Blazing Dead skeletons, a charmed ally) keep their relevance.
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (currentTarget && currentTarget == felmyst)
    {
        if (dynamic_cast<AttackAction const*>(action) ||
            dynamic_cast<ReachMeleeAction const*>(action) ||
            dynamic_cast<ReachSpellAction const*>(action) ||
            dynamic_cast<ReachTargetAction const*>(action) ||
            dynamic_cast<CastSpellAction const*>(action))
            return 0.01f;
    }

    return 1.0f;
}

float FelmystPostBreathFreezeMultiplier::GetValue(Action* action)
{
    uint32 instance = bot->GetInstanceId();

    // The breath is signalled by SPEED_BURST (45495) on the dragon: cast when the sweep
    // starts (boss_felmyst.cpp POINT_LANE +5s) and removed when it finishes crossing
    // (POINT_AIR_BREATH_END). We suppress movements in two windows:
    //   1) BREATH GUIDING (45495 up): "stop what you're doing and dodge" - every normal
    //      movement is shelved so the shelter/escape actions own the bot's legs. Skeleton
    //      melee DPS and healing casts (non-movement) still run.
    //   2) POST-BREATH 10s (45495 just dropped): lingering fog is lethal, stand still, keep
    //      casting (skeleton DPS/heals) - the original freeze window.
    Unit* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    bool const p2 = felmyst && felmyst->IsAlive() && felmyst->IsFlying();
    bool const breathingNow = p2 && felmyst->HasAura(SPELL_FELMYST_SPEED_BURST);

    bool wasBreathing = _wasBreathing[instance];
    _wasBreathing[instance] = breathingNow;

    if (breathingNow)
    {
        // Breath is being guided NOW: STOP everything that moves and do not chase skeletons -
        // the priority is purely dodging (shelter/escape outrank this). Only casts/heals pass.
        _breathEndTimer.erase(instance);
        return FreezeIfGenericMovement(action, /*allowSkeletonDps=*/false);
    }

    if (wasBreathing && !breathingNow)
    {
        // Edge: breath just finished -> start the 10s stand-still window.
        _breathEndTimer[instance] = getMSTime();
    }

    auto it = _breathEndTimer.find(instance);
    if (!p2 || it == _breathEndTimer.end())
        return 1.0f;

    if (getMSTimeDiff(it->second, getMSTime()) > FELMYST_POST_BREATH_FREEZE_MS)
    {
        _breathEndTimer.erase(instance);
        return 1.0f;
    }

    // Post-breath: still no wandering, but skeleton DPS from the safe zone is encouraged.
    return FreezeIfGenericMovement(action, /*allowSkeletonDps=*/true);
}

float FelmystPostBreathFreezeMultiplier::FreezeIfGenericMovement(Action* action, bool allowSkeletonDps)
{
    // Emergency escapes are ALWAYS allowed - standing still while poison/fog is under you
    // means death, and those runs must keep working even inside the freeze.
    if (dynamic_cast<FelmystVaporTrailEscapeAction const*>(action) ||
        dynamic_cast<FelmystVaporKiteAction const*>(action) ||
        dynamic_cast<FelmystDeepBreathAvoidAction const*>(action))
        return 1.0f;

    // Post-breath in the safe zone: thin the skeletons (user: 到安全区后专心打小怪). During
    // the breath guide window this is NOT exempt - nobody chases skeletons mid-sweep.
    if (allowSkeletonDps)
    {
        if (dynamic_cast<FelmystBlazingDeadAttackAction const*>(action) ||
            dynamic_cast<FelmystBlazingDeadPickupAction const*>(action))
            return 1.0f;
    }

    // Everything else that involves moving is silenced; casts/heals are not MovementAction so
    // they pass through untouched (the "stand still but keep casting" requirement).
    if (dynamic_cast<MovementAction const*>(action))
        return 0.0f;

    return 1.0f;
}
