/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SWPTriggers.h"
#include "AiObjectContext.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Playerbots.h"
#include "Spell.h"

#include <cmath>

namespace
{
// Spectral Rift portals live ~15s (smart_scripts despawn) and spawn on the teleported
// player's position; 60y covers bots spread to the room edges.
float const SPECTRAL_RIFT_SEARCH_RANGE = 60.0f;
// Guide: keep away from the rift spawn point; the teleported player blasts ~6000 dmg within 5 yards.
float const RIFT_BLAST_RADIUS = 5.0f;
float const RIFT_SAFE_DISTANCE = 8.0f;

// Boss lookups by creature entry. "find target" only sees units the bot itself has threat on
// (TargetValue.cpp GetThreatenedByMeList), which blinds healers and freshly-swapped bots;
// the entry-based lookup works for every bot.
Creature* FindSunwellBoss(Player* bot, uint32 entry, float maxRange = 100.0f)
{
    return bot->FindNearestCreature(entry, maxRange, true);
}

// Sathrovarr lives in the spectral realm (Z < 50); Kalecgos' dragon body stays outside (Z > 50).
Creature* FindSathrovarr(Player* bot)
{
    Creature* creature = bot->FindNearestCreature(NPC_SATHROVARR, 100.0f, true);
    return (creature && creature->GetPositionZ() < 50.0f) ? creature : nullptr;
}

Creature* FindKalecgosDragon(Player* bot)
{
    Creature* creature = bot->FindNearestCreature(NPC_KALECGOS_DRAGON, 100.0f, true);
    return (creature && creature->GetPositionZ() > 50.0f) ? creature : nullptr;
}

// Rift GOs despawn into the spectral floor (z < 50) at phase end but stay in the
// grid; grid searches ignore z, so an outer bot would chase a gone portal.
// Outer-realm users must only see rifts parked on the outer floor (z > 50).
GameObject* FindOuterRift(Player* bot)
{
    GameObject* rift = bot->FindNearestGameObject(GO_SPECTRAL_RIFT, SPECTRAL_RIFT_SEARCH_RANGE);
    return (rift && rift->GetPositionZ() > 50.0f) ? rift : nullptr;
}
} // namespace

// ---- Felmyst flight-phase lane helpers (see SunwellFelmystIDs in SWPTriggers.h).
// Out-of-namespace on purpose: the breath-avoid action in SWPActions.cpp calls them too,
// so they need external linkage (declared in SWPTriggers.h). ----

// X centre of a breath lane. Lane index order matches boss_felmyst.cpp: 0=top, 1=middle, 2=bottom.
float FelmystLaneX(int lane)
{
    switch (lane)
    {
        case 0: return static_cast<float>(FELMYST_LANE_X_TOP);
        case 1: return static_cast<float>(FELMYST_LANE_X_MID);
        default: return static_cast<float>(FELMYST_LANE_X_BOT);
    }
}

