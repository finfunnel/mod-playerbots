/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "SWPActions.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "Playerbots.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellMgr.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
// The rift spawns on the teleported player and despawns after 15s; the room spreads
// wider than 30y, so search far enough that edge bots also see their portal.
float const SPECTRAL_RIFT_SEARCH_RANGE = 60.0f;
// Portal interact range; click once close enough
float const RIFT_USE_DISTANCE = 3.0f;
float const RIFT_SAFE_DISTANCE = 8.0f;
} // namespace

bool KalecgosEnterSpectralRiftAction::isUseful()
{
    return !bot->HasAura(SPELL_SPECTRAL_REALM) && !bot->HasAura(SPELL_SPECTRAL_EXHAUSTION);
}

bool KalecgosEnterSpectralRiftAction::Execute(Event /*event*/)
{
    GameObject* rift = bot->FindNearestGameObject(GO_SPECTRAL_RIFT, SPECTRAL_RIFT_SEARCH_RANGE);
    if (!rift)
        return false;

    if (bot->GetExactDist2d(rift) > RIFT_USE_DISTANCE)
    {
        // MOVEMENT_FORCED is the whole fix: combat-priority moves get vetoed by
        // IsWaitingForLastMove (same-priority window) and overridden every tick by the
        // attack strategies' chase moves, so bots ping-ponged and never reached the
        // door. FORCED always passes the priority gate - same pattern as the proven
        // RS Halion portal (RSActions_HAL.cpp) and Uld Yogg exit portal approaches.
        return MoveTo(bot->GetMapId(), rift->GetPositionX(), rift->GetPositionY(),
                      rift->GetPositionZ(), false, false, false, false,
                      MovementPriority::MOVEMENT_FORCED);
    }

    // In range: use the native GOOBER interaction - the EXACT path a human click takes
    // (GameObject::Use -> spell 44811 goober script -> 46019 teleport). CastSpell(44811)
    // directly was the old approach and it failed in practice: 44811 is Kalecgos'
    // multi-line SAY spell, not a player castable in the narrow portal window, so bots
    // reached the door but never teleported. Use() also runs the core's single-user
    // GO_FLAG_IN_USE arbitration so it behaves exactly like a sitting player.
    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    bot->SetFacingToObject(rift);
    rift->Use(bot);
    return true;
}

bool KalecgosSpreadFromRiftAction::isUseful()
{
    return !bot->HasAura(SPELL_SPECTRAL_REALM);
}

bool KalecgosSpreadFromRiftAction::Execute(Event /*event*/)
{
    GameObject* rift = bot->FindNearestGameObject(GO_SPECTRAL_RIFT, SPECTRAL_RIFT_SEARCH_RANGE);
    if (!rift)
        return false;

    // Move directly away from the rift spawn point until past the blast radius
    float angle = rift->GetAngle(bot);
    float distance = RIFT_SAFE_DISTANCE - bot->GetExactDist2d(rift);
    if (distance <= 0.0f)
        return false;

    return Move(angle, distance + 1.0f);
}

bool KalecgosAttackSathrovarrAction::Execute(Event /*event*/)
{
    // Entry-based lookup: "find target" only sees units the bot has threat on (healers never do),
    // and its full-name match would need the full "Sathrovarr the Corruptor" string anyway.
    Creature* sath = bot->FindNearestCreature(NPC_SATHROVARR, 100.0f, true);
    if (!sath || !sath->isTargetableForAttack() || sath->GetPositionZ() >= 50.0f)
        return false;

    // Tanks taunt an un-tanked Sathrovarr when present; without any tank inside the
    // team just fights normally (no tank-only gating - Sathrovarr melee-chases whoever
    // he is angry at, heals keep that player up).
    Unit* victim = sath->GetVictim();
    if (botAI->IsTank(bot) && (!victim || !victim->IsAlive()))
    {
        switch (bot->getClass())
        {
            case CLASS_PALADIN:
                if (botAI->CastSpell("hand of reckoning", sath))
                    return true;
                break;
            case CLASS_DEATH_KNIGHT:
                if (botAI->CastSpell("dark command", sath))
                    return true;
                break;
            case CLASS_DRUID:
                if (botAI->CastSpell("growl", sath))
                    return true;
                break;
            case CLASS_WARRIOR:
                if (botAI->CastSpell("taunt", sath))
                    return true;
                break;
            default:
                break;
        }
    }

    if (AI_VALUE(Unit*, "current target") == sath)
        return false;

    return Attack(sath);
}

bool KalecgosBalanceRealmDpsAction::isUseful()
{
    // Tanks never hold (threat must not pause - the boss must stay attacked and
    // stuck on a victim on both sides of the portal). Healers never stop either:
    // their "work" is healing, which the hold action would not stop anyway.
    return !botAI->IsTank(bot) && !botAI->IsHeal(bot);
}

