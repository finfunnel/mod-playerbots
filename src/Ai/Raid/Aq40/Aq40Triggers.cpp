/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "Aq40Triggers.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Creature.h"
#include "DynamicObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Playerbots.h"

namespace
{
// Entry-based multi-target lookup (pattern from ICCActions_SG.cpp:1264-1266): works for
// every bot regardless of threat state and returns ALL alive copies, boss included.
void FindSkeramCopies(Player* bot, float range, std::list<Creature*>& copies)
{
    std::list<Unit*> units;
    Acore::AnyUnitInObjectRangeCheck check(bot, range);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(bot, units, check);
    Cell::VisitObjects(bot, searcher, range);

    for (Unit* unit : units)
    {
        Creature* creature = unit->ToCreature();
        if (creature && creature->IsAlive() && creature->GetEntry() == NPC_THE_PROPHET_SKERAM)
            copies.push_back(creature);
    }
}

// Summoned copy = image; a non-summon 15263 is the boss body itself.
bool IsSkeramImage(Creature* creature)
{
    return creature->IsSummon();
}

// Any living Sartura-family unit (boss 15516 or Royal Guard 15984) currently whirlwinding.
bool IsSarturaFamily(Unit* unit)
{
    return unit->GetEntry() == NPC_SARTURA || unit->GetEntry() == NPC_SARTURA_ROYAL_GUARD;
}

// Kri's corpse poison cloud (26590) is a DynamicObject on the ground.
bool IsPoisonCloudNear(Player* bot, float scanRange, DynamicObject*& outCloud)
{
    std::vector<WorldObject*> objs;
    Acore::AllWorldObjectsInRange check(bot, scanRange);
    Acore::WorldObjectListSearcher<Acore::AllWorldObjectsInRange> searcher(
        bot, objs, check, GRID_MAP_TYPE_MASK_DYNAMICOBJECT);
    Cell::VisitObjects(bot, searcher, scanRange);

    for (WorldObject* obj : objs)
    {
        DynamicObject* dyn = obj->ToDynObject();
        if (!dyn || dyn->GetSpellId() != SPELL_KRI_POISON_CLOUD)
            continue;

        float radius = dyn->GetRadius();
        if (radius <= 0.0f)
            radius = POISON_CLOUD_ESCAPE_RANGE;

        if (bot->GetExactDist2d(dyn) <= radius + 2.0f)
        {
            outCloud = dyn;
            return true;
        }
    }

    return false;
}
} // namespace

bool SkeramMemberMindControlledTrigger::IsActiveInEncounter()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (member && member != bot && member->IsAlive() && member->HasAura(SPELL_TRUE_FULFILLMENT))
            return true;
    }

    return false;
}

bool SkeramImageUpTrigger::IsActiveInEncounter()
{
    // Split is up while any summoned 15263 copy exists; images spawn on the platforms.
    std::list<Creature*> copies;
    FindSkeramCopies(bot, SKERAM_SEARCH_RANGE, copies);

    for (Creature* creature : copies)
        if (IsSkeramImage(creature) && creature->isTargetableForAttack())
            return true;

    return false;
}

bool SkeramUntankedTrigger::IsActiveInEncounter()
{
    if (!botAI->IsTank(bot))
        return false;

    std::list<Creature*> copies;
    FindSkeramCopies(bot, SKERAM_SEARCH_RANGE, copies);

    for (Creature* creature : copies)
    {
        if (!creature->isTargetableForAttack())
            continue;

        // During the split ceremony the boss is immune/rooted (EVENT_TELEPORT 2s) - skip
        Unit* victim = creature->GetVictim();
        if (victim && victim->IsAlive())
            continue; // already tanked

        return true; // this tank found an un-tanked boss/image
    }

    return false;
}

// ---- Battleguard Sartura ----