// Which lane the dragon is currently sweeping, judged from its own position (X + Y gate).
// boss_felmyst.cpp flies straight along one lane from north to south (or back), so while it
// breathes the dragon's X is always near one lane centre AND its Y is inside the lane corridor
// (515..704). The reposition corners are OUTSIDE that Y range (LeftSide 1469,729 / RightSide
// 1458,502) but their X ≈ MID lane - the Y gate is what prevents a false "MID swept" at corners.
// Returns -1 if the dragon is airborne but not actively on a lane (repositioning, takeoff,
// landing or the ground phase).
int FelmystCurrentLane(Player* bot)
{
    Unit* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    if (!felmyst || !felmyst->IsAlive() || !felmyst->IsFlying())
        return -1;

    // Lane corridor bounds from boss_felmyst.cpp: LeftSideLanes Y~701-704 (north)
    // RightSideLanes Y~515-520 (south). Reposition corners are outside these.
    float const y = felmyst->GetPositionY();
    if (y > 515.0f && y < 704.0f)
    {
        float const x = felmyst->GetPositionX();
        if (std::fabs(x - static_cast<float>(FELMYST_LANE_X_TOP)) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
            return 0;
        if (std::fabs(x - static_cast<float>(FELMYST_LANE_X_MID)) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
            return 1;
        if (std::fabs(x - static_cast<float>(FELMYST_LANE_X_BOT)) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
            return 2;
    }
    return -1;
}

// The safe (non-swept) lane to run to when THIS lane is being breathed on, chosen as the
// farthest of the two remaining lanes from the bot's CURRENT X. Keeps the bot decisive: once it
// commits to the far side it runs straight there (no back-and-forth as the dragon sweeps).
int FelmystSafeLane(Player* bot, int sweptLane)
{
    // The other two lanes; prefer the one farthest from the bot so a fog-strip edge cannot clip.
    int candidateA = -1, candidateB = -1;
    for (int lane = 0; lane < 3; ++lane)
    {
        if (lane != sweptLane)
        {
            if (candidateA < 0) candidateA = lane;
            else                 candidateB = lane;
        }
    }
    float const distA = std::fabs(bot->GetPositionX() - FelmystLaneX(candidateA));
    float const distB = std::fabs(bot->GetPositionX() - FelmystLaneX(candidateB));
    return (distA >= distB) ? candidateA : candidateB;
}

// Lateral (EAST-WEST) flee target away from a hazard, used by the vapor-trail escape and the
// eye-beam kite (user: 东西方向引, 不穿场南北). Uses the BOT's X, not the hazard's: push the bot
// `distance` further away from the hazard along X, keeping the bot's CURRENT Y. North/south
// movement is what drags poison across the raid or off the arena, so this never changes Y.
bool FelmystLateralEscapePoint(Player* bot, Unit* hazard, float distance, float& outX, float& outY)
{
    outY = bot->GetPositionY(); // keep our current Y - never move north/south

    float const botX = bot->GetPositionX();
    float const hazardX = hazard ? hazard->GetPositionX() : botX;
    // Away from the hazard along X: if we are east of it keep moving east, west -> west.
    outX = (botX >= hazardX) ? botX + distance : botX - distance;
    return true;
}

bool KalecgosSpectralRiftAvailableTrigger::IsActiveInEncounter()
{
    // Inside already or locked out by Spectral Exhaustion -> cannot use a portal
    if (bot->HasAura(SPELL_SPECTRAL_REALM) || bot->HasAura(SPELL_SPECTRAL_EXHAUSTION))
        return false;

    GameObject* rift = FindOuterRift(bot);
    if (!rift)
        return false;

    // User entry-order rules (simplified from live feedback: bots did not click the
    // portal because DPS was gated behind a 1 tank + 3 healer foundation and healers
    // could not all reach it. New rules):
    //   1. A tank goes in FIRST (Sathrovarr needs someone to hit). If only one tank
    //      is alive it stays out on the dragon - two tanks split one in / one out.
    //   2. Healers enter up to the cap (rest stay outside healing the dragon team).
    //   3. Everyone else (DPS) enters freely; the ONLY hard cap is the inside
    //      headcount ceiling below, and a Spectral-Exhaustion victim cannot enter.
    Group* group = bot->GetGroup();
    if (!group)
        return true; // solo bot: just enter

    uint32 inside = 0, insideTanks = 0, insideHealers = 0;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || !member->IsAlive() || !member->HasAura(SPELL_SPECTRAL_REALM))
            continue;

        ++inside;
        if (PlayerbotAI::IsTank(member))
            ++insideTanks;
        else if (PlayerbotAI::IsHeal(member))
            ++insideHealers;
    }

    // Count living raid (both realms) for the inside cap.
    uint32 alive = 0;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member->IsAlive())
            ++alive;
    }
    uint32 const targetInside = std::min<uint32>(
        alive * KALECGOS_MAX_INSIDE_FRACTION_NUM / KALECGOS_MAX_INSIDE_FRACTION_DEN, 15u);

    // Cap the inside team at ~half the living raid (both realms need damage).
    if (inside >= targetInside)
        return false;

    // Healer cap: no more healers inside than the configured ceiling (the rest must
    // stay out healing the dragon-tank team in the outer realm).
    if (botAI->IsHeal(bot) && insideHealers >= KALECGOS_MAX_INSIDE_HEALERS)
        return false;

    // Tank cap: one tank in is enough for Sathrovarr - unless only ONE tank is alive
    // in the whole raid, in which case that tank must stay OUT on the dragon body
    // (a lone tank going in leaves the invincible outer boss non-priority and the
    // dragon off-tank kills the raid). Two+ tanks: second one may enter.
    if (botAI->IsTank(bot))
    {
        if (insideTanks >= 1)
            return false;

        uint32 outerTanks = 0;
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (member && member->IsAlive() && !member->HasAura(SPELL_SPECTRAL_REALM) &&
                PlayerbotAI::IsTank(member))
                ++outerTanks;
        }
        if (outerTanks >= 2)
            return true;          // second tank is free to enter
        return false;             // this is (or would be) the only outer tank: stay
    }

    // DPS / everyone else: enter freely while the inside cap has headroom.
    return true;
}