bool KalecgosBalanceRealmDpsAction::Execute(Event /*event*/)
{
    // Hold DPS only: stop swinging, then return false so lower-priority actions
    // (movement, survival, heals) still run. Returning true here starved the whole
    // queue in the previous version and left bots standing idle.
    // Works both sides: inside bots stop attacking Sathrovarr when he lags; outside
    // bots stop attacking the dragon when he races ahead (mirror triggers, same action).
    if (Unit* target = bot->GetVictim())
        if (target->GetEntry() == NPC_SATHROVARR || target->GetEntry() == NPC_KALECGOS_DRAGON)
            bot->AttackStop();

    return false;
}

// ---- Brutallus ----

bool BrutallusBurnSpreadAction::isUseful()
{
    // Non-tanks always flee with Burn. A tank only holds the soak cone in place
    // while he is the ACTIVE victim - the raid's "burn nearby" trigger keeps
    // everyone clear of him instead. An off-duty tank now waits in the rear
    // cluster behind the boss, so he must spread too or he re-seeds Burn through
    // the whole camp waiting there (Burn jumps within 3 yards).
    if (!botAI->IsTank(bot))
        return true;

    Unit* brutallus = AI_VALUE2(Unit*, "find target", "brutallus");
    return brutallus && brutallus->GetVictim() != bot;
}

bool BrutallusBurnSpreadAction::Execute(Event event)
{
    // Guide: Burn's damage scales with the stack, so blowing an immunity now is
    // worth more than a stack or two of spread - self-cleanse then move out.
    // Self-cleanses first, base-class move spread second; tank rule stays in isUseful.
    if (bot->HasAura(SPELL_BURN_DAMAGE))
    {
        switch (bot->getClass())
        {
            case CLASS_ROGUE:
                if (botAI->CastSpell("cloak of shadows", bot))
                    return true;
                break;
            case CLASS_MAGE:
                if (botAI->CastSpell("ice block", bot))
                    return true;
                break;
            case CLASS_PALADIN:
                if (botAI->CastSpell("divine shield", bot))
                    return true;
                break;
            default:
                break;
        }
    }

    return MoveAwayFromPlayerWithDebuffAction::Execute(event);
}

bool BrutallusTankSwapAction::isUseful()
{
    return botAI->IsTank(bot);
}

bool BrutallusTankSwapAction::Execute(Event /*event*/)
{
    Unit* brutallus = AI_VALUE2(Unit*, "find target", "brutallus");
    if (!brutallus || !brutallus->IsAlive())
        return false;

    // Already tanking: stay in, keep attacking (threat) from the frontal soak position
    if (brutallus->GetVictim() == bot)
        return false;

    // Class taunt (pattern borrowed from RsCastClassTaunt): warrior/paladin/druid/DK
    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            if (botAI->CastSpell("hand of reckoning", brutallus))
                return true;
            break;
        case CLASS_DEATH_KNIGHT:
            if (botAI->CastSpell("dark command", brutallus))
                return true;
            break;
        case CLASS_DRUID:
            if (botAI->CastSpell("growl", brutallus))
                return true;
            break;
        case CLASS_WARRIOR:
            if (botAI->CastSpell("taunt", brutallus))
                return true;
            break;
        default:
            break;
    }

    // Fallback: move in and attack to build threat even without a taunt available
    return Attack(brutallus);
}

namespace
{
// Off-cone waiting spot behind Brutallus, opposite the ACTIVE cone (direction
// boss->victim, flipped). This server's Meteor Slash cone is 120 degrees wide and
// 65 yards long (spell_cone table -> Spell::SelectImplicitConeTargets), so behind
// the boss is the one place no boss facing can hit.
void GetBrutallusBehindSpot(Unit* brutallus, Unit* fallback, float& x, float& y)
{
    float behindAngle = brutallus->GetAngle(fallback) + M_PI;
    if (Unit* victim = brutallus->GetVictim())
        behindAngle = brutallus->GetAngle(victim) + M_PI;

    float const waitDist = brutallus->GetCombatReach() + 6.0f;
    x = brutallus->GetPositionX() + waitDist * std::cos(behindAngle);
    y = brutallus->GetPositionY() + waitDist * std::sin(behindAngle);
}
} // namespace