bool SarturaWhirlwindActiveTrigger::IsActiveInEncounter()
{
    // Whirlwind (26083 boss / 26038 guard) lasts 15s/8s with random hops and constant
    // threat wipes - anyone nearby is a potential target, so ALL bots flee it (guide:
    // 旋风斩期间停手躲避，不要挂dot/持续治疗以免OT).
    std::list<Unit*> units;
    Acore::AnyUnitInObjectRangeCheck check(bot, SARTURA_SEARCH_RANGE);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(bot, units, check);
    Cell::VisitObjects(bot, searcher, SARTURA_SEARCH_RANGE);

    for (Unit* unit : units)
        if (IsSarturaFamily(unit) && unit->IsAlive()
            && (unit->HasAura(SPELL_WHIRLWIND_SARTURA) || unit->HasAura(SPELL_GUARD_WHIRLWIND)))
            return true;

    return false;
}

bool SarturaRoyalGuardUpTrigger::IsActiveInEncounter()
{
    // Guide: kill the three Royal Guards first, then the boss.
    std::list<Unit*> units;
    Acore::AnyUnitInObjectRangeCheck check(bot, SARTURA_SEARCH_RANGE);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(bot, units, check);
    Cell::VisitObjects(bot, searcher, SARTURA_SEARCH_RANGE);

    for (Unit* unit : units)
    {
        Creature* creature = unit->ToCreature();
        if (creature && creature->IsAlive() && creature->GetEntry() == NPC_SARTURA_ROYAL_GUARD
            && creature->isTargetableForAttack())
            return true;
    }

    return false;
}

// ---- Bug Trio ----

bool BugTrioFearPrepTrigger::IsActiveInEncounter()
{
    // Yauj alive + in combat = the periodic AoE fear (26580) is a live threat: keep
    // Fear Ward / Tremor Totem up (pattern from BwlNefarianFearWardTrigger / MgT).
    if (bot->getClass() != CLASS_PRIEST && bot->getClass() != CLASS_SHAMAN)
        return false;

    Creature* yauj = bot->FindNearestCreature(NPC_YAUJ, BUG_TRIO_SEARCH_RANGE, true);
    return yauj && yauj->IsInCombat();
}

bool BugTrioPoisonCloudTrigger::IsActiveInEncounter()
{
    DynamicObject* cloud = nullptr;
    return IsPoisonCloudNear(bot, BUG_TRIO_SEARCH_RANGE, cloud);
}

bool BugTrioKillOrderTargetTrigger::IsActiveInEncounter()
{
    // Follow the guide's kill order while any bug is still alive: Yauj -> Vem -> Kri.
    // (Tanks keep normal assignment; the trigger only retargets damage.)
    Creature* yauj = bot->FindNearestCreature(NPC_YAUJ, BUG_TRIO_SEARCH_RANGE, true);
    if (yauj && yauj->isTargetableForAttack())
        return true;

    Creature* vem = bot->FindNearestCreature(NPC_VEM, BUG_TRIO_SEARCH_RANGE, true);
    if (vem && vem->isTargetableForAttack())
        return true;

    Creature* kri = bot->FindNearestCreature(NPC_KRI, BUG_TRIO_SEARCH_RANGE, true);
    return kri && kri->isTargetableForAttack();
}

// ---- Fankriss ----

bool FankrissMortalWoundStacksTrigger::IsActiveInEncounter()
{
    // A tank NOT currently tanking Fankriss steps up when the active tank's Mortal Wound
    // stack reaches the threshold (guide: 换坦; stack check pattern from OCActions).
    if (!botAI->IsTank(bot))
        return false;

    Creature* fankriss = bot->FindNearestCreature(NPC_FANKRISS, FANKRISS_SEARCH_RANGE, true);
    if (!fankriss)
        return false;

    Unit* victim = fankriss->GetVictim();
    if (!victim || !victim->IsAlive() || victim == bot)
        return false; // nobody to relieve / I am the victim

    Aura* wounds = victim->GetAura(SPELL_MORTAL_WOUND_FANKRISS);
    return wounds && wounds->GetStackAmount() >= FANKRISS_MORTAL_WOUND_SWAP_STACKS;
}