bool KalecgosInSpectralRealmTrigger::IsActiveInEncounter()
{
    return bot->HasAura(SPELL_SPECTRAL_REALM);
}

bool KalecgosTooCloseToRiftTrigger::IsActiveInEncounter()
{
    // Only an outer-realm concern
    if (bot->HasAura(SPELL_SPECTRAL_REALM))
        return false;

    // Bots cleared to enter are SUPPOSED to stand on the portal - the ~6000 blast
    // already happened when Spectral Blast teleported its victim away.
    if (KalecgosSpectralRiftAvailableTrigger(botAI).IsActive())
        return false;

    GameObject* rift = bot->FindNearestGameObject(GO_SPECTRAL_RIFT, SPECTRAL_RIFT_SEARCH_RANGE);
    return rift && bot->GetExactDist2d(rift) < RIFT_SAFE_DISTANCE;
}

bool KalecgosDragonRacingAheadTrigger::IsActiveInEncounter()
{
    // User rule (rewritten): OUTSIDE burns the dragon down to 30% and then STOPS.
    // The outer boss is invincible on this server (SetInvincibility) and can only die
    // via the scripted kill once Sathrovarr (inside) is banished at 1%. So the correct
    // pace is: dragon to 30% -> outside DPS holds permanently until the inside kills
    // Sathrovarr -> then both get burned out together. Only the tank keeps swinging so
    // the dragon stays on him (never lets the invincible body run off killing the raid).
    if (bot->HasAura(SPELL_SPECTRAL_REALM))
        return false;

    Unit* kalecgos = FindKalecgosDragon(bot);
    if (!kalecgos)
        return false;

    // Not yet at the hold line: keep fighting normally.
    if (!kalecgos->HealthBelowPct(KALECGOS_BALANCE_THRESHOLD_PCT))
        return false;

    // Hold ENDS once Sathrovarr is banished (<=1% -> the boss kill is scripted):
    // then the dragon goes down too and everyone outside resumes full burn.
    if (Unit* sath = FindSathrovarr(bot))
        if (!sath->IsAlive() || sath->HealthBelowPct(1))
            return false;

    // Tank never holds - the dragon must stay attacked (and stuck on a victim).
    return !botAI->IsTank(bot);
}

bool KalecgosSathrovarrLaggingTrigger::IsActiveInEncounter()
{
    // REMOVED. With the new pace the INSIDE never holds: it burns Sathrovarr down and
    // kills him (>=1% enrage both bosses), and the OUTSIDE waits at 30% for that kill.
    // Holding inside too would deadlock - the drain needs one side to finish.
    return false;
}

// ---- Brutallus ----

bool BrutallusBurnOnSelfTrigger::IsActiveInEncounter()
{
    return bot->HasAura(SPELL_BURN_DAMAGE);
}

bool BrutallusBurnNearbyTrigger::IsActiveInEncounter()
{
    if (bot->HasAura(SPELL_BURN_DAMAGE))
        return false; // own spread action covers this bot

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member != bot && member->IsAlive() &&
            member->HasAura(SPELL_BURN_DAMAGE) &&
            bot->GetExactDist2d(member) < static_cast<float>(BRUTALLUS_BURN_SPREAD_RANGE) + 2.0f)
            return true;
    }

    return false;
}