bool BrutallusSoakPositionAction::Execute(Event /*event*/)
{
    Unit* brutallus = AI_VALUE2(Unit*, "find target", "brutallus");
    if (!brutallus || !brutallus->IsAlive())
        return false;

    float const tolerance = static_cast<float>(BRUTALLUS_SLASH_TOLERANCE);

    // Off-duty tank (the trigger guarantees the boss' victim is not me): wait behind
    // the boss together with the off-duty camp. Any front-side idle spot keeps eating
    // the 120-degree slash cone and stacking the +75% fire vulnerability, which blocks
    // my taunt turn (the swap trigger waits for the debuff to fully expire). Behind
    // the boss no facing reaches me, and when I taunt the cone flips onto me with my
    // camp already stacked next to me, so the first slash stays shared.
    if (PlayerbotAI::IsTank(bot))
    {
        float x, y;
        GetBrutallusBehindSpot(brutallus, bot, x, y);
        if (bot->GetExactDist2d(x, y) > tolerance + 2.0f)
            // FORCED, not COMBAT: combat-priority moves get vetoed by the same-priority
            // window and overridden every tick by the attack strategies' chase moves, so
            // the tank ping-pongs mid-transit (Kalecgos rift approach, same pattern).
            return MoveTo(bot->GetMapId(), x, y, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_FORCED, true);
        // PARKED (user: tank不要乱动): keep consuming the tick - the engine breaks on the
        // first action that returns true (Engine.cpp DoNextAction) - so the generic melee
        // chase cannot drag the tank back into the boss's back and shuffle the cone around.
        // The taunt (RAID+3) and burn spread (EMERGENCY+3) outrank this and still fire; the
        // ACTIVE tank is moved by nothing at all (the trigger excludes the victim).
        return true;
    }

    // Anchor assignment: living tanks in group order define the camps (group order
    // is stable for the fight), this bot's camp = its index parity among living
    // non-tank non-healer members -> camps stay balanced as bodies drop.
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Living tanks in group order define the camps; this bot's camp is picked by its
    // index among living non-tank non-healer members (odd/even -> alternating tanks).
    std::vector<Player*> tanks;
    uint32 myIndex = 0, memberIndex = 0;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (PlayerbotAI::IsTank(member))
            tanks.push_back(member);
        else if (!PlayerbotAI::IsHeal(member))
        {
            if (member == bot)
                myIndex = memberIndex;
            ++memberIndex;
        }
    }

    if (tanks.empty())
        return false; // no tanks alive: soak formation is moot, default behaviour

    // Camp anchor: alternate tanks by this bot's index parity
    Player* anchor = tanks[myIndex % tanks.size()];

    bool anchorTanking = brutallus->GetVictim() == anchor;

    if (anchorTanking)
    {
        // My camp's tank is eating the cone: stand with him, share the slash.
        // In position already -> hold (return false keeps other actions running).
        float dist = bot->GetExactDist2d(anchor);
        if (dist <= tolerance + 2.0f)
            return false;
        return MoveNear(anchor, tolerance);
    }

    // Anchor tank is off-duty: wait behind the boss, opposite the ACTIVE cone.
    // Out of the slash and clear for the next swap.
    float x, y;
    GetBrutallusBehindSpot(brutallus, anchor, x, y);
    if (bot->GetExactDist2d(x, y) <= tolerance + 2.0f)
        return false;
    return MoveTo(bot->GetMapId(), x, y, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT, true);
}

// ---- Felmyst ----

bool FelmystEncapsulateSpreadAction::Execute(Event /*event*/)
{
    Unit* felmyst = AI_VALUE2(Unit*, "find target", "felmyst");
    if (!felmyst)
        return false;

    Spell* channel = felmyst->FindCurrentSpellBySpellId(SPELL_FELMYST_ENCAPSULATE_CHANNEL);
    if (!channel)
        return false;

    Unit* encapsTarget = channel->m_targets.GetUnitTarget();
    if (!encapsTarget)
        return false;

    // Straight away from the lifted player; do not run too far (guide: Gas Nova follows
    // Encapsulate - stay inside dispel range of the group).
    float angle = encapsTarget->GetAngle(bot);
    return Move(angle, FELMYST_ENCAPSULATE_SPREAD_RANGE);
}

bool FelmystVaporKiteAction::Execute(Event /*event*/)
{
    Creature* vapor = bot->FindNearestCreature(NPC_DEMONIC_VAPOR, 30.0f, true);
    if (!vapor)
        return false;

    // The beam follows this bot. Kite it LATERALLY (east-west) so the poison trail it leaves
    // runs across the arena sideways and never through the north shelter where the raid stands.
    // The beam is dragged by our movement; moving X-only keeps the trail off the north line
    // (user: 东西方向引, 不穿场南北).
    float outX = 0.0f, outY = 0.0f;
    FelmystLateralEscapePoint(bot, vapor, 15.0f, outX, outY);
    return MoveTo(bot->GetMapId(), outX, outY, bot->GetPositionZ());
}

bool FelmystVaporTrailEscapeAction::Execute(Event /*event*/)
{
    Creature* trail = bot->FindNearestCreature(NPC_DEMONIC_VAPOR_TRAIL, 40.0f, true);
    if (!trail)
        return false;

    // The poison cloud is large and lingers: move well out of it (20 yards). LATERAL (east-west)
    // only - never drift north/south, or the escape drags the poison trail across the raid or
    // off the corridor into the shelter/wall (user: 东西方向引, 不穿场南北).
    float outX = 0.0f, outY = 0.0f;
    FelmystLateralEscapePoint(bot, trail, 20.0f, outX, outY);
    return MoveTo(bot->GetMapId(), outX, outY, bot->GetPositionZ());
}