bool FankrissSpawnUpTrigger::IsActiveInEncounter()
{
    // Sandworms (Spawn of Fankriss 15630) hit extremely hard and die first (guide).
    Creature* spawn = bot->FindNearestCreature(NPC_SPAWN_OF_FANKRISS, FANKRISS_SEARCH_RANGE, true);
    return spawn && spawn->isTargetableForAttack();
}

// ---- Viscidus ----

bool ViscidusNeedsFrostTrigger::IsActiveInEncounter()
{
    // Only mages contribute meaningful frost-hit counter stacking (guide: 法师搓寒冰箭)
    if (bot->getClass() != CLASS_MAGE)
        return false;

    Creature* viscidus = bot->FindNearestCreature(NPC_VISCIDUS, VISCIDUS_SEARCH_RANGE, true);
    if (!viscidus || !viscidus->isTargetableForAttack())
        return false;

    // Stack phases: slowed (26034) -> slowed more (26036) -> frozen (25937). Keep casting
    // frost until fully frozen; also skip the glob-split phase (boss untargetable).
    return !viscidus->HasAura(SPELL_VISCIDUS_FREEZE);
}

bool ViscidusFrozenTrigger::IsActiveInEncounter()
{
    Creature* viscidus = bot->FindNearestCreature(NPC_VISCIDUS, VISCIDUS_SEARCH_RANGE, true);
    return viscidus && viscidus->HasAura(SPELL_VISCIDUS_FREEZE) && viscidus->isTargetableForAttack();
}

bool ViscidusGlobUpTrigger::IsActiveInEncounter()
{
    Creature* glob = bot->FindNearestCreature(NPC_GLOB_OF_VISCIDUS, VISCIDUS_SEARCH_RANGE, true);
    return glob && glob->isTargetableForAttack();
}

bool ViscidusToxinNearTrigger::IsActiveInEncounter()
{
    // Toxic Slime (15925) is stationary and radiates the poison aura (26575): stay out
    // of its cloud radius (guide: 躲开毒云).
    Creature* slime = bot->FindNearestCreature(NPC_TOXIC_SLIME, VISCIDUS_SEARCH_RANGE, true);
    if (!slime)
        return false;

    return bot->GetExactDist2d(slime) <= static_cast<float>(TOXIN_ESCAPE_RANGE);
}

// ---- Huhuran ----

bool HuhuranBerserkTrigger::IsActiveInEncounter()
{
    // 30% berserk: Poison Bolt (26052) spams the 15 nearest players (guide). Soakers
    // (melee) stack in; this trigger drives the soaker positioning action.
    Creature* huhuran = bot->FindNearestCreature(NPC_HUHURAN, HUHURAN_SEARCH_RANGE, true);
    return huhuran && huhuran->HasAura(SPELL_HUHURAN_BERSERK);
}

// ---- Twin Emperors ----

bool TwinEmpVeklorUntankedTrigger::IsActiveInEncounter()
{
    // Warlock (spell tank, soul link) or any ranged caster steps up when Veklor has no
    // living victim (guide: 术士抗魔皇).
    if (bot->getClass() != CLASS_WARLOCK || !PlayerbotAI::IsRanged(bot))
        return false;

    Creature* veklor = bot->FindNearestCreature(NPC_VEKLOR, TWIN_EMPERORS_SEARCH_RANGE, true);
    if (!veklor)
        return false;

    Unit* victim = veklor->GetVictim();
    return !victim || !victim->IsAlive();
}

bool TwinEmpTankSwapTrigger::IsActiveInEncounter()
{
    // Warrior tank relieves the active Veknilash tank after Unbalancing Strike stacks
    // -100 defense (guide: 重压打击减防御，战士注意换坦).
    if (!botAI->IsTank(bot))
        return false;

    Creature* veknilash = bot->FindNearestCreature(NPC_VEKNILASH, TWIN_EMPERORS_SEARCH_RANGE, true);
    if (!veknilash)
        return false;

    Unit* victim = veknilash->GetVictim();
    if (!victim || !victim->IsAlive() || victim == bot)
        return false;

    return victim->HasAura(SPELL_TWINS_UNBALANCING_STRIKE);
}