bool BrutallusTankSwapTrigger::IsActiveInEncounter()
{
    // Only tank bots care
    if (!botAI->IsTank(bot))
        return false;

    Unit* brutallus = AI_VALUE2(Unit*, "find target", "brutallus");
    if (!brutallus || !brutallus->isTargetableForAttack())
        return false;

    Unit* victim = brutallus->GetVictim();
    if (!victim || !victim->IsAlive())
        return false;

    if (victim == bot)
        return false; // I am tanking; someone else's job to swap

    // This bot is the one about to take over: do NOT taunt while still carrying the
    // old Meteor Slash vulnerability. The debuff lasts 30s and lingers across swaps,
    // so an immediate re-negotiation just adds the new stack on top of the residue -
    // the stacks ladder up forever. Wait until my own stacks have FULLY expired so the
    // new tank starts clean and the old tank gets recovery time (guide: 给别的坦恢复时间).
    if (bot->HasAura(SPELL_METEOR_SLASH_SWP))
        return false;

    // NEVER swap mid-cast: Meteor Slash resolves on the current victim, a taunt now
    // flips the cone onto this (unpositioned) tank who eats ~20000 alone and dies
    // (user: 流星会秒人). Wait for the cast to land first. Stomp gets the same
    // protection - it shares the boss's cast channel, so taunting during a Stomp
    // can eat an immediately following Meteor Slash with no soak camp behind him.
    if (brutallus->FindCurrentSpellBySpellId(SPELL_METEOR_SLASH_SWP) ||
        brutallus->FindCurrentSpellBySpellId(SPELL_STOMP))
        return false;

    // Taunt ONLY from the boss's back (user: tank要注意boss的朝向). His facing carries
    // the 120-degree cone: a taunt from the side flips the cone sideways across the raid
    // mid-transit and slashes camps nobody set up to soak. GetRelativeAngle is measured
    // against his facing - the cone's own frame (Position.h) - so >= 120 degrees puts the
    // taunter inside the rear 60-degree sector and the flip lands as a clean 180 onto his
    // own camp (which is already stacked there with him).
    if (brutallus->GetRelativeAngle(bot) < 2.0f * static_cast<float>(M_PI) / 3.0f)
        return false;

    // Swap when the active tank's slash vulnerability reaches the stack threshold
    // (stack idiom from FankrissMortalWoundStacksTrigger) or he is Stomped.
    if (victim->HasAura(SPELL_STOMP))
        return true;

    if (Aura* slash = victim->GetAura(SPELL_METEOR_SLASH_SWP))
        return slash->GetStackAmount() >= BRUTALLUS_SLASH_SWAP_STACKS;

    return false;
}

bool BrutallusSoakPositionTrigger::IsActiveInEncounter()
{
    // Healers must keep raid-wide range, they soak by default engine placement near
    // the camps. Everyone else belongs to a camp. The ACTIVE tank holds the cone
    // where he stands; every other tank joins too: an off-duty tank idling anywhere
    // in front keeps stacking the Meteor Slash fire vulnerability on himself (the
    // cone is 120 degrees wide and 65 yards long on this server, spell_cone table),
    // which blocks his taunt turn - the swap trigger waits for the debuff to expire,
    // so the rotation would stall and the active tank would ladder stacks forever.
    if (botAI->IsHeal(bot))
        return false;

    Unit* brutallus = AI_VALUE2(Unit*, "find target", "brutallus");
    if (!brutallus || !brutallus->isTargetableForAttack())
        return false;

    if (botAI->IsTank(bot))
        return brutallus->GetVictim() != bot; // off-duty tank: wait behind the boss

    // Only while the raid is positioned for a slash cycle: always true in practice,
    // gating kept so the trigger dies the moment the fight ends.
    return true;
}

// ---- Felmyst ----

bool FelmystEncapsulateNearbyTrigger::IsActiveInEncounter()
{
    Unit* felmyst = AI_VALUE2(Unit*, "find target", "felmyst");
    if (!felmyst)
        return false;

    // The Encapsulate channel (45661) is up on someone near this bot
    Spell* channel = felmyst->FindCurrentSpellBySpellId(SPELL_FELMYST_ENCAPSULATE_CHANNEL);
    if (!channel)
        return false;

    Unit* encapsTarget = channel->m_targets.GetUnitTarget();
    if (!encapsTarget || !encapsTarget->IsAlive())
        return false;

    return bot->GetExactDist2d(encapsTarget) < static_cast<float>(FELMYST_ENCAPSULATE_SPREAD_RANGE);
}

bool FelmystVaporChasingTrigger::IsActiveInEncounter()
{
    // P2 only: Felmyst airborne. IMPORTANT: use IsFlying(), not isTargetableForAttack() -
    // during P2 the dragon keeps no NON_ATTACKABLE flag (core SetInvincibility is a private
    // bool), so isTargetableForAttack() stays TRUE and would defeat this trigger entirely.
    Unit* felmyst = AI_VALUE2(Unit*, "find target", "felmyst");
    if (felmyst && !felmyst->IsFlying())
        return false;

    // The orb follows its victim closely; being within a few yards means it is on us
    return bot->FindNearestCreature(NPC_DEMONIC_VAPOR, 5.0f, true) != nullptr;
}