bool FelmystCorrosionAction::Execute(Event /*event*/)
{
    Creature* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    if (!felmyst || !felmyst->IsAlive())
        return false;

    Unit* tank = felmyst->GetVictim();
    if (!tank || !tank->IsAlive() || !tank->HasAura(SPELL_FELMYST_CORROSION))
        return false;

    // Magic debuff on the tank: dispel if we can, otherwise return false and let the
    // default heal engine keep him up. TBC magic dispel: priest/paladin/shaman only.
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
            return botAI->CastSpell("dispel magic", tank);
        case CLASS_PALADIN:
            return botAI->CastSpell("cleanse", tank);
        case CLASS_SHAMAN:
            return botAI->CastSpell("cleanse spirit", tank);
        default:
            return false;
    }
}

bool FelmystGasNovaSelfDispelAction::Execute(Event /*event*/)
{
    if (!bot->HasAura(SPELL_FELMYST_GAS_NOVA))
        return false;

    // Dispel the magic debuff off ourselves. TBC magic dispel: priest/paladin/shaman only.
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
            return botAI->CastSpell("dispel magic", bot);
        case CLASS_PALADIN:
            return botAI->CastSpell("cleanse", bot);
        case CLASS_SHAMAN:
            return botAI->CastSpell("cleanse spirit", bot);
        default:
            return false;
    }
}

bool FelmystDeepBreathAvoidAction::Execute(Event /*event*/)
{
    Unit* felmyst = bot->FindNearestCreature(NPC_FELMYST, 100.0f, true);
    if (!felmyst || !felmyst->IsAlive() || !felmyst->IsFlying())
        return false;

    // Dynamic lane dodge (user: 看龙在哪一条 lane 就是喷哪边): when the dragon is sweeping
    // THIS bot's current lane, run sideways to the farthest non-swept lane. When it sweeps a
    // different lane, stay put - standing on an unswept lane is safe. When it is repositioning
    // (FelmystCurrentLane < 0) there is no active fog strip, so also stay put.
    int const sweptLane = FelmystCurrentLane(bot);
    if (sweptLane < 0)
        return false;                          // no lane is actively being breathed on

    // This bot is unsafe only if it currently stands inside the swept lane.
    if (std::fabs(bot->GetPositionX() - FelmystLaneX(sweptLane)) > static_cast<float>(FELMYST_LANE_TOLERANCE))
        return false;                          // already safe on another lane - do not move

    int const safeLane = FelmystSafeLane(bot, sweptLane);
    float const targetX = FelmystLaneX(safeLane);

    // Already on the safe lane: nothing to do.
    if (std::fabs(bot->GetPositionX() - targetX) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
        return false;

    return MoveTo(bot->GetMapId(), targetX, bot->GetPositionY(), bot->GetPositionZ());
}

bool FelmystBlazingDeadAttackAction::isUseful()
{
    // EVERY non-tank kills skeletons (user: 除了被点名的,其他人优先击杀小骷髅). Tanks keep
    // picking them up instead; the "chased by the eye beam" bot is handled separately by the
    // vapor kite action at a higher priority. Melee included, but they only engage skeletons
    // already within reach (enforced in Execute) - no chasing.
    return !botAI->IsTank(bot);
}

bool FelmystBlazingDeadAttackAction::Execute(Event /*event*/)
{
    // Only engage skeletons while the dragon is airborne (P2) - when it lands (P1) everyone
    // returns to the normal boss fight. Same reliable signal as the hold multiplier: IsFlying().
    Unit* felmyst = AI_VALUE2(Unit*, "find target", "felmyst");
    if (!felmyst || !felmyst->IsAlive() || !felmyst->IsFlying())
        return false;

    // Dynamic-lane rule: never fight a skeleton while standing inside the lane being swept -
    // the breath fog (45717 charm = death) arrives faster than the kill. The deep-breath avoid
    // action (higher priority) pulls us out first; this guard stops us stopping to fight in it.
    int const currentLane = FelmystCurrentLane(bot);
    if (currentLane >= 0 &&
        std::fabs(bot->GetPositionX() - FelmystLaneX(currentLane)) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
        return false; // standing in the lane being swept - do not stop to fight

    Creature* skeleton = bot->FindNearestCreature(NPC_BLAZING_DEAD, 60.0f, true);
    if (!skeleton || !skeleton->IsAlive())
        return false;

    // Melee pre-filter: only engage a skeleton already within reach - otherwise AttackAction would
    // path us onto it, drifting us off our safe lane. Ranged (non-melee) reach straight from
    // wherever they stand (60y search). FindNearestCreature picks the closest; if a melee bot
    // has none within reach it has no legal target and simply keeps its position on the lane.
    if (PlayerbotAI::IsMelee(bot) &&
        bot->GetExactDist(skeleton) > static_cast<float>(FELMYST_BLAZING_DEAD_PICKUP_RANGE))
        return false; // melee: only hit the pack that is already within reach

    // IsInCombat no longer gates the kill: a freshly-summoned skeleton may not have engaged
    // yet. Everyone (melee + ranged) burns it without leaving their current lane.

    // Do not commit to a skeleton while a poison cloud is near us - moving to the skeleton
    // (or its stray pathing) could step into the vapor that just wiped us.
    if (bot->FindNearestCreature(NPC_DEMONIC_VAPOR_TRAIL, 15.0f, true))
        return false;

    return Attack(skeleton);
}

bool FelmystCharmedAllyAction::isUseful()
{
    // Only react when an ally (not self) carries the charm.
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member != bot && member->IsAlive() && member->HasAura(SPELL_FELMYST_FOG_CHARM))
            return true;
    }
    return false;
}