bool TwinEmpWrongDamageTypeTrigger::IsActiveInEncounter()
{
    // Physical damage dealers must not swing at the magic-immune caster emperor - they
    // deal zero damage and eat Arcane Bursts (guide: 注意自己的输出伤害类型).
    if (PlayerbotAI::IsRanged(bot))
        return false; // ranged casters hit the correct twin via their own targeting

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || target->GetEntry() != NPC_VEKLOR)
        return false;

    // Physical school only (pattern from AttackersValue.cpp:158-159): SPELL_SCHOOL_MASK_MELEE
    // does not exist in this codebase's school-mask enum.
    return target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL);
}

// ---- Ouro ----

bool OuroSubmergedTrigger::IsActiveInEncounter()
{
    // Boss body despawns while submerged; the phase is live while dirt mounds (15712)
    // roam and Ouro himself is not targetable (guide: 钻地后土堆追击玩家).
    Creature* ouro = bot->FindNearestCreature(NPC_OURO, OURO_SEARCH_RANGE, true);
    if (ouro && ouro->isTargetableForAttack())
        return false; // surfaced and fighting - not the submerge phase

    Creature* mound = bot->FindNearestCreature(NPC_DIRT_MOUND, OURO_SEARCH_RANGE, true);
    return mound && mound->IsInCombat();
}

bool OuroScarabsUpTrigger::IsActiveInEncounter()
{
    // Scarabs (15718) spawn on re-emerge and stream during the 20% berserk
    // (guide: 钻出后刷新12只小虫；狂暴后不断刷新).
    Creature* scarab = bot->FindNearestCreature(NPC_OURO_SCARAB, OURO_SEARCH_RANGE, true);
    return scarab && scarab->isTargetableForAttack();
}

// ---- C'Thun ----

bool CthunDarkGlareTrigger::IsActiveInEncounter()
{
    // Eye goes red (22518) 3s before the 35-tick Dark Glare sweep (26029); while either
    // aura is up everyone keeps repositioning (guide: 眼睛绿变红，全体跑位).
    Creature* eye = bot->FindNearestCreature(NPC_EYE_OF_CTHUN, CTHUN_SEARCH_RANGE, true);
    if (!eye)
        return false;

    return eye->HasAura(SPELL_RED_COLORATION) || eye->HasAura(SPELL_DARK_GLARE);
}

bool CthunTentaclesUpTrigger::IsActiveInEncounter()
{
    // Outside the stomach only (z > 0, pattern from NotInStomachSelector).
    if (bot->GetPositionZ() < CTHUN_STOMACH_Z_F)
        return false;

    Creature* tentacle = bot->FindNearestCreature(NPC_CLAW_TENTACLE, CTHUN_SEARCH_RANGE, true);
    if (tentacle && tentacle->isTargetableForAttack())
        return true;

    tentacle = bot->FindNearestCreature(NPC_EYE_TENTACLE, CTHUN_SEARCH_RANGE, true);
    return tentacle && tentacle->isTargetableForAttack();
}

bool CthunGiantTentacleUpTrigger::IsActiveInEncounter()
{
    if (bot->GetPositionZ() < CTHUN_STOMACH_Z_F)
        return false;

    Creature* giant = bot->FindNearestCreature(NPC_GIANT_CLAW_TENTACLE, CTHUN_SEARCH_RANGE, true);
    if (giant && giant->isTargetableForAttack())
        return true;

    giant = bot->FindNearestCreature(NPC_GIANT_EYE_TENTACLE, CTHUN_SEARCH_RANGE, true);
    return giant && giant->isTargetableForAttack();
}

bool CthunInStomachTrigger::IsActiveInEncounter()
{
    // Swallowed players sit at z = -70 with the stacking Digestive Acid (26476) debuff
    // (pattern from NotInStomachSelector: outside = z > 0).
    return bot->GetPositionZ() < CTHUN_STOMACH_Z_F || bot->HasAura(SPELL_DIGESTIVE_ACID);
}