bool FelmystVaporTrailNearbyTrigger::IsActiveInEncounter()
{
    if (bot->HasAura(SPELL_DEMONIC_VAPOR_DOT))
        return true;

    // React well before the bot walks into the poisonous cloud. The trail zones are large
    // (they cover a green swath where the beam passed); 12 yards give us time to steer clear.
    return bot->FindNearestCreature(NPC_DEMONIC_VAPOR_TRAIL, 12.0f, true) != nullptr;
}

bool FelmystCorrosionTrigger::IsActiveInEncounter()
{
    // Only classes that can remove a MAGIC debuff from a friendly unit (TBC: mage/druid cannot).
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_PALADIN:
            break;
        default:
            return false;
    }

    Creature* felmyst = FindSunwellBoss(bot, NPC_FELMYST);
    if (!felmyst || !felmyst->IsAlive())
        return false;

    // Find the main tank (the one Felmyst is hitting) and check for Corrosion on him.
    Unit* tank = felmyst->GetVictim();
    if (!tank || !tank->IsAlive())
        return false;

    if (!tank->HasAura(SPELL_FELMYST_CORROSION))
        return false;

    // Only answer when we can actually reach and dispel him.
    return bot->GetExactDist2d(tank) <= static_cast<float>(FELMYST_CORROSION_DISPEL_RANGE);
}

bool FelmystGasNovaOnSelfTrigger::IsActiveInEncounter()
{
    // Explicit, raid-aware node for the Gas Nova magic debuff. The default class cure
    // strategies already deal with it; this gives the fight a labelled hook that reacts
    // even when the generic cure prioritises something else. TBC: only priest/paladin/
    // shaman can dispel magic from a friendly unit.
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_PALADIN:
            break;
        default:
            return false;
    }

    return bot->HasAura(SPELL_FELMYST_GAS_NOVA);
}

bool FelmystDeepBreathTrigger::IsActiveInEncounter()
{
    // DYNAMIC LANE DODGE (user: 看龙在哪一条 lane 就是喷哪边). Active only while the dragon is
    // sweeping the lane THIS bot currently stands inside. P2 flight signal is IsFlying() -
    // isTargetableForAttack() stays TRUE during P2 (SetInvincibility is a private bool, so no
    // NON_ATTACKABLE flag). When the dragon sweeps a different lane, or is repositioning between
    // lanes (FelmystCurrentLane < 0), there is nothing to dodge and we hold position.
    Unit* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    if (!felmyst || !felmyst->IsAlive() || !felmyst->IsFlying())
        return false;

    int const currentLane = FelmystCurrentLane(bot);
    if (currentLane < 0)
        return false;   // no lane is actively being breathed on (repositioning / approaching)

    // Only react when THIS bot stands inside the swept lane.
    return std::fabs(bot->GetPositionX() - FelmystLaneX(currentLane)) <= static_cast<float>(FELMYST_LANE_TOLERANCE);
}

bool FelmystBlazingDeadNearbyTrigger::IsActiveInEncounter()
{
    // User: 出小骷髅, 除了被点名的,其他人优先击杀小骷髅, 绝不出北边.
    // Tanks pick up within close range; RANGED attack far (60y); MELEE only what is already
    // within attack reach so no one is ever tempted to chase south. All non-tanks share this
    // trigger; the strategy picks the action.
    // No IsInCombat() gate: freshly-summoned skeletons are still burning kills.
    float radius = 60.0f;
    if (botAI->IsTank(bot))
        radius = static_cast<float>(FELMYST_BLAZING_DEAD_PICKUP_RANGE);
    else if (botAI->IsMelee(bot))
        radius = static_cast<float>(FELMYST_BLAZING_DEAD_PICKUP_RANGE); // melee only hits the nearby pack

    Creature* skeleton = bot->FindNearestCreature(NPC_BLAZING_DEAD, radius, true);
    if (!skeleton || !skeleton->IsAlive())
        return false;

    // Non-tanks only act on skeletons within range - the exact reach check lives in the action.
    if (!botAI->IsTank(bot))
        return bot->GetExactDist2d(skeleton) <= radius;

    return true;
}

bool FelmystCharmedAllyNearbyTrigger::IsActiveInEncounter()
{
    // Scan the raid for an ally carrying the Fog of Corruption charm (45717). That ally is
    // mind-controlled (and will die when the charm drops). We must kill them if the charm
    // turns them hostile, or at minimum keep clear of them.
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        if (member->HasAura(SPELL_FELMYST_FOG_CHARM))
            return true;
    }
    return false;
}