bool FelmystCharmedAllyAction::Execute(Event /*event*/)
{
    Unit* charmed = nullptr;
    Group* group = bot->GetGroup();
    if (group)
    {
        float nearest = 100.0f;
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (member && member != bot && member->IsAlive() && member->HasAura(SPELL_FELMYST_FOG_CHARM))
            {
                float d = bot->GetExactDist2d(member);
                if (d < nearest)
                {
                    nearest = d;
                    charmed = member;
                }
            }
        }
    }
    if (!charmed)
        return false;

    // The charm may flip the ally hostile (user/guide: mind-controlled ally becomes an enemy).
    // Attack() refuses friendly targets, so this only lands if the charm made them hostile;
    // otherwise we fall through to keeping clear of them.
    if (!bot->IsFriendlyTo(charmed) && Attack(charmed))
        return true;

    // Cannot kill them (still friendly): keep well away until the charm drops - they may be
    // mind-controlled attacking us, and their removal kills them anyway.
    float angle = charmed->GetAngle(bot);
    return Move(angle, 12.0f);
}

bool FelmystBlazingDeadPickupAction::isUseful()
{
    return botAI->IsTank(bot);
}

bool FelmystBlazingDeadPickupAction::Execute(Event /*event*/)
{
    // Dynamic-lane rule: the tank picks up only while NOT standing in the lane being swept (the
    // deep-breath avoid action owns the move out of it first). Standing on an unswept lane the
    // tank collects skeletons exactly like before.
    int const currentLane = FelmystCurrentLane(bot);
    if (currentLane >= 0 &&
        std::fabs(bot->GetPositionX() - FelmystLaneX(currentLane)) <= static_cast<float>(FELMYST_LANE_TOLERANCE))
        return false; // currently inside the swept lane - the deep-breath avoid moves us out first

    Creature* skeleton = bot->FindNearestCreature(NPC_BLAZING_DEAD,
        static_cast<float>(FELMYST_BLAZING_DEAD_PICKUP_RANGE), true);
    // No IsInCombat() gate: a freshly-summoned skeleton is picked up the same as an engaged one
    // (user: 远程优先打小骷髅 -> the tank must be ready to hold the pack the ranged is burning).
    if (!skeleton || !skeleton->IsAlive())
        return false;

    // Do not walk into poison to collect a skeleton - the tank dies just like anyone else.
    // Vapor escape (+6) takes priority over this (+2) anyway; wait until it clears.
    if (bot->FindNearestCreature(NPC_DEMONIC_VAPOR_TRAIL, 10.0f, true))
        return false;

    if (skeleton->GetVictim() && skeleton->GetVictim()->IsAlive() && skeleton->GetVictim() != bot)
        return false; // someone already has it - stay out of the way

    if (skeleton->GetVictim() == bot)
        return Attack(skeleton); // already mine: keep hitting it for threat

    // Pick it up: taunt if available, otherwise move in and attack
    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            if (botAI->CastSpell("hand of reckoning", skeleton))
                return true;
            break;
        case CLASS_DEATH_KNIGHT:
            if (botAI->CastSpell("dark command", skeleton))
                return true;
            break;
        case CLASS_DRUID:
            if (botAI->CastSpell("growl", skeleton)
                || botAI->CastSpell("challenging roar", skeleton))
                return true;
            break;
        case CLASS_WARRIOR:
            if (botAI->CastSpell("taunt", skeleton)
                || botAI->CastSpell("challenging shout", skeleton))
                return true;
            break;
        default:
            break;
    }

    return Attack(skeleton);
}

bool TwinsBlazeEscapeAction::Execute(Event /*event*/)
{
    // Collect every Blaze trap within search range. FindNearestGameObject only returns the
    // closest one, but the ground can hold several traps at once, so we check them all.
    std::vector<GameObject*> blazes;
    GuidVector gos = AI_VALUE(GuidVector, "nearest game objects");
    for (ObjectGuid const& guid : gos)
    {
        GameObject* go = botAI->GetGameObject(guid);
        if (!go || !go->isSpawned() || go->GetEntry() != GO_BLAZE_TWINS)
            continue;
        if (go->GetExactDist2d(bot) > static_cast<float>(TWINS_BLAZE_SEARCH_RANGE))
            continue;
        blazes.push_back(go);
    }

    if (blazes.empty())
        return false;

    // Sample 8 directions x 3y steps and pick the spot that keeps the largest min-distance
    // to every Blaze. Mirrors the engine's MoveAwayFromPlayerWithDebuffAction search.
    constexpr int directions = 8;
    constexpr float step = 3.0f;
    constexpr float maxScan = 12.0f;

    float bestX = bot->GetPositionX();
    float bestY = bot->GetPositionY();
    float bestZ = bot->GetPositionZ();
    float bestMin = -1.0f;

    for (int i = 0; i < directions; ++i)
    {
        float const angle = (i * 2.0f * M_PI) / directions;
        for (float d = step; d <= maxScan; d += step)
        {
            float const x = bot->GetPositionX() + d * std::cos(angle);
            float const y = bot->GetPositionY() + d * std::sin(angle);

            bool safe = true;
            float minDist = std::numeric_limits<float>::max();
            for (GameObject* blaze : blazes)
            {
                float const dist = blaze->GetExactDist2d(x, y);
                if (dist < static_cast<float>(TWINS_BLAZE_ESCAPE_RANGE))
                {
                    safe = false;
                    break;
                }
                minDist = std::min(minDist, dist);
            }

            if (safe && bot->IsWithinLOS(x, y, bot->GetPositionZ()))
            {
                if (minDist > bestMin)
                {
                    bestMin = minDist;
                    bestX = x;
                    bestY = y;
                    bestZ = bot->GetPositionZ();
                }
            }
        }
    }

    if (bestMin < 0.0f)
        return false; // no safe spot found; stay put rather than kiting into a bigger fire

    return MoveTo(bot->GetMapId(), bestX, bestY, bestZ, false, false, false, false,
                  MovementPriority::MOVEMENT_NORMAL);
}

// ---- Eredar Twins ----

bool TwinsConflagSpreadAction::Execute(Event /*event*/)
{
    Creature* alythess = bot->FindNearestCreature(NPC_ALYTHESS, 100.0f, true);
    if (!alythess)
        return false;

    Spell* cast = alythess->FindCurrentSpellBySpellId(SPELL_CONFLAGRATION_TWINS);
    if (!cast)
        return false;

    Unit* target = cast->m_targets.GetUnitTarget();
    if (!target)
        return false;

    if (target == bot)
    {
        // The carrier runs away from the raid pile (guide: you have 3 seconds)
        Group* group = bot->GetGroup();
        if (group)
        {
            float cx = 0.0f, cy = 0.0f;
            uint32 count = 0;
            for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
            {
                Player* member = gref->GetSource();
                if (member && member->IsAlive() && member != bot)
                {
                    cx += member->GetPositionX();
                    cy += member->GetPositionY();
                    ++count;
                }
            }
            if (count > 0)
            {
                Position const center(cx / count, cy / count, bot->GetPositionZ());
                return Move(center.GetAngle(bot), TWINS_CONFLAG_SPREAD_RANGE);
            }
        }
        return Move(alythess->GetAngle(bot), TWINS_CONFLAG_SPREAD_RANGE);
    }

    // Someone else is the target: run away from them
    float angle = target->GetAngle(bot);
    return Move(angle, TWINS_CONFLAG_SPREAD_RANGE);
}

bool TwinsDispelPyrogenicsAction::isUseful()
{
    // Mage steals it for himself, priest/shaman just remove it
    switch (bot->getClass())
    {
        case CLASS_MAGE:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return true;
        default:
            return false;
    }
}

bool TwinsDispelPyrogenicsAction::Execute(Event /*event*/)
{
    Creature* alythess = bot->FindNearestCreature(NPC_ALYTHESS, 100.0f, true);
    if (!alythess || !alythess->HasAura(SPELL_PYROGENICS))
        return false;

    switch (bot->getClass())
    {
        case CLASS_MAGE:
            return botAI->CastSpell("spellsteal", alythess);
        case CLASS_PRIEST:
            return botAI->CastSpell("dispel magic", alythess);
        case CLASS_SHAMAN:
            return botAI->CastSpell("purge", alythess);
        default:
            return false;
    }
}

bool TwinsTankSwapAction::isUseful()
{
    return botAI->IsTank(bot);
}

bool TwinsTankSwapAction::Execute(Event /*event*/)
{
    Creature* sacrolash = bot->FindNearestCreature(NPC_SACROLASH, 100.0f, true);
    if (!sacrolash || !sacrolash->IsAlive())
        return false;

    if (sacrolash->GetVictim() == bot)
        return false;

    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            if (botAI->CastSpell("hand of reckoning", sacrolash))
                return true;
            break;
        case CLASS_DEATH_KNIGHT:
            if (botAI->CastSpell("dark command", sacrolash))
                return true;
            break;
        case CLASS_DRUID:
            if (botAI->CastSpell("growl", sacrolash))
                return true;
            break;
        case CLASS_WARRIOR:
            if (botAI->CastSpell("taunt", sacrolash))
                return true;
            break;
        default:
            break;
    }

    return Attack(sacrolash);
}

// ---- M'uru ----