// ---- Eredar Twins ----

bool TwinsConflagNearbyTrigger::IsActiveInEncounter()
{
    // The fire sister channels/casts Conflagration (3s cast, AoE around the target)
    Creature* alythess = FindSunwellBoss(bot, NPC_ALYTHESS);
    if (!alythess)
        return false;

    Spell* cast = alythess->FindCurrentSpellBySpellId(SPELL_CONFLAGRATION_TWINS);
    if (!cast)
        return false;

    Unit* target = cast->m_targets.GetUnitTarget();
    if (!target || !target->IsAlive())
        return false;

    return bot->GetExactDist2d(target) < static_cast<float>(TWINS_CONFLAG_SPREAD_RANGE) + 2.0f;
}

bool TwinsPyrogenicsTrigger::IsActiveInEncounter()
{
    // Only classes that can actually remove a magic buff from an enemy react
    switch (bot->getClass())
    {
        case CLASS_MAGE:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            break;
        default:
            return false;
    }

    Creature* alythess = FindSunwellBoss(bot, NPC_ALYTHESS);
    return alythess && alythess->HasAura(SPELL_PYROGENICS);
}

bool TwinsTankSwapTrigger::IsActiveInEncounter()
{
    if (!botAI->IsTank(bot))
        return false;

    Creature* sacrolash = FindSunwellBoss(bot, NPC_SACROLASH);
    if (!sacrolash || !sacrolash->isTargetableForAttack())
        return false;

    Unit* victim = sacrolash->GetVictim();
    if (!victim || !victim->IsAlive() || victim == bot)
        return false;

    return victim->HasAura(SPELL_CONFOUNDING_BLOW);
}

bool TwinsBlazeNearbyTrigger::IsActiveInEncounter()
{
    // Blaze is a ground-fire trap GO (175566) under the target's feet, ~2-3 yards wide.
    // React only when one is actually burning under/near us; the action picks the escape.
    GameObject* blaze = bot->FindNearestGameObject(GO_BLAZE_TWINS,
        static_cast<float>(TWINS_BLAZE_SEARCH_RANGE));
    return blaze && blaze->isSpawned();
}

// ---- M'uru ----

bool MuruDarkFiendNearbyTrigger::IsActiveInEncounter()
{
    // Only priests (Dispel Magic) and shamans (Purge) can remove a Dark Fiend
    if (bot->getClass() != CLASS_PRIEST && bot->getClass() != CLASS_SHAMAN)
        return false;

    return bot->FindNearestCreature(NPC_DARK_FIEND_SWP, MURU_FIEND_DISPEL_RANGE, true) != nullptr;
}

bool MuruDarknessTrigger::IsActiveInEncounter()
{
    return bot->FindNearestCreature(NPC_DARKNESS, MURU_DARKNESS_ESCAPE_RANGE, true) != nullptr;
}

bool MuruSingularityNearbyTrigger::IsActiveInEncounter()
{
    return bot->FindNearestCreature(NPC_SINGULARITY, MURU_SINGULARITY_ESCAPE_RANGE, true) != nullptr;
}

bool MuruVoidSentinelSpawnTrigger::IsActiveInEncounter()
{
    if (!botAI->IsTank(bot))
        return false;

    Creature* sentinel = bot->FindNearestCreature(NPC_VOID_SENTINEL_SWP, MURU_SENTINEL_PICKUP_RANGE, true);
    if (!sentinel)
        return false;

    // Already tanked by someone (including this bot): pickup done
    if (sentinel->GetVictim() && sentinel->GetVictim()->IsAlive())
        return false;

    // This bot is not the Muru/Entropius main tank (who stays on the boss)
    Unit* boss = FindSunwellBoss(bot, NPC_ENTROPIUS);
    if (!boss)
        boss = FindSunwellBoss(bot, NPC_MURU);
    if (boss && boss->GetVictim() == bot)
        return false;

    return true;
}