bool MuruDispelDarkFiendAction::Execute(Event /*event*/)
{
    Creature* fiend = bot->FindNearestCreature(NPC_DARK_FIEND_SWP, MURU_FIEND_DISPEL_RANGE, true);
    if (!fiend)
        return false;

    switch (bot->getClass())
    {
        // Priest: a single Mass Dispel clears the whole wave of 8 Dark Fiends at once
        // (guide: "黑暗魔可被牧师群/单体驱散"). Try the AoE first, fall back to single.
        case CLASS_PRIEST:
            if (botAI->CanCastSpell("mass dispel", fiend) &&
                botAI->CastSpell("mass dispel", fiend))
                return true;
            return botAI->CastSpell("dispel magic", fiend);
        case CLASS_SHAMAN:
            return botAI->CastSpell("purge", fiend);
        default:
            return false;
    }
}

bool MuruDarknessEscapeAction::Execute(Event /*event*/)
{
    Creature* darkness = bot->FindNearestCreature(NPC_DARKNESS, static_cast<float>(MURU_DARKNESS_ESCAPE_RANGE) + 5.0f, true);
    if (!darkness)
        return false;

    float angle = darkness->GetAngle(bot);
    return Move(angle, MURU_DARKNESS_ESCAPE_RANGE);
}

bool MuruSingularityEscapeAction::Execute(Event /*event*/)
{
    Creature* singularity = bot->FindNearestCreature(NPC_SINGULARITY, static_cast<float>(MURU_SINGULARITY_ESCAPE_RANGE) + 5.0f, true);
    if (!singularity)
        return false;

    float angle = singularity->GetAngle(bot);
    return Move(angle, MURU_SINGULARITY_ESCAPE_RANGE);
}

bool MuruVoidSentinelPickupAction::isUseful()
{
    return botAI->IsTank(bot);
}

bool MuruVoidSentinelPickupAction::Execute(Event /*event*/)
{
    Creature* sentinel = bot->FindNearestCreature(NPC_VOID_SENTINEL_SWP, MURU_SENTINEL_PICKUP_RANGE, true);
    if (!sentinel || !sentinel->IsAlive())
        return false;

    if (sentinel->GetVictim() && sentinel->GetVictim()->IsAlive() && sentinel->GetVictim() != bot)
        return false; // someone else has it

    if (sentinel->GetVictim() == bot)
        return Attack(sentinel); // already mine: keep hitting it for threat

    // Pick it up: taunt if available, otherwise move in and attack
    switch (bot->getClass())
    {
        case CLASS_PALADIN:
            if (botAI->CastSpell("hand of reckoning", sentinel))
                return true;
            break;
        case CLASS_DEATH_KNIGHT:
            if (botAI->CastSpell("dark command", sentinel))
                return true;
            break;
        case CLASS_DRUID:
            if (botAI->CastSpell("growl", sentinel)
                || botAI->CastSpell("challenging roar", sentinel))
                return true;
            break;
        case CLASS_WARRIOR:
            if (botAI->CastSpell("taunt", sentinel)
                || botAI->CastSpell("challenging shout", sentinel))
                return true;
            break;
        default:
            break;
    }

    return Attack(sentinel);
}

namespace
{
// Whether a spell the bot could cast actually stops a cast-in-progress. Mirrors MgT's
// StopsACast(): a dedicated interrupt only counts if the spell has the silence-prevention
// + interrupt flag or an INTERRUPT_CAST effect; control spells need a stun/silence effect.
bool MuruSpellStopsACast(uint32 spellId)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
    if (!info)
        return false;

    if ((info->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT) &&
        info->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
        return true;

    for (uint8 i = EFFECT_0; i <= EFFECT_2; ++i)
    {
        if (info->Effects[i].Effect == SPELL_EFFECT_INTERRUPT_CAST)
            return true;

        if (info->Effects[i].Effect != SPELL_EFFECT_APPLY_AURA)
            continue;

        if (info->Effects[i].ApplyAuraName == SPELL_AURA_MOD_SILENCE ||
            info->Effects[i].ApplyAuraName == SPELL_AURA_MOD_STUN)
            return true;
    }

    return false;
}

// Try the dedicated interrupt spells first, then the control spells. Returns the spell
// name actually cast, empty if nothing usable.
std::string MuruCastInterrupt(PlayerbotAI* botAI, Unit* target)
{
    static std::vector<std::pair<char const*, bool>> const interruptSpells = {
        { "kick", false },      { "pummel", false },      { "wind shear", false },
        { "mind freeze", false }, { "counterspell", false }, { "spell lock", false },
        { "shield bash", false }, { "silencing shot", false },
        // control fellas only if nothing dedicated is available
        { "shockwave", true },     { "shadowfury", true },   { "concussion blow", true },
        { "hammer of justice", true }, { "maim", true },     { "kidney shot", true },
        { "silence", true },       { "bash", true },        { "strangulate", true },
        { "arcane torrent", true },
    };

    for (auto const& [name, isControl] : interruptSpells)
    {
        uint32 const spellId = botAI->GetAiObjectContext()->GetValue<uint32>("spell id", name)->Get();
        if (!spellId || !MuruSpellStopsACast(spellId))
            continue;

        if (!botAI->CanCastSpell(name, target))
            continue;

        if (botAI->CastSpell(name, target))
            return name;
    }

    return {};
}
} // namespace

bool MuruInterruptFuryMageAction::isUseful()
{
    // Only classes that can stop a cast react; covers everyone in InterruptSpells().
    return bot->getClass() == CLASS_WARRIOR ||
           bot->getClass() == CLASS_ROGUE ||
           bot->getClass() == CLASS_MAGE ||
           bot->getClass() == CLASS_WARLOCK ||
           bot->getClass() == CLASS_SHAMAN ||
           bot->getClass() == CLASS_DEATH_KNIGHT ||
           bot->getClass() == CLASS_HUNTER ||
           bot->getClass() == CLASS_PALADIN;
}

bool MuruInterruptFuryMageAction::Execute(Event /*event*/)
{
    Creature* furyMage = bot->FindNearestCreature(NPC_FURY_MAGE_SWP,
        static_cast<float>(MURU_FURY_MAGE_INTERRUPT_RANGE), true);
    if (!furyMage)
        return false;

    Spell const* spell = furyMage->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell || !spell->GetSpellInfo() || spell->GetSpellInfo()->Id != SPELL_FEL_FIREBALL)
        return false;

    // Still casting? If the cast already finished, nothing to interrupt (avoid wasting gcd).
    if (!furyMage->IsNonMeleeSpellCast(true))
        return false;

    std::string const used = MuruCastInterrupt(botAI, furyMage);
    if (used.empty())
        return false;

    return true;
}

// ---- Kil'jaeden ----

bool KiljaedenShadowSpikeSpreadAction::Execute(Event /*event*/)
{
    Creature* kiljaeden = bot->FindNearestCreature(NPC_KILJAEDEN_SWP, 100.0f, true);
    if (!kiljaeden)
        return false;

    Spell* channel = kiljaeden->FindCurrentSpellBySpellId(SPELL_SHADOW_SPIKE);
    if (!channel)
        return false;

    Unit* target = channel->m_targets.GetUnitTarget();
    if (!target)
        return false;

    float angle = target->GetAngle(bot);
    return Move(angle, KJ_SHADOW_SPIKE_SPREAD_RANGE);
}

bool KiljaedenArmageddonEscapeAction::Execute(Event /*event*/)
{
    Creature* marker = bot->FindNearestCreature(NPC_ARMAGEDDON_TARGET, static_cast<float>(KJ_ARMAGEDDON_ESCAPE_RANGE) + 5.0f, true);
    if (!marker)
        return false;

    float angle = marker->GetAngle(bot);
    return Move(angle, KJ_ARMAGEDDON_ESCAPE_RANGE);
}

bool KiljaedenShieldOrbAction::isUseful()
{
    return PlayerbotAI::IsRanged(bot);
}

bool KiljaedenShieldOrbAction::Execute(Event /*event*/)
{
    Creature* orb = bot->FindNearestCreature(NPC_SHIELD_ORB, KJ_ORB_ENGAGE_RANGE, true);
    if (!orb || !orb->isTargetableForAttack())
        return false;

    if (AI_VALUE(Unit*, "current target") == orb)
        return false; // already on it

    return Attack(orb);
}

bool KiljaedenFelfireFiendAction::Execute(Event /*event*/)
{
    Creature* fiend = bot->FindNearestCreature(NPC_VOLATILE_FELFIRE_FIEND, KJ_FIEND_ENGAGE_RANGE, true);
    if (!fiend || !fiend->isTargetableForAttack())
        return false;

    if (AI_VALUE(Unit*, "current target") == fiend)
        return false;

    return Attack(fiend);
}

bool KiljaedenClickBlueOrbAction::isUseful()
{
    // Same duty filter as the trigger: only the dedicated carriers click an orb, and only when
    // they are not already in drake form (the 45839 aura cannot be refreshed while active).
    if (bot->GetGUID().GetCounter() % KJ_DRAKE_DUTY_GROUP_SIZE != 0)
        return false;

    return !bot->HasAura(SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT);
}

bool KiljaedenClickBlueOrbAction::Execute(Event /*event*/)
{
    // Nearest empowered (selectable) Blue Dragonflight Orb. The four entries share one GO
    // family; find the first one that is not locked by EmpowerOrb (GO_FLAG_NOT_SELECTABLE).
    GameObject* orbs[] =
    {
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_1, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_2, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_3, KJ_DRAKE_DUTY_RANGE),
        bot->FindNearestGameObject(GO_ORB_OF_THE_BLUE_DRAGONFLIGHT_4, KJ_DRAKE_DUTY_RANGE)
    };

    GameObject* orb = nullptr;
    for (GameObject* go : orbs)
    {
        if (go && !go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
        {
            orb = go;
            break;
        }
    }
    if (!orb)
        return false;

    // Move within interaction distance, then click (modelled on EnterTwilightPortalAction).
    if (!orb->IsAtInteractDistance(bot))
        return MoveTo(orb, fmaxf(orb->GetInteractionDistance() - 1.0f, 0.0f));

    WorldPacket data(CMSG_GAMEOBJ_USE);
    data << orb->GetGUID();
    bot->GetSession()->HandleGameObjectUseOpcode(data);

    return true;
}