bool MuruFuryMageInterruptTrigger::IsActiveInEncounter()
{
    // Only bots that actually carry an interrupt / silence react; non-interrupters would
    // just burn time running at the mage. The action performs the real per-class check.
    if (bot->getClass() != CLASS_WARRIOR &&
        bot->getClass() != CLASS_ROGUE &&
        bot->getClass() != CLASS_MAGE &&
        bot->getClass() != CLASS_WARLOCK &&
        bot->getClass() != CLASS_SHAMAN &&
        bot->getClass() != CLASS_DEATH_KNIGHT &&
        bot->getClass() != CLASS_HUNTER &&
        bot->getClass() != CLASS_PALADIN)
        return false;

    // A Fury Mage in range is casting its Fel Fireball -> interrupt it before the tank eats 5-9k.
    Creature* furyMage = bot->FindNearestCreature(NPC_FURY_MAGE_SWP,
        static_cast<float>(MURU_FURY_MAGE_INTERRUPT_RANGE), true);
    if (!furyMage)
        return false;

    Spell const* spell = furyMage->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    return spell && spell->GetSpellInfo() && spell->GetSpellInfo()->Id == SPELL_FEL_FIREBALL;
}

// ---- Kil'jaeden ----

bool KiljaedenFireBloomOnSelfTrigger::IsActiveInEncounter()
{
    return bot->HasAura(SPELL_FIRE_BLOOM);
}

bool KiljaedenFireBloomNearbyTrigger::IsActiveInEncounter()
{
    if (bot->HasAura(SPELL_FIRE_BLOOM))
        return false; // own spread action covers this bot

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member != bot && member->IsAlive() &&
            member->HasAura(SPELL_FIRE_BLOOM) &&
            bot->GetExactDist2d(member) < static_cast<float>(KJ_FIRE_BLOOM_SPREAD_RANGE) + 2.0f)
            return true;
    }

    return false;
}

bool KiljaedenShadowSpikeNearbyTrigger::IsActiveInEncounter()
{
    Creature* kiljaeden = FindSunwellBoss(bot, NPC_KILJAEDEN_SWP);
    if (!kiljaeden)
        return false;

    Spell* channel = kiljaeden->FindCurrentSpellBySpellId(SPELL_SHADOW_SPIKE);
    if (!channel)
        return false;

    Unit* target = channel->m_targets.GetUnitTarget();
    if (!target || !target->IsAlive())
        return false;

    return bot->GetExactDist2d(target) < static_cast<float>(KJ_SHADOW_SPIKE_SPREAD_RANGE);
}

bool KiljaedenArmageddonNearbyTrigger::IsActiveInEncounter()
{
    return bot->FindNearestCreature(NPC_ARMAGEDDON_TARGET, KJ_ARMAGEDDON_ESCAPE_RANGE, true) != nullptr;
}

bool KiljaedenShieldOrbTrigger::IsActiveInEncounter()
{
    // Melee cannot reach a floating orb
    if (!PlayerbotAI::IsRanged(bot))
        return false;

    return bot->FindNearestCreature(NPC_SHIELD_ORB, KJ_ORB_ENGAGE_RANGE, true) != nullptr;
}

bool KiljaedenFelfireFiendTrigger::IsActiveInEncounter()
{
    return bot->FindNearestCreature(NPC_VOLATILE_FELFIRE_FIEND, KJ_FIEND_ENGAGE_RANGE, true) != nullptr;
}

bool KiljaedenBlueOrbAvailableTrigger::IsActiveInEncounter()
{
    // Only the dedicated drake bots may click an orb (see KJ_DRAKE_DUTY_GROUP_SIZE).
    // Stable selection by the bot's object counter: ~1 in KJ_DRAKE_DUTY_GROUP_SIZE bots is a
    // carrier, independent of raid subgroup layout/role.
    if (bot->GetGUID().GetCounter() % KJ_DRAKE_DUTY_GROUP_SIZE != 0)
        return false;

    // Already a drake: the boss script removes active drakes from the orb target search and the
    // 45839 aura cannot be refreshed, so we must not click again.
    if (bot->HasAura(SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT))
        return false;

    // Boss must be alive.
    Creature* kiljaeden = FindSunwellBoss(bot, NPC_KILJAEDEN_SWP);
    if (!kiljaeden || !kiljaeden->IsAlive())
        return false;

    // Any empowered (selectable) Blue Dragonflight orb in range - EmpowerOrb runs at 85/50/25%.
    GameObject* orbs[] =
    {
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_1, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_2, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_3, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_4, KJ_DRAKE_DUTY_RANGE)
    };

    for (GameObject* go : orbs)
        if (go && !go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
            return true;

    return false;
}
